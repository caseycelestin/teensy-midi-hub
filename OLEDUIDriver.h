#ifndef OLEDUIDRIVER_H
#define OLEDUIDRIVER_H

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Font configuration - change this to use a different font
#define UI_FONT FreeMonoBold9pt7b
#include "UIDriver.h"

// OLED UI driver implementation for 128x64 SSD1306
class OLEDUIDriver : public UIDriver {
public:
    OLEDUIDriver() : oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1), initialized(false),
                     lastSelectedRow(-1), scrollOffset(0), lastScrollTime(0), scrollPauseUntil(0),
                     ballX(20), ballY(20), ballVx(1), ballVy(1), lastBallUpdate(0) {}

    bool begin() {
        Wire.begin();

        // Try 0x3D first (common for Adafruit 1.3" OLED), then 0x3C
        if (!oled.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS_ALT)) {
            if (!oled.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
                return false;
            }
        }

        initialized = true;
        oled.clearDisplay();
        oled.setFont(&UI_FONT);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(8, 36);
        oled.print("MIDI Hub");
        oled.display();
        delay(500);

        return true;
    }

    void beginFrame() override {
        if (!initialized) return;
        oled.clearDisplay();
        oled.setFont(&UI_FONT);
        oled.setTextWrap(false);
    }

    void endFrame() override {
        if (!initialized) return;
        oled.display();
    }

    void drawHeader(const char* title) override {
        if (!initialized) return;

        int width = getTextWidth(title);
        int x = (SCREEN_WIDTH - width) / 2;
        oled.setCursor(x, FONT_HEIGHT);  // Row 0 baseline
        oled.setTextColor(SSD1306_WHITE);
        oled.print(title);
    }

    void drawListItem(const char* text, bool selected, int row) override {
        if (!initialized) return;
        if (row < 0 || row > 2) return;  // Only rows 0-2 (screen rows 1-3)

        int y = (row + 1) * ROW_HEIGHT + FONT_HEIGHT - 1;  // Rows 1-3 baseline (shifted up 1px for descenders)
        int boxTop = (row + 1) * ROW_HEIGHT;

        // Reset scroll when selection changes to a different row
        if (selected && row != lastSelectedRow) {
            lastSelectedRow = row;
            scrollOffset = 0;
            scrollPauseUntil = millis() + SCROLL_INITIAL_PAUSE;
        }

        if (selected) {
            oled.fillRect(0, boxTop, SCREEN_WIDTH, ROW_HEIGHT, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);

            // Auto-scroll long text
            int textWidth = getTextWidth(text);
            if (textWidth > SCREEN_WIDTH - LEFT_PADDING * 2) {
                updateScroll();
                // Calculate display offset using modulo for smooth wrap-around
                int maxScroll = textWidth - SCREEN_WIDTH + LEFT_PADDING * 2 + 20;
                int totalCycle = maxScroll + SCROLL_RESET_PAUSE_PIXELS;
                int displayOffset = scrollOffset % totalCycle;
                if (displayOffset > maxScroll) displayOffset = 0;  // Pause at start before repeat
                oled.setCursor(LEFT_PADDING - displayOffset, y);
            } else {
                oled.setCursor(LEFT_PADDING, y);
            }
        } else {
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(LEFT_PADDING, y);
        }

        oled.print(text);
    }

    void drawScrollIndicators(bool canScrollUp, bool canScrollDown) override {
        if (!initialized) return;

        const int arrowSize = 3;
        const int rightPadding = 3;  // Padding from right edge
        const int arrowX = SCREEN_WIDTH - rightPadding - arrowSize;  // Center of arrow
        const int maskWidth = arrowSize * 2 + rightPadding * 2;  // Equal padding on both sides
        const int maskX = SCREEN_WIDTH - maskWidth;

        if (canScrollUp) {
            // Up arrow in first list row (row 1)
            int rowTop = ROW_HEIGHT;  // Row 1 starts here
            // Draw black box to mask text - full row height
            oled.fillRect(maskX, rowTop, maskWidth, ROW_HEIGHT, SSD1306_BLACK);
            // Draw arrow centered in row
            int arrowY = rowTop + ROW_HEIGHT / 2;
            oled.fillTriangle(
                arrowX, arrowY - arrowSize,
                arrowX - arrowSize, arrowY + arrowSize,
                arrowX + arrowSize, arrowY + arrowSize,
                SSD1306_WHITE
            );
        }

        if (canScrollDown) {
            // Down arrow in last list row (row 3)
            int rowTop = ROW_HEIGHT * 3;  // Row 3 starts here
            // Draw black box to mask text - full row height
            oled.fillRect(maskX, rowTop, maskWidth, ROW_HEIGHT, SSD1306_BLACK);
            // Draw arrow centered in row
            int arrowY = rowTop + ROW_HEIGHT / 2;
            oled.fillTriangle(
                arrowX, arrowY + arrowSize,
                arrowX - arrowSize, arrowY - arrowSize,
                arrowX + arrowSize, arrowY - arrowSize,
                SSD1306_WHITE
            );
        }
    }

    void drawScreensaver() override {
        if (!initialized) return;

        // Update ball position
        unsigned long now = millis();
        if (now - lastBallUpdate >= BALL_UPDATE_MS) {
            lastBallUpdate = now;

            ballX += ballVx;
            ballY += ballVy;

            // Bounce off edges
            if (ballX <= 0 || ballX >= SCREEN_WIDTH - BALL_SIZE) {
                ballVx = -ballVx;
                ballX = constrain(ballX, 0, SCREEN_WIDTH - BALL_SIZE);
            }
            if (ballY <= 0 || ballY >= SCREEN_HEIGHT - BALL_SIZE) {
                ballVy = -ballVy;
                ballY = constrain(ballY, 0, SCREEN_HEIGHT - BALL_SIZE);
            }
        }

        // Draw
        oled.clearDisplay();
        oled.fillRect(ballX, ballY, BALL_SIZE, BALL_SIZE, SSD1306_WHITE);
        oled.display();
    }

    void resetScreensaver() {
        // Randomize starting position and direction
        ballX = random(10, SCREEN_WIDTH - 10);
        ballY = random(10, SCREEN_HEIGHT - 10);
        ballVx = random(0, 2) ? 1 : -1;
        ballVy = random(0, 2) ? 1 : -1;
    }

    void displayOff() override {
        if (!initialized) return;
        oled.ssd1306_command(SSD1306_DISPLAYOFF);
    }

    void displayOn() override {
        if (!initialized) return;
        oled.ssd1306_command(SSD1306_DISPLAYON);
    }

