# Teensy MIDI Hub

A USB MIDI hub for Teensy 4.1 that routes MIDI messages between USB host and device ports.

## Requirements

- Teensy 4.1
- Arduino CLI with Teensy core installed
- teensy_loader_cli

## Building

Compile the project:

```bash
arduino-cli compile --fqbn teensy:avr:teensy41 --output-dir build .
```

## Uploading

1. Connect your Teensy 4.1 via USB

2. Upload the firmware:
   ```bash
   teensy_loader_cli --mcu=TEENSY41 -w -v build/teensy-midi-hub.ino.hex
   ```

3. Press the button on the Teensy to enter bootloader mode

## Serial Monitor

Use `tio` for serial monitoring (auto-reconnects on replug/reprogram):

```bash
tio /dev/ttyACM0
```

Quit with `ctrl-t q`.

Install tio if needed: `sudo apt install tio`

## USB Tools Menu Settings

When compiling, ensure these USB Type settings in Arduino IDE or via build flags:
- **USB Type**: MIDI (or Serial + MIDI for debugging)

## Project Structure

```
teensy-midi-hub/
├── teensy-midi-hub.ino   # Main entry point
├── build/                # Compiled output (generated)
└── README.md             # This file
```

## License

MIT
