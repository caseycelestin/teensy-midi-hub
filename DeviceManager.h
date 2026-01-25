#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <USBHost_t36.h>
#include "Config.h"

// Information about a connected MIDI device
struct MidiDeviceInfo {
    bool connected;
    uint16_t vid;
    uint16_t pid;
    char name[32];
    MIDIDevice_BigBuffer* device;
};

// Manages USB MIDI device connections and provides device info
class DeviceManager {
public:
    DeviceManager();

    // Initialize with USB host MIDI device pointers
    void init(MIDIDevice_BigBuffer* devices[], int count);

    // Call in main loop to check for connect/disconnect
    void update();

    // Get number of currently connected devices
    int getConnectedCount() const;

    // Get info for a specific slot (0 to MAX_MIDI_DEVICES-1)
    const MidiDeviceInfo* getDeviceBySlot(int slot) const;

    // Find device slot by VID:PID, returns -1 if not found
    int findDeviceByVidPid(uint16_t vid, uint16_t pid) const;

    // Get the underlying MIDIDevice for a slot (for sending MIDI)
    MIDIDevice_BigBuffer* getMidiDevice(int slot) const;

    // Check if a specific slot is connected
    bool isConnected(int slot) const;

    // Callback for connection changes (optional)
    void setConnectionCallback(void (*callback)(int slot, bool connected));

private:
    MidiDeviceInfo devices[MAX_MIDI_DEVICES];
    int deviceCount;
    void (*connectionCallback)(int slot, bool connected);

    void updateDeviceName(int slot);
};

#endif
