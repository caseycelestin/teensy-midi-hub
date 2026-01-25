#include "Pages.h"
#include <Arduino.h>

// ============================================
// PageManager Implementation
// ============================================

PageManager::PageManager() : display(nullptr), currentPage(PageId::MAIN_MENU), stackDepth(0),
    selectedSourceSlot(-1), selectedDestSlot(-1),
    selectedSourceVid(0), selectedSourcePid(0), selectedDestVid(0), selectedDestPid(0),
    notificationHead(0), notificationTail(0), notificationEndTime(0) {
    selectedSourceName[0] = '\0';
    selectedDestName[0] = '\0';
    for (int i = 0; i < 5; i++) {
        pages[i] = nullptr;
    }
}

void PageManager::setPage(PageId id, Page* page) {
    pages[static_cast<int>(id)] = page;
}

void PageManager::navigateTo(PageId id) {
    Page* current = pages[static_cast<int>(currentPage)];
    if (current) {
        current->exit();
    }

    // Push current page to stack
    if (stackDepth < 4) {
        pageStack[stackDepth++] = currentPage;
    }

    currentPage = id;
    Page* next = pages[static_cast<int>(id)];
    if (next) {
        next->enter();
    }
}

void PageManager::goBack() {
    if (stackDepth > 0) {
        Page* current = pages[static_cast<int>(currentPage)];
        if (current) {
            current->exit();
        }

        currentPage = pageStack[--stackDepth];
        Page* prev = pages[static_cast<int>(currentPage)];
        if (prev) {
            prev->enter();
        }
    }
}

void PageManager::update() {
    // Check if current notification expired, move to next in queue
    if (notificationHead != notificationTail && millis() >= notificationEndTime) {
        // Advance to next notification
        notificationHead = (notificationHead + 1) % MAX_NOTIFICATIONS;
        if (notificationHead != notificationTail) {
            // More notifications in queue, start timer for next one
            notificationEndTime = millis() + NOTIFICATION_TIMEOUT_MS;
        }
        requestRedraw();
    }

    Page* current = pages[static_cast<int>(currentPage)];
    if (current) {
        current->update();
    }
}

void PageManager::render() {
    Page* current = pages[static_cast<int>(currentPage)];
    if (current && current->needsRender()) {
        current->render();

        // Show notification at the bottom if there's one in the queue
        if (notificationHead != notificationTail && display) {
            display->printNotification(notificationQueue[notificationHead]);
        }

        current->markClean();
    }
}

void PageManager::showNotification(const char* msg) {
    // Check if queue is full
    int nextTail = (notificationTail + 1) % MAX_NOTIFICATIONS;
    if (nextTail == notificationHead) {
        // Queue full, drop oldest notification
        notificationHead = (notificationHead + 1) % MAX_NOTIFICATIONS;
    }

    // Add to queue
    strncpy(notificationQueue[notificationTail], msg, 63);
    notificationQueue[notificationTail][63] = '\0';

    // If this is the first notification, start the timer
    if (notificationHead == notificationTail) {
        notificationEndTime = millis() + NOTIFICATION_TIMEOUT_MS;
    }

    notificationTail = nextTail;
    requestRedraw();
}

void PageManager::requestRedraw() {
    Page* current = pages[static_cast<int>(currentPage)];
    if (current) {
        current->enter();  // This sets needsRedraw = true
    }
}

void PageManager::handleInput(InputEvent event) {
    Page* current = pages[static_cast<int>(currentPage)];
    if (current) {
        current->handleInput(event);
    }
}

// ============================================
// MainMenuPage Implementation
// ============================================

MainMenuPage::MainMenuPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm)
    : Page(pm, disp, inp, dm, rm), selectedIndex(0) {}

void MainMenuPage::enter() {
    Page::enter();
    selectedIndex = 0;
}

