#ifndef OLEDUIDRIVER_H
#define OLEDUIDRIVER_H

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "UIDriver.h"

// OLED UI driver implementation for 128x64 SSD1306
class OLEDUIDriver : public UIDriver {
public:
    OLEDUIDriver() : oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1), initialized(false),
                     lastSelectedIndex(-1), scrollOffset(0), lastScrollTime(0), scrollPauseUntil(0),
                     lastToastMessage(nullptr), toastScrollOffset(0), toastScrollComplete(false),
                     lastToastScrollTime(0), toastScrollPauseUntil(0),
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
        oled.setFont(&FreeMonoBold9pt7b);
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
    }

    void drawList(const ListView& list) override {
        if (!initialized) return;

        // Check if selection changed
        if (list.selectedIndex != lastSelectedIndex) {
            lastSelectedIndex = list.selectedIndex;
            scrollOffset = 0;
            scrollPauseUntil = millis() + SCROLL_INITIAL_PAUSE;
        }

        // Update horizontal scroll for selected item text
        unsigned long now = millis();
        if (now >= scrollPauseUntil && now - lastScrollTime >= SCROLL_SPEED_MS) {
            lastScrollTime = now;
            scrollOffset += SCROLL_STEP;
        }

        // Calculate vertical scroll to keep selection visible
        int viewStart = 0;
        if (list.selectedIndex >= VISIBLE_ITEMS) {
            viewStart = list.selectedIndex - VISIBLE_ITEMS + 1;
        }
        int viewEnd = viewStart + VISIBLE_ITEMS;
        if (viewEnd > list.count) viewEnd = list.count;

        oled.setFont(&FreeMonoBold9pt7b);
        oled.setTextWrap(false);  // Prevent text from wrapping to next line

        for (int i = viewStart; i < viewEnd; i++) {
            int rowIndex = i - viewStart;
            int boxTop = rowIndex * ROW_HEIGHT;
            int y = boxTop + FONT_HEIGHT;  // Text baseline

            const ListItem& item = list.items[i];
            bool selected = (i == list.selectedIndex);

            // Selection highlight
            if (selected) {
                oled.fillRect(0, boxTop, SCREEN_WIDTH, ROW_HEIGHT, SSD1306_WHITE);
                oled.setTextColor(SSD1306_BLACK);
            } else {
                oled.setTextColor(SSD1306_WHITE);
            }

            // Calculate total text width
            int leftWidth = item.left ? strlen(item.left) * CHAR_WIDTH_APPROX : 0;
            int centerWidth = item.center ? strlen(item.center) * CHAR_WIDTH_APPROX : 0;
            int rightWidth = item.right ? strlen(item.right) * CHAR_WIDTH_APPROX : 0;

            // For items with left text only (typical list items)
            if (item.left && !item.center && !item.right) {
                int textWidth = leftWidth;
                if (selected && textWidth > SCREEN_WIDTH - 4) {
                    // Scroll: calculate offset, wrap around
                    int maxScroll = textWidth - SCREEN_WIDTH + 20;  // 20px padding at end
                    int offset = scrollOffset % (maxScroll + SCROLL_RESET_PAUSE_PIXELS);
                    if (offset > maxScroll) offset = 0;  // Pause at end
                    oled.setCursor(LEFT_PADDING - offset, y);
                } else {
                    oled.setCursor(LEFT_PADDING, y);
                }
                oled.print(item.left);
            } else {
                // Mixed layout (left + center + right)
                if (item.left) {
                    oled.setCursor(LEFT_PADDING, y);
                    oled.print(item.left);
                }

                if (item.center) {
                    int centerX = (SCREEN_WIDTH - centerWidth) / 2;
                    if (centerX < 2) centerX = 2;
                    oled.setCursor(centerX, y);
                    oled.print(item.center);
                }

                if (item.right) {
                    oled.setCursor(SCREEN_WIDTH - rightWidth - 2, y);
                    oled.print(item.right);
                }
            }

            if (selected) {
                oled.setTextColor(SSD1306_WHITE);
            }
        }
    }

    bool drawToast(const char* message) override {
        if (!initialized) return false;

        // Check if toast message changed
        if (message != lastToastMessage) {
            lastToastMessage = message;
            toastScrollOffset = 0;
            toastScrollComplete = false;
            toastScrollPauseUntil = millis() + TOAST_SCROLL_INITIAL_PAUSE;
            lastToastScrollTime = millis();
        }

        oled.setFont(&FreeMonoBold9pt7b);
        oled.setTextWrap(false);

        // Box is 95% of screen width max
        int maxBoxWidth = SCREEN_WIDTH * 95 / 100;
        int textWidth = strlen(message) * CHAR_WIDTH_APPROX;
        int boxWidth = textWidth + 8;
        if (boxWidth > maxBoxWidth) boxWidth = maxBoxWidth;

        int boxHeight = FONT_HEIGHT + 8;
        int boxX = (SCREEN_WIDTH - boxWidth) / 2;
        int boxY = (SCREEN_HEIGHT - boxHeight) / 2;
        int innerWidth = boxWidth - 8;  // Text area width

        // Draw box background
        oled.fillRect(boxX, boxY, boxWidth, boxHeight, SSD1306_BLACK);
        oled.drawRect(boxX, boxY, boxWidth, boxHeight, SSD1306_WHITE);

        // Draw message (with scrolling if needed)
        oled.setTextColor(SSD1306_WHITE);
        int textY = boxY + FONT_HEIGHT + 2;

        if (textWidth <= innerWidth) {
            // Text fits - center it
            int textX = boxX + (boxWidth - textWidth) / 2;
            oled.setCursor(textX, textY);
            oled.print(message);
            toastScrollComplete = true;
        } else {
            // Text needs scrolling
            unsigned long now = millis();
            if (now >= toastScrollPauseUntil && now - lastToastScrollTime >= TOAST_SCROLL_SPEED_MS) {
                lastToastScrollTime = now;
                toastScrollOffset += TOAST_SCROLL_STEP;
            }

            int maxScroll = textWidth - innerWidth;  // Stop when right edge reaches right padding
            if (toastScrollOffset >= maxScroll) {
                toastScrollOffset = maxScroll;  // Clamp to exact position
                toastScrollComplete = true;
            }

            // Draw text (may overflow box)
            int textX = boxX + 4 - toastScrollOffset;
            oled.setCursor(textX, textY);
            oled.print(message);

            // Clip by redrawing box edges to mask overflow
            // Left edge
            oled.fillRect(0, boxY, boxX + 1, boxHeight, SSD1306_BLACK);
            // Right edge
            oled.fillRect(boxX + boxWidth - 1, boxY, SCREEN_WIDTH - boxX - boxWidth + 1, boxHeight, SSD1306_BLACK);
            // Redraw border
            oled.drawRect(boxX, boxY, boxWidth, boxHeight, SSD1306_WHITE);
        }

        return !toastScrollComplete;
    }

    void drawConfirmation(const char* question,
                           const char* yesLabel,
                           const char* noLabel,
                           bool yesSelected) override {
        if (!initialized) return;

        oled.setFont(&FreeMonoBold9pt7b);

        // Fixed box size: 80% of screen
        int boxWidth = SCREEN_WIDTH * 80 / 100;
        int boxHeight = SCREEN_HEIGHT * 80 / 100;
        int boxX = (SCREEN_WIDTH - boxWidth) / 2;
        int boxY = (SCREEN_HEIGHT - boxHeight) / 2;

        int qWidth = strlen(question) * CHAR_WIDTH_APPROX;
        int yWidth = strlen(yesLabel) * CHAR_WIDTH_APPROX;
        int nWidth = strlen(noLabel) * CHAR_WIDTH_APPROX;

        // Draw box background
        oled.fillRect(boxX, boxY, boxWidth, boxHeight, SSD1306_BLACK);
        oled.drawRect(boxX, boxY, boxWidth, boxHeight, SSD1306_WHITE);

        // Draw question (centered horizontally, upper third vertically)
        oled.setTextColor(SSD1306_WHITE);
        int qX = boxX + (boxWidth - qWidth) / 2;
        int qY = boxY + boxHeight / 3;
        oled.setCursor(qX, qY);
        oled.print(question);

        // Draw options (lower third)
        int optY = boxY + boxHeight * 2 / 3 + FONT_HEIGHT / 2;
        int yesX = boxX + boxWidth / 4 - yWidth / 2;
        int noX = boxX + boxWidth * 3 / 4 - nWidth / 2;

        // Yes option
        if (yesSelected) {
            oled.fillRect(yesX - 2, optY - FONT_HEIGHT + 2, yWidth + 4, FONT_HEIGHT + 2, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
        } else {
            oled.setTextColor(SSD1306_WHITE);
        }
        oled.setCursor(yesX, optY);
        oled.print(yesLabel);

        // No option
        if (!yesSelected) {
            oled.fillRect(noX - 2, optY - FONT_HEIGHT + 2, nWidth + 4, FONT_HEIGHT + 2, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
        } else {
            oled.setTextColor(SSD1306_WHITE);
        }
        oled.setCursor(noX, optY);
        oled.print(noLabel);

        oled.setTextColor(SSD1306_WHITE);
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

    void endFrame() override {
        if (!initialized) return;
        oled.display();
    }

private:
    Adafruit_SSD1306 oled;
    bool initialized;

    // List item scroll state
    int lastSelectedIndex;
    int scrollOffset;
    unsigned long lastScrollTime;
    unsigned long scrollPauseUntil;

    // Toast scroll state
    const char* lastToastMessage;
    int toastScrollOffset;
    bool toastScrollComplete;
    unsigned long lastToastScrollTime;
    unsigned long toastScrollPauseUntil;

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
    static const int LEFT_PADDING = 4;  // Padding for left-aligned text

    // Scroll settings
    static const int SCROLL_SPEED_MS = 25;       // ms between scroll steps
    static const int SCROLL_STEP = 3;            // pixels per step
    static const int SCROLL_INITIAL_PAUSE = 400; // ms pause before scrolling starts
    static const int SCROLL_RESET_PAUSE_PIXELS = 30;  // "pixels" of pause at end before reset
    static const int TOAST_SCROLL_SPEED_MS = 10;  // Toast scrolls much faster
    static const int TOAST_SCROLL_STEP = 4;       // Larger steps for toast
    static const int TOAST_SCROLL_INITIAL_PAUSE = 800;  // ms pause before toast scrolling starts

    // Screensaver settings
    static const int BALL_SIZE = 3;          // 3x3 pixel ball
    static const int BALL_UPDATE_MS = 30;    // Update every 30ms (~33 fps)
};

#endif
