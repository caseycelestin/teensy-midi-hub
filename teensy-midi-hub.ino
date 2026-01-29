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
#include "UIDriver.h"
#include "UIManager.h"
#ifdef UI_OLED
#include "OLEDUIDriver.h"
#endif
#ifdef UI_SERIAL
#include "SerialUIDriver.h"
#endif
#include "DeviceManager.h"
#include "RouteManager.h"
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
#ifdef UI_OLED
OLEDUIDriver oledDriver;
UIDriver* uiDriver = &oledDriver;
#endif
#ifdef UI_SERIAL
SerialUIDriver serialDriver;
UIDriver* uiDriver = &serialDriver;
#endif
UIManager ui;

// UI state machine
enum class UIState {
    MAIN_MENU,
    SOURCE_LIST,
    DEST_LIST
};

UIState currentState = UIState::MAIN_MENU;
bool needsListRebuild = true;
int mainMenuCursor = 0;  // Track cursor position for main menu

// Shared state for route creation
int selectedSourceSlot = -1;
uint16_t selectedSourceVid = 0;
uint16_t selectedSourcePid = 0;
char selectedSourceName[32] = "";
int selectedDestSlot = -1;
uint16_t selectedDestVid = 0;
uint16_t selectedDestPid = 0;
char selectedDestName[32] = "";

// Cached device lists (to detect changes)
int connectedSlots[MAX_MIDI_DEVICES];
int connectedCount = 0;
int availableSlots[MAX_MIDI_DEVICES];
int availableCount = 0;

// Timing
unsigned long lastUiUpdate = 0;

// Forward declarations
void buildMainMenu();
void buildSourceList();
void buildDestList();
void buildConfirmRoute();
void handleMainMenuInput(InputEvent event);
void handleSourceListInput(InputEvent event);
void handleDestListInput(InputEvent event);
void handleConfirmRouteInput(InputEvent event);
void refreshConnectedDevices();
void refreshAvailableDevices();
void onDeleteConfirm(bool confirmed);
void onCreateConfirm(bool confirmed);
void updateLedForSelection();

// Check if a route has a disconnected member
bool isRouteIncomplete(const Route* route) {
    if (!route) return false;
    bool srcConnected = deviceManager.findDeviceByVidPid(route->sourceVid, route->sourcePid) >= 0;
    bool dstConnected = deviceManager.findDeviceByVidPid(route->destVid, route->destPid) >= 0;
    return !srcConnected || !dstConnected;
}

// Update LED color based on current selection (or off if sleeping)
void updateLedForSelection() {
    // Turn off LED when sleeping
    if (ui.isSleeping()) {
        input->setColor(0, 0, 0);
        return;
    }

    if (currentState == UIState::MAIN_MENU) {
        ListView& list = ui.getList();
        if (list.selectedIndex > 0) {
            // On a route - check if incomplete
            const Route* route = routeManager.getRoute(list.selectedIndex - 1);
            if (isRouteIncomplete(route)) {
                input->setColor(60, 0, 0);  // Red for incomplete
                return;
            }
        }
    }
    // Default: dim blue
    input->setColor(0, 0, 30);
}

// Connection change callback for MIDI devices
void onMidiConnectionChange(int slot, bool connected) {
    const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(slot);
    char msg[64];

    if (connected && info) {
        snprintf(msg, sizeof(msg), "+ %s", info->name);
        ui.showToast(msg);
    } else if (info && info->name[0]) {
        snprintf(msg, sizeof(msg), "- %s", info->name);
        ui.showToast(msg);
    } else {
        ui.showToast("- device");
    }

    // Refresh list on device change
    needsListRebuild = true;
}

