#include "QwiicTwistInput.h"
#include <Wire.h>

QwiicTwistInput::QwiicTwistInput()
    : pendingEvent(InputEvent::NONE), lastCount(0), initialized(false), lastClickTime(0) {
}

bool QwiicTwistInput::begin() {
    Wire.begin();

    if (!twist.begin()) {
        return false;
    }

    initialized = true;
    lastCount = twist.getCount();

    // Set initial color (dim blue to indicate ready)
    twist.setColor(0, 0, 30);

    return true;
}

bool QwiicTwistInput::hasInput() {
    if (!initialized) return false;

    if (pendingEvent != InputEvent::NONE) {
        return true;
    }

    // Check for button click with debounce
    if (twist.isClicked()) {
        unsigned long now = millis();
        if (now - lastClickTime >= CLICK_DEBOUNCE_MS) {
            lastClickTime = now;
            pendingEvent = InputEvent::ENTER;
            return true;
        }
    }

    // Check for rotation
    int16_t currentCount = twist.getCount();
    int16_t diff = currentCount - lastCount;

    if (diff != 0) {
        lastCount = currentCount;

        if (diff > 0) {
            // Clockwise = DOWN (next item in list)
            pendingEvent = InputEvent::DOWN;
        } else {
            // Counter-clockwise = UP (previous item in list)
            pendingEvent = InputEvent::UP;
        }
        return true;
    }

    return false;
}

InputEvent QwiicTwistInput::getInput() {
    InputEvent result = pendingEvent;
    pendingEvent = InputEvent::NONE;
    return result;
}

void QwiicTwistInput::setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (initialized) {
        twist.setColor(r, g, b);
    }
}
