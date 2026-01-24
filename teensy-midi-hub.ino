/*
 * Teensy MIDI Hub
 *
 * A USB MIDI hub for Teensy 4.1
 * Detects and displays connected USB MIDI devices via the USB Host port.
 */

#include <USBHost_t36.h>

// USB Host objects
USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);

// MIDI device with BigBuffer for high-speed (480 Mbit) support
MIDIDevice_BigBuffer midi1(myusb);

// Track connection state
bool midiConnected = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // Wait for USB to fully enumerate (helps with WSL/usbipd after upload)
  delay(2000);

  Serial.begin(115200);

  // Wait for serial connection with DTR
  while (!Serial.dtr() && millis() < 5000) {
    delay(10);
  }
  delay(100);  // Extra settling time

  Serial.println("Teensy MIDI Hub");
  Serial.println("===============");

  // Initialize USB Host
  myusb.begin();

  Serial.println("Waiting for USB MIDI devices...");
}

void loop() {
  // Process USB Host tasks
  myusb.Task();

  // Check for MIDI device connection changes
  if (midi1 && !midiConnected) {
    // Device just connected
    midiConnected = true;
    Serial.println();
    Serial.println("MIDI Device Connected!");

    // Print device info
    const uint8_t *mfg = midi1.manufacturer();
    const uint8_t *prod = midi1.product();
    const uint8_t *serial = midi1.serialNumber();

    Serial.print("  Manufacturer: ");
    Serial.println(mfg ? (const char*)mfg : "Unknown");

    Serial.print("  Product:      ");
    Serial.println(prod ? (const char*)prod : "Unknown");

    Serial.print("  Serial:       ");
    Serial.println(serial ? (const char*)serial : "None");

    Serial.print("  VID:PID:      ");
    Serial.print(midi1.idVendor(), HEX);
    Serial.print(":");
    Serial.println(midi1.idProduct(), HEX);
    Serial.println();

  } else if (!midi1 && midiConnected) {
    // Device just disconnected
    midiConnected = false;
    Serial.println("MIDI Device Disconnected");
    Serial.println();
  }

  // Read any incoming MIDI (required to keep connection active)
  midi1.read();

  // Heartbeat LED
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    digitalToggle(LED_BUILTIN);
  }
}
