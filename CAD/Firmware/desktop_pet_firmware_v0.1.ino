#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_BMP085.h>
#include "Adafruit_TinyUSB.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define BUZZER_PIN 26
#define TOUCH_PIN 27

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BMP085 bmp;

uint8_t const desc_hid_report[] = { TUD_HID_REPORT_DESC_CONSUMER() };
Adafruit_USBD_HID usb_hid;

int menuState = 0;
int eyeExpression = 0;

int launcherTile = 0;
int launcherApp  = 0;
#define LAUNCHER_TILES 4
#define LAUNCHER_APPS  8

bool inScreenOff = false;

bool inScreensaver = false;
int screensaverSelected = 0;
int screensaverMode = -1;
bool ssSelecting = true;

#define SS_COUNT 17

struct Ball { float x, y, dx, dy; int r; };
Ball ssBalls[3];

int ssRainY[16];
int ssRainLen[16];

struct Star { int x, y; int speed; };
Star ssStars[20];

int ssSnakeX[64], ssSnakeY[64];
int ssSnakeLen = 8;
int ssSnakeDir = 0;
int ssSnakeFoodX = 60, ssSnakeFoodY = 20;
unsigned long ssSnakeLast = 0;

int ssMatrixY[16];
unsigned long ssMatrixLast = 0;

struct Particle { float x, y, dx, dy; bool active; };
Particle ssParticles[20];
unsigned long ssFireLast = 0;

struct Ripple { int cx, cy, r; bool active; };
Ripple ssRipples[4];
unsigned long ssRippleLast = 0;

#define MAZE_COLS 16
#define MAZE_ROWS 8
uint8_t ssMazeWalls[MAZE_ROWS][MAZE_COLS];
bool    ssMazeVisited[MAZE_ROWS][MAZE_COLS];
int     ssMazeStkX[MAZE_COLS * MAZE_ROWS];
int     ssMazeStkY[MAZE_COLS * MAZE_ROWS];
int     ssMazeStkTop = 0;
bool    ssMazeGenDone = false;
unsigned long ssMazeLast = 0;

int     ssMazeCurX = 0, ssMazeCurY = 0;
int     ssMazeCurDir = 0;
unsigned long ssMazeCurLast = 0;

float ssPlasmaT = 0;
unsigned long ssPlasmeLast = 0;

unsigned long ssClockLast = 0;
float ssClockSec = 0;

float ssLissT = 0;
unsigned long ssLissLast = 0;
int ssLissX[80], ssLissY[80];
int ssLissHead = 0;

float ssSpiralT = 0;
unsigned long ssSpiralLast = 0;

struct WarpStar { float x, y, z; };
WarpStar ssWarpStars[60];
unsigned long ssWarpLast = 0;

int ssEqH[16];
int ssEqTarget[16];
unsigned long ssEqLast = 0;

#define GOL_W 32
#define GOL_H 16
uint8_t ssGolGrid[GOL_H][GOL_W];
uint8_t ssGolNext[GOL_H][GOL_W];
unsigned long ssGolLast = 0;
int ssGolGen = 0;

#define SAND_N 80
struct SandP { int x, y; bool active; };
SandP ssSand[SAND_N];
uint8_t ssSandGrid[64][128];

#define SAND_GW 64
#define SAND_GH 32
bool ssSandGrid2[SAND_GH][SAND_GW];
unsigned long ssSandLast = 0;
int ssSandSpawn = 0;

float ssPendA1 = 2.0, ssPendA2 = 2.5;
float ssPendV1 = 0,   ssPendV2 = 0;
unsigned long ssPendLast = 0;

int ssPendTX[60], ssPendTY[60];
int ssPendTHead = 0;

const char* ssNames[SS_COUNT] = {
  "Bouncing Balls",
  "Rain",
  "Stars",
  "Snake",
  "Matrix",
  "Fireworks",
  "Ripples",
  "Maze",
  "Plasma",
  "Clock",
  "Lissajous",
  "Spiral",
  "Warp",
  "Equalizer",
  "Game of Life",
  "Sand",
  "Pendulum"
};
bool inFaceManager = false;
int selectedFace = 0;
#define FACE_COUNT 7
const char* faceNames[FACE_COUNT] = {
  "Normal", "Love", "Triangle", "Look Up", "Sleep", "Angry", "Surprised"
};
bool lastTouchState = LOW;
unsigned long touchStartTime = 0;
bool longPressTriggered = false;
unsigned long lastInteraction = 0;

bool buzzerEnabled = true;
bool autoSwitch = false;
int selectedSetting = 0;
bool inSettingsMenu = false;
unsigned long lastAutoSwitch = 0;
unsigned long autoSuspendTimeout = 0;

bool inSensors = false;

bool inStopwatch = false;
bool swRunning = false;
unsigned long swStartTime = 0;
unsigned long swElapsed = 0;

bool inPong = false;
bool pongLost = false;

float ballX = 64, ballY = 32;
float ballDX = 2.2, ballDY = 1.5;

int playerY = 27;
int aiY = 27;
const int PAD_H = 16;
const int PAD_W = 3;
int pongScore = 0;

bool paddleGoingDown = true;
bool touchHeld = false;

bool inXO = false;
uint8_t xoBoard[9];
int xoCursor = 0;
bool xoGameOver = false;
int xoWinner = 0;
bool xoPlayerTurn = true;
unsigned long xoAiDelay = 0;
bool xoAiThinking = false;

bool inMediaMenu = false;
int selectedMedia = 0;

unsigned long volRepeatLast = 0;
bool volHolding = false;

float currentTemp = 0;
long currentPressure = 0;

int eyeWidth = 42; int eyeHeight = 36; int cornerRadius = 12;
int baseLeftX = 16; int baseRightX = 70; int baseY = 14;
float currentX = 0; float currentY = 0;
float targetX = 0; float targetY = 0;
unsigned long lastBlink = 0; unsigned long lastUpdate = 0;

void playTone(int freq, int dur) {
  if (buzzerEnabled) tone(BUZZER_PIN, freq, dur);
}

void sendMediaKey(uint16_t key) {
  if (usb_hid.ready()) {
    usb_hid.sendReport16(0, key);
    delay(10);
    usb_hid.sendReport16(0, 0);
  }
}

void drawEyeShape(int x, int y, int h) {
  int offset = (eyeHeight - h) / 2;
  switch(eyeExpression) {
    case 0:
      display.fillRoundRect(x, y + offset, eyeWidth, h, cornerRadius, SSD1306_WHITE);
      break;
    case 1:
      display.fillCircle(x + 12, y + 12, 11, SSD1306_WHITE);
      display.fillCircle(x + 30, y + 12, 11, SSD1306_WHITE);
      display.fillTriangle(x + 2, y + 18, x + 40, y + 18, x + 21, y + 35, SSD1306_WHITE);
      break;
    case 2:
      display.fillTriangle(x + 21, y, x, y + 36, x + 42, y + 36, SSD1306_WHITE);
      display.fillTriangle(x + 21, y + 10, x + 10, y + 36, x + 32, y + 36, SSD1306_BLACK);
      break;
    case 3:
      display.fillRoundRect(x, y + 10, eyeWidth, eyeHeight - 10, cornerRadius, SSD1306_WHITE);
      display.fillCircle(x + 21, y + 25, 15, SSD1306_BLACK);
      display.fillRect(x, y + 25, 42, 15, SSD1306_BLACK);
      break;
    case 4:
      display.fillRoundRect(x + 4, y + eyeHeight/2 - 3, eyeWidth - 8, 7, 3, SSD1306_WHITE);
      break;
    case 5: {
      bool isLeft = (x < 50);
      display.fillRoundRect(x, y + 6, eyeWidth, eyeHeight - 6, 6, SSD1306_WHITE);
      if (isLeft) {
        display.fillTriangle(x + eyeWidth/2, y + 6,
                             x + eyeWidth + 1, y + 6,
                             x + eyeWidth + 1, y + 24, SSD1306_BLACK);
      } else {
        display.fillTriangle(x - 1, y + 6,
                             x + eyeWidth/2, y + 6,
                             x - 1, y + 24, SSD1306_BLACK);
      }
      break;
    }
    case 6:
      display.fillCircle(x + eyeWidth/2, y + eyeHeight/2, eyeHeight/2, SSD1306_WHITE);
      display.fillCircle(x + eyeWidth/2, y + eyeHeight/2, 7, SSD1306_BLACK);
      display.fillCircle(x + eyeWidth/2 + 3, y + eyeHeight/2 - 3, 3, SSD1306_WHITE);
      break;
  }
}

void drawFaceExtras(int xOff) {
  int ey = baseY + (int)currentY;
  switch(eyeExpression) {
    case 4: {
      int zx = xOff + 88, zy = 4;
      display.setTextColor(SSD1306_WHITE);
      display.setTextSize(1); display.setCursor(zx,      zy + 10); display.print("z");
      display.setTextSize(1); display.setCursor(zx + 7,  zy + 4);  display.print("z");
      display.setTextSize(2); display.setCursor(zx + 14, zy - 2);  display.print("Z");
      break;
    }
    default: break;
  }
}

void drawFaceManager() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_BLACK);
  display.setCursor(4, 3); display.print("(^_^)");
  display.setCursor(38, 3); display.print("FACE MANAGER");
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);

  const int ROW_H = 14, START_Y = 14;
  for (int slot = 0; slot < 3; slot++) {
    int idx = selectedFace - 1 + slot;
    if (idx < 0 || idx >= FACE_COUNT) continue;
    int y = START_Y + slot * ROW_H;
    bool isSel = (slot == 1);
    if (isSel) display.fillRoundRect(0, y, 128, ROW_H - 1, 2, SSD1306_WHITE);
    uint16_t col = isSel ? SSD1306_BLACK : SSD1306_WHITE;
    display.setTextColor(col); display.setTextSize(1);
    display.setCursor(4, y + 3); display.print(idx + 1); display.print(".");
    display.setCursor(18, y + 3); display.print(faceNames[idx]);

    int px = 90, py = y + 3;
    if (idx == 0) {
      display.fillRoundRect(px,    py, 12, 7, 2, col);
      display.fillRoundRect(px+15, py, 12, 7, 2, col);
    } else if (idx == 1) {
      display.fillCircle(px+3,  py+2, 3, col); display.fillCircle(px+8,  py+2, 3, col);
      display.fillTriangle(px,py+4,px+11,py+4,px+5,py+8,col);
      display.fillCircle(px+18, py+2, 3, col); display.fillCircle(px+23, py+2, 3, col);
      display.fillTriangle(px+15,py+4,px+26,py+4,px+20,py+8,col);
    } else if (idx == 2) {
      display.fillTriangle(px+6,py,px,py+8,px+12,py+8,col);
      display.fillTriangle(px+21,py,px+15,py+8,px+27,py+8,col);
    } else if (idx == 3) {
      display.fillRoundRect(px,    py+3, 12, 5, 2, col);
      display.fillRoundRect(px+15, py+3, 12, 5, 2, col);
    } else if (idx == 4) {
      display.fillRoundRect(px+1,  py+3, 10, 3, 1, col);
      display.fillRoundRect(px+16, py+3, 10, 3, 1, col);
    } else if (idx == 5) {
      display.fillRoundRect(px,    py+2, 12, 6, 1, col);
      display.fillTriangle(px+6,py+2,px+12,py+2,px+12,py+6,isSel?SSD1306_WHITE:SSD1306_BLACK);
      display.fillRoundRect(px+15, py+2, 12, 6, 1, col);
      display.fillTriangle(px+15,py+2,px+21,py+2,px+15,py+6,isSel?SSD1306_WHITE:SSD1306_BLACK);
    } else if (idx == 6) {
      display.drawCircle(px+6,  py+4, 5, col);
      display.drawCircle(px+21, py+4, 5, col);
    }
    if (isSel) {
      int ax = 116, ay = y + 4;
      display.fillTriangle(ax, ay, ax, ay+5, ax+4, ay+2, SSD1306_BLACK);
    }
  }

  display.drawLine(0, 56, 128, 56, SSD1306_WHITE);
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 58); display.print("TAP=NEXT  HOLD=SELECT");
  display.display();
}

