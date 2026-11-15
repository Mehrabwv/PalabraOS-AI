/*
  PalabraOS - a tiny Spanish vocabulary trainer
  ------------------------------------------------------------
  Board:    ESP32-C3 (Arduino IDE, "ESP32C3 Dev Module")
  Display:  WeAct Studio 2.13" e-paper, black/white, 250 x 122
  Input:    5 buttons - UP, DOWN, LEFT, RIGHT, CENTER

  Libraries (Arduino IDE > Tools > Manage Libraries):
    - GxEPD2                  by Jean-Marc Zingg
    - U8g2_for_Adafruit_GFX   by oliver
    - Adafruit GFX Library    (say "Install all" when asked about dependencies)

  Files:
    config.h   pins and behaviour settings
    words.h    your vocabulary list
    this file  the program

  How the program is organised (read top to bottom):
    1. Hardware objects        5. Drawing helpers
    2. Buttons                 6. One draw function per screen
    3. Saved progress          7. One button handler per screen
    4. Choosing which word     8. setup() and loop()
       to show next

  Controls: UP/DOWN move, CENTER selects, LEFT goes back,
            hold CENTER = Home from anywhere.
*/

#include <Arduino.h>
#include <SPI.h>
#include <Preferences.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "config.h"
#include "words.h"

// =============================================================
//  1. HARDWARE OBJECTS
// =============================================================
GxEPD2_BW<EPD_PANEL, EPD_PANEL::HEIGHT> display(EPD_PANEL(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
U8G2_FOR_ADAFRUIT_GFX u8g2;   // text drawing with accents (á, ñ, ¿ ...)
Preferences prefs;            // small key/value storage in the ESP32's flash

const int SCREEN_W = 250;
const int SCREEN_H = 122;

// Fonts. "_tf" fonts include the accented Latin letters Spanish needs.
#define FONT_SMALL       u8g2_font_helvR08_tf
#define FONT_SMALL_BOLD  u8g2_font_helvB08_tf
#define FONT_TEXT        u8g2_font_helvR10_tf
#define FONT_BOLD        u8g2_font_helvB10_tf
#define FONT_MENU        u8g2_font_helvB12_tf

// Biggest to smallest; setFittingFont() walks down this list until the text fits.
const uint8_t* const BIG_FONTS[] = {
  u8g2_font_helvB24_tf, u8g2_font_helvB18_tf, u8g2_font_helvB14_tf,
  u8g2_font_helvB12_tf, u8g2_font_helvB10_tf, u8g2_font_helvB08_tf
};
const uint8_t BIG_FONT_COUNT = sizeof(BIG_FONTS) / sizeof(BIG_FONTS[0]);

// =============================================================
//  2. BUTTONS
// =============================================================
enum Button : uint8_t { BTN_NONE, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_CENTER, BTN_HOME };

const uint8_t BUTTON_PINS[5] = { PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT, PIN_CENTER };
bool     btnPressed[5];       // current (debounced) state of each button
uint32_t btnChangedAt[5];     // when that state last changed
bool     centerLongFired;     // true once a long press has been reported

// Returns one event per press. UP/DOWN/LEFT/RIGHT fire when pushed down.
// CENTER fires when released (short press) or as BTN_HOME when held.
Button readButton() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < 5; i++) {
    bool down = (digitalRead(BUTTON_PINS[i]) == LOW);   // pressed = connected to GND
    if (down == btnPressed[i]) continue;                // nothing changed
    if (now - btnChangedAt[i] < DEBOUNCE_MS) continue;  // too soon: contact bounce
    btnPressed[i] = down;
    btnChangedAt[i] = now;
    if (i < 4) {
      if (down) return (Button)(BTN_UP + i);
    } else if (down) {
      centerLongFired = false;
    } else if (!centerLongFired) {
      return BTN_CENTER;
    }
  }
  if (btnPressed[4] && !centerLongFired && now - btnChangedAt[4] >= LONG_PRESS_MS) {
    centerLongFired = true;
    return BTN_HOME;
  }
  return BTN_NONE;
}

// =============================================================
//  3. SAVED PROGRESS
// =============================================================
// Every word sits in a "box" (the Leitner flashcard system):
//   0 = never seen, 1 = just learned / got it wrong, ... 5 = mastered.
// A right answer moves the word up one box, a wrong answer sends it back to 1.
uint8_t box[sizeof(WORDS) / sizeof(WORDS[0])];