void MainMenuPage::render() {
    display->clear();
    display->printHeader("Main Menu");
    display->printMenuItem(0, "Add Route", selectedIndex == 0);
    display->printMenuItem(1, "View Connections", selectedIndex == 1);
    display->printFooter("W/S: navigate, E: select");
}

void MainMenuPage::handleInput(InputEvent event) {
    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                needsRedraw = true;
            }
            break;
        case InputEvent::DOWN:
            if (selectedIndex < MENU_ITEMS - 1) {
                selectedIndex++;
                needsRedraw = true;
            }
            break;
        case InputEvent::ENTER:
            if (selectedIndex == 0) {
                pageManager->navigateTo(PageId::SOURCE_LIST);
            } else {
                pageManager->navigateTo(PageId::CONNECTIONS);
            }
            break;
        default:
            break;
    }
}

// ============================================
// SourceListPage Implementation
// ============================================

SourceListPage::SourceListPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm)
    : Page(pm, disp, inp, dm, rm), selectedIndex(0), connectedCount(0) {}

void SourceListPage::enter() {
    Page::enter();
    selectedIndex = 0;
    refreshDeviceList();
}

void SourceListPage::update() {
    // Check if device list changed
    int oldCount = connectedCount;
    refreshDeviceList();
    if (connectedCount != oldCount) {
        if (selectedIndex >= connectedCount && connectedCount > 0) {
            selectedIndex = connectedCount - 1;
        }
        needsRedraw = true;
    }
}

void SourceListPage::refreshDeviceList() {
    connectedCount = 0;
    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        if (deviceManager->isConnected(i)) {
            connectedSlots[connectedCount++] = i;
        }
    }
}

void SourceListPage::render() {
    display->clear();
    display->printHeader("Select Source");

    if (connectedCount == 0) {
        display->printMessage("No MIDI devices connected");
        display->printMessage("");
    } else {
        for (int i = 0; i < connectedCount; i++) {
            const MidiDeviceInfo* info = deviceManager->getDeviceBySlot(connectedSlots[i]);
            display->printMenuItem(i, info->name, i == selectedIndex);
        }
    }

    display->printMenuItem(connectedCount, "<- Back", selectedIndex == connectedCount);
    display->printFooter("W/S: navigate, E: select");
}

void SourceListPage::handleInput(InputEvent event) {
    int menuSize = connectedCount + 1;  // devices + Back

    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                needsRedraw = true;
            }
            break;
        case InputEvent::DOWN:
            if (selectedIndex < menuSize - 1) {
                selectedIndex++;
                needsRedraw = true;
            }
            break;
        case InputEvent::ENTER:
            if (selectedIndex == connectedCount) {
                // Back selected
                pageManager->goBack();
            } else if (connectedCount > 0) {
                int slot = connectedSlots[selectedIndex];
                const MidiDeviceInfo* info = deviceManager->getDeviceBySlot(slot);
                if (info) {
                    pageManager->setSelectedSource(slot, info->vid, info->pid, info->name);
                    pageManager->navigateTo(PageId::DEST_LIST);
                }
            }
            break;
        default:
            break;
    }
}

// ============================================
// DestListPage Implementation
// ============================================

DestListPage::DestListPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm)
    : Page(pm, disp, inp, dm, rm), selectedIndex(0), availableCount(0) {}

void DestListPage::enter() {
    Page::enter();
    selectedIndex = 0;
    refreshDeviceList();
}

void DestListPage::update() {
    int oldCount = availableCount;
    refreshDeviceList();
    if (availableCount != oldCount) {
        if (selectedIndex >= availableCount && availableCount > 0) {
            selectedIndex = availableCount - 1;
        }
        needsRedraw = true;
    }
}

void DestListPage::refreshDeviceList() {
    int sourceSlot = pageManager->getSelectedSource();
    availableCount = 0;

    for (int i = 0; i < MAX_MIDI_DEVICES; i++) {
        // Exclude the source device
        if (i != sourceSlot && deviceManager->isConnected(i)) {
            availableSlots[availableCount++] = i;
        }
    }
}

