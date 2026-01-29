#ifndef UIMANAGER_H
#define UIMANAGER_H

#include <Arduino.h>
#include <string.h>
#include "Config.h"
#include "ListItem.h"
#include "UIDriver.h"
#include "Input.h"

// Confirmation callback type
typedef void (*ConfirmCallback)(bool confirmed);

// Toast duration
const unsigned long TOAST_DURATION_MS = 2000;

// Maximum queued toasts
const int MAX_TOASTS = 8;

// Central UI controller
class UIManager {
public:
    UIManager() : driver(nullptr), needsRedraw(true),
                  toastHead(0), toastTail(0), toastEndTime(0), toastScrolling(false),
                  confirmActive(false), confirmYesSelected(true), confirmCallback(nullptr),
                  lastActivityTime(0), sleeping(false), deepSleeping(false), sleepStartTime(0) {
        confirmQuestion[0] = '\0';
        confirmYes[0] = '\0';
        confirmNo[0] = '\0';
    }

    void setDriver(UIDriver* d) { driver = d; }

    // Access the list view for building UI
    ListView& getList() { return list; }

    // Mark UI as needing redraw
    void requestRedraw() { needsRedraw = true; }

    // Show a temporary toast message
    void showToast(const char* message) {
        int nextTail = (toastTail + 1) % MAX_TOASTS;
        if (nextTail == toastHead) {
            // Queue full, drop oldest
            toastHead = (toastHead + 1) % MAX_TOASTS;
        }

        strncpy(toastQueue[toastTail], message, 63);
        toastQueue[toastTail][63] = '\0';

        if (toastHead == toastTail) {
            toastEndTime = millis() + TOAST_DURATION_MS;
        }

        toastTail = nextTail;
        needsRedraw = true;
    }

    // Show a confirmation dialog (modal)
    void showConfirmation(const char* question, const char* yes, const char* no, ConfirmCallback callback) {
        strncpy(confirmQuestion, question, 63);
        confirmQuestion[63] = '\0';
        strncpy(confirmYes, yes, 31);
        confirmYes[31] = '\0';
        strncpy(confirmNo, no, 31);
        confirmNo[31] = '\0';

        confirmCallback = callback;
        confirmYesSelected = true;  // Default to "Yes"
        confirmActive = true;
        needsRedraw = true;
    }

    // Check if confirmation is active
    bool isConfirmActive() const { return confirmActive; }

    // Check if sleeping (screensaver active)
    bool isSleeping() const { return sleeping; }

    // Register activity (resets sleep timer, wakes if sleeping)
    void activity() {
        lastActivityTime = millis();
        if (deepSleeping) {
            deepSleeping = false;
            sleeping = false;
            if (driver) driver->displayOn();
            needsRedraw = true;
        } else if (sleeping) {
            sleeping = false;
            needsRedraw = true;
        }
    }

    // Handle input (confirmation captures when active)
    // Returns true if input was consumed
    bool handleInput(InputEvent event) {
        if (!confirmActive) return false;

        switch (event) {
            case InputEvent::UP:
            case InputEvent::DOWN:
                confirmYesSelected = !confirmYesSelected;
                needsRedraw = true;
                return true;

            case InputEvent::ENTER:
                confirmActive = false;
                if (confirmCallback) {
                    confirmCallback(confirmYesSelected);
                }
                needsRedraw = true;
                return true;

            default:
                return false;
        }
    }

    // Update toast expiration and sleep state
    void update() {
        // Only expire toast if time is up AND scrolling is complete
        if (toastHead != toastTail && millis() >= toastEndTime && !toastScrolling) {
            toastHead = (toastHead + 1) % MAX_TOASTS;
            if (toastHead != toastTail) {
                toastEndTime = millis() + TOAST_DURATION_MS;
            }
            needsRedraw = true;
        }

        // Check for sleep timeout
        if (SLEEP_TIMEOUT_MS > 0 && !sleeping && !deepSleeping) {
            if (millis() - lastActivityTime >= SLEEP_TIMEOUT_MS) {
                sleeping = true;
                sleepStartTime = millis();
            }
        }

        // Check for deep sleep timeout (after screensaver has been running)
        if (DEEP_SLEEP_TIMEOUT_MS > 0 && sleeping && !deepSleeping) {
            if (millis() - sleepStartTime >= DEEP_SLEEP_TIMEOUT_MS) {
                deepSleeping = true;
                if (driver) driver->displayOff();
            }
        }
    }

    // Render the UI (always render to support scrolling animations)
    void render() {
        if (!driver) return;

        // Deep sleep - display is off, nothing to render
        if (deepSleeping) {
            return;
        }

        // Screensaver mode
        if (sleeping) {
            driver->drawScreensaver();
            return;
        }

        driver->beginFrame();

        // Draw the list
        driver->drawList(list);

        // Draw confirmation overlay if active
        if (confirmActive) {
            driver->drawConfirmation(confirmQuestion, confirmYes, confirmNo, confirmYesSelected);
        }
        // Draw toast overlay if present (but not during confirmation)
        else if (toastHead != toastTail) {
            toastScrolling = driver->drawToast(toastQueue[toastHead]);
        }

        driver->endFrame();
        needsRedraw = false;
    }

private:
    UIDriver* driver;
    ListView list;
    bool needsRedraw;

    // Toast queue
    char toastQueue[MAX_TOASTS][64];
    int toastHead;
    int toastTail;
    unsigned long toastEndTime;
    bool toastScrolling;

    // Confirmation state
    bool confirmActive;
    char confirmQuestion[64];
    char confirmYes[32];
    char confirmNo[32];
    bool confirmYesSelected;
    ConfirmCallback confirmCallback;

    // Sleep state
    unsigned long lastActivityTime;
    bool sleeping;
    bool deepSleeping;
    unsigned long sleepStartTime;
};

#endif
