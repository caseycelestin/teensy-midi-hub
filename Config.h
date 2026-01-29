#ifndef CONFIG_H
#define CONFIG_H

// Input device selection (uncomment one)
#define INPUT_QWIIC_TWIST
// #define INPUT_SERIAL

// UI driver selection (uncomment one)
#define UI_OLED
// #define UI_SERIAL

// Maximum MIDI devices supported
#define MAX_MIDI_DEVICES 8

// Maximum number of routes that can be stored
const int MAX_ROUTES = 16;

// EEPROM storage
const int EEPROM_MAGIC = 0x4D52;  // "MR" for MIDI Routes
const int EEPROM_VERSION = 2;  // v2: added device names to routes
const int EEPROM_START_ADDR = 0;

// UI refresh rate
const unsigned long UI_REFRESH_MS = 100;

// Sleep/screensaver timeout (ms) - 0 to disable
const unsigned long SLEEP_TIMEOUT_MS = 30000;  // 30 seconds

// Deep sleep timeout (ms after screensaver starts) - display turns off completely
const unsigned long DEEP_SLEEP_TIMEOUT_MS = 600000;  // 10 minutes after screensaver

// Serial input timeout for multi-byte sequences
const unsigned long INPUT_TIMEOUT_MS = 50;

#endif
