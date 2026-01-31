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

// Timing
unsigned long lastUiUpdate = 0;
unsigned long lastActivityTime = 0;
bool displaySleeping = false;

// Page state
enum class Page { DEVICES, DEVICE_ROUTES, DELETE_ROUTE, ADD_ROUTE };
Page currentPage = Page::DEVICES;

// UI state for list navigation
int selectedIndex = 0;    // Currently selected item in list
int viewStart = 0;        // First visible item (for scrolling)
const int VISIBLE_ROWS = 3;  // Rows 1-3 on display

// Connected device cache (rebuilt each frame)
int connectedSlots[MAX_MIDI_DEVICES];
int connectedCount = 0;

// Selected device for routes page
int selectedDeviceSlot = -1;
uint16_t selectedDeviceVid = 0;
uint16_t selectedDevicePid = 0;
char selectedDeviceName[32] = "";

// Routes for selected device (indices into routeManager)
int deviceRouteIndices[MAX_ROUTES];
bool deviceRouteActive[MAX_ROUTES];  // true if destination is connected
int deviceRouteCount = 0;

// Disconnected devices with routes (shown on devices page)
struct DisconnectedDevice {
    uint16_t vid;
    uint16_t pid;
    char name[24];
};
DisconnectedDevice disconnectedWithRoutes[MAX_ROUTES];
int disconnectedCount = 0;

// Route to delete
int deleteRouteIndex = -1;

// Available destinations for add route (other connected devices)
int destSlots[MAX_MIDI_DEVICES];
int destCount = 0;

// Forward declarations
void routeMidi();
void updateDeviceList();
void updateDeviceRoutes();
void updateAddRouteDestinations();
void renderDevicesPage();
void renderDeviceRoutesPage();
void renderDeleteRoutePage();
void renderAddRoutePage();
void handleDevicesInput(InputEvent event);
void handleDeviceRoutesInput(InputEvent event);
void handleDeleteRouteInput(InputEvent event);
void handleAddRouteInput(InputEvent event);

// Update LED color based on current selection
void updateLedColor() {
    if (displaySleeping) {
        input->setColor(0, 0, 0);
        return;
    }

    bool onDisconnected = false;
    if (currentPage == Page::DEVICES) {
        onDisconnected = selectedIndex >= connectedCount && (connectedCount + disconnectedCount) > 0;
    } else if (currentPage == Page::DEVICE_ROUTES) {
        onDisconnected = selectedIndex < deviceRouteCount && !deviceRouteActive[selectedIndex];
    }

    if (onDisconnected) {
        input->setColor(60, 0, 0);
    } else {
        input->setColor(0, 0, 30);
    }
}

// Center-locked scrolling: keeps cursor in middle row when possible
void updateViewForSelection(int totalItems) {
    const int centerRow = VISIBLE_ROWS / 2;  // Row 1 for 3 rows

    // Calculate ideal viewStart to center the selection
    viewStart = selectedIndex - centerRow;

    // Clamp to valid range
    if (viewStart < 0) {
        viewStart = 0;
    }
    if (viewStart > totalItems - VISIBLE_ROWS) {
        viewStart = totalItems - VISIBLE_ROWS;
    }
    if (viewStart < 0) {
        viewStart = 0;  // In case totalItems < VISIBLE_ROWS
    }
}

// Connection change callback for MIDI devices
void onMidiConnectionChange(int slot, bool connected) {
    (void)slot;
    (void)connected;
    // Connection handled by UI refresh
}

// Callback for non-MIDI devices or overflow
void onUSBDeviceEvent(const char* message) {
    (void)message;
    // Non-MIDI devices ignored
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);

    // Wait for USB to fully enumerate (helps with WSL/usbipd after upload)
    delay(3000);

#ifndef UI_OLED
    Serial.begin(115200);
    // Wait for serial connection with DTR (only needed for serial UI)
    while (!Serial.dtr() && millis() < 10000) {
        delay(10);
    }
    delay(500);  // Extra delay after DTR for terminal to be ready
#endif

#ifdef INPUT_QWIIC_TWIST
    // Initialize Qwiic Twist input
    qwiicInput.begin();
#endif

#ifdef UI_OLED
    // Initialize OLED display
    oledDriver.begin();
