#include "SerialInput.h"
#include <Arduino.h>

SerialInput::SerialInput() : pendingEvent(InputEvent::NONE), escapeStartTime(0), escapeState(0) {
}

bool SerialInput::hasInput() {
    // Check for timeout on escape sequence
    if (escapeState > 0 && (millis() - escapeStartTime > INPUT_TIMEOUT_MS)) {
        escapeState = 0;
    }

    if (pendingEvent != InputEvent::NONE) {
        return true;
    }

    while (Serial.available()) {
        int c = Serial.read();

        // Handle escape sequences for arrow keys
        if (escapeState == 0 && c == 27) {  // ESC
            escapeState = 1;
            escapeStartTime = millis();
            continue;
        }

        if (escapeState == 1) {
            if (c == '[') {
                escapeState = 2;
                continue;
            } else {
                escapeState = 0;
            }
        }

        if (escapeState == 2) {
            escapeState = 0;
            switch (c) {
                case 'A':  // Up arrow
                    pendingEvent = InputEvent::UP;
                    return true;
                case 'B':  // Down arrow
                    pendingEvent = InputEvent::DOWN;
                    return true;
                case 'C':  // Right arrow (treat as ENTER)
                    pendingEvent = InputEvent::ENTER;
                    return true;
            }
            continue;
        }

        // Regular key handling
        switch (c) {
            case 'w':
            case 'W':
                pendingEvent = InputEvent::UP;
                return true;
            case 's':
            case 'S':
                pendingEvent = InputEvent::DOWN;
                return true;
            case 'e':
            case 'E':
            case '\r':
            case '\n':
                pendingEvent = InputEvent::ENTER;
                return true;
        }
    }

    return pendingEvent != InputEvent::NONE;
}

InputEvent SerialInput::getInput() {
    InputEvent result = pendingEvent;
    pendingEvent = InputEvent::NONE;
    return result;
}
