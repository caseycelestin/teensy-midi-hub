/*
 * Teensy MIDI Hub
 *
 * A USB MIDI hub for Teensy 4.1 with configurable routing.
 * Routes MIDI between USB Host MIDI devices based on user-configured routes.
 * Computer connection is used for Serial configuration UI only.
 */

#include <USBHost_t36.h>
#include "Config.h"
#include "Input.h"
#ifdef INPUT_QWIIC_TWIST
#include "QwiicTwistInput.h"
#endif
#ifdef INPUT_SERIAL
#include "SerialInput.h"
#endif
#include "SerialDisplay.h"
#include "DeviceManager.h"
#include "RouteManager.h"
#include "Pages.h"
#include "USBDeviceMonitor.h"

// USB Host objects
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);

// USB Host MIDI devices FIRST (so they get first chance to claim)
#if MAX_MIDI_DEVICES >= 1
MIDIDevice_BigBuffer midi1(myusb);
#endif
#if MAX_MIDI_DEVICES >= 2
MIDIDevice_BigBuffer midi2(myusb);
#endif
#if MAX_MIDI_DEVICES >= 3
MIDIDevice_BigBuffer midi3(myusb);
#endif
#if MAX_MIDI_DEVICES >= 4
MIDIDevice_BigBuffer midi4(myusb);
#endif
#if MAX_MIDI_DEVICES >= 5
MIDIDevice_BigBuffer midi5(myusb);
#endif
#if MAX_MIDI_DEVICES >= 6
MIDIDevice_BigBuffer midi6(myusb);
#endif
#if MAX_MIDI_DEVICES >= 7
MIDIDevice_BigBuffer midi7(myusb);
#endif
#if MAX_MIDI_DEVICES >= 8
MIDIDevice_BigBuffer midi8(myusb);
#endif

// Catch-all LAST (only sees what MIDIDevices didn't claim)
USBDeviceMonitor usbMonitor(myusb);

// Array of host MIDI device pointers
MIDIDevice_BigBuffer* midiDevices[] = {
#if MAX_MIDI_DEVICES >= 1
    &midi1,
#endif
#if MAX_MIDI_DEVICES >= 2
    &midi2,
#endif
#if MAX_MIDI_DEVICES >= 3
    &midi3,
#endif
#if MAX_MIDI_DEVICES >= 4
    &midi4,
#endif
#if MAX_MIDI_DEVICES >= 5
    &midi5,
#endif
#if MAX_MIDI_DEVICES >= 6
    &midi6,
#endif
#if MAX_MIDI_DEVICES >= 7
    &midi7,
#endif
#if MAX_MIDI_DEVICES >= 8
    &midi8,
#endif
};

// Core managers
DeviceManager deviceManager;
RouteManager routeManager;

// UI components
#ifdef INPUT_QWIIC_TWIST
QwiicTwistInput qwiicInput;
Input* input = &qwiicInput;
#endif
#ifdef INPUT_SERIAL
SerialInput serialInput;
Input* input = &serialInput;
#endif
SerialDisplay serialDisplay;
PageManager pageManager;

// Pages
MainMenuPage mainMenuPage(&pageManager, &serialDisplay, input, &deviceManager, &routeManager);
SourceListPage sourceListPage(&pageManager, &serialDisplay, input, &deviceManager, &routeManager);
DestListPage destListPage(&pageManager, &serialDisplay, input, &deviceManager, &routeManager);
ConfirmRoutePage confirmRoutePage(&pageManager, &serialDisplay, input, &deviceManager, &routeManager);
ConnectionsPage connectionsPage(&pageManager, &serialDisplay, input, &deviceManager, &routeManager);

// Timing
unsigned long lastUiUpdate = 0;

// Connection change callback for MIDI devices
void onMidiConnectionChange(int slot, bool connected) {
    const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(slot);
    char msg[64];

    if (connected && info) {
        int count = deviceManager.getConnectedCount();
        snprintf(msg, sizeof(msg), "%s connected! %d/%d", info->name, count, MAX_MIDI_DEVICES);
        pageManager.showNotification(msg);
    } else if (info && info->name[0]) {
        snprintf(msg, sizeof(msg), "%s disconnected", info->name);
        pageManager.showNotification(msg);
    } else {
        pageManager.showNotification("Device disconnected");
    }
}