#endif

    // Initialize device manager
    deviceManager.init(midiDevices, MAX_MIDI_DEVICES);
    deviceManager.setConnectionCallback(onMidiConnectionChange);

    // Set up USB monitor for non-MIDI devices and overflow
    usbMonitor.setCallback(onUSBDeviceEvent);

    // Load saved routes from EEPROM
    routeManager.load();

    // Initialize USB Host
    myusb.begin();

    // Initialize activity tracking (LED set on first frame after device list populated)
    lastActivityTime = millis();
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

        // Check for input
        bool hadInput = false;
        InputEvent event = InputEvent::NONE;
        if (input->hasInput()) {
            event = input->getInput();
            hadInput = true;
            lastActivityTime = now;

            // Wake from sleep on any input
            if (displaySleeping) {
                displaySleeping = false;
                uiDriver->displayOn();
                event = InputEvent::NONE;  // Consume input for wake
            }
        }

        // Check for deep sleep timeout
        if (!displaySleeping && DEEP_SLEEP_TIMEOUT_MS > 0) {
            if (now - lastActivityTime >= DEEP_SLEEP_TIMEOUT_MS) {
                displaySleeping = true;
                uiDriver->displayOff();
                input->setColor(0, 0, 0);
            }
        }

        // Skip rendering if sleeping
        if (displaySleeping) {
            // Still need to do the UI update check for next iteration
        } else {
            // Update and render based on current page
            if (currentPage == Page::DEVICES) {
                updateDeviceList();
                if (hadInput && event != InputEvent::NONE) {
                    handleDevicesInput(event);
                }
                updateLedColor();
                renderDevicesPage();
            } else if (currentPage == Page::DEVICE_ROUTES) {
                updateDeviceRoutes();
                if (hadInput && event != InputEvent::NONE) {
                    handleDeviceRoutesInput(event);
                }
                updateLedColor();
                renderDeviceRoutesPage();
            } else if (currentPage == Page::DELETE_ROUTE) {
                if (hadInput && event != InputEvent::NONE) {
                    handleDeleteRouteInput(event);
                }
                updateLedColor();
                renderDeleteRoutePage();
            } else if (currentPage == Page::ADD_ROUTE) {
                updateAddRouteDestinations();
                if (hadInput && event != InputEvent::NONE) {
                    handleAddRouteInput(event);
                }
                updateLedColor();
                renderAddRoutePage();
            }
        }
    }

    // Heartbeat LED
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink >= 1000) {
        lastBlink = millis();
        digitalToggle(LED_BUILTIN);
    }
}

// Count routes where device is the source
int countRoutesForDevice(uint16_t vid, uint16_t pid) {
    int count = 0;
    int totalRoutes = routeManager.getRouteCount();
    for (int i = 0; i < totalRoutes; i++) {
        const Route* route = routeManager.getRoute(i);
        if (route && route->sourceVid == vid && route->sourcePid == pid) {
            count++;
        }
    }
    return count;
}

