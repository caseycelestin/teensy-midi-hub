#ifndef SERIAL_INPUT_H
#define SERIAL_INPUT_H

#include "Input.h"
#include "Config.h"

// Serial-based input implementation
// Commands:
//   w/W or Up Arrow    = UP
//   s/S or Down Arrow  = DOWN
//   e/E or Enter       = ENTER
//   q/Q or ESC         = BACK
class SerialInput : public Input {
public:
    SerialInput();
    bool hasInput() override;
    InputEvent getInput() override;

private:
    InputEvent pendingEvent;
    unsigned long escapeStartTime;
    int escapeState;  // 0=none, 1=got ESC, 2=got [

    void processEscapeSequence();
};

#endif