void blinkSmooth() {
  if (menuState != 0 || eyeExpression != 0 || inSettingsMenu || inMediaMenu || inStopwatch || inPong || inSensors || inScreensaver || inFaceManager || inXO) return;
  for (int h = eyeHeight; h >= 2; h -= 12) {
    display.clearDisplay();
    drawEyeShape(baseLeftX + (int)currentX, baseY + (int)currentY, h);
    drawEyeShape(baseRightX + (int)currentX, baseY + (int)currentY, h);
    display.display();
  }
  for (int h = 2; h <= eyeHeight; h += 12) {
    display.clearDisplay();
    drawEyeShape(baseLeftX + (int)currentX, baseY + (int)currentY, h);
    drawEyeShape(baseRightX + (int)currentX, baseY + (int)currentY, h);
    display.display();
  }
  display.clearDisplay();
  drawEyeShape(baseLeftX + (int)currentX, baseY + (int)currentY, eyeHeight);
  drawEyeShape(baseRightX + (int)currentX, baseY + (int)currentY, eyeHeight);
  display.display();
}

void drawIconBuzzer(int x, int y, uint16_t color) {
  display.fillRect(x,   y+2, 3, 6, color);
  display.fillRect(x+3, y+1, 2, 8, color);
  display.fillRect(x+5, y,   2, 10, color);
  display.drawPixel(x+8, y+3, color);
  display.drawPixel(x+8, y+5, color);
  display.drawPixel(x+9, y+2, color);
  display.drawPixel(x+9, y+7, color);
  display.drawLine(x+8, y+4, x+9, y+4, color);
}

void drawIconAuto(int x, int y, uint16_t color) {
  display.drawLine(x+1, y+3, x+5, y+1, color);
  display.drawLine(x+5, y+1, x+9, y+3, color);
  display.drawPixel(x+7, y+1, color);
  display.drawPixel(x+9, y+1, color);
  display.drawPixel(x+9, y+3, color);
  display.drawLine(x+1, y+7, x+5, y+9, color);
  display.drawLine(x+5, y+9, x+9, y+7, color);
  display.drawPixel(x+1, y+7, color);
  display.drawPixel(x+1, y+9, color);
  display.drawPixel(x+3, y+9, color);
}

void drawIconExit(int x, int y, uint16_t color) {
  display.drawRect(x, y, 7, 10, color);
  display.drawLine(x+8,  y+5, x+11, y+5, color);
  display.drawLine(x+9,  y+3, x+11, y+5, color);
  display.drawLine(x+9,  y+7, x+11, y+5, color);
  display.drawLine(x+4,  y+3, x+4,  y+7, color);
}

void drawIconPrev(int x, int y, uint16_t color) {
  display.fillRect(x, y+1, 2, 9, color);
  display.fillTriangle(x+11, y+1, x+11, y+9, x+5, y+5, color);
  display.fillTriangle(x+6,  y+1, x+6,  y+9, x+2, y+5, color);
}

void drawIconPauseStop(int x, int y, uint16_t color) {
  display.fillTriangle(x, y+1, x, y+9, x+5, y+5, color);
  display.fillRect(x+7,  y+1, 2, 9, color);
  display.fillRect(x+10, y+1, 2, 9, color);
}

void drawIconNext(int x, int y, uint16_t color) {
  display.fillRect(x+10, y+1, 2, 9, color);
  display.fillTriangle(x,   y+1, x,   y+9, x+6, y+5, color);
  display.fillTriangle(x+5, y+1, x+5, y+9, x+9, y+5, color);
}

void drawIconVolDown(int x, int y, uint16_t color) {
  display.fillRect(x,   y+3, 3, 5, color);
  display.fillRect(x+3, y+2, 2, 7, color);
  display.fillRect(x+5, y,   2, 11, color);
  display.drawLine(x+8, y+5, x+12, y+5, color);
}

void drawIconVolUp(int x, int y, uint16_t color) {
  display.fillRect(x,   y+3, 3, 5, color);
  display.fillRect(x+3, y+2, 2, 7, color);
  display.fillRect(x+5, y,   2, 11, color);
  display.drawLine(x+8,  y+5, x+12, y+5, color);
  display.drawLine(x+10, y+3, x+10, y+7, color);
}

void drawIconExitX(int x, int y, uint16_t color) {
  display.drawLine(x,   y,   x+8, y+8, color);
  display.drawLine(x+1, y,   x+9, y+8, color);
  display.drawLine(x+8, y,   x,   y+8, color);
  display.drawLine(x+9, y,   x+1, y+8, color);
}

void drawIconVHS(int x, int y, uint16_t color) {
  display.drawRoundRect(x, y, 16, 10, 1, color);
  display.fillCircle(x+4,  y+6, 2, color);
  display.fillCircle(x+11, y+6, 2, color);
  display.fillRect(x+1, y+1, 14, 3, color);
  display.fillRect(x+3, y+1, 10, 3, SSD1306_BLACK);
}

void drawIconVHSLarge(int x, int y, uint16_t color) {
  display.drawRoundRect(x, y, 32, 20, 2, color);
  display.fillCircle(x+8,  y+13, 4, color);
  display.fillCircle(x+23, y+13, 4, color);
  display.fillRect(x+2, y+2, 28, 6, color);
  display.fillRect(x+6, y+2, 20, 6, SSD1306_BLACK);
}

void drawIconScreensaverSmall(int x, int y, uint16_t color) {
  display.drawRoundRect(x, y, 11, 8, 1, color);
  display.drawLine(x+3, y+8, x+2, y+10, color);
  display.drawLine(x+7, y+8, x+8, y+10, color);
  display.drawLine(x+1, y+10, x+9, y+10, color);
  display.drawPixel(x+5, y+4, color);
  display.drawPixel(x+4, y+3, color);
  display.drawPixel(x+6, y+3, color);
  display.drawPixel(x+4, y+5, color);
  display.drawPixel(x+6, y+5, color);
}

void drawIconScreensaverLarge(int x, int y, uint16_t color) {
  display.drawRoundRect(x, y, 30, 20, 2, color);
  display.drawLine(x+10, y+20, x+8, y+24, color);
  display.drawLine(x+20, y+20, x+22, y+24, color);
  display.drawLine(x+6, y+24, x+24, y+24, color);
  display.fillCircle(x+15, y+10, 4, color);
  display.drawLine(x+15, y+4, x+15, y+6, color);
  display.drawLine(x+15, y+14, x+15, y+16, color);
  display.drawLine(x+9,  y+10, x+11, y+10, color);
  display.drawLine(x+19, y+10, x+21, y+10, color);
}

void initScreensaverMode(int mode) {
  switch(mode) {
    case 0:
      for (int i = 0; i < 3; i++) {
        ssBalls[i].x = random(10, 118);
        ssBalls[i].y = random(10, 54);
        ssBalls[i].dx = (random(0,2) ? 1.8 : -1.8) + random(-5,6)*0.1;
        ssBalls[i].dy = (random(0,2) ? 1.4 : -1.4) + random(-5,6)*0.1;
        ssBalls[i].r = 3 + i;
      }
      break;
    case 1:
      for (int i = 0; i < 16; i++) {
        ssRainY[i] = random(0, 64);
        ssRainLen[i] = random(4, 12);
      }
      break;
    case 2:
      for (int i = 0; i < 20; i++) {
        ssStars[i].x = random(0, 128);
        ssStars[i].y = random(0, 64);
        ssStars[i].speed = 1 + i % 3;
      }
      break;
    case 3:
      ssSnakeLen = 8;
      ssSnakeDir = 0;
      for (int i = 0; i < ssSnakeLen; i++) { ssSnakeX[i] = 20 - i*4; ssSnakeY[i] = 32; }
      ssSnakeFoodX = random(5, 120); ssSnakeFoodY = random(5, 58);
      ssSnakeLast = millis();
      break;
    case 4:
      for (int i = 0; i < 16; i++) ssMatrixY[i] = random(0, 64);
      ssMatrixLast = millis();
      break;
    case 5:
      for (int i = 0; i < 20; i++) ssParticles[i].active = false;
      ssFireLast = millis();
      break;
    case 6:
      for (int i = 0; i < 4; i++) ssRipples[i].active = false;
      ssRippleLast = millis();
      break;
    case 7: {

      for (int y = 0; y < MAZE_ROWS; y++)
        for (int x = 0; x < MAZE_COLS; x++) {
          ssMazeWalls[y][x] = 0;
          ssMazeVisited[y][x] = false;
        }
      ssMazeStkTop = 0;
      ssMazeStkX[0] = 0; ssMazeStkY[0] = 0;
      ssMazeStkTop = 1;
      ssMazeVisited[0][0] = true;
      ssMazeGenDone = false;
      ssMazeLast = millis();

      ssMazeCurX = 0; ssMazeCurY = 0; ssMazeCurDir = 0;
      ssMazeCurLast = millis();
      display.clearDisplay();
      break;
    }
    case 8:
      ssPlasmaT = 0;
      ssPlasmeLast = millis();
      break;
    case 9:
      ssClockSec = 0;
      ssClockLast = millis();
      break;
    case 10:
      ssLissT = 0;
      ssLissLast = millis();
      ssLissHead = 0;
      for (int i = 0; i < 80; i++) { ssLissX[i] = 64; ssLissY[i] = 32; }
      break;
    case 11:
      ssSpiralT = 0;
      ssSpiralLast = millis();
      break;
    case 12:
      for (int i = 0; i < 60; i++) {
        ssWarpStars[i].x = random(-100, 100);
        ssWarpStars[i].y = random(-50, 50);
        ssWarpStars[i].z = random(1, 100);
      }
      ssWarpLast = millis();
      break;
    case 13:
      for (int i = 0; i < 16; i++) {
        ssEqH[i] = random(2, 30);
        ssEqTarget[i] = random(2, 45);
      }
      ssEqLast = millis();
      break;
    case 14:
      for (int y = 0; y < GOL_H; y++)
        for (int x = 0; x < GOL_W; x++)
          ssGolGrid[y][x] = random(3) == 0 ? 1 : 0;
      ssGolLast = millis();
      ssGolGen = 0;
      break;
    case 15:
      for (int y = 0; y < SAND_GH; y++)
        for (int x = 0; x < SAND_GW; x++)
          ssSandGrid2[y][x] = false;
      ssSandLast = millis();
      ssSandSpawn = 0;
      break;
    case 16:
      ssPendA1 = 2.0; ssPendA2 = 2.5;
      ssPendV1 = 0;   ssPendV2 = 0;
      ssPendTHead = 0;
      for (int i = 0; i < 60; i++) { ssPendTX[i] = 64; ssPendTY[i] = 20; }
      ssPendLast = millis();
      break;
  }
}