void updateDeviceList() {
    connectedCount = 0;

    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        if (deviceManager.isConnected(i)) {
            connectedSlots[connectedCount++] = i;
        }
    }

    // Sort connected devices by route count (descending)
    for (int i = 0; i < connectedCount - 1; i++) {
        for (int j = i + 1; j < connectedCount; j++) {
            const MidiDeviceInfo* infoI = deviceManager.getDeviceBySlot(connectedSlots[i]);
            const MidiDeviceInfo* infoJ = deviceManager.getDeviceBySlot(connectedSlots[j]);
            int countI = infoI ? countRoutesForDevice(infoI->vid, infoI->pid) : 0;
            int countJ = infoJ ? countRoutesForDevice(infoJ->vid, infoJ->pid) : 0;
            if (countJ > countI) {
                int tmp = connectedSlots[i];
                connectedSlots[i] = connectedSlots[j];
                connectedSlots[j] = tmp;
            }
        }
    }

    // Find disconnected devices that have routes
    disconnectedCount = 0;
    int totalRoutes = routeManager.getRouteCount();
    for (int i = 0; i < totalRoutes; i++) {
        const Route* route = routeManager.getRoute(i);
        if (!route) continue;

        // Check if source is disconnected
        if (deviceManager.findDeviceByVidPid(route->sourceVid, route->sourcePid) < 0) {
            // Check if we already added this device
            bool found = false;
            for (int j = 0; j < disconnectedCount; j++) {
                if (disconnectedWithRoutes[j].vid == route->sourceVid &&
                    disconnectedWithRoutes[j].pid == route->sourcePid) {
                    found = true;
                    break;
                }
            }
            if (!found && disconnectedCount < MAX_ROUTES) {
                disconnectedWithRoutes[disconnectedCount].vid = route->sourceVid;
                disconnectedWithRoutes[disconnectedCount].pid = route->sourcePid;
                snprintf(disconnectedWithRoutes[disconnectedCount].name,
                         sizeof(disconnectedWithRoutes[disconnectedCount].name),
                         "%s", route->sourceName);
                disconnectedCount++;
            }
        }
    }

    // Sort disconnected devices by route count (descending)
    for (int i = 0; i < disconnectedCount - 1; i++) {
        for (int j = i + 1; j < disconnectedCount; j++) {
            int countI = countRoutesForDevice(disconnectedWithRoutes[i].vid, disconnectedWithRoutes[i].pid);
            int countJ = countRoutesForDevice(disconnectedWithRoutes[j].vid, disconnectedWithRoutes[j].pid);
            if (countJ > countI) {
                DisconnectedDevice tmp = disconnectedWithRoutes[i];
                disconnectedWithRoutes[i] = disconnectedWithRoutes[j];
                disconnectedWithRoutes[j] = tmp;
            }
        }
    }

    // Total items = connected + disconnected
    int totalItems = connectedCount + disconnectedCount;

    // Clamp selection and update view
    if (totalItems > 0) {
        if (selectedIndex >= totalItems) {
            selectedIndex = totalItems - 1;
        }
        if (selectedIndex < 0) {
            selectedIndex = 0;
        }
        updateViewForSelection(totalItems);
    } else {
        selectedIndex = 0;
        viewStart = 0;
    }
}

void updateDeviceRoutes() {
    // Find all routes where selected device is the source
    // First pass: collect active routes
    deviceRouteCount = 0;
    int totalRoutes = routeManager.getRouteCount();

    for (int i = 0; i < totalRoutes; i++) {
        const Route* route = routeManager.getRoute(i);
        if (route && route->sourceVid == selectedDeviceVid && route->sourcePid == selectedDevicePid) {
            bool active = (deviceManager.findDeviceByVidPid(route->destVid, route->destPid) >= 0);
            if (active) {
                deviceRouteIndices[deviceRouteCount] = i;
                deviceRouteActive[deviceRouteCount] = true;
                deviceRouteCount++;
            }
        }
    }

    // Second pass: collect inactive routes
    for (int i = 0; i < totalRoutes; i++) {
        const Route* route = routeManager.getRoute(i);
        if (route && route->sourceVid == selectedDeviceVid && route->sourcePid == selectedDevicePid) {
            bool active = (deviceManager.findDeviceByVidPid(route->destVid, route->destPid) >= 0);
            if (!active) {
                deviceRouteIndices[deviceRouteCount] = i;
                deviceRouteActive[deviceRouteCount] = false;
                deviceRouteCount++;
            }
        }
    }

    // Total items: routes + "+ add route" + "< back"
    int totalItems = deviceRouteCount + 2;

    // Clamp selection and update view
    if (selectedIndex >= totalItems) {
        selectedIndex = totalItems - 1;
    }
    if (selectedIndex < 0) {
        selectedIndex = 0;
    }
    updateViewForSelection(totalItems);
}

void handleDevicesInput(InputEvent event) {
    int totalItems = connectedCount + disconnectedCount;

    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                updateViewForSelection(totalItems);
            }
            break;
        case InputEvent::DOWN:
            if (selectedIndex < totalItems - 1) {
                selectedIndex++;
                updateViewForSelection(totalItems);
            }
            break;
        case InputEvent::ENTER:
            if (selectedIndex < connectedCount) {
                // Connected device selected
                int slot = connectedSlots[selectedIndex];
                const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(slot);
                if (info) {
                    selectedDeviceSlot = slot;
                    selectedDeviceVid = info->vid;
                    selectedDevicePid = info->pid;
                    snprintf(selectedDeviceName, sizeof(selectedDeviceName), "%s", info->name);

                    currentPage = Page::DEVICE_ROUTES;
                    selectedIndex = 0;
                    viewStart = 0;
                }
            } else if (selectedIndex < totalItems) {
                // Disconnected device selected
                int idx = selectedIndex - connectedCount;
                selectedDeviceSlot = -1;  // Not connected
                selectedDeviceVid = disconnectedWithRoutes[idx].vid;
                selectedDevicePid = disconnectedWithRoutes[idx].pid;
                snprintf(selectedDeviceName, sizeof(selectedDeviceName), "%s", disconnectedWithRoutes[idx].name);

                currentPage = Page::DEVICE_ROUTES;
                selectedIndex = 0;
                viewStart = 0;
            }
            break;
        default:
            break;
    }
}

