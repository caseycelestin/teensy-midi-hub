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

    void drawList(const ListView& list) override {
        Serial.println();

        for (int i = 0; i < list.count; i++) {
            const ListItem& item = list.items[i];
            bool selected = (i == list.selectedIndex);

            // Selection indicator
            if (selected) {
                Serial.print("> ");
            } else {
                Serial.print("  ");
            }

            // Left text
            if (item.left) {
                Serial.print(item.left);
                Serial.print(" ");
            }

            // Center text
            if (item.center) {
                Serial.print(item.center);
            }

            // Right text
            if (item.right) {
                Serial.print(" ");
                Serial.print(item.right);
            }

            Serial.println();
        }

        Serial.println();
    }

    bool drawToast(const char* message) override {
        // Draw centered box with message
        int len = strlen(message);
        int boxWidth = len + 4;

        Serial.println();

        // Top border
        Serial.print("  +");
        for (int i = 0; i < boxWidth - 2; i++) Serial.print("-");
        Serial.println("+");

        // Message line
        Serial.print("  | ");
        Serial.print(message);
        Serial.println(" |");

        // Bottom border
        Serial.print("  +");
        for (int i = 0; i < boxWidth - 2; i++) Serial.print("-");
        Serial.println("+");

        return false;  // Serial doesn't scroll
    }

    void drawConfirmation(const char* question,
                           const char* yesLabel,
                           const char* noLabel,
                           bool yesSelected) override {
        // Calculate box width
        int qLen = strlen(question);
        int yLen = strlen(yesLabel);
        int nLen = strlen(noLabel);
        int optionsLen = yLen + nLen + 7;  // "[Yes]  [No]" format
        int contentWidth = (qLen > optionsLen) ? qLen : optionsLen;
        int boxWidth = contentWidth + 4;

        Serial.println();

        // Top border
        Serial.print("  +");
        for (int i = 0; i < boxWidth - 2; i++) Serial.print("-");
        Serial.println("+");

        // Question line (centered)
        Serial.print("  | ");
        int qPad = (contentWidth - qLen) / 2;
        for (int i = 0; i < qPad; i++) Serial.print(" ");
        Serial.print(question);
        for (int i = 0; i < contentWidth - qLen - qPad; i++) Serial.print(" ");
        Serial.println(" |");

        // Empty line
        Serial.print("  |");
        for (int i = 0; i < boxWidth - 2; i++) Serial.print(" ");
        Serial.println("|");

        // Options line
        Serial.print("  | ");
        if (yesSelected) {
            Serial.print("[");
            Serial.print(yesLabel);
            Serial.print("]  ");
            Serial.print(noLabel);
        } else {
            Serial.print(yesLabel);
            Serial.print("  [");
            Serial.print(noLabel);
            Serial.print("]");
        }
        // Pad to fill box
        int usedWidth = yLen + nLen + 5;
        for (int i = usedWidth; i < contentWidth; i++) Serial.print(" ");
        Serial.println(" |");

        // Bottom border
        Serial.print("  +");
        for (int i = 0; i < boxWidth - 2; i++) Serial.print("-");
        Serial.println("+");
    }

    void endFrame() override {
        // Serial is unbuffered, nothing to flush
    }
};

#endif