// Callback for non-MIDI devices or overflow
void onUSBDeviceEvent(const char* message) {
    ui.showToast(message);
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

#ifdef UI_OLED
    // Initialize OLED display
    if (!oledDriver.begin()) {
        Serial.println("ERROR: OLED display not found! Check I2C connection.");
    }
#endif

    // Initialize device manager
    deviceManager.init(midiDevices, MAX_MIDI_DEVICES);
    deviceManager.setConnectionCallback(onMidiConnectionChange);

    // Set up USB monitor for non-MIDI devices and overflow
    usbMonitor.setCallback(onUSBDeviceEvent);

    // Load saved routes from EEPROM
    routeManager.load();

    // Set up UI
    ui.setDriver(uiDriver);

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

        // Update UI (toast expiration)
        ui.update();

        // Check for device list changes in source/dest states
        if (currentState == UIState::SOURCE_LIST) {
            int oldCount = connectedCount;
            refreshConnectedDevices();
            if (connectedCount != oldCount) {
                needsListRebuild = true;
            }
        } else if (currentState == UIState::DEST_LIST) {
            int oldCount = availableCount;
            refreshAvailableDevices();
            if (availableCount != oldCount) {
                needsListRebuild = true;
            }
        }

        // Rebuild list if needed
        if (needsListRebuild) {
            switch (currentState) {
                case UIState::MAIN_MENU:    buildMainMenu(); break;
                case UIState::SOURCE_LIST:  buildSourceList(); break;
                case UIState::DEST_LIST:    buildDestList(); break;
            }
            needsListRebuild = false;
            updateLedForSelection();
            ui.requestRedraw();
        }

        // Check for input
        if (input->hasInput()) {
            InputEvent event = input->getInput();

            // Wake from sleep on any input
            bool wasSleeping = ui.isSleeping();
            ui.activity();

            if (wasSleeping) {
                // Just woke up - restore LED and skip processing this input
                updateLedForSelection();
            } else {
                // Let UI manager handle confirmation dialog first
                if (ui.handleInput(event)) {
                    // Input was consumed by confirmation
                } else {
                    // Handle based on current state
                    switch (currentState) {
                        case UIState::MAIN_MENU:    handleMainMenuInput(event); break;
                        case UIState::SOURCE_LIST:  handleSourceListInput(event); break;
                        case UIState::DEST_LIST:    handleDestListInput(event); break;
                    }
                }
            }
        }

        // Check if just entered sleep mode
        static bool wasSleeping = false;
        if (ui.isSleeping() != wasSleeping) {
            wasSleeping = ui.isSleeping();
            updateLedForSelection();  // Turn LED off/on
        }

        // Render
        ui.render();
    }

    // Heartbeat LED
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 1000) {
        lastBlink = millis();
        digitalToggle(LED_BUILTIN);
    }
}

// ============================================
// List Building Functions
// ============================================

// Static buffers for menu item text (needed because ListView stores pointers)
static char menuBuf[MAX_LIST_ITEMS][32];

void buildMainMenu() {
    ListView& list = ui.getList();
    list.clear();

    // First item: "routes" centered with "+" on right
    list.add(nullptr, "routes", "+");

    // Existing routes (left-justified)
    int routeCount = routeManager.getRouteCount();
    for (int i = 0; i < routeCount && list.count < MAX_LIST_ITEMS; i++) {
        const Route* route = routeManager.getRoute(i);
        snprintf(menuBuf[list.count], sizeof(menuBuf[0]), "%s>%s", route->sourceName, route->destName);
        list.add(menuBuf[list.count], nullptr, nullptr);
    }

    // Set cursor position (clamped to valid range)
    if (mainMenuCursor >= list.count) {
        mainMenuCursor = list.count - 1;
    }
    if (mainMenuCursor < 0) {
        mainMenuCursor = 0;
    }
    list.selectedIndex = mainMenuCursor;
}

void buildSourceList() {
    refreshConnectedDevices();

    ListView& list = ui.getList();
    list.clear();

    // First item: back
    list.add("<", "sources", nullptr);

    // Connected devices (left-justified)
    for (int i = 0; i < connectedCount && list.count < MAX_LIST_ITEMS; i++) {
        const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(connectedSlots[i]);
        if (info) {
            list.add(info->name, nullptr, nullptr);
        }
    }
}

void buildDestList() {
    refreshAvailableDevices();

    ListView& list = ui.getList();
    list.clear();

    // First item: back
    list.add("<", "sinks", nullptr);

    // Available destinations (left-justified)
    for (int i = 0; i < availableCount && list.count < MAX_LIST_ITEMS; i++) {
        const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(availableSlots[i]);
        if (info) {
            list.add(info->name, nullptr, nullptr);
        }
    }
}

// ============================================
// Input Handling Functions
// ============================================