void handleDeviceRoutesInput(InputEvent event) {
    // Order: routes, + add route, < back
    int totalItems = deviceRouteCount + 2;  // routes + "+ add route" + "< back"

    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                updateViewForSelection(totalItems);
            }
            break;
        case InputEvent::DOWN:
            if (selectedIndex < totalItems - 1) {
                selectedIndex++;
                updateViewForSelection(totalItems);
            }
            break;
        case InputEvent::ENTER:
            if (selectedIndex < deviceRouteCount) {
                // Route selected - go to delete confirmation
                deleteRouteIndex = deviceRouteIndices[selectedIndex];
                currentPage = Page::DELETE_ROUTE;
                selectedIndex = 0;  // Default to "no"
                viewStart = 0;
            } else if (selectedIndex == deviceRouteCount) {
                // "+ add route" - go to destination selection
                currentPage = Page::ADD_ROUTE;
                selectedIndex = 0;
                viewStart = 0;
            } else {
                // "< back" - return to devices page
                currentPage = Page::DEVICES;
                selectedIndex = 0;
                viewStart = 0;
            }
            break;
        default:
            break;
    }
}

void renderDevicesPage() {
    uiDriver->beginFrame();
    uiDriver->drawHeader("devices");

    int totalItems = connectedCount + disconnectedCount;
    static char nameBuf[32];

    for (int row = 0; row < VISIBLE_ROWS; row++) {
        int itemIndex = viewStart + row;
        if (itemIndex >= totalItems) break;

        bool selected = (itemIndex == selectedIndex);

        if (itemIndex < connectedCount) {
            // Connected device
            int slot = connectedSlots[itemIndex];
            const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(slot);
            if (info) {
                uiDriver->drawListItem(info->name, selected, row);
            }
        } else {
            // Disconnected device with routes - show with ~ prefix
            int idx = itemIndex - connectedCount;
            snprintf(nameBuf, sizeof(nameBuf), "~%s", disconnectedWithRoutes[idx].name);
            uiDriver->drawListItem(nameBuf, selected, row);
        }
    }

    // Scroll indicators
    bool canScrollUp = (viewStart > 0);
    bool canScrollDown = (viewStart + VISIBLE_ROWS < totalItems);
    uiDriver->drawScrollIndicators(canScrollUp, canScrollDown);

    uiDriver->endFrame();
}

void renderDeviceRoutesPage() {
    uiDriver->beginFrame();
    uiDriver->drawHeader(selectedDeviceName);

    // Order: routes, + add route, < back
    int totalItems = deviceRouteCount + 2;  // routes + "+ add route" + "< back"
    static char nameBuf[32];

    for (int row = 0; row < VISIBLE_ROWS; row++) {
        int itemIndex = viewStart + row;
        if (itemIndex >= totalItems) break;

        bool selected = (itemIndex == selectedIndex);

        if (itemIndex < deviceRouteCount) {
            // Route item - show destination name (with ~ if inactive)
            int routeIdx = deviceRouteIndices[itemIndex];
            const Route* route = routeManager.getRoute(routeIdx);
            if (route) {
                if (deviceRouteActive[itemIndex]) {
                    uiDriver->drawListItem(route->destName, selected, row);
                } else {
                    snprintf(nameBuf, sizeof(nameBuf), "~%s", route->destName);
                    uiDriver->drawListItem(nameBuf, selected, row);
                }
            }
        } else if (itemIndex == deviceRouteCount) {
            uiDriver->drawListItem("+ add route", selected, row);
        } else {
            uiDriver->drawListItem("< back", selected, row);
        }
    }

    // Scroll indicators
    bool canScrollUp = (viewStart > 0);
    bool canScrollDown = (viewStart + VISIBLE_ROWS < totalItems);
    uiDriver->drawScrollIndicators(canScrollUp, canScrollDown);

    uiDriver->endFrame();
}