void drawScreensaverSelection() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  drawIconScreensaverSmall(3, 1, SSD1306_BLACK);
  display.setTextSize(1); display.setTextColor(SSD1306_BLACK);
  display.setCursor(18, 3); display.print("SCREENSAVER");
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);

  const int ROW_H = 14;
  const int START_Y = 14;

  for (int slot = 0; slot < 3; slot++) {
    int idx = screensaverSelected - 1 + slot;

    if (idx < 0 || idx >= SS_COUNT) continue;
    int y = START_Y + slot * ROW_H;
    bool isSel = (slot == 1);

    if (isSel) {
      display.fillRoundRect(0, y, 128, ROW_H - 1, 2, SSD1306_WHITE);
    }

    uint16_t col = isSel ? SSD1306_BLACK : SSD1306_WHITE;
    display.setTextColor(col);
    display.setTextSize(1);

    display.setCursor(4, y + 3);
    display.print(idx + 1);
    display.print(".");

    display.setCursor(18, y + 3);
    display.print(ssNames[idx]);

    if (isSel) {
      int ax = 116, ay = y + 4;
      display.fillTriangle(ax, ay, ax, ay + 5, ax + 4, ay + 2, SSD1306_BLACK);
    }
  }

  display.drawLine(0, 56, 128, 56, SSD1306_WHITE);

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 58);
  display.print("TAP=NEXT  HOLD=START");

  display.display();
}

