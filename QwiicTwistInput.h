#ifndef QWIIC_TWIST_INPUT_H
#define QWIIC_TWIST_INPUT_H

#include "Input.h"
#include <SparkFun_Qwiic_Twist_Arduino_Library.h>

// Qwiic Twist rotary encoder input implementation
// - Rotate clockwise = DOWN (next item)
// - Rotate counter-clockwise = UP (previous item)
// - Press button = ENTER (select)
class QwiicTwistInput : public Input {
public:
    QwiicTwistInput();

    bool begin();  // Initialize I2C and device
    bool hasInput() override;
    InputEvent getInput() override;

    // Set LED color (0-255 for each component)
    void setColor(uint8_t r, uint8_t g, uint8_t b);

private:
    TWIST twist;
    InputEvent pendingEvent;
    int16_t lastCount;
    bool initialized;
};

#endif