private:
    Adafruit_SSD1306 oled;
    bool initialized;

    // List item scroll state (row-based)
    int lastSelectedRow;
    int scrollOffset;
    unsigned long lastScrollTime;
    unsigned long scrollPauseUntil;

    // Screensaver ball state
    int ballX;
    int ballY;
    int ballVx;
    int ballVy;
    unsigned long lastBallUpdate;

    static const int SCREEN_WIDTH = 128;
    static const int SCREEN_HEIGHT = 64;
    static const int I2C_ADDRESS = 0x3C;
    static const int I2C_ADDRESS_ALT = 0x3D;
    static const int FONT_HEIGHT = 13;  // FreeMonoBold9pt
    static const int CHAR_WIDTH_APPROX = 11;  // Approximate char width for this font
    static const int ROW_HEIGHT = 16;   // 64px / 4 rows = 16px per row
    static const int LEFT_PADDING = 2;  // Padding for left-aligned text

    // Scroll settings
    static const int SCROLL_SPEED_MS = 25;       // ms between scroll steps
    static const int SCROLL_STEP = 3;            // pixels per step
    static const int SCROLL_INITIAL_PAUSE = 400; // ms pause before scrolling starts
    static const int SCROLL_RESET_PAUSE_PIXELS = 30;  // "pixels" of pause at end before reset

    // Screensaver settings
    static const int BALL_SIZE = 3;          // 3x3 pixel ball
    static const int BALL_UPDATE_MS = 30;    // Update every 30ms (~33 fps)

    // Get text width for current font
    int getTextWidth(const char* text) {
        return strlen(text) * CHAR_WIDTH_APPROX;
    }

    // Update scroll offset (called each frame for selected long text)
    void updateScroll() {
        unsigned long now = millis();
        if (now >= scrollPauseUntil && now - lastScrollTime >= SCROLL_SPEED_MS) {
            lastScrollTime = now;
            scrollOffset += SCROLL_STEP;
        }
    }
};

#endif