void updateDrawScreensaver(int mode) {
  unsigned long now = millis();

  switch(mode) {
    case 0: {
      display.clearDisplay();
      for (int i = 0; i < 3; i++) {
        ssBalls[i].x += ssBalls[i].dx;
        ssBalls[i].y += ssBalls[i].dy;
        if (ssBalls[i].x - ssBalls[i].r <= 0)  { ssBalls[i].x = ssBalls[i].r; ssBalls[i].dx = abs(ssBalls[i].dx); }
        if (ssBalls[i].x + ssBalls[i].r >= 127) { ssBalls[i].x = 127 - ssBalls[i].r; ssBalls[i].dx = -abs(ssBalls[i].dx); }
        if (ssBalls[i].y - ssBalls[i].r <= 0)  { ssBalls[i].y = ssBalls[i].r; ssBalls[i].dy = abs(ssBalls[i].dy); }
        if (ssBalls[i].y + ssBalls[i].r >= 63) { ssBalls[i].y = 63 - ssBalls[i].r; ssBalls[i].dy = -abs(ssBalls[i].dy); }
        display.drawCircle((int)ssBalls[i].x, (int)ssBalls[i].y, ssBalls[i].r, SSD1306_WHITE);
        display.fillCircle((int)ssBalls[i].x, (int)ssBalls[i].y, ssBalls[i].r - 1, SSD1306_WHITE);
      }
      display.display();
      return;
    }
    case 1: {
      display.clearDisplay();
      for (int i = 0; i < 16; i++) {
        int xpos = i * 8 + 4;
        ssRainY[i] += 3;
        if (ssRainY[i] > 64 + ssRainLen[i]) {
          ssRainY[i] = -ssRainLen[i];
          ssRainLen[i] = random(4, 14);
        }
        for (int d = 0; d < ssRainLen[i]; d++) {
          int py = ssRainY[i] - d;
          if (py >= 0 && py < 64) {
            display.drawPixel(xpos, py, SSD1306_WHITE);
            if (d == 0 && xpos-1>=0) display.drawPixel(xpos-1, py, SSD1306_WHITE);
          }
        }
      }
      display.display();
      return;
    }
    case 2: {
      display.clearDisplay();
      for (int i = 0; i < 20; i++) {
        ssStars[i].x -= ssStars[i].speed;
        if (ssStars[i].x < 0) { ssStars[i].x = 127; ssStars[i].y = random(0, 64); }
        display.drawPixel(ssStars[i].x, ssStars[i].y, SSD1306_WHITE);
        if (ssStars[i].speed >= 2) display.drawPixel(ssStars[i].x+1, ssStars[i].y, SSD1306_WHITE);
        if (ssStars[i].speed == 3) display.drawPixel(ssStars[i].x+2, ssStars[i].y, SSD1306_WHITE);
      }
      display.display();
      return;
    }
    case 3: {
      if (now - ssSnakeLast > 120) {
        ssSnakeLast = now;
        int hx = ssSnakeX[0], hy = ssSnakeY[0];
        int dx = ssSnakeFoodX - hx, dy = ssSnakeFoodY - hy;
        if (abs(dx) > abs(dy)) ssSnakeDir = (dx > 0) ? 0 : 2;
        else ssSnakeDir = (dy > 0) ? 1 : 3;
        for (int i = ssSnakeLen - 1; i > 0; i--) { ssSnakeX[i] = ssSnakeX[i-1]; ssSnakeY[i] = ssSnakeY[i-1]; }
        if (ssSnakeDir==0) ssSnakeX[0] += 4;
        else if (ssSnakeDir==1) ssSnakeY[0] += 4;
        else if (ssSnakeDir==2) ssSnakeX[0] -= 4;
        else ssSnakeY[0] -= 4;
        if (ssSnakeX[0] < 0) ssSnakeX[0] = 124;
        if (ssSnakeX[0] > 127) ssSnakeX[0] = 0;
        if (ssSnakeY[0] < 0) ssSnakeY[0] = 60;
        if (ssSnakeY[0] > 63) ssSnakeY[0] = 0;
        if (abs(ssSnakeX[0]-ssSnakeFoodX) < 5 && abs(ssSnakeY[0]-ssSnakeFoodY) < 5) {
          ssSnakeFoodX = random(5, 120); ssSnakeFoodY = random(5, 58);
          if (ssSnakeLen < 30) ssSnakeLen++;
        }
        display.clearDisplay();
        for (int i = 0; i < ssSnakeLen; i++)
          display.fillRect(ssSnakeX[i]-2, ssSnakeY[i]-2, 4, 4, SSD1306_WHITE);
        display.fillRect(ssSnakeFoodX-1, ssSnakeFoodY-1, 3, 3, SSD1306_WHITE);
        display.display();
      }
      return;
    }
    case 4: {
      if (now - ssMatrixLast > 80) {
        ssMatrixLast = now;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        for (int i = 0; i < 16; i++) {
          ssMatrixY[i] += 8;
          if (ssMatrixY[i] > 64) ssMatrixY[i] = random(-20, 0);
          int xpos = i * 8;
          for (int j = 0; j < 4; j++) {
            int ypos = ssMatrixY[i] - j * 8;
            if (ypos >= 0 && ypos < 64) {
              char c = '0' + random(10);
              display.setCursor(xpos, ypos);
              display.print(c);
            }
          }
        }
        display.display();
      }
      return;
    }
    case 5: {
      if (now - ssFireLast > 600) {
        ssFireLast = now;
        int cx = random(20, 108), cy = random(10, 40);
        int count = 0;
        for (int i = 0; i < 20 && count < 8; i++) {
          if (!ssParticles[i].active) {
            ssParticles[i].active = true;
            ssParticles[i].x = cx; ssParticles[i].y = cy;
            float angle = count * 3.14159 * 2 / 8;
            float speed = 1.5 + random(0,10)*0.15;
            ssParticles[i].dx = cos(angle) * speed;
            ssParticles[i].dy = sin(angle) * speed;
            count++;
          }
        }
      }
      display.clearDisplay();
      for (int i = 0; i < 20; i++) {
        if (ssParticles[i].active) {
          ssParticles[i].x += ssParticles[i].dx;
          ssParticles[i].y += ssParticles[i].dy;
          ssParticles[i].dy += 0.08;
          if (ssParticles[i].x < 0 || ssParticles[i].x > 127 || ssParticles[i].y > 63)
            ssParticles[i].active = false;
          else
            display.drawPixel((int)ssParticles[i].x, (int)ssParticles[i].y, SSD1306_WHITE);
        }
      }
      display.display();
      return;
    }
    case 6: {
      if (now - ssRippleLast > 700) {
        ssRippleLast = now;
        for (int i = 0; i < 4; i++) {
          if (!ssRipples[i].active) {
            ssRipples[i].cx = random(20, 108);
            ssRipples[i].cy = random(10, 54);
            ssRipples[i].r = 1;
            ssRipples[i].active = true;
            break;
          }
        }
      }
      display.clearDisplay();
      for (int i = 0; i < 4; i++) {
        if (ssRipples[i].active) {
          display.drawCircle(ssRipples[i].cx, ssRipples[i].cy, ssRipples[i].r, SSD1306_WHITE);
          if (ssRipples[i].r > 2)
            display.drawCircle(ssRipples[i].cx, ssRipples[i].cy, ssRipples[i].r - 2, SSD1306_WHITE);
          ssRipples[i].r += 2;
          if (ssRipples[i].r > 40) ssRipples[i].active = false;
        }
      }
      display.display();
      return;
    }
    case 7: {

      if (!ssMazeGenDone && now - ssMazeLast > 10) {
        ssMazeLast = now;

        for (int step = 0; step < 4 && !ssMazeGenDone; step++) {
          if (ssMazeStkTop == 0) { ssMazeGenDone = true; break; }
          int cx = ssMazeStkX[ssMazeStkTop - 1];
          int cy = ssMazeStkY[ssMazeStkTop - 1];

          int dirs[4] = {0,1,2,3};

          for (int i = 3; i > 0; i--) {
            int j = random(i + 1);
            int tmp = dirs[i]; dirs[i] = dirs[j]; dirs[j] = tmp;
          }
          bool found = false;
          for (int d = 0; d < 4; d++) {
            int nx = cx, ny = cy;
            if      (dirs[d] == 0) nx++;
            else if (dirs[d] == 1) ny++;
            else if (dirs[d] == 2) nx--;
            else                   ny--;
            if (nx >= 0 && nx < MAZE_COLS && ny >= 0 && ny < MAZE_ROWS && !ssMazeVisited[ny][nx]) {

              if      (dirs[d] == 0) ssMazeWalls[cy][cx]  |= 0x01;
              else if (dirs[d] == 1) ssMazeWalls[cy][cx]  |= 0x02;
              else if (dirs[d] == 2) ssMazeWalls[ny][nx]  |= 0x01;
              else                   ssMazeWalls[ny][nx]  |= 0x02;
              ssMazeVisited[ny][nx] = true;
              ssMazeStkX[ssMazeStkTop] = nx;
              ssMazeStkY[ssMazeStkTop] = ny;
              ssMazeStkTop++;
              found = true;
              break;
            }
          }
          if (!found) ssMazeStkTop--;
        }

        display.clearDisplay();
        for (int gy = 0; gy < MAZE_ROWS; gy++) {
          for (int gx = 0; gx < MAZE_COLS; gx++) {
            int px = gx * 8;
            int py = gy * 8;

            if (!(ssMazeWalls[gy][gx] & 0x01) && gx < MAZE_COLS - 1)
              display.drawLine(px+8, py, px+8, py+8, SSD1306_WHITE);

            if (!(ssMazeWalls[gy][gx] & 0x02) && gy < MAZE_ROWS - 1)
              display.drawLine(px, py+8, px+8, py+8, SSD1306_WHITE);
          }
        }

        display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
        display.display();
        return;
      }

      if (ssMazeGenDone) {

        if (now - ssMazeCurLast > 80) {
          ssMazeCurLast = now;

          int tries = 0;
          while (tries < 4) {
            int nx = ssMazeCurX, ny = ssMazeCurY;
            bool canGo = false;
            if      (ssMazeCurDir == 0) { nx++; canGo = (nx < MAZE_COLS) && (ssMazeWalls[ssMazeCurY][ssMazeCurX] & 0x01); }
            else if (ssMazeCurDir == 1) { ny++; canGo = (ny < MAZE_ROWS) && (ssMazeWalls[ssMazeCurY][ssMazeCurX] & 0x02); }
            else if (ssMazeCurDir == 2) { nx--; canGo = (nx >= 0)        && (nx >= 0) && (ssMazeWalls[ssMazeCurY][nx] & 0x01); }
            else                        { ny--; canGo = (ny >= 0)        && (ny >= 0) && (ssMazeWalls[ny][ssMazeCurX] & 0x02); }
            if (canGo) { ssMazeCurX = nx; ssMazeCurY = ny; break; }
            ssMazeCurDir = random(4);
            tries++;
          }
        }

        display.clearDisplay();
        for (int gy = 0; gy < MAZE_ROWS; gy++) {
          for (int gx = 0; gx < MAZE_COLS; gx++) {
            int px = gx * 8, py = gy * 8;
            if (!(ssMazeWalls[gy][gx] & 0x01) && gx < MAZE_COLS - 1)
              display.drawLine(px+8, py, px+8, py+8, SSD1306_WHITE);
            if (!(ssMazeWalls[gy][gx] & 0x02) && gy < MAZE_ROWS - 1)
              display.drawLine(px, py+8, px+8, py+8, SSD1306_WHITE);
          }
        }
        display.drawRect(0, 0, 128, 64, SSD1306_WHITE);

        int cpx = ssMazeCurX * 8 + 2, cpy = ssMazeCurY * 8 + 2;
        display.fillRect(cpx, cpy, 4, 4, SSD1306_WHITE);
        display.display();
        return;
      }
      display.display();
      return;
    }
    case 8: {
      if (now - ssPlasmeLast > 60) {
        ssPlasmeLast = now;
        ssPlasmaT += 0.18;
        display.clearDisplay();
        for (int gy = 0; gy < 4; gy++) {
          for (int gx = 0; gx < 8; gx++) {
            float cx = gx * 16 + 8, cy = gy * 16 + 8;
            float v = sin(cx * 0.05 + ssPlasmaT)
                    + sin(cy * 0.08 + ssPlasmaT * 0.7)
                    + sin((cx + cy) * 0.04 + ssPlasmaT * 1.3);
            int density = (int)((v + 3.0) / 6.0 * 5);
            for (int dy = 0; dy < 16; dy++) {
              for (int dx = 0; dx < 16; dx++) {
                const int bayer[4][4] = {{0,8,2,10},{12,4,14,6},{3,11,1,9},{15,7,13,5}};
                int thr = bayer[dy & 3][dx & 3];
                if (density * 3 > thr)
                  display.drawPixel(gx*16 + dx, gy*16 + dy, SSD1306_WHITE);
              }
            }
          }
        }
        display.display();
      }
      return;
    }
    case 9: {
      if (now - ssClockLast > 50) {
        ssClockLast = now;
        ssClockSec += 0.5;
        if (ssClockSec >= 60) ssClockSec = 0;
        display.clearDisplay();
        int cx = 64, cy = 32, r = 28;
        display.drawCircle(cx, cy, r, SSD1306_WHITE);
        display.drawCircle(cx, cy, r - 1, SSD1306_WHITE);
        for (int h = 0; h < 12; h++) {
          float a = h * 3.14159 * 2 / 12 - 3.14159 / 2;
          int x1 = cx + (int)(cos(a) * (r - 2));
          int y1 = cy + (int)(sin(a) * (r - 2));
          int x2 = cx + (int)(cos(a) * (r - 5));
          int y2 = cy + (int)(sin(a) * (r - 5));
          display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
        }
        float sa = ssClockSec / 60.0 * 3.14159 * 2 - 3.14159 / 2;
        display.drawLine(cx, cy, cx + (int)(cos(sa)*(r-4)), cy + (int)(sin(sa)*(r-4)), SSD1306_WHITE);
        float ma = (ssClockSec / 4.0) / 60.0 * 3.14159 * 2 - 3.14159 / 2;
        int mlen = r - 8;
        display.drawLine(cx, cy, cx + (int)(cos(ma)*mlen), cy + (int)(sin(ma)*mlen), SSD1306_WHITE);
        display.drawLine(cx+1, cy, cx+1 + (int)(cos(ma)*mlen), cy + (int)(sin(ma)*mlen), SSD1306_WHITE);
        float ha = (ssClockSec / 12.0) / 60.0 * 3.14159 * 2 - 3.14159 / 2;
        int hlen = r - 13;
        display.drawLine(cx, cy, cx + (int)(cos(ha)*hlen), cy + (int)(sin(ha)*hlen), SSD1306_WHITE);
        display.drawLine(cx+1, cy, cx+1 + (int)(cos(ha)*hlen), cy + (int)(sin(ha)*hlen), SSD1306_WHITE);
        display.drawLine(cx, cy+1, cx + (int)(cos(ha)*hlen), cy+1 + (int)(sin(ha)*hlen), SSD1306_WHITE);
        display.fillCircle(cx, cy, 2, SSD1306_WHITE);
        display.display();
      }
      return;
    }
    case 10: {
      if (now - ssLissLast > 30) {
        ssLissLast = now;
        ssLissT += 0.07;
        float lx = 64 + 55 * sin(3.0 * ssLissT + 0.785);
        float ly = 32 + 28 * sin(2.0 * ssLissT);
        ssLissX[ssLissHead] = (int)lx;
        ssLissY[ssLissHead] = (int)ly;
        ssLissHead = (ssLissHead + 1) % 80;
        display.clearDisplay();
        for (int i = 0; i < 80; i++) {
          int idx = (ssLissHead + i) % 80;
          if (i > 20 || (i % 2 == 0))
            display.drawPixel(ssLissX[idx], ssLissY[idx], SSD1306_WHITE);
          if (i > 50)
            display.drawPixel(ssLissX[idx]+1, ssLissY[idx], SSD1306_WHITE);
        }
        display.display();
      }
      return;
    }
    case 11: {
      if (now - ssSpiralLast > 25) {
        ssSpiralLast = now;
        ssSpiralT += 0.15;
        display.clearDisplay();
        for (int arm = 0; arm < 3; arm++) {
          float offset = arm * 3.14159 * 2 / 3;
          for (float t = 0; t < 8.0 * 3.14159; t += 0.12) {
            float r2 = t * 3.8;
            if (r2 > 31) break;
            float angle = t + ssSpiralT + offset;
            int px = 64 + (int)(cos(angle) * r2);
            int py = 32 + (int)(sin(angle) * r2);
            if (px >= 0 && px < 128 && py >= 0 && py < 64)
              display.drawPixel(px, py, SSD1306_WHITE);
            if (t > 1.0 && px+1 < 128)
              display.drawPixel(px+1, py, SSD1306_WHITE);
          }
        }
        display.display();
      }
      return;
    }
    case 12: {
      if (now - ssWarpLast > 30) {
        ssWarpLast = now;
        display.clearDisplay();
        for (int i = 0; i < 60; i++) {
          ssWarpStars[i].z -= 2.5;
          if (ssWarpStars[i].z <= 0) {
            ssWarpStars[i].x = random(-100, 100);
            ssWarpStars[i].y = random(-50, 50);
            ssWarpStars[i].z = 100;
          }
          float fz = ssWarpStars[i].z / 100.0;
          int sx = 64 + (int)(ssWarpStars[i].x / fz * 0.6);
          int sy = 32 + (int)(ssWarpStars[i].y / fz * 0.6);
          if (sx < 0 || sx >= 128 || sy < 0 || sy >= 64) {
            ssWarpStars[i].z = 100;
            continue;
          }
          int sz = (int)((1.0 - fz) * 3) + 1;
          if (sz == 1) display.drawPixel(sx, sy, SSD1306_WHITE);
          else         display.fillCircle(sx, sy, sz - 1, SSD1306_WHITE);
          if (ssWarpStars[i].z < 60) {
            float fz2 = (ssWarpStars[i].z + 5) / 100.0;
            int ox = 64 + (int)(ssWarpStars[i].x / fz2 * 0.6);
            int oy = 32 + (int)(ssWarpStars[i].y / fz2 * 0.6);
            display.drawLine(ox, oy, sx, sy, SSD1306_WHITE);
          }
        }
        display.display();
      }
      return;
    }
    case 13: {
      if (now - ssEqLast > 80) {
        ssEqLast = now;
        display.clearDisplay();
        for (int i = 0; i < 16; i++) {
          if (ssEqH[i] < ssEqTarget[i]) ssEqH[i] += random(1, 4);
          else if (ssEqH[i] > ssEqTarget[i]) ssEqH[i] -= random(1, 3);
          if (abs(ssEqH[i] - ssEqTarget[i]) < 3)
            ssEqTarget[i] = random(2, 50);
          int h = constrain(ssEqH[i], 2, 50);
          int x = i * 8;
          int y = 62 - h;
          display.fillRect(x + 1, y, 6, h, SSD1306_WHITE);

          if (y >= 2) display.drawLine(x + 1, y - 2, x + 6, y - 2, SSD1306_WHITE);
        }
        display.drawLine(0, 63, 127, 63, SSD1306_WHITE);
        display.display();
      }
      return;
    }
    case 14: {
      if (now - ssGolLast > 150) {
        ssGolLast = now;
        ssGolGen++;
        for (int y = 0; y < GOL_H; y++) {
          for (int x = 0; x < GOL_W; x++) {
            int n = 0;
            for (int dy = -1; dy <= 1; dy++)
              for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;
                int nx2 = (x + dx + GOL_W) % GOL_W;
                int ny2 = (y + dy + GOL_H) % GOL_H;
                n += ssGolGrid[ny2][nx2];
              }
            if (ssGolGrid[y][x])
              ssGolNext[y][x] = (n == 2 || n == 3) ? 1 : 0;
            else
              ssGolNext[y][x] = (n == 3) ? 1 : 0;
          }
        }
        memcpy(ssGolGrid, ssGolNext, sizeof(ssGolGrid));
        int alive = 0;
        for (int y = 0; y < GOL_H; y++)
          for (int x = 0; x < GOL_W; x++)
            alive += ssGolGrid[y][x];
        if (alive < 10 || ssGolGen > 200) {
          for (int y = 0; y < GOL_H; y++)
            for (int x = 0; x < GOL_W; x++)
              ssGolGrid[y][x] = random(3) == 0 ? 1 : 0;
          ssGolGen = 0;
        }
        display.clearDisplay();
        for (int y = 0; y < GOL_H; y++)
          for (int x = 0; x < GOL_W; x++)
            if (ssGolGrid[y][x])
              display.fillRect(x * 4, y * 4, 3, 3, SSD1306_WHITE);
        display.display();
      }
      return;
    }
    case 15: {
      if (now - ssSandLast > 40) {
        ssSandLast = now;
        if (ssSandSpawn < SAND_GW * SAND_GH / 3) {
          int sx = random(SAND_GW / 3, SAND_GW * 2 / 3);
          if (!ssSandGrid2[0][sx]) {
            ssSandGrid2[0][sx] = true;
            ssSandSpawn++;
          }
        }
        for (int y = SAND_GH - 2; y >= 0; y--) {
          for (int x = 0; x < SAND_GW; x++) {
            if (!ssSandGrid2[y][x]) continue;
            if (!ssSandGrid2[y + 1][x]) {
              ssSandGrid2[y + 1][x] = true;
              ssSandGrid2[y][x] = false;
            } else {
              int side = random(2) ? 1 : -1;
              int nx2 = x + side;
              if (nx2 >= 0 && nx2 < SAND_GW && !ssSandGrid2[y + 1][nx2]) {
                ssSandGrid2[y + 1][nx2] = true;
                ssSandGrid2[y][x] = false;
              } else {
                nx2 = x - side;
                if (nx2 >= 0 && nx2 < SAND_GW && !ssSandGrid2[y + 1][nx2]) {
                  ssSandGrid2[y + 1][nx2] = true;
                  ssSandGrid2[y][x] = false;
                }
              }
            }
          }
        }
        display.clearDisplay();
        for (int y = 0; y < SAND_GH; y++)
          for (int x = 0; x < SAND_GW; x++)
            if (ssSandGrid2[y][x])
              display.fillRect(x * 2, y * 2, 2, 2, SSD1306_WHITE);
        display.display();
      }
      return;
    }
    case 16: {
      if (now - ssPendLast > 20) {
        ssPendLast = now;
        float g = 9.8, l1 = 1.0, l2 = 1.0;
        float dt = 0.05;
        float d = ssPendA1 - ssPendA2;
        float sd = sin(d), cd = cos(d);
        float s1 = sin(ssPendA1), s2 = sin(ssPendA2);
        float denom = 2.0 - cd * cd;
        float acc1 = (-g*(2*s1+s2*cd) - sin(d)*(ssPendV2*ssPendV2*l2+ssPendV1*ssPendV1*l1*cd)) / (l1 * denom);
        float acc2 = ((2*s1*cd+2*s2) * g + sin(d)*(2*ssPendV1*ssPendV1*l1+ssPendV2*ssPendV2*l2*cd)) / (l2 * denom);
        ssPendV1 += acc1 * dt; ssPendV2 += acc2 * dt;
        ssPendV1 *= 0.999; ssPendV2 *= 0.999;
        ssPendA1 += ssPendV1 * dt; ssPendA2 += ssPendV2 * dt;
        int px0 = 64, py0 = 10;
        int L = 22;
        int x1 = px0 + (int)(sin(ssPendA1) * L);
        int y1 = py0 + (int)(cos(ssPendA1) * L);
        int x2 = x1  + (int)(sin(ssPendA2) * L);
        int y2 = y1  + (int)(cos(ssPendA2) * L);
        ssPendTX[ssPendTHead] = x2;
        ssPendTY[ssPendTHead] = y2;
        ssPendTHead = (ssPendTHead + 1) % 60;
        display.clearDisplay();
        for (int i = 0; i < 60; i++) {
          int idx = (ssPendTHead + i) % 60;
          if (i > 10) display.drawPixel(ssPendTX[idx], ssPendTY[idx], SSD1306_WHITE);
          if (i > 40) display.drawPixel(ssPendTX[idx]+1, ssPendTY[idx], SSD1306_WHITE);
        }
        display.drawLine(px0, py0, x1, y1, SSD1306_WHITE);
        display.drawLine(px0+1, py0, x1+1, y1, SSD1306_WHITE);
        display.drawLine(x1, y1, x2, y2, SSD1306_WHITE);
        display.drawLine(x1+1, y1, x2+1, y2, SSD1306_WHITE);
        display.fillCircle(px0, py0, 2, SSD1306_WHITE);
        display.fillCircle(x1, y1, 3, SSD1306_WHITE);
        display.fillCircle(x2, y2, 4, SSD1306_WHITE);
        display.display();
      }
      return;
    }
  }
  display.display();
}

