// =============================================================
//  PalabraOS - hardware & behaviour settings   (board: ESP32-C3)
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

// ---- E-paper wiring (module label -> ESP32-C3 GPIO) ---------
//   VCC -> 3V3      GND -> GND
#define EPD_SCK   4    // module pin "SCL"  (SPI clock)
#define EPD_MOSI  6    // module pin "SDA"  (SPI data)
#define EPD_CS    7    // module pin "CS"
#define EPD_DC    3    // module pin "D/C"
#define EPD_RST   10   // module pin "RES"
#define EPD_BUSY  5    // module pin "BUSY"

// ---- Buttons ------------------------------------------------
// Each button connects its GPIO to GND. No resistors needed:
// the ESP32's internal pull-ups are switched on in software.
//
// The C3 only has a handful of pins, so two buttons sit on "boot" pins:
//   GPIO 9 is the BOOT pin: holding CENTER while plugging in power puts
//          the chip in upload mode instead of starting PalabraOS.
//   GPIO 8 is the on-board LED on many C3 boards: it may blink when you
//          press RIGHT. That is harmless.
// So: just don't hold a button down while powering up.
// Do NOT use GPIO 18/19 (USB) and avoid GPIO 2 and 21 for buttons.
#define PIN_UP      0
#define PIN_DOWN    1
#define PIN_LEFT    20
#define PIN_RIGHT   8
#define PIN_CENTER  9

// ---- Behaviour ----------------------------------------------
#define DEBOUNCE_MS         30     // ignore button "bounce" shorter than this
#define LONG_PRESS_MS       700    // hold CENTER this long = go Home
#define FULL_REFRESH_EVERY  25     // fast updates before one slow, ghost-clearing flash
#define AUTOSAVE_AFTER_MS   3000   // save progress after this much idle time
#define PANEL_OFF_AFTER_MS  15000  // switch panel voltage off after this much idle time

// How many words you are actively learning at once. New words from the
// list are only introduced while fewer than this many are "in progress".
#define ACTIVE_WORDS        15

// Display driver debug messages on the Serial Monitor. 0 = off.
// To use: set to 115200 AND choose Tools > USB CDC On Boot > Enabled.
// (Leave at 0 otherwise: without USB CDC, Serial would grab GPIO 20/21.)
#define DEBUG_BAUD          0