void DestListPage::render() {
    display->clear();
    display->printHeader("Select Destination");

    char buf[64];
    snprintf(buf, sizeof(buf), "From: %s", pageManager->getSelectedSourceName());
    display->printMessage(buf);
    display->printMessage("");

    if (availableCount == 0) {
        display->printMessage("No other devices connected");
        display->printMessage("");
    } else {
        for (int i = 0; i < availableCount; i++) {
            const MidiDeviceInfo* info = deviceManager->getDeviceBySlot(availableSlots[i]);
            display->printMenuItem(i, info->name, i == selectedIndex);
        }
    }

    display->printMenuItem(availableCount, "<- Back", selectedIndex == availableCount);
    display->printFooter("W/S: navigate, E: select");
}

void DestListPage::handleInput(InputEvent event) {
    int menuSize = availableCount + 1;  // devices + Back

    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                needsRedraw = true;
            }
            break;
        case InputEvent::DOWN:
            if (selectedIndex < menuSize - 1) {
                selectedIndex++;
                needsRedraw = true;
            }
            break;
        case InputEvent::ENTER:
            if (selectedIndex == availableCount) {
                // Back selected
                pageManager->goBack();
            } else if (availableCount > 0) {
                int slot = availableSlots[selectedIndex];
                const MidiDeviceInfo* info = deviceManager->getDeviceBySlot(slot);
                if (info) {
                    pageManager->setSelectedDest(slot, info->vid, info->pid, info->name);
                    pageManager->navigateTo(PageId::CONFIRM_ROUTE);
                }
            }
            break;
        default:
            break;
    }
}

// ============================================
// ConfirmRoutePage Implementation
// ============================================

ConfirmRoutePage::ConfirmRoutePage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm)
    : Page(pm, disp, inp, dm, rm), selectedIndex(0) {}

void ConfirmRoutePage::enter() {
    Page::enter();
    selectedIndex = 0;
}

void ConfirmRoutePage::render() {
    display->clear();
    display->printHeader("Confirm Route");

    char buf[80];
    snprintf(buf, sizeof(buf), "Route: %s", pageManager->getSelectedSourceName());
    display->printMessage(buf);
    snprintf(buf, sizeof(buf), "    -> %s", pageManager->getSelectedDestName());
    display->printMessage(buf);
    display->printMessage("");

    display->printConfirmation("Create this route?", "Yes, create route", "No, cancel", selectedIndex);

    display->printFooter("W/S: navigate, E: select");
}

void ConfirmRoutePage::handleInput(InputEvent event) {
    switch (event) {
        case InputEvent::UP:
        case InputEvent::DOWN:
            selectedIndex = 1 - selectedIndex;
            needsRedraw = true;
            break;
        case InputEvent::ENTER:
            if (selectedIndex == 0) {
                // Yes - Create the route using stored VID:PID and names
                uint16_t srcVid = pageManager->getSelectedSourceVid();
                uint16_t srcPid = pageManager->getSelectedSourcePid();
                const char* srcName = pageManager->getSelectedSourceName();
                uint16_t dstVid = pageManager->getSelectedDestVid();
                uint16_t dstPid = pageManager->getSelectedDestPid();
                const char* dstName = pageManager->getSelectedDestName();

                bool added = routeManager->addRoute(srcVid, srcPid, srcName, dstVid, dstPid, dstName);
                if (added) {
                    pageManager->showNotification("Route created");
                } else if (routeManager->getRouteCount() >= MAX_ROUTES) {
                    pageManager->showNotification("Max routes reached!");
                } else {
                    pageManager->showNotification("Route already exists");
                }
                // Return to main menu (go back 3 levels: Confirm -> Dest -> Source -> Main)
                pageManager->goBack();
                pageManager->goBack();
                pageManager->goBack();
            } else {
                // No - just go back one level
                pageManager->goBack();
            }
            break;
        default:
            break;
    }
}

