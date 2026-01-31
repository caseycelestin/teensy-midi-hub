#ifndef UIDRIVER_H
#define UIDRIVER_H

// Abstract UI driver interface - allows different display implementations
class UIDriver {
public:
    virtual ~UIDriver() {}

    // Begin a new frame (clear buffer)
    virtual void beginFrame() = 0;

    // End frame (flush to display)
    virtual void endFrame() = 0;

    // Core drawing primitives

    // Draw centered header text on row 0 (non-selectable)
    virtual void drawHeader(const char* title) = 0;

    // Draw left-justified list item on rows 1-3
    // row: 0-2 (maps to screen rows 1-3)
    // selected: inverted colors, triggers auto-scroll for long text
    // Auto-scroll state managed internally by driver
    virtual void drawListItem(const char* text, bool selected, int row) = 0;

    // Draw scroll indicators (arrows when list extends beyond visible area)
    virtual void drawScrollIndicators(bool canScrollUp, bool canScrollDown) = 0;

    // Draw screensaver (called instead of normal render when sleeping)
    virtual void drawScreensaver() {}

    // Turn display off completely (deep sleep)
    virtual void displayOff() {}

    // Turn display back on
    virtual void displayOn() {}
};

#endif
