#ifndef PAGE_H
#define PAGE_H

#include <string.h>
#include "Input.h"
#include "Display.h"
#include "DeviceManager.h"
#include "RouteManager.h"

// Forward declaration
class PageManager;

// Page identifiers
enum class PageId {
    MAIN_MENU,
    SOURCE_LIST,
    DEST_LIST,
    CONFIRM_ROUTE,
    CONNECTIONS
};

// Base class for all UI pages
class Page {
public:
    Page(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm)
        : pageManager(pm), display(disp), input(inp), deviceManager(dm), routeManager(rm), needsRedraw(true) {}

    virtual ~Page() {}

    // Called when page becomes active
    virtual void enter() { needsRedraw = true; }

    // Called when page becomes inactive
    virtual void exit() {}

    // Called each frame to handle logic
    virtual void update() {}

    // Called to render the page
    virtual void render() = 0;

    // Called when input is received
    virtual void handleInput(InputEvent event) = 0;

    // Check if page needs to be redrawn
    bool needsRender() const { return needsRedraw; }
    void markClean() { needsRedraw = false; }

protected:
    PageManager* pageManager;
    Display* display;
    Input* input;
    DeviceManager* deviceManager;
    RouteManager* routeManager;
    bool needsRedraw;
};

// Notification settings
const unsigned long NOTIFICATION_TIMEOUT_MS = 2000;
const int MAX_NOTIFICATIONS = 8;

// Manages page navigation and notifications
class PageManager {
public:
    PageManager();

    void setDisplay(Display* disp) { display = disp; }
    void setPage(PageId id, Page* page);
    void navigateTo(PageId id);
    void goBack();

    void update();
    void render();
    void handleInput(InputEvent event);

    // Show a notification that auto-dismisses
    void showNotification(const char* msg);

    // Force current page to redraw
    void requestRedraw();

    // Data passing between pages - stores slot, VID:PID, and name
    void setSelectedSource(int slot, uint16_t vid, uint16_t pid, const char* name) {
        selectedSourceSlot = slot;
        selectedSourceVid = vid;
        selectedSourcePid = pid;
        strncpy(selectedSourceName, name, sizeof(selectedSourceName) - 1);
        selectedSourceName[sizeof(selectedSourceName) - 1] = '\0';
    }
    int getSelectedSource() const { return selectedSourceSlot; }
    uint16_t getSelectedSourceVid() const { return selectedSourceVid; }
    uint16_t getSelectedSourcePid() const { return selectedSourcePid; }
    const char* getSelectedSourceName() const { return selectedSourceName; }

    void setSelectedDest(int slot, uint16_t vid, uint16_t pid, const char* name) {
        selectedDestSlot = slot;
        selectedDestVid = vid;
        selectedDestPid = pid;
        strncpy(selectedDestName, name, sizeof(selectedDestName) - 1);
        selectedDestName[sizeof(selectedDestName) - 1] = '\0';
    }
    int getSelectedDest() const { return selectedDestSlot; }
    uint16_t getSelectedDestVid() const { return selectedDestVid; }
    uint16_t getSelectedDestPid() const { return selectedDestPid; }
    const char* getSelectedDestName() const { return selectedDestName; }

private:
    Page* pages[5];  // One for each PageId
    Display* display;
    PageId currentPage;
    PageId pageStack[4];
    int stackDepth;

    // Shared state for route creation flow
    int selectedSourceSlot;
    int selectedDestSlot;
    uint16_t selectedSourceVid;
    uint16_t selectedSourcePid;
    uint16_t selectedDestVid;
    uint16_t selectedDestPid;
    char selectedSourceName[32];
    char selectedDestName[32];

    // Notification queue
    char notificationQueue[MAX_NOTIFICATIONS][64];
    int notificationHead;
    int notificationTail;
    unsigned long notificationEndTime;
};

#endif
