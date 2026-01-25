#ifndef USB_DEVICE_MONITOR_H
#define USB_DEVICE_MONITOR_H

#include <USBHost_t36.h>

// Callback type for USB device events
typedef void (*USBDeviceCallback)(const char* message);

// Monitors for USB devices that aren't claimed by MIDI drivers
// This catches: non-MIDI devices and overflow MIDI devices when slots are full
class USBDeviceMonitor : public USBDriver {
public:
    USBDeviceMonitor(USBHost &host);

    void setCallback(USBDeviceCallback cb) { callback = cb; }

protected:
    virtual bool claim(Device_t *device, int type, const uint8_t *descriptors, uint32_t len);
    virtual void disconnect();

private:
    USBDeviceCallback callback;
    Device_t *connectedDevice;

    bool isMidiDevice(const uint8_t *descriptors, uint32_t len);
};

#endif
