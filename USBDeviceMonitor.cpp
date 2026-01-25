#include "USBDeviceMonitor.h"

// USB class codes
#define USB_CLASS_AUDIO 0x01
#define USB_SUBCLASS_MIDISTREAMING 0x03

USBDeviceMonitor::USBDeviceMonitor(USBHost &host) : callback(nullptr), connectedDevice(nullptr) {
    // Register with USB host - this will be called for unclaimed devices
    driver_ready_for_device(this);
}

bool USBDeviceMonitor::claim(Device_t *device, int type, const uint8_t *descriptors, uint32_t len) {
    // We only want to catch devices at the interface level
    if (type != 1) return false;

    // Check if THIS interface is MIDI
    // Interface descriptor: [0]=length, [1]=type(4), [5]=class, [6]=subclass
    if (len >= 9 && descriptors[1] == 4) {
        uint8_t interfaceClass = descriptors[5];
        uint8_t interfaceSubClass = descriptors[6];

        // Audio class (0x01), MIDI Streaming subclass (0x03)
        if (interfaceClass == 0x01 && interfaceSubClass == 0x03) {
            // MIDI interface that MIDIDevice didn't claim = we're at max
            if (callback) {
                callback("Max MIDI devices reached!");
            }
            connectedDevice = device;
            return true;
        }
    }

    // Non-MIDI interface - don't notify for every interface
    // (devices have many interfaces, would spam notifications)
    return false;
}

void USBDeviceMonitor::disconnect() {
    connectedDevice = nullptr;
    // No notification needed on disconnect for non-MIDI devices
}

bool USBDeviceMonitor::isMidiDevice(const uint8_t *descriptors, uint32_t len) {
    // Parse descriptors looking for MIDI interface
    uint32_t i = 0;
    while (i < len) {
        uint8_t descLen = descriptors[i];
        uint8_t descType = descriptors[i + 1];

        if (descLen == 0) break;

        // Interface descriptor
        if (descType == 4 && descLen >= 9) {
            uint8_t interfaceClass = descriptors[i + 5];
            uint8_t interfaceSubClass = descriptors[i + 6];

            // Check for Audio class, MIDI Streaming subclass
            if (interfaceClass == USB_CLASS_AUDIO && interfaceSubClass == USB_SUBCLASS_MIDISTREAMING) {
                return true;
            }
        }

        i += descLen;
    }
    return false;
}
