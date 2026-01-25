#ifndef SERIAL_DISPLAY_H
#define SERIAL_DISPLAY_H

#include "Display.h"

// Serial terminal display implementation
class SerialDisplay : public Display {
public:
    SerialDisplay();
    void clear() override;
    void printHeader(const char* title) override;
    void printMenuItem(int index, const char* text, bool selected) override;
    void printMessage(const char* msg) override;
    void printConfirmation(const char* question, const char* option1, const char* option2, int selected) override;
    void printFooter(const char* hint) override;
    void printNotification(const char* msg) override;
};

#endif