enum Direction : uint8_t { DIR_ES_EN, DIR_EN_ES, DIR_MIXED };
uint8_t  direction       = DIR_ES_EN;
uint8_t  cardsPerSession = 10;
uint32_t quizRight = 0, quizTotal = 0, sessionsDone = 0;
bool     progressDirty = false;   // true = something changed since the last save

void loadProgress() {
  prefs.begin("palabra", false);
  direction       = prefs.getUChar("dir", DIR_ES_EN);
  cardsPerSession = prefs.getUChar("cards", 10);
  if (direction > DIR_MIXED) direction = DIR_ES_EN;
  if (cardsPerSession < 5 || cardsPerSession > 20) cardsPerSession = 10;

  // Only reuse old progress if it was saved for this version of the word list.
  bool sameList = prefs.getUShort("listver", 0) == WORD_LIST_VERSION;
  size_t stored = prefs.isKey("box") ? prefs.getBytesLength("box") : 0;
  if (sameList && stored > 0 && stored <= WORD_COUNT) {
    prefs.getBytes("box", box, stored);          // words added at the end stay at 0
    quizRight    = prefs.getUInt("qright", 0);
    quizTotal    = prefs.getUInt("qtotal", 0);
    sessionsDone = prefs.getUInt("sessions", 0);
  }
  for (uint16_t i = 0; i < WORD_COUNT; i++) {
    if (box[i] > 5) box[i] = 5;
  }
}

void saveProgress() {
  if (!progressDirty) return;
  prefs.putUShort("listver", WORD_LIST_VERSION);
  prefs.putBytes("box", box, WORD_COUNT);
  prefs.putUInt("qright", quizRight);
  prefs.putUInt("qtotal", quizTotal);
  prefs.putUInt("sessions", sessionsDone);
  progressDirty = false;
}

void saveSettings() {
  prefs.putUChar("dir", direction);
  prefs.putUChar("cards", cardsPerSession);
}

void gradeWord(uint16_t i, bool knewIt) {
  if (knewIt) {
    uint8_t b = (box[i] < 1 ? 1 : box[i]) + 1;   // a new word you already know jumps to box 2
    box[i] = (b > 5) ? 5 : b;
  } else {
    box[i] = 1;
  }
  progressDirty = true;
}

// =============================================================
//  4. CHOOSING WHICH WORD TO SHOW NEXT
// =============================================================
uint16_t recentWords[3] = { 0xFFFF, 0xFFFF, 0xFFFF };   // don't repeat these straight away
uint8_t  recentPos = 0;

bool isRecent(uint16_t i) {
  return i == recentWords[0] || i == recentWords[1] || i == recentWords[2];
}

// How likely a word is to come up for review: box 1 = 16 ... box 5 = 1.
// So weak words appear often and mastered words only now and then.
uint32_t reviewWeight(uint16_t i) {
  if (box[i] == 0 || isRecent(i)) return 0;
  return 1UL << (5 - box[i]);
}

uint16_t pickWord(bool allowNew) {
  uint16_t firstNew = WORD_COUNT;   // WORD_COUNT means "there is none"
  uint16_t inProgress = 0;
  uint32_t totalWeight = 0;
  for (uint16_t i = 0; i < WORD_COUNT; i++) {
    if (box[i] == 0) {
      if (firstNew == WORD_COUNT) firstNew = i;
      continue;
    }
    if (box[i] <= 3) inProgress++;
    totalWeight += reviewWeight(i);
  }

  uint16_t pick = 0;
  bool haveNew = allowNew && firstNew < WORD_COUNT;
  if (haveNew && (totalWeight == 0 || (inProgress < ACTIVE_WORDS && random(2) == 0))) {
    pick = firstNew;                              // introduce the next new word
  } else if (totalWeight == 0) {
    for (uint8_t tries = 0; tries < 10; tries++) {  // nothing to review yet: any word
      pick = random(WORD_COUNT);
      if (!isRecent(pick)) break;
    }
  } else {
    uint32_t r = random(totalWeight);             // weighted lottery among seen words
    for (uint16_t i = 0; i < WORD_COUNT; i++) {
      uint32_t w = reviewWeight(i);
      if (r < w) { pick = i; break; }
      r -= w;
    }
  }
  recentWords[recentPos] = pick;
  recentPos = (recentPos + 1) % 3;
  return pick;
}

// false = show Spanish and ask for English, true = the other way round
bool pickReversed() {
  if (direction == DIR_MIXED) return random(2) == 1;
  return direction == DIR_EN_ES;
}

