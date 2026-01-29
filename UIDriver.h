#ifndef UIDRIVER_H
#define UIDRIVER_H

#include "ListItem.h"

// Abstract UI driver interface - allows different display implementations
class UIDriver {
public:
    virtual ~UIDriver() {}

    // Begin a new frame (clear buffer)
    virtual void beginFrame() = 0;

    // Draw the main list view
    virtual void drawList(const ListView& list) = 0;

    // Draw a toast overlay (temporary message)
    // Returns true if toast is still scrolling and needs more time
    virtual bool drawToast(const char* message) = 0;

    // Draw a confirmation dialog overlay
    virtual void drawConfirmation(const char* question,
                                   const char* yesLabel,
                                   const char* noLabel,
                                   bool yesSelected) = 0;

    // Draw screensaver (called instead of normal render when sleeping)
    virtual void drawScreensaver() {}

    // Turn display off completely (deep sleep)
    virtual void displayOff() {}

    // Turn display back on
    virtual void displayOn() {}

    // End frame (flush to display)
    virtual void endFrame() = 0;
};

#endif
