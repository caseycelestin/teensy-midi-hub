#ifndef DISPLAY_H
#define DISPLAY_H

// Abstract display interface - allows different display implementations
class Display {
public:
    virtual ~Display() {}
    virtual void clear() = 0;
    virtual void printHeader(const char* title) = 0;
    virtual void printMenuItem(int index, const char* text, bool selected) = 0;
    virtual void printMessage(const char* msg) = 0;
    virtual void printConfirmation(const char* question, const char* option1, const char* option2, int selected) = 0;
    virtual void printFooter(const char* hint) = 0;
    virtual void printNotification(const char* msg) = 0;
};

#endif