void drawIconStopwatch(int x, int y, uint16_t color) {
  display.drawCircle(x+10, y+12, 9, color);
  display.drawLine(x+7,  y+1, x+13, y+1, color);
  display.drawLine(x+10, y+1, x+10, y+3, color);
  display.drawLine(x+10, y+3, x+10, y+8, color);
}

void drawIconStopwatchLarge(int x, int y, uint16_t color) {
  display.drawCircle(x+12, y+15, 12, color);
  display.drawLine(x+9,  y+1, x+15, y+1, color);
  display.drawLine(x+12, y+1, x+12, y+4, color);
  display.drawLine(x+12, y+4, x+12, y+10, color);
  display.drawPixel(x+18, y+5, color);
  display.drawPixel(x+19, y+6, color);
}

void drawStopwatch() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
  drawIconStopwatch(3, 2, SSD1306_BLACK);
  display.setTextSize(1); display.setTextColor(SSD1306_BLACK);
  display.setCursor(26, 3); display.print("CRONOMETRU");
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);

  unsigned long elapsed = swElapsed;
  if (swRunning) elapsed += millis() - swStartTime;

  unsigned long totalSec = elapsed / 1000;
  unsigned long hh = totalSec / 3600;
  unsigned long mm = (totalSec % 3600) / 60;
  unsigned long ss = totalSec % 60;

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);
  char buf[9];
  sprintf(buf, "%02lu:%02lu:%02lu", hh, mm, ss);

  display.setTextSize(2);
  display.setCursor(16, 28);
  display.print(buf);

  display.setTextSize(1);
  if (swRunning) {
    display.setCursor(10, 52);
    display.print("TAP=STOP  HOLD=RESET");
  } else {
    display.setCursor(10, 52);
    display.print("TAP=START HOLD=EXIT");
  }

  display.display();
}

void drawIconPongLarge(int x, int y, uint16_t color) {
  display.fillRect(x,    y+3,  2, 14, color);
  display.fillRect(x+28, y+3,  2, 14, color);
  display.fillCircle(x+15, y+10, 2, color);
  display.drawLine(x, y, x+30, y, color);
  display.drawLine(x, y+20, x+30, y+20, color);
}

void drawPong() {
  display.clearDisplay();

  if (pongLost) {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(28, 8);
    display.print("YOU");
    display.setCursor(28, 26);
    display.print("LOST");
    display.setTextSize(1);
    display.setCursor(10, 52);
    display.print("TAP=RETRY HOLD=EXIT");
    display.display();
    return;
  }

  display.drawLine(0, 0, 128, 0, SSD1306_WHITE);
  display.drawLine(0, 63, 128, 63, SSD1306_WHITE);

  for (int yy = 2; yy < 62; yy += 5)
    display.fillRect(63, yy, 2, 3, SSD1306_WHITE);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(44, 2);
  display.print(pongScore);
  display.setCursor(72, 2);
  display.print("AI");

  display.fillRect(1,   playerY - PAD_H/2, PAD_W, PAD_H, SSD1306_WHITE);
  display.fillRect(124, aiY     - PAD_H/2, PAD_W, PAD_H, SSD1306_WHITE);

  display.fillRect((int)ballX - 2, (int)ballY - 2, 4, 4, SSD1306_WHITE);

  display.display();
}