// =============================================================
//  APP STATE - which screen is showing and what is on it
// =============================================================
enum Screen : uint8_t {
  SCR_HOME, SCR_CARD_FRONT, SCR_CARD_BACK, SCR_QUIZ, SCR_QUIZ_ANSWER, SCR_RESULT,
  SCR_LIST, SCR_DETAIL, SCR_STATS, SCR_SETTINGS, SCR_CONFIRM_RESET
};
Screen screen = SCR_HOME;
bool   needsRedraw = true;

uint8_t  homeSel = 0;            // highlighted row on the Home menu
uint16_t curWord = 0;            // word on the current card / question
bool     curReversed = false;    // is the current card English -> Spanish?
uint8_t  roundPos = 0;           // cards/questions done in this session
uint8_t  roundScore = 0;         // ... and how many were right
bool     roundIsQuiz = false;
uint16_t quizOption[4];          // the four words offered as answers
uint8_t  quizAnswer = 0;         // which of the four is correct
uint8_t  quizSel = 0;            // which one is highlighted
uint16_t listSel = 0;            // highlighted word in the word list
uint8_t  settingsSel = 0;

const char* promptOf(uint16_t i, bool reversed) { return reversed ? WORDS[i].en : WORDS[i].es; }
const char* answerOf(uint16_t i, bool reversed) { return reversed ? WORDS[i].es : WORDS[i].en; }

void nextCard() {
  curWord = pickWord(true);
  curReversed = pickReversed();
  screen = SCR_CARD_FRONT;
}

// Rough word type guessed from the text: 1 = noun ("el ..." / "la ..."),
// 2 = verb ("to ..."), 0 = anything else. Used to make quiz choices harder:
// if the answer is a verb, the wrong choices should be verbs too.
uint8_t wordKind(uint16_t i) {
  const char* es = WORDS[i].es;
  if (!strncmp(es, "el ", 3) || !strncmp(es, "la ", 3) || !strncmp(es, "los ", 4) || !strncmp(es, "las ", 4)) return 1;
  if (!strncmp(WORDS[i].en, "to ", 3)) return 2;
  return 0;
}

void nextQuestion() {
  curWord = pickWord(false);
  curReversed = pickReversed();
  quizAnswer = random(4);
  quizSel = 0;
  for (uint8_t slot = 0; slot < 4; slot++) {
    if (slot == quizAnswer) { quizOption[slot] = curWord; continue; }
    // Find a wrong answer that doesn't look the same as the right one
    // or as a wrong answer we already picked.
    uint16_t cand = 0;
    for (uint8_t tries = 0; tries < 100; tries++) {
      cand = random(WORD_COUNT);
      // Prefer the same type of word; give up on that after 60 tries (small lists).
      if (tries < 60 && wordKind(cand) != wordKind(curWord)) continue;
      // (comparing the question side too keeps "ser" out when "estar" = "to be" is asked)
      bool clash = strcmp(answerOf(cand, curReversed), answerOf(curWord, curReversed)) == 0 ||
                   strcmp(promptOf(cand, curReversed), promptOf(curWord, curReversed)) == 0;
      for (uint8_t k = 0; k < slot && !clash; k++) {
        if (k != quizAnswer && strcmp(answerOf(cand, curReversed), answerOf(quizOption[k], curReversed)) == 0) clash = true;
      }
      if (!clash) break;
    }
    quizOption[slot] = cand;
  }
  screen = SCR_QUIZ;
}

void startRound(bool quiz) {
  roundIsQuiz = quiz;
  roundPos = 0;
  roundScore = 0;
  if (quiz) nextQuestion(); else nextCard();
}

// One card or question is finished: move on, or show the result screen.
void advanceRound() {
  roundPos++;
  if (roundPos >= cardsPerSession) {
    sessionsDone++;
    progressDirty = true;
    saveProgress();
    screen = SCR_RESULT;
  } else if (roundIsQuiz) {
    nextQuestion();
  } else {
    nextCard();
  }
}

void goHome() {
  saveProgress();
  screen = SCR_HOME;
}

// =============================================================
//  5. DRAWING HELPERS
// =============================================================
uint16_t inkColor = GxEPD_BLACK;
bool     wantFullRefresh = true;   // first screen after power-on gets a full refresh
uint8_t  fastUpdates = 0;          // fast (partial) updates since the last full one
bool     panelPowered = false;
uint32_t lastDrawAt = 0;

void ink(uint16_t color) {
  inkColor = color;
  u8g2.setForegroundColor(color);
}

// Always change fonts through this function. The library forgets "transparent
// mode" whenever the font changes, and would then paint a box behind each letter.
void setFont(const uint8_t* font) {
  u8g2.setFont(font);
  u8g2.setFontMode(1);   // 1 = draw only the letters themselves
}

