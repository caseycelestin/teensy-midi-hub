#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

// Input events that can be generated
enum class InputEvent {
    NONE,
    UP,
    DOWN,
    ENTER
};

// Abstract input interface - allows different input implementations
class Input {
public:
    virtual ~Input() {}
    virtual bool hasInput() = 0;
    virtual InputEvent getInput() = 0;

    // Optional: Set LED color (for inputs with RGB LED like Qwiic Twist)
    virtual void setColor(uint8_t r, uint8_t g, uint8_t b) { (void)r; (void)g; (void)b; }
};

#endif