void updatePong(bool held) {
  if (pongLost) return;

  int speed = 3;
  if (held) {
    if (paddleGoingDown) {
      playerY += speed;
      if (playerY + PAD_H/2 >= 62) { playerY = 62 - PAD_H/2; paddleGoingDown = false; }
    } else {
      playerY -= speed;
      if (playerY - PAD_H/2 <= 2) { playerY = 2 + PAD_H/2; paddleGoingDown = true; }
    }
  }

  int aiSpeed = 2;
  if (aiY < ballY - 2) aiY = min(aiY + aiSpeed, 62 - PAD_H/2);
  if (aiY > ballY + 2) aiY = max(aiY - aiSpeed, 2  + PAD_H/2);

  ballX += ballDX;
  ballY += ballDY;

  if (ballY <= 2)  { ballY = 2;  ballDY = abs(ballDY); }
  if (ballY >= 62) { ballY = 62; ballDY = -abs(ballDY); }

  if (ballX <= 6 && ballDX < 0) {
    if (ballY >= playerY - PAD_H/2 - 3 && ballY <= playerY + PAD_H/2 + 3) {
      ballX = 7;
      ballDX = abs(ballDX) * 1.04;
      if (ballDX > 4.5) ballDX = 4.5;
      ballDY += random(-10, 11) * 0.12;
    }
  }

  if (ballX >= 122 && ballDX > 0) {
    if (ballY >= aiY - PAD_H/2 - 3 && ballY <= aiY + PAD_H/2 + 3) {
      ballX = 121;
      ballDX = -abs(ballDX) * 1.04;
      if (ballDX < -4.5) ballDX = -4.5;
      ballDY += random(-10, 11) * 0.12;
    }
  }

  if (ballX > 128) {
    pongScore++;
    playTone(1800, 60);
    ballX = 64; ballY = 32;
    ballDX = -2.2; ballDY = 1.5;
  }

  if (ballX < 0) {
    pongLost = true;
    playTone(400, 300);
  }
}

void drawSensors() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);

  display.drawRoundRect(3, 2, 4, 8, 1, SSD1306_BLACK);
  display.fillCircle(5, 11, 2, SSD1306_BLACK);
  display.fillRect(4, 4, 2, 6, SSD1306_BLACK);
  display.setTextSize(1); display.setTextColor(SSD1306_BLACK);
  display.setCursor(12, 3); display.print("SENSORS");
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);

  display.drawRoundRect(4, 17, 7, 18, 3, SSD1306_WHITE);
  display.fillCircle(7, 37, 5, SSD1306_WHITE);
  display.fillRect(6, 21, 3, 14, SSD1306_WHITE);
  display.setTextSize(3); display.setCursor(18, 17);
  display.setTextColor(SSD1306_WHITE);
  display.print((int)currentTemp);
  display.drawCircle(74, 19, 2, SSD1306_WHITE);
  display.setTextSize(2); display.setCursor(79, 22); display.print("C");

  display.drawLine(0, 43, 128, 43, SSD1306_WHITE);

  display.drawCircle(10, 54, 7, SSD1306_WHITE);
  display.drawCircle(10, 54, 2, SSD1306_WHITE);
  display.drawLine(10, 54, 15, 48, SSD1306_WHITE);
  display.setTextSize(1); display.setCursor(22, 48);
  display.print(currentPressure);
  display.setCursor(22, 57); display.print("hPa");

  display.display();
}

void xoReset() {
  for(int i = 0; i < 9; i++) xoBoard[i] = 0;
  xoCursor = 0; xoGameOver = false; xoWinner = 0;
  xoPlayerTurn = true; xoAiThinking = false;
}

int xoCheckWinner() {
  const int lines[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},
    {0,3,6},{1,4,7},{2,5,8},
    {0,4,8},{2,4,6}
  };
  for(int i = 0; i < 8; i++) {
    int a=lines[i][0], b=lines[i][1], c=lines[i][2];
    if(xoBoard[a] && xoBoard[a]==xoBoard[b] && xoBoard[b]==xoBoard[c])
      return xoBoard[a];
  }
  bool full = true;
  for(int i = 0; i < 9; i++) if(!xoBoard[i]) { full=false; break; }
  return full ? 3 : 0;
}

int xoAiMove() {
  const int lines[8][3] = {
    {0,1,2},{3,4,5},{6,7,8},
    {0,3,6},{1,4,7},{2,5,8},
    {0,4,8},{2,4,6}
  };
  for(int i = 0; i < 8; i++) {
    int cntO=0, emptyIdx=-1;
    for(int j=0;j<3;j++){ if(xoBoard[lines[i][j]]==2) cntO++; else if(xoBoard[lines[i][j]]==0) emptyIdx=lines[i][j]; }
    if(cntO==2 && emptyIdx>=0) return emptyIdx;
  }
  for(int i = 0; i < 8; i++) {
    int cntX=0, emptyIdx=-1;
    for(int j=0;j<3;j++){ if(xoBoard[lines[i][j]]==1) cntX++; else if(xoBoard[lines[i][j]]==0) emptyIdx=lines[i][j]; }
    if(cntX==2 && emptyIdx>=0) return emptyIdx;
  }
  if(!xoBoard[4]) return 4;
  int corners[4]={0,2,6,8};
  for(int i=0;i<4;i++) if(!xoBoard[corners[i]]) return corners[i];
  for(int i=0;i<9;i++) if(!xoBoard[i]) return i;
  return -1;
}

void drawXO() {
  display.clearDisplay();
  const int SX = 37, SY = 5, CS = 18;
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(2, 2); display.print("X&O");
  if(!xoGameOver) {
    display.setCursor(1, 16);
    if(xoPlayerTurn) {
      display.print("YOUR");
      display.setCursor(1, 25); display.print("TURN");
      display.setCursor(4, 36); display.print("X");
    } else {
      display.print("AI");
      display.setCursor(1, 25); display.print("THINK");
    }
    display.setCursor(0, 50); display.print("TAP=");
    display.setCursor(0, 58); display.print("MOVE");
  } else {
    if(xoWinner==1){ display.setCursor(1,14); display.print("YOU"); display.setCursor(1,23); display.print("WIN!"); }
    else if(xoWinner==2){ display.setCursor(1,14); display.print("AI"); display.setCursor(1,23); display.print("WIN!"); }
    else { display.setCursor(1,14); display.print("DRAW"); }
    display.setCursor(0,40); display.print("TAP=");
    display.setCursor(0,49); display.print("PLAY");
    display.setCursor(0,57); display.print("HLD=X");
  }
  display.drawLine(SX+CS,   SY, SX+CS,   SY+CS*3, SSD1306_WHITE);
  display.drawLine(SX+CS*2, SY, SX+CS*2, SY+CS*3, SSD1306_WHITE);
  display.drawLine(SX, SY+CS,   SX+CS*3, SY+CS,   SSD1306_WHITE);
  display.drawLine(SX, SY+CS*2, SX+CS*3, SY+CS*2, SSD1306_WHITE);
  display.drawRect(SX, SY, CS*3, CS*3, SSD1306_WHITE);
  for(int i = 0; i < 9; i++) {
    int col=i%3, row=i/3;
    int cx=SX+col*CS+CS/2, cy=SY+row*CS+CS/2;
    if(!xoGameOver && xoPlayerTurn && i==xoCursor)
      display.drawRoundRect(SX+col*CS+2, SY+row*CS+2, CS-4, CS-4, 2, SSD1306_WHITE);
    if(xoBoard[i]==1) {
      int m=4;
      display.drawLine(cx-m,cy-m,cx+m,cy+m,SSD1306_WHITE);
      display.drawLine(cx-m+1,cy-m,cx+m,cy+m-1,SSD1306_WHITE);
      display.drawLine(cx+m,cy-m,cx-m,cy+m,SSD1306_WHITE);
      display.drawLine(cx+m-1,cy-m,cx-m,cy+m-1,SSD1306_WHITE);
    } else if(xoBoard[i]==2) {
      display.drawCircle(cx,cy,5,SSD1306_WHITE);
      display.drawCircle(cx,cy,6,SSD1306_WHITE);
    }
  }
  if(xoGameOver && xoWinner<3) {
    const int lines[8][3] = {
      {0,1,2},{3,4,5},{6,7,8},
      {0,3,6},{1,4,7},{2,5,8},
      {0,4,8},{2,4,6}
    };
    for(int i=0;i<8;i++) {
      int a=lines[i][0], b=lines[i][1], c=lines[i][2];
      if(xoBoard[a] && xoBoard[a]==xoBoard[b] && xoBoard[b]==xoBoard[c]) {
        int x1=SX+(a%3)*CS+CS/2, y1=SY+(a/3)*CS+CS/2;
        int x2=SX+(c%3)*CS+CS/2, y2=SY+(c/3)*CS+CS/2;
        display.drawLine(x1,y1,x2,y2,SSD1306_WHITE);
        display.drawLine(x1+1,y1,x2+1,y2,SSD1306_WHITE);
        break;
      }
    }
  }
  display.display();
}

void drawIconXOLarge(int x, int y, uint16_t color) {
  const int CS=6, SX=x, SY=y;
  display.drawLine(SX+CS,   SY, SX+CS,   SY+CS*3, color);
  display.drawLine(SX+CS*2, SY, SX+CS*2, SY+CS*3, color);
  display.drawLine(SX, SY+CS,   SX+CS*3, SY+CS,   color);
  display.drawLine(SX, SY+CS*2, SX+CS*3, SY+CS*2, color);
  int cx=SX+CS/2, cy=SY+CS/2, m=2;
  display.drawLine(cx-m,cy-m,cx+m,cy+m,color);
  display.drawLine(cx+m,cy-m,cx-m,cy+m,color);
  display.drawCircle(SX+CS+CS/2, SY+CS+CS/2, 2, color);
  cx=SX+CS*2+CS/2; cy=SY+CS*2+CS/2;
  display.drawLine(cx-m,cy-m,cx+m,cy+m,color);
  display.drawLine(cx+m,cy-m,cx-m,cy+m,color);
}

void drawLauncherAppIcon(int appId, int cx, int cy, uint16_t color) {
  switch(appId) {
    case 0:
      drawIconVHSLarge(cx - 16, cy - 10, color);
      break;
    case 1:
      drawIconPongLarge(cx - 15, cy - 10, color);
      break;
    case 2:
      drawIconXOLarge(cx - 9, cy - 9, color);
      break;
    case 3:
      drawIconStopwatchLarge(cx - 12, cy - 8, color);
      break;
    case 4:
      display.drawRoundRect(cx - 14, cy - 7, 5, 14, 2, color);
      display.fillCircle(cx - 12, cy + 9, 4, color);
      display.fillRect(cx - 13, cy - 4, 3, 10, color);
      display.drawCircle(cx + 8, cy, 8, color);
      display.drawCircle(cx + 8, cy, 2, color);
      display.drawLine(cx + 8, cy, cx + 12, cy - 5, color);
      break;
    case 5:
      drawIconScreensaverLarge(cx - 15, cy - 10, color);
      break;
    case 6:
      display.drawCircle(cx, cy, 8, color);
      display.drawCircle(cx, cy, 3, color);
      for(int i = 0; i < 360; i += 45) {
        float rad = i * 3.14159f / 180.0f;
        display.drawLine(
          cx + (int)(cos(rad)*8), cy + (int)(sin(rad)*8),
          cx + (int)(cos(rad)*11), cy + (int)(sin(rad)*11),
          color);
      }
      break;
    case 7:
      display.drawCircle(cx, cy + 2, 7, color);
      display.drawCircle(cx, cy + 2, 6, color);
      display.fillRect(cx - 1, cy - 9, 3, 8, color);
      display.fillRect(cx - 2, cy - 5, 5, 6, SSD1306_BLACK);
      break;

  }
}