void handleDeleteRouteInput(InputEvent event) {
    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) selectedIndex--;
            break;
        case InputEvent::DOWN:
            if (selectedIndex < 1) selectedIndex++;
            break;
        case InputEvent::ENTER:
            if (selectedIndex == 1) {
                // "yes" - delete the route
                routeManager.removeRouteByIndex(deleteRouteIndex);
            }
            // Both options return to device routes page
            currentPage = Page::DEVICE_ROUTES;
            selectedIndex = 0;
            viewStart = 0;
            // LED updated after updateDeviceRoutes() runs
            break;
        default:
            break;
    }
}

void renderDeleteRoutePage() {
    uiDriver->beginFrame();
    uiDriver->drawHeader("delete");
    uiDriver->drawListItem("no", selectedIndex == 0, 0);
    uiDriver->drawListItem("yes", selectedIndex == 1, 1);
    uiDriver->endFrame();
}

void updateAddRouteDestinations() {
    // Find all connected devices except the selected source that don't already have a route
    destCount = 0;
    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        if (i != selectedDeviceSlot && deviceManager.isConnected(i)) {
            const MidiDeviceInfo* destInfo = deviceManager.getDeviceBySlot(i);
            if (destInfo) {
                // Skip if route already exists
                if (routeManager.shouldRoute(selectedDeviceVid, selectedDevicePid,
                                              destInfo->vid, destInfo->pid)) {
                    continue;
                }
                destSlots[destCount++] = i;
            }
        }
    }

    // Total items: destinations + "< back"
    int totalItems = destCount + 1;

    // Clamp selection and update view
    if (selectedIndex >= totalItems) {
        selectedIndex = totalItems - 1;
    }
    if (selectedIndex < 0) {
        selectedIndex = 0;
    }
    updateViewForSelection(totalItems);
}

void handleAddRouteInput(InputEvent event) {
    int totalItems = destCount + 1;  // destinations + "< back"

    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                updateViewForSelection(totalItems);
            }
            break;
        case InputEvent::DOWN:
            if (selectedIndex < totalItems - 1) {
                selectedIndex++;
                updateViewForSelection(totalItems);
            }
            break;
        case InputEvent::ENTER:
            if (selectedIndex == totalItems - 1) {
                // "< back" - return to device routes
                currentPage = Page::DEVICE_ROUTES;
                selectedIndex = 0;
                viewStart = 0;
                // LED updated after updateDeviceRoutes() runs
            } else if (selectedIndex < destCount) {
                // Destination selected - create route
                int destSlot = destSlots[selectedIndex];
                const MidiDeviceInfo* destInfo = deviceManager.getDeviceBySlot(destSlot);
                if (destInfo) {
                    routeManager.addRoute(
                        selectedDeviceVid, selectedDevicePid, selectedDeviceName,
                        destInfo->vid, destInfo->pid, destInfo->name
                    );
                }
                currentPage = Page::DEVICE_ROUTES;
                selectedIndex = 0;
                viewStart = 0;
                // LED updated after updateDeviceRoutes() runs
            }
            break;
        default:
            break;
    }
}

void renderAddRoutePage() {
    uiDriver->beginFrame();
    uiDriver->drawHeader("add route");

    int totalItems = destCount + 1;  // destinations + "< back"

    for (int row = 0; row < VISIBLE_ROWS; row++) {
        int itemIndex = viewStart + row;
        if (itemIndex >= totalItems) break;

        bool selected = (itemIndex == selectedIndex);

        if (itemIndex == totalItems - 1) {
            uiDriver->drawListItem("< back", selected, row);
        } else {
            int slot = destSlots[itemIndex];
            const MidiDeviceInfo* info = deviceManager.getDeviceBySlot(slot);
            if (info) {
                uiDriver->drawListItem(info->name, selected, row);
            }
        }
    }

    // Scroll indicators
    bool canScrollUp = (viewStart > 0);
    bool canScrollDown = (viewStart + VISIBLE_ROWS < totalItems);
    uiDriver->drawScrollIndicators(canScrollUp, canScrollDown);

    uiDriver->endFrame();
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
