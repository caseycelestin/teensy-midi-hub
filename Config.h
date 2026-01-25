#ifndef CONFIG_H
#define CONFIG_H

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

// Serial input timeout for multi-byte sequences
const unsigned long INPUT_TIMEOUT_MS = 50;

#endif