int textWidth(const char* s) { return u8g2.getUTF8Width(s); }

// y is always the text BASELINE (the line the letters sit on)
void drawLeft(int x, int y, const char* s)    { u8g2.drawUTF8(x, y, s); }
void drawRight(int x, int y, const char* s)   { u8g2.drawUTF8(x - textWidth(s), y, s); }
void drawCentered(int y, const char* s)       { u8g2.drawUTF8((SCREEN_W - textWidth(s)) / 2, y, s); }

// Selects the largest font, starting at BIG_FONTS[first], in which s fits in maxW pixels.
void setFittingFont(const char* s, int maxW, uint8_t first) {
  for (uint8_t f = first; f < BIG_FONT_COUNT; f++) {
    setFont(BIG_FONTS[f]);
    if (textWidth(s) <= maxW) return;
  }
}

// Returns s, shortened with ".." if it is wider than maxW in the current font.
const char* fitText(const char* s, int maxW) {
  static char out[64];
  const size_t room = sizeof(out) - 3;          // keep space for ".." and the end marker
  size_t n = strlen(s);
  bool cut = n > room;
  if (cut) n = room;
  memcpy(out, s, n);
  out[n] = 0;
  if (!cut && textWidth(out) <= maxW) return out;
  while (n > 0) {
    n--;
    while (n > 0 && (out[n] & 0xC0) == 0x80) n--;   // never split an accented letter in half
    strcpy(out + n, "..");
    if (textWidth(out) <= maxW) break;
  }
  return out;
}

// Draws text with automatic line breaks between words.
void drawWrapped(const char* s, int x, int y, int maxW, int lineH, int maxLines) {
  char line[100] = "";
  char test[100];
  int lines = 0;
  while (*s && lines < maxLines) {
    const char* end = s;
    while (*end && *end != ' ') end++;            // end of the next word
    snprintf(test, sizeof(test), "%s%s%.*s", line, line[0] ? " " : "", (int)(end - s), s);
    if (line[0] && textWidth(test) > maxW) {      // word doesn't fit: print the line, start a new one
      drawLeft(x, y, line);
      y += lineH;
      lines++;
      line[0] = 0;
      continue;
    }
    strcpy(line, test);
    s = end;
    while (*s == ' ') s++;
  }
  if (line[0] && lines < maxLines) drawLeft(x, y, line);
}

// Black bar across the top: screen name on the left, extra info on the right.
void drawTitleBar(const char* title, const char* info) {
  display.fillRect(0, 0, SCREEN_W, 15, GxEPD_BLACK);
  ink(GxEPD_WHITE);
  setFont(FONT_SMALL_BOLD);
  drawLeft(4, 11, title);
  setFont(FONT_SMALL);
  drawRight(SCREEN_W - 4, 11, info);
  ink(GxEPD_BLACK);
}

// Bottom line telling you what LEFT / CENTER / RIGHT do. Pass "" to leave one out.
void drawHints(const char* left, const char* center, const char* right) {
  const int y = 119;
  ink(GxEPD_BLACK);
  setFont(FONT_SMALL);
  if (left[0]) {
    display.fillTriangle(3, y - 4, 8, y - 8, 8, y, GxEPD_BLACK);
    drawLeft(12, y, left);
  }
  if (center[0]) {
    int x = (SCREEN_W - (textWidth(center) + 11)) / 2;
    display.fillCircle(x + 3, y - 4, 3, GxEPD_BLACK);
    drawLeft(x + 11, y, center);
  }
  if (right[0]) {
    int x = SCREEN_W - 4;
    display.fillTriangle(x, y - 4, x - 5, y - 8, x - 5, y, GxEPD_BLACK);
    drawRight(x - 9, y, right);
  }
}

// Five little squares showing a word's box: filled = level reached.
void drawLevel(int x, int y, uint8_t level) {
  for (uint8_t i = 0; i < 5; i++) {
    if (i < level) display.fillRect(x + i * 7, y, 5, 5, inkColor);
    else           display.drawRect(x + i * 7, y, 5, 5, inkColor);
  }
}

// Starts a full-width menu row. The selected row is white text on black.
void beginRow(int y, int h, bool selected) {
  if (selected) display.fillRect(0, y, SCREEN_W, h, GxEPD_BLACK);
  ink(selected ? GxEPD_WHITE : GxEPD_BLACK);
}

