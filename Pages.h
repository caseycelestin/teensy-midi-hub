#ifndef PAGES_H
#define PAGES_H

#include "Page.h"

// Main Menu Page - Entry point
class MainMenuPage : public Page {
public:
    MainMenuPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm);
    void enter() override;
    void render() override;
    void handleInput(InputEvent event) override;

private:
    int selectedIndex;
    static const int MENU_ITEMS = 2;
};

// Source List Page - Select source device for routing
class SourceListPage : public Page {
public:
    SourceListPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm);
    void enter() override;
    void update() override;
    void render() override;
    void handleInput(InputEvent event) override;

private:
    int selectedIndex;
    int connectedSlots[MAX_MIDI_DEVICES];
    int connectedCount;

    void refreshDeviceList();
};

// Destination List Page - Select destination device(s) for routing
class DestListPage : public Page {
public:
    DestListPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm);
    void enter() override;
    void update() override;
    void render() override;
    void handleInput(InputEvent event) override;

private:
    int selectedIndex;
    int availableSlots[MAX_MIDI_DEVICES];
    int availableCount;

    void refreshDeviceList();
};

// Confirmation Page - Confirm route creation
class ConfirmRoutePage : public Page {
public:
    ConfirmRoutePage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm);
    void enter() override;
    void render() override;
    void handleInput(InputEvent event) override;

private:
    int selectedIndex;
};

// Connections Page - View and delete existing routes
class ConnectionsPage : public Page {
public:
    ConnectionsPage(PageManager* pm, Display* disp, Input* inp, DeviceManager* dm, RouteManager* rm);
    void enter() override;
    void render() override;
    void handleInput(InputEvent event) override;

private:
    int selectedIndex;
    bool confirmingDelete;
    int confirmSelection;  // 0 = Yes, 1 = No
};

#endif
