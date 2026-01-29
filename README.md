# Teensy MIDI Hub

A USB MIDI hub for Teensy 4.1 with configurable routing between USB MIDI devices.

## Features

- **Configurable Routing**: Create routes between any connected MIDI devices
- **Persistent Storage**: Routes saved to EEPROM and restored on boot
- **OLED Display**: 128x64 SSD1306 display with scrolling text and animations
- **Serial UI**: Text-based fallback interface for configuration via terminal
- **Hot-plug Support**: Devices can be connected/disconnected at any time
- **Up to 8 MIDI Devices**: Support for multiple USB MIDI devices via USB hub
- **Up to 16 Routes**: Configure complex routing setups
- **Screensaver & Sleep**: Bouncing ball screensaver, deep sleep for OLED longevity
- **Status LED**: Qwiic Twist LED indicates route status (red = disconnected device)

## Requirements

- Teensy 4.1
- Arduino CLI with Teensy core installed
- teensy_loader_cli
- USB hub (for connecting multiple MIDI devices)

### Optional Hardware

- **OLED Display**: 128x64 SSD1306 I2C display (0x3C or 0x3D address)
- **Qwiic Twist**: SparkFun Qwiic Twist RGB rotary encoder for input

### Libraries

```bash
# For OLED display
arduino-cli lib install "Adafruit SSD1306"
arduino-cli lib install "Adafruit GFX Library"

# For Qwiic Twist input
arduino-cli lib install "SparkFun Qwiic Twist Arduino Library"
```

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

### Configuration

Input and display types are selected at compile time in `Config.h`:

```c
// Input device selection (uncomment one)
#define INPUT_QWIIC_TWIST
// #define INPUT_SERIAL

// UI driver selection (uncomment one)
#define UI_OLED
// #define UI_SERIAL
```

**Input Options:**
- **Serial Input** - Use keyboard over serial terminal
- **Qwiic Twist Input** - SparkFun Qwiic Twist RGB rotary encoder

**Display Options:**
- **OLED** - 128x64 SSD1306 display with animations
- **Serial** - Text-based terminal interface

### I2C Wiring

Both OLED and Qwiic Twist use I2C:
- SDA → Pin 18
- SCL → Pin 19
- 3.3V → 3.3V
- GND → GND

Or use Qwiic cables with a breakout board for easy daisy-chaining.

### Navigation

**Serial Input:**

| Key | Action |
|-----|--------|
| `W` / `Up Arrow` | Move up |
| `S` / `Down Arrow` | Move down |
| `E` / `Enter` | Select |

**Qwiic Twist Input:**

| Action | Result |
|--------|--------|
| Rotate counter-clockwise | Move up |
| Rotate clockwise | Move down |
| Press button | Select |

### Creating a Route

1. From Routes page, select **+** (add route)
2. Select the **source** device from the sources list
3. Select the **destination** device from the sinks list
4. Route is created immediately

### Managing Routes

1. From Routes page, select an existing route
2. Confirm deletion when prompted

### Notifications

- Toast messages appear for device connect/disconnect (e.g., "+ launchpad pro")
- Long messages scroll automatically
- Routes with disconnected devices show red LED on Qwiic Twist

### Sleep Mode

- **Screensaver**: Bouncing ball after 30 seconds of inactivity
- **Deep Sleep**: Display turns off after 10 minutes of screensaver
- Any input wakes the display

## Hardware Setup

```
                    ┌─────────────────┐
                    │   Teensy 4.1    │
                    │                 │
   Computer ────────┤ USB Device Port │ (Serial debug only)
                    │                 │
                    │  USB Host Port  ├──── USB Hub
                    │                 │        │
                    │   I2C (18/19)   │        ├── MIDI Keyboard
                    └────────┬────────┘        ├── MIDI Synth
                             │                 └── MIDI Controller
                    ┌────────┴────────┐
                    │   I2C Devices   │
                    │                 │
                    ├── OLED Display  │ (0x3C or 0x3D)
                    └── Qwiic Twist   │ (0x3F)
```

The computer connection is for serial debug only - MIDI is routed directly between devices on the USB Host port. The OLED and Qwiic Twist can be daisy-chained via Qwiic connectors.

## Project Structure

```
teensy-midi-hub/
├── teensy-midi-hub.ino   # Main entry point, state machine, MIDI routing
├── Config.h              # Configuration constants
├── Input.h               # Input interface
├── SerialInput.*         # Serial/keyboard input implementation
├── QwiicTwistInput.*     # Qwiic Twist rotary encoder input (with RGB LED)
├── UIDriver.h            # Abstract UI driver interface
├── UIManager.h           # Central UI controller (lists, toasts, dialogs, sleep)
├── ListItem.h            # ListView and ListItem data structures
├── OLEDUIDriver.h        # OLED display driver with scrolling/animations
├── SerialUIDriver.h      # Serial terminal display driver
├── DeviceManager.*       # MIDI device tracking
├── RouteManager.*        # Route storage and EEPROM persistence
├── USBDeviceMonitor.*    # Overflow device detection
├── build/                # Compiled output (generated)
└── README.md
```

## Architecture

The UI uses a data-driven architecture:

- **UIManager** - Central controller managing ListView, toast queue, confirmation dialogs, and sleep state
- **UIDriver** - Abstract interface for display implementations (OLED, Serial)
- **Input** - Abstract interface for input implementations (Qwiic Twist, Serial)

Navigation is handled by a simple state machine in the main sketch, with UIManager handling overlays (toasts, confirmations) and sleep transitions.

## USB Type Settings

When compiling, ensure USB Type is set to **Serial** (not MIDI). The computer connection is used for the configuration UI only - all MIDI routing happens between USB Host devices.

## License

MIT
