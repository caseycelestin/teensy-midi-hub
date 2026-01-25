#include "SerialDisplay.h"
#include <Arduino.h>

SerialDisplay::SerialDisplay() {
}

void SerialDisplay::clear() {
    // ANSI clear screen and move cursor to top-left
    Serial.print("\033[2J\033[H");
}

void SerialDisplay::printHeader(const char* title) {
    Serial.println();
    Serial.print("=== MIDI HUB: ");
    Serial.print(title);
    Serial.println(" ===");
    Serial.println();
}

void SerialDisplay::printMenuItem(int index, const char* text, bool selected) {
    if (selected) {
        Serial.print("> ");
    } else {
        Serial.print("  ");
    }
    Serial.println(text);
}

void SerialDisplay::printMessage(const char* msg) {
    Serial.println(msg);
}

void SerialDisplay::printConfirmation(const char* question, const char* option1, const char* option2, int selected) {
    Serial.println(question);
    Serial.println();
    printMenuItem(0, option1, selected == 0);
    printMenuItem(1, option2, selected == 1);
}

void SerialDisplay::printFooter(const char* hint) {
    Serial.println();
    Serial.print("[");
    Serial.print(hint);
    Serial.println("]");
}

void SerialDisplay::printNotification(const char* msg) {
    // Print notification with inverse video (highlighted)
    Serial.print("\033[7m");  // Inverse video on
    Serial.print(" ");
    Serial.print(msg);
    Serial.print(" ");
    Serial.println("\033[0m");  // Reset formatting
}
