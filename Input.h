#ifndef INPUT_H
#define INPUT_H

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
};

#endif