void drawCheckMark(int x, int y) {
  for (int t = 0; t < 2; t++) {   // drawn twice, one pixel apart, to look bold
    display.drawLine(x - 5, y + t, x - 2, y + 3 + t, inkColor);
    display.drawLine(x - 2, y + 3 + t, x + 5, y - 4 + t, inkColor);
  }
}

void drawCross(int x, int y) {
  for (int t = 0; t < 2; t++) {
    display.drawLine(x - 4, y - 4 + t, x + 4, y + 4 + t, inkColor);
    display.drawLine(x - 4, y + 4 + t, x + 4, y - 4 + t, inkColor);
  }
}

// Word + translation + example sentence. Used by the card back and the word list.
void drawWordFace(uint16_t i, bool reversed) {
  const char* prompt = promptOf(i, reversed);
  const char* answer = answerOf(i, reversed);
  ink(GxEPD_BLACK);
  setFittingFont(prompt, SCREEN_W - 56, 3);
  drawLeft(6, 33, fitText(prompt, SCREEN_W - 56));
  drawLevel(SCREEN_W - 6 - 33, 25, box[i]);
  display.drawFastHLine(6, 39, SCREEN_W - 12, GxEPD_BLACK);
  setFittingFont(answer, SCREEN_W - 12, 1);
  drawLeft(6, 62, fitText(answer, SCREEN_W - 12));
  if (WORDS[i].ex && WORDS[i].ex[0]) {
    setFont(FONT_TEXT);
    drawWrapped(WORDS[i].ex, 6, 80, SCREEN_W - 12, 14, 2);
  }
}

// =============================================================
//  6. SCREENS - one draw function each
// =============================================================
const char* const HOME_LABEL[5] = { "Aprender",   "Examen", "Palabras",  "Progreso", "Ajustes"  };
const char* const HOME_NOTE[5]  = { "flashcards", "quiz",   "word list", "stats",    "settings" };

void drawHome() {
  char info[20];
  snprintf(info, sizeof(info), "%u palabras", (unsigned)WORD_COUNT);
  drawTitleBar("PalabraOS", info);
  for (uint8_t r = 0; r < 5; r++) {
    int y = 16 + r * 21;
    beginRow(y, 21, r == homeSel);
    setFont(FONT_MENU);
    drawLeft(8, y + 15, HOME_LABEL[r]);
    setFont(FONT_SMALL);
    drawRight(SCREEN_W - 8, y + 14, HOME_NOTE[r]);
  }
}

void drawCardFront() {
  char info[12];
  snprintf(info, sizeof(info), "%u/%u", roundPos + 1, cardsPerSession);
  drawTitleBar("Aprender", info);
  setFont(FONT_SMALL);
  drawCentered(31, curReversed ? "¿Cómo se dice en español?" : "¿Qué significa?");
  const char* prompt = promptOf(curWord, curReversed);
  setFittingFont(prompt, SCREEN_W - 12, 0);
  drawCentered(72, fitText(prompt, SCREEN_W - 12));
  if (box[curWord] == 0) {
    setFont(FONT_SMALL);
    drawCentered(94, "¡palabra nueva!");
  } else {
    drawLevel((SCREEN_W - 33) / 2, 87, box[curWord]);
  }
  drawHints("", "ver respuesta", "");
}

void drawCardBack() {
  char info[12];
  snprintf(info, sizeof(info), "%u/%u", roundPos + 1, cardsPerSession);
  drawTitleBar("Aprender", info);
  drawWordFace(curWord, curReversed);
  drawHints("no lo sé", "", "lo sé");
}

void drawQuiz(bool showAnswer) {
  char info[12];
  snprintf(info, sizeof(info), "%u/%u", roundPos + 1, cardsPerSession);
  bool right = (quizSel == quizAnswer);
  drawTitleBar(!showAnswer ? "Examen" : (right ? "¡Correcto!" : "Incorrecto"), info);

  const char* prompt = promptOf(curWord, curReversed);
  setFittingFont(prompt, SCREEN_W - 12, 2);
  drawCentered(33, fitText(prompt, SCREEN_W - 12));

  for (uint8_t r = 0; r < 4; r++) {
    int y = 40 + r * 20;
    // While choosing, the highlighted row is black. Afterwards the CORRECT row is black.
    beginRow(y, 20, showAnswer ? (r == quizAnswer) : (r == quizSel));
    setFont(FONT_TEXT);
    drawLeft(10, y + 14, fitText(answerOf(quizOption[r], curReversed), SCREEN_W - 40));
    if (showAnswer && r == quizAnswer) drawCheckMark(SCREEN_W - 14, y + 10);
    if (showAnswer && r == quizSel && !right) {
      display.drawRect(0, y, SCREEN_W, 20, GxEPD_BLACK);
      drawCross(SCREEN_W - 14, y + 10);
    }
  }
  ink(GxEPD_BLACK);
}

