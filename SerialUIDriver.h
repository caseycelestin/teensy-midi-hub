#ifndef SERIALUIDRIVER_H
#define SERIALUIDRIVER_H

#include <Arduino.h>
#include "UIDriver.h"

// Serial terminal UI driver implementation
class SerialUIDriver : public UIDriver {
public:
    void beginFrame() override {
        // ANSI clear screen and move cursor to top-left
        Serial.print("\033[2J\033[H");
    }

    void endFrame() override {
        // Serial is unbuffered, nothing to flush
    }

    void drawHeader(const char* title) override {
        Serial.println(title);
        Serial.println("---");
    }

    void drawListItem(const char* text, bool selected, int row) override {
        (void)row;  // Row not used for serial output
        Serial.print(selected ? "> " : "  ");
        Serial.println(text);
    }

    void drawScrollIndicators(bool canScrollUp, bool canScrollDown) override {
        if (canScrollUp) Serial.println("  ^ more above");
        if (canScrollDown) Serial.println("  v more below");
    }
};

#endif