const char* launcherAppNames[9] = {
  "MEDIA", "PONG", "X & O", "TIMER", "SENSORS", "SCRNSAVER", "SETTINGS", "OFF", ""
};

int tileAppId(int tile, int slot) { return tile * 2 + slot; }

void drawLauncherTile(int tile, int selApp, int xOffset) {

  const int ICON_Y = 22;
  const int LABEL_Y = 43;
  const int DIVIDER_X = 64;

  for (int slot = 0; slot < 2; slot++) {
    int appId = tileAppId(tile, slot);
    if(appId >= LAUNCHER_APPS) continue;
    int cx = xOffset + 32 + slot * 64;
    bool isSel = (slot == selApp);

    if (isSel) {
      display.fillRoundRect(xOffset + slot * 64 + 2, 2, 60, 53, 4, SSD1306_WHITE);
      drawLauncherAppIcon(appId, cx, ICON_Y, SSD1306_BLACK);
      display.setTextColor(SSD1306_BLACK);
    } else {
      display.drawRoundRect(xOffset + slot * 64 + 2, 2, 60, 53, 4, SSD1306_WHITE);
      drawLauncherAppIcon(appId, cx, ICON_Y, SSD1306_WHITE);
      display.setTextColor(SSD1306_WHITE);
    }

    display.setTextSize(1);

    int labelLen = strlen(launcherAppNames[appId]) * 6;
    display.setCursor(cx - labelLen / 2, LABEL_Y);
    display.print(launcherAppNames[appId]);
  }

  const int DOT_R = 2;
  const int DOT_SPACING = 8;
  int dotsStartX = 64 - (LAUNCHER_TILES - 1) * DOT_SPACING / 2;
  for (int d = 0; d < LAUNCHER_TILES; d++) {
    int dx = dotsStartX + d * DOT_SPACING;
    int dy = 59;
    if (d == tile) {
      display.fillCircle(dx, dy, DOT_R, SSD1306_WHITE);
    } else {
      display.drawCircle(dx, dy, DOT_R, SSD1306_WHITE);
    }
  }
}

void drawContent(int state, int xOffset) {
  if (xOffset <= -SCREEN_WIDTH || xOffset >= SCREEN_WIDTH) return;
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);
  switch(state) {
    case 0:
      drawEyeShape(xOffset + baseLeftX + (int)currentX, baseY + (int)currentY, eyeHeight);
      drawEyeShape(xOffset + baseRightX + (int)currentX, baseY + (int)currentY, eyeHeight);
      drawFaceExtras(xOffset);
      break;
    case 1:
      drawLauncherTile(launcherTile, launcherApp, xOffset);
      break;

    case 10:
      display.drawRoundRect(xOffset + 10, 12, 8, 30, 4, SSD1306_WHITE);
      display.fillCircle(xOffset + 14, 45, 7, SSD1306_WHITE);
      display.fillRect(xOffset + 13, 18, 3, 22, SSD1306_WHITE);
      display.setTextSize(4); display.setCursor(xOffset + 35, 18); display.print((int)currentTemp);
      display.drawCircle(xOffset + 85, 20, 3, SSD1306_WHITE);
      display.setTextSize(2); display.setCursor(xOffset + 95, 25); display.print("C");
      break;
    case 11:
      display.drawCircle(xOffset + 20, 32, 14, SSD1306_WHITE);
      display.drawCircle(xOffset + 20, 32, 2,  SSD1306_WHITE);
      display.drawLine(xOffset + 20, 32, xOffset + 28, 22, SSD1306_WHITE);
      display.setTextSize(3); display.setCursor(xOffset + 45, 15); display.print(currentPressure);
      display.setTextSize(1); display.setCursor(xOffset + 45, 45); display.print("hPa");
      break;
  }
}

void drawSettingsMenu() {
  display.clearDisplay();
  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);

  int gx = 3, gy = 2;
  display.drawPixel(gx+2, gy, SSD1306_BLACK); display.drawPixel(gx+4, gy, SSD1306_BLACK);
  display.drawLine(gx+1, gy+1, gx+6, gy+1, SSD1306_BLACK);
  display.drawLine(gx,   gy+2, gx+7, gy+2, SSD1306_BLACK);
  display.fillRect(gx+1, gy+3, 2, 2, SSD1306_BLACK);
  display.fillRect(gx+5, gy+3, 2, 2, SSD1306_BLACK);
  display.drawLine(gx,   gy+5, gx+7, gy+5, SSD1306_BLACK);
  display.drawLine(gx+1, gy+6, gx+6, gy+6, SSD1306_BLACK);
  display.drawPixel(gx+2, gy+7, SSD1306_BLACK); display.drawPixel(gx+4, gy+7, SSD1306_BLACK);

  display.setTextSize(1); display.setTextColor(SSD1306_BLACK);
  display.setCursor(16, 3); display.print("SETTINGS");
  display.setCursor(78, 3); display.print("TAP/HOLD");
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);

  const int ROW_H = 16, START_Y = 15;
  for (int i = 0; i < 3; i++) {
    int y = START_Y + i * ROW_H;
    bool sel = (i == selectedSetting);
    if (sel) display.fillRoundRect(0, y, 128, ROW_H-1, 2, SSD1306_WHITE);
    uint16_t col = sel ? SSD1306_BLACK : SSD1306_WHITE;

    if (i == 0)      drawIconBuzzer(3, y+3, col);
    else if (i == 1) drawIconAuto(3, y+3, col);
    else             drawIconExit(3, y+3, col);

    display.setTextColor(col); display.setTextSize(1); display.setCursor(18, y+4);
    if (i == 0)      display.print("BUZZER");
    else if (i == 1) display.print("AUTO SWITCH");
    else             display.print("EXIT");

    if (i < 2) {
      bool st = (i == 0) ? buzzerEnabled : autoSwitch;
      int mx = 114, my = y+3;
      if (st) {
        display.drawLine(mx,   my+5, mx+3, my+8, col);
        display.drawLine(mx+3, my+8, mx+8, my+2, col);
        display.drawLine(mx,   my+6, mx+3, my+9, col);
        display.drawLine(mx+3, my+9, mx+8, my+3, col);
      } else {
        display.drawLine(mx,   my+1, mx+8, my+9, col);
        display.drawLine(mx+1, my+1, mx+9, my+9, col);
        display.drawLine(mx+8, my+1, mx,   my+9, col);
        display.drawLine(mx+9, my+1, mx+1, my+9, col);
      }
    }
    if (sel && i == 2) {
      int ax = 116, ay = y+5;
      display.fillTriangle(ax, ay, ax, ay+5, ax+4, ay+2, SSD1306_BLACK);
    }
  }
  display.display();
}

void drawMediaMenu() {
  display.clearDisplay();

  display.fillRect(0, 0, 128, 13, SSD1306_WHITE);

  drawIconVHS(3, 2, SSD1306_BLACK);

  display.setTextSize(1); display.setTextColor(SSD1306_BLACK);
  display.setCursor(22, 3); display.print("MEDIA");
  display.setCursor(72, 3); display.print("TAP/HOLD");
  display.drawLine(0, 13, 128, 13, SSD1306_WHITE);

  const int btnW = 42, btnH = 25;
  const int startX = 1, startY = 14;

  for (int i = 0; i < 6; i++) {
    int col = i % 3;
    int row = i / 3;
    int x = startX + col * btnW;
    int y = startY + row * btnH;
    bool sel = (i == selectedMedia);

    if (sel) {
      display.fillRoundRect(x, y, btnW-1, btnH-1, 3, SSD1306_WHITE);
    } else {
      display.drawRoundRect(x, y, btnW-1, btnH-1, 3, SSD1306_WHITE);
    }

    uint16_t c = sel ? SSD1306_BLACK : SSD1306_WHITE;

    int ix = x + 14, iy = y + 7;

    if      (i == 0) drawIconPrev(ix, iy, c);
    else if (i == 1) drawIconPauseStop(ix, iy, c);
    else if (i == 2) drawIconNext(ix, iy, c);
    else if (i == 3) drawIconVolDown(ix, iy, c);
    else if (i == 4) drawIconVolUp(ix, iy, c);
    else             drawIconExitX(ix+1, iy+1, c);
  }

  display.display();
}

void performScrollTransition(int oldS, int newS) {
  for (int i = 0; i <= SCREEN_WIDTH; i += 16) {
    display.clearDisplay();
    drawContent(oldS, -i);
    drawContent(newS, SCREEN_WIDTH - i);
    if (i % 32 == 0) playTone(800 + i, 10);
    display.display();
  }
  currentTemp = bmp.readTemperature();
  currentPressure = bmp.readPressure() / 100;
  display.clearDisplay(); drawContent(newS, 0); display.display();
  lastAutoSwitch = millis();
}

void performLauncherTileScroll(int oldTile, int newTile) {

  int savedTile = launcherTile;
  int savedApp  = launcherApp;

  bool goRight = (newTile > oldTile);
  for (int i = 0; i <= SCREEN_WIDTH; i += 16) {
    display.clearDisplay();

    launcherTile = oldTile; launcherApp = savedApp;
    drawContent(1, goRight ? -i : i);

    launcherTile = newTile; launcherApp = 0;
    drawContent(1, goRight ? SCREEN_WIDTH - i : -(SCREEN_WIDTH - i));
    if (i % 32 == 0) playTone(800 + i, 10);
    display.display();
  }
  launcherTile = newTile;
  launcherApp  = 0;
  display.clearDisplay(); drawContent(1, 0); display.display();
}

void setup() {

  usb_hid.setPollInterval(2);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.begin();
  while (!TinyUSBDevice.mounted()) delay(1);

  Wire.setSDA(28); Wire.setSCL(29); Wire.begin();
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  bmp.begin();
  randomSeed(analogRead(25));
  currentTemp = bmp.readTemperature();
  currentPressure = bmp.readPressure() / 100;
  display.clearDisplay(); drawContent(0, 0); display.display();
  lastAutoSwitch = millis();
  lastInteraction = millis();
}