void drawResult() {
  drawTitleBar(roundIsQuiz ? "Examen terminado" : "Sesión terminada", "");
  char buf[24];
  setFont(FONT_TEXT);
  drawCentered(36, roundIsQuiz ? "Respuestas correctas" : "Palabras que sabías");
  snprintf(buf, sizeof(buf), "%u / %u", roundScore, cardsPerSession);
  setFont(BIG_FONTS[0]);
  drawCentered(72, buf);
  setFont(FONT_BOLD);
  uint8_t percent = (uint8_t)(100UL * roundScore / cardsPerSession);
  drawCentered(94, percent >= 90 ? "¡Excelente!" : percent >= 60 ? "¡Muy bien!" : "¡Sigue practicando!");
  drawHints("inicio", "otra vez", "");
}

void drawList() {
  char info[16];
  snprintf(info, sizeof(info), "%u/%u", (unsigned)(listSel + 1), (unsigned)WORD_COUNT);
  drawTitleBar("Palabras", info);
  uint16_t top = (listSel / 5) * 5;             // first word of the current page
  for (uint8_t r = 0; r < 5 && top + r < WORD_COUNT; r++) {
    uint16_t i = top + r;
    int y = 16 + r * 21;
    beginRow(y, 21, i == listSel);
    setFont(FONT_BOLD);
    drawLeft(5, y + 15, fitText(WORDS[i].es, 104));
    setFont(FONT_TEXT);
    drawLeft(118, y + 15, fitText(WORDS[i].en, 88));
    drawLevel(SCREEN_W - 5 - 33, y + 8, box[i]);
  }
  ink(GxEPD_BLACK);
}

void drawDetail() {
  char info[16];
  snprintf(info, sizeof(info), "%u/%u", (unsigned)(listSel + 1), (unsigned)WORD_COUNT);
  drawTitleBar("Palabra", info);
  drawWordFace(listSel, false);
  drawHints("lista", "", "");
}

void drawStatRow(int y, const char* label, uint16_t n) {
  char buf[8];
  setFont(FONT_TEXT);
  drawLeft(6, y, label);
  display.drawRect(92, y - 9, 118, 9, GxEPD_BLACK);
  display.fillRect(92, y - 9, (int)(118L * n / WORD_COUNT), 9, GxEPD_BLACK);
  snprintf(buf, sizeof(buf), "%u", (unsigned)n);
  drawRight(SCREEN_W - 6, y, buf);
}

void drawStats() {
  uint16_t fresh = 0, learning = 0, mastered = 0;
  for (uint16_t i = 0; i < WORD_COUNT; i++) {
    if (box[i] == 0) fresh++;
    else if (box[i] <= 3) learning++;
    else mastered++;
  }
  char buf[40];
  snprintf(buf, sizeof(buf), "%u palabras", (unsigned)WORD_COUNT);
  drawTitleBar("Progreso", buf);
  drawStatRow(34, "Nuevas", fresh);
  drawStatRow(52, "Aprendiendo", learning);
  drawStatRow(70, "Dominadas", mastered);
  display.drawFastHLine(6, 79, SCREEN_W - 12, GxEPD_BLACK);
  setFont(FONT_TEXT);
  unsigned percent = quizTotal ? (unsigned)(100UL * quizRight / quizTotal) : 0;
  snprintf(buf, sizeof(buf), "Examen: %lu/%lu (%u%%)", (unsigned long)quizRight, (unsigned long)quizTotal, percent);
  drawLeft(6, 96, buf);
  snprintf(buf, sizeof(buf), "Sesiones: %lu", (unsigned long)sessionsDone);
  drawRight(SCREEN_W - 6, 96, buf);
  drawHints("inicio", "", "");
}

const char* const DIRECTION_NAME[3] = { "español > inglés", "inglés > español", "las dos" };

void drawSettings() {
  drawTitleBar("Ajustes", "");
  char cards[8];
  snprintf(cards, sizeof(cards), "%u", cardsPerSession);
  const char* label[3] = { "Dirección", "Tarjetas por sesión", "Borrar progreso" };
  const char* value[3] = { DIRECTION_NAME[direction], cards, "" };
  for (uint8_t r = 0; r < 3; r++) {
    int y = 18 + r * 22;
    beginRow(y, 22, r == settingsSel);
    setFont(FONT_BOLD);
    drawLeft(8, y + 15, label[r]);
    setFont(FONT_TEXT);
    drawRight(SCREEN_W - 8, y + 15, value[r]);
  }
  drawHints("inicio", "cambiar", "");
}

