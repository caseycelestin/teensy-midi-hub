#include "DeviceManager.h"
#include <Arduino.h>

DeviceManager::DeviceManager() : deviceCount(0), connectionCallback(nullptr) {
    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        devices[i].connected = false;
        devices[i].vid = 0;
        devices[i].pid = 0;
        devices[i].name[0] = '\0';
        devices[i].device = nullptr;
    }
}

void DeviceManager::init(MIDIDevice_BigBuffer* devicePtrs[], int count) {
    deviceCount = min(count, MAX_MIDI_DEVICES);
    for (int i = 0; i < deviceCount; i++) {
        devices[i].device = devicePtrs[i];
    }
}

void DeviceManager::update() {
    // Check all hardware slots for connect/disconnect
    for (int i = 0; i < deviceCount; i++) {
        MIDIDevice_BigBuffer* dev = devices[i].device;
        bool wasConnected = devices[i].connected;
        bool isNowConnected = (*dev);

        if (isNowConnected && !wasConnected) {
            // Device connected
            devices[i].connected = true;
            devices[i].vid = dev->idVendor();
            devices[i].pid = dev->idProduct();
            updateDeviceName(i);

            if (connectionCallback) {
                connectionCallback(i, true);
            }
        } else if (!isNowConnected && wasConnected) {
            // Device disconnected - call callback BEFORE clearing info
            // so the name is still available
            if (connectionCallback) {
                connectionCallback(i, false);
            }

            devices[i].connected = false;
            devices[i].vid = 0;
            devices[i].pid = 0;
            devices[i].name[0] = '\0';
        }
    }
}

int DeviceManager::getConnectedCount() const {
    int count = 0;
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].connected) count++;
    }
    return count;
}

const MidiDeviceInfo* DeviceManager::getDeviceBySlot(int slot) const {
    if (slot < 0 || slot >= deviceCount) return nullptr;
    return &devices[slot];
}

int DeviceManager::findDeviceByVidPid(uint16_t vid, uint16_t pid) const {
    for (int i = 0; i < deviceCount; i++) {
        if (devices[i].connected && devices[i].vid == vid && devices[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

MIDIDevice_BigBuffer* DeviceManager::getMidiDevice(int slot) const {
    if (slot < 0 || slot >= deviceCount) return nullptr;
    return devices[slot].device;
}

bool DeviceManager::isConnected(int slot) const {
    if (slot < 0 || slot >= deviceCount) return false;
    return devices[slot].connected;
}

void DeviceManager::setConnectionCallback(void (*callback)(int slot, bool connected)) {
    connectionCallback = callback;
}

void DeviceManager::updateDeviceName(int slot) {
    MIDIDevice_BigBuffer* dev = devices[slot].device;
    const uint8_t* prod = dev->product();

    if (prod && prod[0]) {
        strncpy(devices[slot].name, (const char*)prod, sizeof(devices[slot].name) - 1);
        devices[slot].name[sizeof(devices[slot].name) - 1] = '\0';
    } else {
        // Fallback to VID:PID
        snprintf(devices[slot].name, sizeof(devices[slot].name), "Device %04X:%04X",
                 devices[slot].vid, devices[slot].pid);
    }
}