// Callback for non-MIDI devices or overflow
void onUSBDeviceEvent(const char* message) {
    pageManager.showNotification(message);
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // Wait for USB to fully enumerate (helps with WSL/usbipd after upload)
    delay(3000);

    Serial.begin(115200);

    // Wait for serial connection with DTR (longer timeout for tio to connect)
    while (!Serial.dtr() && millis() < 10000) {
        delay(10);
    }
    delay(500);  // Extra delay after DTR for terminal to be ready

#ifdef INPUT_QWIIC_TWIST
    // Initialize Qwiic Twist input
    if (!qwiicInput.begin()) {
        Serial.println("ERROR: Qwiic Twist not found! Check I2C connection.");
    }
#endif

    // Initialize device manager
    deviceManager.init(midiDevices, MAX_MIDI_DEVICES);
    deviceManager.setConnectionCallback(onMidiConnectionChange);

    // Set up USB monitor for non-MIDI devices and overflow
    usbMonitor.setCallback(onUSBDeviceEvent);

    // Load saved routes from EEPROM
    routeManager.load();

    // Set up page system
    pageManager.setDisplay(&serialDisplay);
    pageManager.setPage(PageId::MAIN_MENU, &mainMenuPage);
    pageManager.setPage(PageId::SOURCE_LIST, &sourceListPage);
    pageManager.setPage(PageId::DEST_LIST, &destListPage);
    pageManager.setPage(PageId::CONFIRM_ROUTE, &confirmRoutePage);
    pageManager.setPage(PageId::CONNECTIONS, &connectionsPage);

    // Start at main menu
    pageManager.navigateTo(PageId::MAIN_MENU);

    // Initialize USB Host
    myusb.begin();

    Serial.println("Teensy MIDI Hub - Configurable Routing");
    Serial.println("======================================");
    Serial.print("Loaded ");
    Serial.print(routeManager.getRouteCount());
    Serial.println(" routes from EEPROM");
    Serial.println();
}

void loop() {
    myusb.Task();

    // Update device manager (handles connect/disconnect)
    deviceManager.update();

    // Route MIDI between devices
    routeMidi();

    // Handle UI updates at fixed rate
    unsigned long now = millis();
    if (now - lastUiUpdate >= UI_REFRESH_MS) {
        lastUiUpdate = now;

        // Update page logic
        pageManager.update();

        // Check for input
        if (input->hasInput()) {
            InputEvent event = input->getInput();
            pageManager.handleInput(event);
        }

        // Render if needed
        pageManager.render();
    }

    // Heartbeat LED
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 1000) {
        lastBlink = millis();
        digitalToggle(LED_BUILTIN);
    }
}

void routeMidi() {
    // Route from each connected host device
    for (int srcSlot = 0; srcSlot < MAX_MIDI_DEVICES; srcSlot++) {
        if (!deviceManager.isConnected(srcSlot)) continue;

        MIDIDevice_BigBuffer* source = deviceManager.getMidiDevice(srcSlot);
        if (!source->read()) continue;

        // Get MIDI message data
        uint8_t type = source->getType();
        uint8_t data1 = source->getData1();
        uint8_t data2 = source->getData2();
        uint8_t channel = source->getChannel();
        uint8_t cable = source->getCable();

        // Get source device info for route lookup
        const MidiDeviceInfo* srcInfo = deviceManager.getDeviceBySlot(srcSlot);
        if (!srcInfo) continue;

        // Check each potential destination
        for (int dstSlot = 0; dstSlot < MAX_MIDI_DEVICES; dstSlot++) {
            if (dstSlot == srcSlot) continue;
            if (!deviceManager.isConnected(dstSlot)) continue;

            const MidiDeviceInfo* dstInfo = deviceManager.getDeviceBySlot(dstSlot);
            if (!dstInfo) continue;

            // Check if this route is configured
            if (!routeManager.shouldRoute(srcInfo->vid, srcInfo->pid, dstInfo->vid, dstInfo->pid)) {
                continue;
            }

            // Route the message
            MIDIDevice_BigBuffer* dest = deviceManager.getMidiDevice(dstSlot);
            if (type == 0xF0) {  // SystemExclusive
                dest->sendSysEx(source->getSysExArrayLength(), source->getSysExArray(), true, cable);
            } else {
                dest->send(type, data1, data2, channel, cable);
            }
        }
    }
}