void drawConfirmReset() {
  drawTitleBar("Ajustes", "");
  setFont(FONT_MENU);
  drawCentered(52, "¿Borrar todo el progreso?");
  setFont(FONT_TEXT);
  drawCentered(74, "No se puede deshacer.");
  drawHints("no", "", "sí, borrar");
}

// Draws the current screen into memory, then sends it to the e-paper.
void render() {
  display.fillScreen(GxEPD_WHITE);
  ink(GxEPD_BLACK);
  switch (screen) {
    case SCR_HOME:          drawHome();          break;
    case SCR_CARD_FRONT:    drawCardFront();     break;
    case SCR_CARD_BACK:     drawCardBack();      break;
    case SCR_QUIZ:          drawQuiz(false);     break;
    case SCR_QUIZ_ANSWER:   drawQuiz(true);      break;
    case SCR_RESULT:        drawResult();        break;
    case SCR_LIST:          drawList();          break;
    case SCR_DETAIL:        drawDetail();        break;
    case SCR_STATS:         drawStats();         break;
    case SCR_SETTINGS:      drawSettings();      break;
    case SCR_CONFIRM_RESET: drawConfirmReset();  break;
  }

  // E-paper has two ways to update:
  //   fast "partial" update (~0.4 s) - leaves faint ghosts after many uses
  //   full refresh (~2 s, flashes)   - cleans the screen completely
  // So we use fast updates and do a full one every now and then.
  bool full = wantFullRefresh || fastUpdates >= FULL_REFRESH_EVERY;
  display.display(!full);
  wantFullRefresh = false;
  fastUpdates = full ? 0 : fastUpdates + 1;
  panelPowered = !full;         // a full refresh switches the panel voltage off by itself
  lastDrawAt = millis();
}

// =============================================================
//  7. BUTTON HANDLERS - one per screen
// =============================================================
// Each handler returns true if the picture changed and must be redrawn.
// Returning false for unused buttons avoids a pointless e-paper refresh.

bool onHome(Button b) {
  if (b == BTN_LEFT) return false;
  if (b == BTN_UP)   homeSel = (homeSel + 4) % 5;
  if (b == BTN_DOWN) homeSel = (homeSel + 1) % 5;
  if (b == BTN_CENTER || b == BTN_RIGHT) {
    switch (homeSel) {
      case 0: startRound(false);                      break;
      case 1: startRound(true);                       break;
      case 2: screen = SCR_LIST;                      break;
      case 3: screen = SCR_STATS;                     break;
      case 4: settingsSel = 0; screen = SCR_SETTINGS; break;
    }
  }
  return true;
}

bool onCardFront(Button b) {
  if (b != BTN_CENTER) return false;
  screen = SCR_CARD_BACK;
  return true;
}

bool onCardBack(Button b) {
  if (b == BTN_UP || b == BTN_DOWN) return false;
  if (b == BTN_CENTER) {
    screen = SCR_CARD_FRONT;                 // peek at the front again
  } else {
    bool knewIt = (b == BTN_RIGHT);          // RIGHT = I knew it, LEFT = I didn't
    gradeWord(curWord, knewIt);
    if (knewIt) roundScore++;
    advanceRound();
  }
  return true;
}

bool onQuiz(Button b) {
  if (b == BTN_LEFT || b == BTN_RIGHT) return false;
  if (b == BTN_UP)   quizSel = (quizSel + 3) % 4;
  if (b == BTN_DOWN) quizSel = (quizSel + 1) % 4;
  if (b == BTN_CENTER) {
    bool right = (quizSel == quizAnswer);
    quizTotal++;
    if (right) { quizRight++; roundScore++; }
    gradeWord(curWord, right);
    screen = SCR_QUIZ_ANSWER;
  }
  return true;
}

bool onQuizAnswer(Button b) {
  advanceRound();                            // any button = next question
  return true;
}

bool onResult(Button b) {
  if (b == BTN_CENTER) { startRound(roundIsQuiz); return true; }
  if (b == BTN_LEFT)   { goHome(); return true; }
  return false;
}

bool onList(Button b) {
  if (b == BTN_UP)     listSel = (listSel + WORD_COUNT - 1) % WORD_COUNT;
  if (b == BTN_DOWN)   listSel = (listSel + 1) % WORD_COUNT;
  if (b == BTN_RIGHT) {                      // jump to the next page
    listSel = (listSel / 5 + 1) * 5;
    if (listSel >= WORD_COUNT) listSel = 0;
  }
  if (b == BTN_CENTER) screen = SCR_DETAIL;
  if (b == BTN_LEFT)   goHome();
  return true;
}