void loop() {
  unsigned long now = millis();
  bool currentTouch = digitalRead(TOUCH_PIN);

  if (currentTouch == HIGH) {
    autoSuspendTimeout = now;
    lastInteraction = now;
  }

  if (currentTouch == HIGH && lastTouchState == LOW) {
    touchStartTime = now;
    longPressTriggered = false;
  }

  if (currentTouch == HIGH && !longPressTriggered) {
    if (now - touchStartTime > 700) {
      longPressTriggered = true;

      if (inMediaMenu) {
        switch(selectedMedia) {
          case 0: sendMediaKey(HID_USAGE_CONSUMER_SCAN_PREVIOUS_TRACK); playTone(1400, 40); break;
          case 1: sendMediaKey(HID_USAGE_CONSUMER_PLAY_PAUSE);          playTone(1800, 40); break;
          case 2: sendMediaKey(HID_USAGE_CONSUMER_SCAN_NEXT_TRACK);     playTone(2000, 40); break;
          case 3: volHolding = true; volRepeatLast = now;
                  sendMediaKey(HID_USAGE_CONSUMER_VOLUME_DECREMENT); playTone(1000, 30); break;
          case 4: volHolding = true; volRepeatLast = now;
                  sendMediaKey(HID_USAGE_CONSUMER_VOLUME_INCREMENT); playTone(2200, 30); break;
          case 5: inMediaMenu = false; menuState = 1; playTone(900, 60); break;
        }
      } else if (inSettingsMenu) {
        if (selectedSetting == 0)      buzzerEnabled = !buzzerEnabled;
        else if (selectedSetting == 1) autoSwitch = !autoSwitch;
        else if (selectedSetting == 2) { inSettingsMenu = false; menuState = 1; }
        playTone(1500, 50);
      } else if (inFaceManager) {

        eyeExpression = selectedFace;
        inFaceManager = false;
        playTone(1500, 50);
        display.clearDisplay(); drawContent(0, 0); display.display();
      } else if (menuState == 0) {

        inFaceManager = true;
        selectedFace = eyeExpression;
        playTone(1200, 50);
        drawFaceManager();
      } else if (inXO) {
        if(xoGameOver) {
          inXO = false; menuState = 1; playTone(900, 60);
        } else if(xoPlayerTurn && xoBoard[xoCursor] == 0) {

          xoBoard[xoCursor] = 1;
          playTone(1600, 40);
          xoWinner = xoCheckWinner();
          if(xoWinner) { xoGameOver = true; }
          else {
            xoPlayerTurn = false;
            xoAiThinking = true;
            xoAiDelay = millis() + 500;
          }
        }
      } else if (inSensors) {
        inSensors = false; menuState = 1; playTone(900, 60);
      } else if (inStopwatch) {
        if (swRunning) {
          swRunning = false; swElapsed = 0; swStartTime = 0;
          playTone(800, 80);
        } else {
          inStopwatch = false; menuState = 1;
          playTone(900, 60);
        }
      } else if (inPong && pongLost) {
        inPong = false; menuState = 1;
        playTone(900, 60);
      } else if (inScreensaver) {
        if (ssSelecting) {
          screensaverMode = screensaverSelected;
          ssSelecting = false;
          initScreensaverMode(screensaverMode);
          playTone(1500, 50);
        } else {
          ssSelecting = true; screensaverMode = -1;
          display.clearDisplay();
          playTone(900, 60);
        }
      } else if (menuState == 1) {

        int appId = tileAppId(launcherTile, launcherApp);
        if(appId < LAUNCHER_APPS) {
        playTone(1500, 50);
        if (appId == 0) {
          inMediaMenu = true; selectedMedia = 0;
        } else if (appId == 1) {
          inPong = true; pongLost = false; pongScore = 0;
          ballX = 64; ballY = 32; ballDX = 2.2; ballDY = 1.5;
          playerY = 32; aiY = 32; paddleGoingDown = true;
        } else if (appId == 2) {
          inXO = true; xoReset();
        } else if (appId == 3) {
          inStopwatch = true; swRunning = false; swElapsed = 0;
        } else if (appId == 4) {
          inSensors = true;
        } else if (appId == 5) {
          inScreensaver = true; ssSelecting = true; screensaverSelected = 0; screensaverMode = -1;
        } else if (appId == 6) {
          inSettingsMenu = true; selectedSetting = 0;
        } else if (appId == 7) {
          inScreenOff = true;
          display.clearDisplay();
          display.display();
          display.ssd1306_command(SSD1306_DISPLAYOFF);
        }
        }
      }
    }
  }

  if (currentTouch == LOW && lastTouchState == HIGH) {
    volHolding = false;
  }

  if (currentTouch == LOW && lastTouchState == HIGH && !longPressTriggered) {
    if (inScreenOff) {

      inScreenOff = false;
      display.ssd1306_command(SSD1306_DISPLAYON);
      menuState = 1;
      launcherTile = 0; launcherApp = 0;
      playTone(1200, 30);
      display.clearDisplay(); drawContent(1, 0); display.display();
    } else if (inSensors) {
    if (now - lastUpdate > 2000) {
      currentTemp = bmp.readTemperature();
      currentPressure = bmp.readPressure() / 100;
      lastUpdate = now;
    }
    drawSensors();
  } else if (inScreensaver) {
      if (ssSelecting) {

        screensaverSelected = (screensaverSelected + 1) % SS_COUNT;
        playTone(1200, 20);
        drawScreensaverSelection();
      } else {

        inScreensaver = false; menuState = 1;
        playTone(900, 60);
      }
    } else if (inXO) {
      if(xoGameOver) {

        xoReset(); playTone(1200, 30);
      } else if(xoPlayerTurn) {

        int next = xoCursor;
        for(int k=1; k<=9; k++) {
          next = (xoCursor + k) % 9;
          if(xoBoard[next] == 0) break;
        }
        xoCursor = next;
        playTone(1200, 20);
      }
    } else if (inPong) {
      if (pongLost) {

        pongLost = false; pongScore = 0;
        ballX = 64; ballY = 32; ballDX = 2.2; ballDY = 1.5;
        playerY = 32; aiY = 32; paddleGoingDown = true;
        playTone(1200, 30);
      }
    } else if (inStopwatch) {
      if (swRunning) {
        swElapsed += millis() - swStartTime;
        swRunning = false;
      } else {
        swStartTime = millis();
        swRunning = true;
      }
      playTone(1200, 20);
    } else if (inFaceManager) {
      selectedFace = (selectedFace + 1) % FACE_COUNT;
      playTone(1200, 20);
      drawFaceManager();
    } else if (inMediaMenu) {
      selectedMedia = (selectedMedia + 1) % 6;
      playTone(1200, 20);
    } else if (inSettingsMenu) {
      selectedSetting = (selectedSetting + 1) % 3;
      playTone(1200, 20);
    } else if (menuState == 0) {

      int oldState = menuState;
      menuState = 1;
      launcherTile = 0; launcherApp = 0;
      playTone(1200, 30);
      performScrollTransition(oldState, menuState);
    } else if (menuState == 1) {

      playTone(1200, 20);
      if (launcherApp == 0) {

        launcherApp = 1;
        display.clearDisplay(); drawContent(1, 0); display.display();
      } else if (launcherTile < LAUNCHER_TILES - 1) {

        int oldTile = launcherTile;
        performLauncherTileScroll(oldTile, oldTile + 1);
      } else {

        menuState = 0;
        launcherTile = 0; launcherApp = 0;
        performScrollTransition(1, 0);
      }
    } else {
      int oldState = menuState;
      if (menuState >= 10) menuState = 0;
      menuState = (menuState + 1) % 7;
      playTone(1200, 30);
      performScrollTransition(oldState, menuState);
    }
  }
  lastTouchState = currentTouch;

  if (volHolding && currentTouch == HIGH && longPressTriggered) {
    if (now - volRepeatLast > 200) {
      volRepeatLast = now;
      if (selectedMedia == 3) { sendMediaKey(HID_USAGE_CONSUMER_VOLUME_DECREMENT); playTone(900, 15); }
      if (selectedMedia == 4) { sendMediaKey(HID_USAGE_CONSUMER_VOLUME_INCREMENT); playTone(2100, 15); }
    }
  }

  if (inScreenOff) {

    delay(50);
  } else if (inSensors) {
    if (now - lastUpdate > 2000) {
      currentTemp = bmp.readTemperature();
      currentPressure = bmp.readPressure() / 100;
      lastUpdate = now;
    }
    drawSensors();
  } else if (inScreensaver) {
    if (ssSelecting) {
      drawScreensaverSelection();
      delay(80);
    } else {
      updateDrawScreensaver(screensaverMode);
      delay(30);
    }
  } else if (inXO) {

    if(xoAiThinking && !xoGameOver && millis() >= xoAiDelay) {
      xoAiThinking = false;
      int mv = xoAiMove();
      if(mv >= 0) {
        xoBoard[mv] = 2;
        playTone(900, 40);
        xoWinner = xoCheckWinner();
        if(xoWinner) xoGameOver = true;
        else { xoPlayerTurn = true; xoCursor = 0; while(xoCursor<9 && xoBoard[xoCursor]) xoCursor++; }
      }
    }
    drawXO();
    delay(40);
  } else if (inPong) {
    bool held = (currentTouch == HIGH);
    updatePong(held);
    drawPong();
  } else if (inFaceManager) {
    drawFaceManager();
  } else if (inStopwatch) {
    drawStopwatch();
  } else if (inSettingsMenu) {
    drawSettingsMenu();
  } else if (inMediaMenu) {
    drawMediaMenu();
  } else {
    if (menuState == 0) {
      display.clearDisplay();
      drawContent(0, 0);
      display.display();
      if (abs(currentX - targetX) > 0.5 || abs(currentY - targetY) > 0.5) {
        currentX += (targetX - currentX) * 0.1;
        currentY += (targetY - currentY) * 0.1;
      } else if (now - lastUpdate > 3000) {
        targetX = random(-10, 11); targetY = random(-6, 7);
        lastUpdate = now;
      }
      if (eyeExpression == 0 && now - lastBlink > (8000 + random(0, 4000))) {
        blinkSmooth(); lastBlink = now;
      }
    } else if (menuState == 1) {

      display.clearDisplay(); drawContent(1, 0); display.display();
      delay(50);
    } else if (menuState >= 10 && now - lastUpdate > 2000) {
      currentTemp = bmp.readTemperature();
      currentPressure = bmp.readPressure() / 100;
      display.clearDisplay(); drawContent(menuState, 0); display.display();
      lastUpdate = now;
    }
  }
}
