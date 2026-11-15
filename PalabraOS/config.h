// =============================================================
//  PalabraOS - hardware & behaviour settings   (board: Seeed XIAO ESP32-S3)
//  Everything you are likely to change lives in this file.
// =============================================================
#pragma once

// ---- E-paper panel ------------------------------------------
// WeAct Studio 2.13" BLACK/WHITE module (250 x 122, SSD1680).
// If the picture looks wrong (mirrored / garbage / very faint),
// try one of the alternatives:
//   GxEPD2_213_B74            (older GDEM0213B74 panels)
//   GxEPD2_213_GDEY0213B74    (newer GDEY0213B74 panels)
#define EPD_PANEL     GxEPD2_213_BN

// 1 = landscape, 3 = landscape upside-down (flip if your case needs it)
#define EPD_ROTATION  1

// ---- E-paper wiring (module label -> XIAO pin) -------------
//   VCC -> 3V3      GND -> GND
// The numbers are the chip's GPIO numbers; the label printed
// next to the pin on the XIAO is given in the comment.
#define EPD_SCK   7    // XIAO D8   <- module pin "SCL"  (SPI clock)
#define EPD_MOSI  9    // XIAO D10  <- module pin "SDA"  (SPI data)
#define EPD_CS    8    // XIAO D9   <- module pin "CS"
#define EPD_DC    43   // XIAO D6   <- module pin "D/C"
#define EPD_RST   6    // XIAO D5   <- module pin "RES"
#define EPD_BUSY  5    // XIAO D4   <- module pin "BUSY"

// ---- Buttons ------------------------------------------------
// Each button connects its pin to GND. No resistors needed:
// the ESP32's internal pull-ups are switched on in software.
//
// The XIAO has exactly 11 pins and this project uses all of them.
// Don't move a button onto D6: the chip sends start-up messages
// out of that pin, which is why it is used for the display instead.
#define PIN_UP      1    // XIAO D0
#define PIN_DOWN    2    // XIAO D1
#define PIN_LEFT    3    // XIAO D2
#define PIN_RIGHT   4    // XIAO D3
#define PIN_CENTER  44   // XIAO D7

// ---- Behaviour ----------------------------------------------
#define DEBOUNCE_MS         30     // ignore button "bounce" shorter than this
#define LONG_PRESS_MS       700    // hold CENTER this long = go Home
#define FULL_REFRESH_EVERY  25     // fast updates before one slow, ghost-clearing flash
#define AUTOSAVE_AFTER_MS   3000   // save progress after this much idle time
#define PANEL_OFF_AFTER_MS  15000  // switch panel voltage off after this much idle time

// How many words you are actively learning at once. New words from the
// list are only introduced while fewer than this many are "in progress".
#define ACTIVE_WORDS        15

// Display driver debug messages on the Serial Monitor (over USB). 0 = off.
// Set to 115200 to see them.
#define DEBUG_BAUD          0