// Track which route is being deleted
static int deleteRouteIndex = -1;

void handleMainMenuInput(InputEvent event) {
    ListView& list = ui.getList();

    switch (event) {
        case InputEvent::UP:
            list.selectPrev();
            mainMenuCursor = list.selectedIndex;
            updateLedForSelection();
            ui.requestRedraw();
            break;

        case InputEvent::DOWN:
            list.selectNext();
            mainMenuCursor = list.selectedIndex;
            updateLedForSelection();
            ui.requestRedraw();
            break;

        case InputEvent::ENTER:
            if (list.selectedIndex == 0) {
                // +Route selected - go to source selection
                currentState = UIState::SOURCE_LIST;
                needsListRebuild = true;
            } else {
                // Route selected - confirm delete
                deleteRouteIndex = list.selectedIndex - 1;
                ui.showConfirmation("delete?", "yes", "no", onDeleteConfirm);
            }
            break;

        default:
            break;
    }
}

void onDeleteConfirm(bool confirmed) {
    if (confirmed && deleteRouteIndex >= 0) {
        routeManager.removeRouteByIndex(deleteRouteIndex);
        ui.showToast("- route");
    }
    deleteRouteIndex = -1;
    needsListRebuild = true;
}

void handleSourceListInput(InputEvent event) {
    ListView& list = ui.getList();

    switch (event) {
        case InputEvent::UP:
            list.selectPrev();
            ui.requestRedraw();
            break;

        case InputEvent::DOWN:
            list.selectNext();
            ui.requestRedraw();
            break;

        case InputEvent::ENTER:
            if (list.selectedIndex == 0) {
                // Back selected
                currentState = UIState::MAIN_MENU;
                needsListRebuild = true;
            } else if (list.selectedIndex - 1 < connectedCount) {
                // Device selected
                int slot = connectedSlots[list.selectedIndex - 1];
                const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(slot);
                if (info) {
                    selectedSourceSlot = slot;
                    selectedSourceVid = info->vid;
                    selectedSourcePid = info->pid;
                    strncpy(selectedSourceName, info->name, sizeof(selectedSourceName) - 1);
                    selectedSourceName[sizeof(selectedSourceName) - 1] = '\0';

                    currentState = UIState::DEST_LIST;
                    needsListRebuild = true;
                }
            }
            break;

        default:
            break;
    }
}

void handleDestListInput(InputEvent event) {
    ListView& list = ui.getList();

    switch (event) {
        case InputEvent::UP:
            list.selectPrev();
            ui.requestRedraw();
            break;

        case InputEvent::DOWN:
            list.selectNext();
            ui.requestRedraw();
            break;

        case InputEvent::ENTER:
            if (list.selectedIndex == 0) {
                // Back selected
                currentState = UIState::SOURCE_LIST;
                needsListRebuild = true;
            } else if (list.selectedIndex - 1 < availableCount) {
                // Device selected - create route immediately
                int slot = availableSlots[list.selectedIndex - 1];
                const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(slot);
                if (info) {
                    bool added = routeManager.addRoute(
                        selectedSourceVid, selectedSourcePid, selectedSourceName,
                        info->vid, info->pid, info->name
                    );

                    if (added) {
                        ui.showToast("+ route");
                        // Set cursor to the newly created route (it's the last one)
                        mainMenuCursor = routeManager.getRouteCount();  // +1 for header row
                    } else if (routeManager.getRouteCount() >= MAX_ROUTES) {
                        ui.showToast("Max routes!");
                        mainMenuCursor = 0;
                    } else {
                        ui.showToast("Route exists");
                        mainMenuCursor = 0;
                    }

                    currentState = UIState::MAIN_MENU;
                    needsListRebuild = true;
                }
            }
            break;

        default:
            break;
    }
}

// ============================================
// Helper Functions
// ============================================

void refreshConnectedDevices() {
    connectedCount = 0;
    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        if (deviceManager.isConnected(i)) {
            connectedSlots[connectedCount++] = i;
        }
    }
}

void refreshAvailableDevices() {
    availableCount = 0;
    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        // Exclude the source device
        if (i != selectedSourceSlot && deviceManager.isConnected(i)) {
            availableSlots[availableCount++] = i;
        }
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