// ============================================
// ConnectionsPage Implementation
// ============================================

ConnectionsPage::ConnectionsPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm)
    : Page(pm, disp, inp, dm, rm), selectedIndex(0), confirmingDelete(false), confirmSelection(0) {}

void ConnectionsPage::enter() {
    Page::enter();
    selectedIndex = 0;
    confirmingDelete = false;
    confirmSelection = 0;
}

void ConnectionsPage::render() {
    display->clear();

    int count = routeManager->getRouteCount();

    if (confirmingDelete && selectedIndex < count) {
        // Show delete confirmation
        display->printHeader("Delete Route?");

        const Route* route = routeManager->getRoute(selectedIndex);

        char buf[80];
        snprintf(buf, sizeof(buf), "%s -> %s", route->sourceName, route->destName);
        display->printMessage(buf);
        display->printMessage("");

        display->printConfirmation("Delete this route?", "Yes, delete", "No, cancel", confirmSelection);
        display->printFooter("W/S: navigate, E: select");
    } else {
        // Show route list
        display->printHeader("Connections");

        if (count == 0) {
            display->printMessage("No routes configured");
            display->printMessage("");
        } else {
            for (int i = 0; i < count; i++) {
                const Route* route = routeManager->getRoute(i);
                char buf[80];
                char srcDisplay[40];
                char dstDisplay[40];

                // Check if devices are currently connected
                int srcSlot = deviceManager->findDeviceByVidPid(route->sourceVid, route->sourcePid);
                int dstSlot = deviceManager->findDeviceByVidPid(route->destVid, route->destPid);

                if (srcSlot >= 0) {
                    snprintf(srcDisplay, sizeof(srcDisplay), "%s", route->sourceName);
                } else {
                    snprintf(srcDisplay, sizeof(srcDisplay), "%s (off)", route->sourceName);
                }

                if (dstSlot >= 0) {
                    snprintf(dstDisplay, sizeof(dstDisplay), "%s", route->destName);
                } else {
                    snprintf(dstDisplay, sizeof(dstDisplay), "%s (off)", route->destName);
                }

                snprintf(buf, sizeof(buf), "%s -> %s", srcDisplay, dstDisplay);
                display->printMenuItem(i, buf, i == selectedIndex);
            }
        }

        display->printMenuItem(count, "<- Back", selectedIndex == count);
        display->printFooter("W/S: navigate, E: select");
    }
}

void ConnectionsPage::handleInput(InputEvent event) {
    int count = routeManager->getRouteCount();

    if (confirmingDelete) {
        // Handle confirmation dialog input
        switch (event) {
            case InputEvent::UP:
            case InputEvent::DOWN:
                confirmSelection = 1 - confirmSelection;
                needsRedraw = true;
                break;
            case InputEvent::ENTER:
                if (confirmSelection == 0) {
                    // Yes - delete the route
                    routeManager->removeRouteByIndex(selectedIndex);
                    if (selectedIndex >= routeManager->getRouteCount() && selectedIndex > 0) {
                        selectedIndex--;
                    }
                }
                confirmingDelete = false;
                needsRedraw = true;
                break;
            default:
                break;
        }
        return;
    }

    // Handle route list input
    int menuSize = count + 1;  // routes + Back

    switch (event) {
        case InputEvent::UP:
            if (selectedIndex > 0) {
                selectedIndex--;
                needsRedraw = true;
            }
            break;
        case InputEvent::DOWN:
            if (selectedIndex < menuSize - 1) {
                selectedIndex++;
                needsRedraw = true;
            }
            break;
        case InputEvent::ENTER:
            if (selectedIndex == count) {
                // Back selected
                pageManager->goBack();
            } else if (count > 0) {
                confirmingDelete = true;
                confirmSelection = 0;
                needsRedraw = true;
            }
            break;
        default:
            break;
    }
}