bool onDetail(Button b) {
  if (b == BTN_RIGHT) return false;
  if (b == BTN_UP)   listSel = (listSel + WORD_COUNT - 1) % WORD_COUNT;
  if (b == BTN_DOWN) listSel = (listSel + 1) % WORD_COUNT;
  if (b == BTN_LEFT || b == BTN_CENTER) screen = SCR_LIST;
  return true;
}

bool onStats(Button b) {
  if (b != BTN_LEFT && b != BTN_CENTER) return false;
  goHome();
  return true;
}

bool onSettings(Button b) {
  if (b == BTN_UP)   settingsSel = (settingsSel + 2) % 3;
  if (b == BTN_DOWN) settingsSel = (settingsSel + 1) % 3;
  if (b == BTN_LEFT) goHome();
  if (b == BTN_CENTER || b == BTN_RIGHT) {
    if (settingsSel == 0) { direction = (direction + 1) % 3; saveSettings(); }
    if (settingsSel == 1) { cardsPerSession = (cardsPerSession >= 20) ? 5 : cardsPerSession + 5; saveSettings(); }
    if (settingsSel == 2) screen = SCR_CONFIRM_RESET;
  }
  return true;
}

bool onConfirmReset(Button b) {
  if (b != BTN_LEFT && b != BTN_RIGHT) return false;
  if (b == BTN_RIGHT) {                      // yes, erase everything
    memset(box, 0, sizeof(box));
    quizRight = quizTotal = sessionsDone = 0;
    recentWords[0] = recentWords[1] = recentWords[2] = 0xFFFF;
    progressDirty = true;
    saveProgress();
  }
  screen = SCR_SETTINGS;
  return true;
}

void handleButton(Button b) {
  bool changed = false;
  if (b == BTN_HOME) {
    // Hold CENTER: go Home from anywhere. Already on Home: clean the screen.
    if (screen == SCR_HOME) wantFullRefresh = true;
    goHome();
    changed = true;
  } else {
    switch (screen) {
      case SCR_HOME:          changed = onHome(b);          break;
      case SCR_CARD_FRONT:    changed = onCardFront(b);     break;
      case SCR_CARD_BACK:     changed = onCardBack(b);      break;
      case SCR_QUIZ:          changed = onQuiz(b);          break;
      case SCR_QUIZ_ANSWER:   changed = onQuizAnswer(b);    break;
      case SCR_RESULT:        changed = onResult(b);        break;
      case SCR_LIST:          changed = onList(b);          break;
      case SCR_DETAIL:        changed = onDetail(b);        break;
      case SCR_STATS:         changed = onStats(b);         break;
      case SCR_SETTINGS:      changed = onSettings(b);      break;
      case SCR_CONFIRM_RESET: changed = onConfirmReset(b);  break;
    }
  }
  if (changed) needsRedraw = true;
}

// =============================================================
//  8. SETUP AND LOOP
// =============================================================
void setup() {
  // Stops the build with a clear message if the word list is too short.
  static_assert(sizeof(WORDS) / sizeof(WORDS[0]) >= 4,
                "words.h needs at least 4 words (the quiz shows 4 choices)");

  for (uint8_t i = 0; i < 5; i++) pinMode(BUTTON_PINS[i], INPUT_PULLUP);

  // Note: random() on the ESP32 already uses the chip's hardware random
  // number generator, so no randomSeed() call is needed.
  loadProgress();

  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);     // tell the ESP32 which pins carry SPI
  display.init(DEBUG_BAUD, true, 50, false);    // 50 ms reset pulse suits the WeAct module
  display.setRotation(EPD_ROTATION);
  u8g2.begin(display);
}

void loop() {
  Button b = readButton();
  if (b != BTN_NONE) handleButton(b);

  if (needsRedraw) {
    needsRedraw = false;
    render();
  }

  // Save your progress to flash as soon as you pause for a moment,
  // so switching the power off mid-session loses nothing.
  if (progressDirty && millis() - lastDrawAt > AUTOSAVE_AFTER_MS) saveProgress();

  // E-paper keeps its picture without power, so switch the panel's
  // driving voltage off when nothing has happened for a while.
  if (panelPowered && millis() - lastDrawAt > PANEL_OFF_AFTER_MS) {
    display.powerOff();
    panelPowered = false;
  }
  delay(5);
}
