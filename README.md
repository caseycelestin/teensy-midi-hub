# Teensy MIDI Hub

A USB MIDI hub for Teensy 4.1 with configurable routing between USB MIDI devices.

## Features

- **Configurable Routing**: Create routes between any connected MIDI devices
- **Persistent Storage**: Routes saved to EEPROM and restored on boot
- **Serial UI**: Text-based menu interface for configuration via terminal
- **Hot-plug Support**: Devices can be connected/disconnected at any time
- **Up to 8 MIDI Devices**: Support for multiple USB MIDI devices via USB hub
- **Up to 16 Routes**: Configure complex routing setups

## Requirements

- Teensy 4.1
- Arduino CLI with Teensy core installed
- teensy_loader_cli
- USB hub (for connecting multiple MIDI devices)

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

## Usage

### Serial Connection

Connect via terminal (115200 baud):

```bash
tio /dev/ttyACM0
```

Quit with `ctrl-t q`. Install tio if needed: `sudo apt install tio`

### Navigation

| Key | Action |
|-----|--------|
| `W` / `Up Arrow` | Move up |
| `S` / `Down Arrow` | Move down |
| `E` / `Enter` | Select |

### Creating a Route

1. From Main Menu, select **Add Route**
2. Select the **source** device (where MIDI comes from)
3. Select the **destination** device (where MIDI goes to)
4. Confirm to create the route

### Managing Routes

1. From Main Menu, select **View Connections**
2. Select a route to delete it
3. Confirm deletion

### Notifications

- Device connect/disconnect events appear at the bottom of the screen
- Shows device name and slot count (e.g., "Launchpad Pro connected! 2/8")
- Disconnected devices in routes show as "(off)"

## Hardware Setup

```
                    ┌─────────────────┐
                    │   Teensy 4.1    │
                    │                 │
   Computer ────────┤ USB Device Port │ (Serial UI only)
                    │                 │
                    │  USB Host Port  ├──── USB Hub
                    └─────────────────┘        │
                                               ├── MIDI Keyboard
                                               ├── MIDI Synth
                                               └── MIDI Controller
```

The computer connection is for configuration only - MIDI is routed directly between devices on the USB Host port.

## Project Structure

```
teensy-midi-hub/
├── teensy-midi-hub.ino   # Main entry point and MIDI routing
├── Config.h              # Configuration constants
├── Input.h               # Input interface
├── SerialInput.*         # Serial input implementation
├── Display.h             # Display interface
├── SerialDisplay.*       # Serial display implementation
├── Page.h                # Page base class and PageManager
├── Pages.*               # UI page implementations
├── DeviceManager.*       # MIDI device tracking
├── RouteManager.*        # Route storage and EEPROM persistence
├── USBDeviceMonitor.*    # Overflow device detection
├── build/                # Compiled output (generated)
└── README.md
```

## Architecture

The UI uses abstracted Input and Display interfaces, allowing future implementations with physical buttons and OLED displays without changing the page logic.

## USB Type Settings

When compiling, ensure USB Type is set to **Serial** (not MIDI). The computer connection is used for the configuration UI only - all MIDI routing happens between USB Host devices.

## License

MIT
