/*
 * =============================================
 *  BRICK BREAKER - ILI9341 / ESP32
 *  25 Unique Levels Edition
 * =============================================
 *  Pin Configuration:
 *    TFT_CS   = 5
 *    TFT_RST  = 22
 *    TFT_DC   = 21
 *    BTN_LEFT  = 27
 *    BTN_RIGHT = 12 (original: 26 — change to match your wiring)
 *    BTN_SELECT = 4 (original: 25 — change to match your wiring)
 *
 *  In your User_Setup.h (TFT_eSPI):
 *    #define ILI9341_DRIVER
 *    #define TFT_CS   5
 *    #define TFT_DC   21
 *    #define TFT_RST  22
 *    #define TFT_MOSI 23
 *    #define TFT_SCLK 18
 *    #define TFT_MISO 19
 * =============================================
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

#define BTN_UP     13
#define BTN_DOWN   12
#define BTN_LEFT   27
#define BTN_RIGHT  26
#define BTN_A      14
#define BTN_B      33
#define BTN_START  32
#define BTN_SELECT 25

// --- Display ---
TFT_eSPI tft = TFT_eSPI();
#define SCREEN_W  320
#define SCREEN_H  240

// --- Colors ---
#define COL_BG        TFT_BLACK
#define COL_PADDLE    TFT_CYAN
#define COL_BALL      TFT_WHITE
#define COL_TEXT      TFT_WHITE
#define COL_SCORE     TFT_YELLOW
#define COL_LIFE      TFT_RED
#define COL_BORDER    0x2104

const uint16_t BRICK_COLORS[] = {
  TFT_RED,
  0xFD20,   // orange
  TFT_YELLOW,
  TFT_GREEN,
  TFT_CYAN,
  0x781F,   // purple
};

// --- EEPROM ---
#define EEPROM_SIZE   4
#define HISCORE_ADDR  0

// --- Game Constants ---
#define PADDLE_W      70
#define PADDLE_H      8
#define PADDLE_Y      (SCREEN_H - 10)
#define PADDLE_SPEED  6

#define BALL_SIZE     5
#define BALL_SPEED_INIT 2.5f

#define BRICK_COLS    10
#define BRICK_ROWS    6
#define BRICK_W       28
#define BRICK_H       12
#define BRICK_PAD_X   2
#define BRICK_PAD_Y   2
#define BRICK_OFFSET_X ((SCREEN_W - (BRICK_COLS * (BRICK_W + BRICK_PAD_X) - BRICK_PAD_X)) / 2)
#define BRICK_OFFSET_Y 30

#define MAX_LIVES     3
#define MAX_LEVELS    25   // <-- expanded to 25

// --- Brick ---
struct Brick {
  bool alive;
  uint8_t hits;
};

// --- Game State ---
enum GameState {
  STATE_TITLE,
  STATE_PLAYING,
  STATE_BALL_LOST,
  STATE_LEVEL_CLEAR,
  STATE_GAME_OVER,
  STATE_HISCORE
};

Brick bricks[BRICK_ROWS][BRICK_COLS];

float   ballX, ballY;
float   ballVX, ballVY;
float   ballSpeed;
int     paddleX;
int     score;
int     hiScore;
int     lives;
int     level;
bool    ballLaunched;
bool    paused;
GameState gameState;

unsigned long lastFrame;
unsigned long msgTimer;

bool btnLeftPrev   = HIGH;
bool btnRightPrev  = HIGH;
bool btnSelectPrev = HIGH;

// =============================================
//  EEPROM helpers
// =============================================
void loadHiScore() {
  EEPROM.begin(EEPROM_SIZE);
  hiScore  = EEPROM.read(HISCORE_ADDR) << 8;
  hiScore |= EEPROM.read(HISCORE_ADDR + 1);
}

void saveHiScore() {
  EEPROM.write(HISCORE_ADDR,     (hiScore >> 8) & 0xFF);
  EEPROM.write(HISCORE_ADDR + 1, hiScore & 0xFF);
  EEPROM.commit();
}

// =============================================
//  25 UNIQUE LEVEL MAPS
//  0 = empty | 1 = 1-hit brick | 2 = 2-hit brick
// =============================================
const uint8_t LEVEL_MAPS[MAX_LEVELS][BRICK_ROWS][BRICK_COLS] = {

  // ── Level 1: Classic Rows (easy warm-up) ──
  {
    {1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 2: Checkerboard ──
  {
    {1,0,1,0,1,0,1,0,1,0},
    {0,1,0,1,0,1,0,1,0,1},
    {1,0,1,0,1,0,1,0,1,0},
    {0,1,0,1,0,1,0,1,0,1},
    {1,0,1,0,1,0,1,0,1,0},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 3: Diamond ──
  {
    {0,0,0,0,1,1,0,0,0,0},
    {0,0,0,1,1,1,1,0,0,0},
    {0,0,1,1,2,2,1,1,0,0},
    {0,0,1,1,2,2,1,1,0,0},
    {0,0,0,1,1,1,1,0,0,0},
    {0,0,0,0,1,1,0,0,0,0},
  },

  // ── Level 4: Fortress ──
  {
    {2,2,2,2,2,2,2,2,2,2},
    {2,0,0,0,0,0,0,0,0,2},
    {2,0,1,1,1,1,1,1,0,2},
    {2,0,1,0,0,0,0,1,0,2},
    {2,0,1,1,1,1,1,1,0,2},
    {2,0,0,0,0,0,0,0,0,2},
  },

  // ── Level 5: Full Hard ──
  {
    {2,2,2,2,2,2,2,2,2,2},
    {2,1,2,1,2,1,2,1,2,2},
    {2,2,1,2,1,2,1,2,1,2},
    {1,2,2,2,2,2,2,2,2,1},
    {2,1,2,1,2,1,2,1,2,2},
    {2,2,2,2,2,2,2,2,2,2},
  },

  // ── Level 6: V-Shape ──
  {
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,0,0,0,0,0,0,1,1},
    {1,1,1,0,0,0,0,1,1,1},
    {1,1,1,1,0,0,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 7: X Cross ──
  {
    {1,0,0,0,1,1,0,0,0,1},
    {0,1,0,1,0,0,1,0,1,0},
    {0,0,2,0,0,0,0,2,0,0},
    {0,1,0,1,0,0,1,0,1,0},
    {1,0,0,0,1,1,0,0,0,1},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 8: Pyramid ──
  {
    {0,0,0,0,2,2,0,0,0,0},
    {0,0,0,1,1,1,1,0,0,0},
    {0,0,1,1,1,1,1,1,0,0},
    {0,1,1,1,1,1,1,1,1,0},
    {1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 9: Stripes ──
  {
    {1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0},
    {2,2,2,2,2,2,2,2,2,2},
    {0,0,0,0,0,0,0,0,0,0},
    {1,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 10: Cross / Plus ──
  {
    {0,0,0,0,2,2,0,0,0,0},
    {0,0,0,0,2,2,0,0,0,0},
    {1,1,1,1,2,2,1,1,1,1},
    {1,1,1,1,2,2,1,1,1,1},
    {0,0,0,0,2,2,0,0,0,0},
    {0,0,0,0,2,2,0,0,0,0},
  },

  // ── Level 11: Corners ──
  {
    {2,2,2,0,0,0,0,2,2,2},
    {2,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,2},
    {2,0,0,0,0,0,0,0,0,2},
    {2,2,2,0,0,0,0,2,2,2},
  },

  // ── Level 12: Arrow Up ──
  {
    {0,0,0,0,1,1,0,0,0,0},
    {0,0,0,1,1,1,1,0,0,0},
    {0,0,1,0,1,1,0,1,0,0},
    {0,1,0,0,1,1,0,0,1,0},
    {0,0,0,0,1,1,0,0,0,0},
    {0,0,0,0,1,1,0,0,0,0},
  },

  // ── Level 13: Hourglass ──
  {
    {2,2,2,2,2,2,2,2,2,2},
    {0,2,2,2,2,2,2,2,2,0},
    {0,0,2,2,2,2,2,2,0,0},
    {0,0,2,2,2,2,2,2,0,0},
    {0,2,2,2,2,2,2,2,2,0},
    {2,2,2,2,2,2,2,2,2,2},
  },

  // ── Level 14: Zigzag ──
  {
    {1,1,0,0,0,0,0,0,1,1},
    {0,1,1,0,0,0,0,1,1,0},
    {0,0,1,1,0,0,1,1,0,0},
    {0,0,0,1,1,1,1,0,0,0},
    {0,0,1,1,0,0,1,1,0,0},
    {0,1,1,0,0,0,0,1,1,0},
  },

  // ── Level 15: Invaders (Space) ──
  {
    {0,1,0,0,1,1,0,0,1,0},
    {0,0,1,1,1,1,1,1,0,0},
    {0,1,1,2,1,1,2,1,1,0},
    {0,1,1,1,1,1,1,1,1,0},
    {0,0,0,1,0,0,1,0,0,0},
    {0,0,1,0,0,0,0,1,0,0},
  },

  // ── Level 16: Diagonal Slash ──
  {
    {2,0,0,0,0,0,0,0,0,1},
    {1,2,0,0,0,0,0,0,1,0},
    {0,1,2,0,0,0,0,1,0,0},
    {0,0,1,2,0,0,1,0,0,0},
    {0,0,0,1,2,1,0,0,0,0},
    {0,0,0,0,1,0,0,0,0,0},
  },

  // ── Level 17: Bullseye ──
  {
    {0,1,1,1,1,1,1,1,1,0},
    {1,0,0,0,0,0,0,0,0,1},
    {1,0,2,2,2,2,2,2,0,1},
    {1,0,2,0,0,0,0,2,0,1},
    {1,0,2,2,2,2,2,2,0,1},
    {0,1,1,1,1,1,1,1,1,0},
  },

  // ── Level 18: Snake ──
  {
    {1,1,1,1,1,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,1,0},
    {0,1,1,1,1,1,1,1,1,0},
    {0,1,0,0,0,0,0,0,0,0},
    {0,1,1,1,1,1,1,1,1,1},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 19: Columns ──
  {
    {1,0,1,0,1,0,1,0,1,0},
    {1,0,1,0,1,0,1,0,1,0},
    {2,0,2,0,2,0,2,0,2,0},
    {2,0,2,0,2,0,2,0,2,0},
    {1,0,1,0,1,0,1,0,1,0},
    {1,0,1,0,1,0,1,0,1,0},
  },

  // ── Level 20: Dominos ──
  {
    {2,2,0,0,2,2,0,0,2,2},
    {2,2,0,0,2,2,0,0,2,2},
    {0,0,2,2,0,0,2,2,0,0},
    {0,0,2,2,0,0,2,2,0,0},
    {2,2,0,0,2,2,0,0,2,2},
    {2,2,0,0,2,2,0,0,2,2},
  },

  // ── Level 21: Walled City ──
  {
    {2,0,2,0,2,2,0,2,0,2},
    {2,0,2,0,2,2,0,2,0,2},
    {2,2,2,2,2,2,2,2,2,2},
    {1,0,0,1,0,0,1,0,0,1},
    {1,0,0,1,0,0,1,0,0,1},
    {0,0,0,0,0,0,0,0,0,0},
  },

  // ── Level 22: Star ──
  {
    {0,0,0,0,2,2,0,0,0,0},
    {1,0,0,0,2,2,0,0,0,1},
    {0,1,1,1,2,2,1,1,1,0},
    {0,1,1,1,2,2,1,1,1,0},
    {1,0,0,0,2,2,0,0,0,1},
    {0,0,0,0,2,2,0,0,0,0},
  },

  // ── Level 23: Labyrinth ──
  {
    {2,2,2,2,0,0,2,2,2,2},
    {2,0,0,2,0,0,2,0,0,2},
    {2,0,2,2,2,2,2,2,0,2},
    {2,0,2,0,0,0,0,2,0,2},
    {2,0,0,0,2,2,0,0,0,2},
    {2,2,2,2,2,2,2,2,2,2},
  },

  // ── Level 24: Chaos ──
  {
    {2,1,2,0,1,2,0,2,1,2},
    {0,2,0,1,2,0,2,1,2,0},
    {1,0,2,2,0,1,2,0,2,1},
    {2,1,0,2,1,2,0,1,0,2},
    {0,2,1,0,2,1,2,0,2,1},
    {1,0,2,1,0,2,1,2,0,1},
  },

  // ── Level 25: Final Boss (all 2-hit, dense) ──
  {
    {2,2,2,2,2,2,2,2,2,2},
    {2,1,2,1,2,1,2,1,2,2},
    {2,2,1,2,1,2,1,2,2,2},
    {2,1,2,2,2,2,2,2,1,2},
    {2,2,1,2,1,2,1,2,2,2},
    {2,2,2,2,2,2,2,2,2,2},
  },

}; // end LEVEL_MAPS

// =============================================
//  Initialize level
// =============================================
void initLevel() {
  int lv = (level - 1) % MAX_LEVELS;
  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      bricks[r][c].hits  = LEVEL_MAPS[lv][r][c];
      bricks[r][c].alive = (bricks[r][c].hits > 0);
    }
  }
  paddleX    = (SCREEN_W - PADDLE_W) / 2;
  ballX      = paddleX + PADDLE_W / 2;
  ballY      = PADDLE_Y - BALL_SIZE - 2;
  ballSpeed  = BALL_SPEED_INIT + (level - 1) * 0.2f;  // gentler ramp over 25 levels
  if (ballSpeed > 7.0f) ballSpeed = 7.0f;              // cap so it stays playable
  ballVX     = ballSpeed;
  ballVY     = -ballSpeed;
  ballLaunched = false;
  paused       = false;
}

void initGame() {
  score  = 0;
  lives  = MAX_LIVES;
  level  = 1;
  initLevel();
  gameState = STATE_PLAYING;
  tft.fillScreen(COL_BG);
  drawHUD();
  drawBricks();
  drawPaddle(paddleX, true);
  drawBall((int)ballX, (int)ballY, true);
}

// =============================================
//  Drawing helpers
// =============================================
void drawPauseMessage() {
  tft.setTextColor(TFT_RED);
  tft.setTextSize(3);
  tft.setCursor(115, 105);
  tft.print("PAUSED");
}

void erasePauseMessage() {
  tft.fillRoundRect(80, 90, 160, 50, 10, COL_BG);
  drawHUD();
}

void drawBrick(int c, int r) {
  if (!bricks[r][c].alive) return;
  int x = BRICK_OFFSET_X + c * (BRICK_W + BRICK_PAD_X);
  int y = BRICK_OFFSET_Y + r * (BRICK_H + BRICK_PAD_Y);
  uint16_t col = BRICK_COLORS[r % 6];
  if (bricks[r][c].hits == 1 && LEVEL_MAPS[(level-1)%MAX_LEVELS][r][c] == 2) {
    col = tft.color565(
      ((col >> 11) & 0x1F) * 12,
      ((col >>  5) & 0x3F) * 6,
      (col & 0x1F) * 12
    );
  }
  tft.fillRect(x, y, BRICK_W, BRICK_H, col);
  tft.drawFastHLine(x, y, BRICK_W, TFT_WHITE);
  tft.drawFastVLine(x, y, BRICK_H, TFT_WHITE);
  tft.drawFastHLine(x, y + BRICK_H - 1, BRICK_W, COL_BG);
  tft.drawFastVLine(x + BRICK_W - 1, y, BRICK_H, COL_BG);
}

void eraseBrick(int c, int r) {
  int x = BRICK_OFFSET_X + c * (BRICK_W + BRICK_PAD_X);
  int y = BRICK_OFFSET_Y + r * (BRICK_H + BRICK_PAD_Y);
  tft.fillRect(x, y, BRICK_W, BRICK_H, COL_BG);
}

void drawBricks() {
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++)
      if (bricks[r][c].alive)
        drawBrick(c, r);
}

void drawPaddle(int x, bool show) {
  tft.fillRoundRect(x, PADDLE_Y, PADDLE_W, PADDLE_H,
                    PADDLE_H / 2, show ? COL_PADDLE : COL_BG);
  if (show)
    tft.drawFastHLine(x + 2, PADDLE_Y + 1, PADDLE_W - 4, TFT_WHITE);
}

void drawBall(int x, int y, bool show) {
  tft.fillCircle(x, y, BALL_SIZE, show ? COL_BALL : COL_BG);
}

void drawHUD() {
  tft.fillRect(0, 0, SCREEN_W, 22, 0x2104);
  tft.setTextColor(COL_SCORE);
  tft.setTextSize(1);
  tft.setCursor(4, 7);
  tft.print("SCR:");
  tft.print(score);

  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(SCREEN_W / 2 - 20, 7);
  tft.print("LV:");
  tft.print(level);
  tft.print("/25");

  tft.setCursor(SCREEN_W - 75, 7);
  tft.setTextColor(COL_LIFE);
  tft.print("LIVES:");
  for (int i = 0; i < MAX_LIVES; i++) {
    if (i < lives)
      tft.fillCircle(SCREEN_W - 12 + (i - MAX_LIVES + 1) * 10, 11, 3, COL_LIFE);
    else
      tft.drawCircle(SCREEN_W - 12 + (i - MAX_LIVES + 1) * 10, 11, 3, COL_LIFE);
  }
}

void drawTitle() {
  tft.fillScreen(COL_BG);
  for (int y = 0; y < SCREEN_H; y += 4)
    tft.drawFastHLine(0, y, SCREEN_W, 0x0821);

  tft.fillRoundRect(30, 30, 260, 60, 10, 0x1082);
  tft.drawRoundRect(30, 30, 260, 60, 10, TFT_CYAN);

  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(3);
  tft.setCursor(55, 42);
  tft.print("BRICK");
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(155, 42);
  tft.print("OUT");

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(72, 72);
  tft.print("25 LEVELS  |  ESP32 + ILI9341");

  tft.setTextColor(TFT_GREEN);
  tft.setCursor(70, 108);
  tft.print("LEFT/RIGHT - Move Paddle");
  tft.setCursor(70, 122);
  tft.print("SELECT     - Launch / Pause");

  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(95, 150);
  tft.print("HI-SCORE: ");
  tft.print(hiScore);

  tft.setTextColor(TFT_CYAN);
  tft.setCursor(80, 185);
  tft.print("Press SELECT to Start!");
}

void drawGameOver() {
  for (int i = 0; i < 80; i++)
    tft.drawFastHLine(40, 60 + i, 240, tft.color565(20 + i/4, 0, 0));

  tft.fillRoundRect(38, 58, 244, 124, 15, TFT_BLACK);
  tft.fillRoundRect(40, 60, 240, 120, 15, tft.color565(15, 0, 0));
  tft.drawRoundRect(40, 60, 240, 120, 15, TFT_RED);

  tft.setTextColor(TFT_RED);   tft.setTextSize(3);
  tft.setCursor(68, 72);  tft.print("GAME");
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(142, 72); tft.print("OVER");

  tft.drawFastHLine(70, 96, 180, TFT_RED);

  tft.fillRoundRect(60, 102, 200, 25, 5, TFT_BLACK);
  tft.drawRoundRect(60, 102, 200, 25, 5, TFT_CYAN);
  tft.setTextColor(TFT_YELLOW); tft.setTextSize(2);
  tft.setCursor(70, 108); tft.print("POINTS:");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(170, 107); tft.print(score);

  if (score >= hiScore) {
    for (int i = 0; i < 3; i++) {
      tft.setTextColor(TFT_YELLOW);
      tft.setCursor(52, 133); tft.print("NEW HI-SCORE!");
      delay(50);
      tft.fillRect(52, 133, 190, 12, tft.color565(15, 0, 0));
      delay(50);
    }
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(52, 133); tft.print("NEW HI-SCORE!");
  } else {
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(65, 133); tft.print("HI-SCORE: ");
    tft.setTextColor(TFT_YELLOW); tft.print(hiScore);
  }

  tft.fillRoundRect(70, 150, 180, 20, 8, tft.color565(0, 20, 0));
  tft.drawRoundRect(70, 150, 180, 20, 8, TFT_GREEN);
  tft.setTextColor(TFT_GREEN); tft.setTextSize(1);
  tft.setCursor(85, 155); tft.print("PRESS SELECT TO RESTART");
}

void drawLevelClear() {
  tft.fillRect(50, 85, 220, 70, 0x1082);
  tft.drawRect(50, 85, 220, 70, TFT_GREEN);
  tft.setTextColor(TFT_GREEN); tft.setTextSize(2);
  tft.setCursor(62, 95);  tft.print("LEVEL CLEAR!");
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.setCursor(78, 120);
  if (level < MAX_LEVELS) {
    tft.print("Level "); tft.print(level); tft.print(" -> "); tft.print(level+1);
  } else {
    tft.setTextColor(TFT_YELLOW);
    tft.print("You Beat All 25 Levels!");
  }
  tft.setTextColor(0xAD75); tft.setCursor(100, 138); tft.print("Get ready...");
}

// =============================================
//  Collision: ball vs bricks
// =============================================
bool checkBrickCollision() {
  int bx = (int)ballX - BALL_SIZE;
  int by = (int)ballY - BALL_SIZE;
  int bw = BALL_SIZE * 2;
  int bh = BALL_SIZE * 2;

  for (int r = 0; r < BRICK_ROWS; r++) {
    for (int c = 0; c < BRICK_COLS; c++) {
      if (!bricks[r][c].alive) continue;

      int rx = BRICK_OFFSET_X + c * (BRICK_W + BRICK_PAD_X);
      int ry = BRICK_OFFSET_Y + r * (BRICK_H + BRICK_PAD_Y);

      if (bx < rx + BRICK_W && bx + bw > rx &&
          by < ry + BRICK_H && by + bh > ry) {

        float overlapLeft  = (bx + bw) - rx;
        float overlapRight = (rx + BRICK_W) - bx;
        float overlapTop   = (by + bh) - ry;
        float overlapBot   = (ry + BRICK_H) - by;
        float minH = min(overlapLeft, overlapRight);
        float minV = min(overlapTop, overlapBot);
        if (minH < minV) ballVX = -ballVX;
        else              ballVY = -ballVY;

        bricks[r][c].hits--;
        if (bricks[r][c].hits <= 0) {
          bricks[r][c].alive = false;
          eraseBrick(c, r);
          score += (r + 1) * 10 * level;
        } else {
          drawBrick(c, r);
        }
        drawHUD();
        return true;
      }
    }
  }
  return false;
}

bool allBricksCleared() {
  for (int r = 0; r < BRICK_ROWS; r++)
    for (int c = 0; c < BRICK_COLS; c++)
      if (bricks[r][c].alive) return false;
  return true;
}

// =============================================
//  Game update
// =============================================
void updateGame() {
  bool leftNow  = digitalRead(BTN_LEFT);
  bool rightNow = digitalRead(BTN_RIGHT);
  bool selNow   = digitalRead(BTN_SELECT);

  if (selNow == LOW && btnSelectPrev == HIGH) {
    if (!ballLaunched) {
      ballLaunched = true;
    } else {
      paused = !paused;
      if (paused) drawPauseMessage();
      else        erasePauseMessage();
    }
  }
  btnSelectPrev = selNow;

  if (paused) return;

  int oldPaddle = paddleX;
  if (leftNow  == LOW) paddleX = max(0, paddleX - PADDLE_SPEED);
  if (rightNow == LOW) paddleX = min(SCREEN_W - PADDLE_W, paddleX + PADDLE_SPEED);
  if (oldPaddle != paddleX) {
    drawPaddle(oldPaddle, false);
    drawPaddle(paddleX, true);
  }

  if (!ballLaunched) {
    int oldBX = (int)ballX, oldBY = (int)ballY;
    ballX = paddleX + PADDLE_W / 2;
    ballY = PADDLE_Y - BALL_SIZE - 1;
    if ((int)ballX != oldBX || (int)ballY != oldBY) {
      drawBall(oldBX, oldBY, false);
      drawBall((int)ballX, (int)ballY, true);
    }
    return;
  }

  int oldBX = (int)ballX, oldBY = (int)ballY;
  ballX += ballVX;
  ballY += ballVY;

  if (ballX - BALL_SIZE < 0)       { ballX = BALL_SIZE;        ballVX =  fabs(ballVX); }
  if (ballX + BALL_SIZE > SCREEN_W){ ballX = SCREEN_W-BALL_SIZE; ballVX = -fabs(ballVX); }
  if (ballY - BALL_SIZE < 22)      { ballY = 22 + BALL_SIZE;   ballVY =  fabs(ballVY); }

  if (ballY + BALL_SIZE >= PADDLE_Y &&
      ballY - BALL_SIZE <= PADDLE_Y + PADDLE_H &&
      ballX >= paddleX && ballX <= paddleX + PADDLE_W &&
      ballVY > 0) {
    float rel   = (ballX - (paddleX + PADDLE_W / 2.0f)) / (PADDLE_W / 2.0f);
    float angle = rel * 60.0f * (PI / 180.0f);
    float spd   = sqrt(ballVX * ballVX + ballVY * ballVY);
    ballVX = spd * sin(angle);
    ballVY = -spd * cos(angle);
    if (fabs(ballVX) < 0.5f) ballVX = (ballVX >= 0 ? 0.5f : -0.5f);
    ballY = PADDLE_Y - BALL_SIZE - 1;
  }

  checkBrickCollision();

  if (ballY + BALL_SIZE > SCREEN_H) {
    lives--;
    drawHUD();
    if (lives <= 0) {
      if (score > hiScore) { hiScore = score; saveHiScore(); }
      gameState = STATE_GAME_OVER;
      drawGameOver();
      msgTimer = millis();
      return;
    }
    gameState = STATE_BALL_LOST;
    tft.setTextColor(TFT_ORANGE); tft.setTextSize(3);
    tft.setCursor(90, 105); tft.print("BALL LOST!");
    return;
  }

  if ((int)ballX != oldBX || (int)ballY != oldBY) {
    drawBall(oldBX, oldBY, false);
    drawBall((int)ballX, (int)ballY, true);
  }

  if (allBricksCleared()) {
    level++;
    gameState = STATE_LEVEL_CLEAR;
    drawLevelClear();
    msgTimer = millis();
  }
}

// =============================================
//  Setup
// =============================================
void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COL_BG);

  loadHiScore();
  gameState = STATE_TITLE;
  drawTitle();
}

// =============================================
//  Loop
// =============================================
void loop() {
  unsigned long now = millis();

  switch (gameState) {

    case STATE_TITLE: {
      static bool blinkOn = false;
      static unsigned long blinkT = 0;
      if (now - blinkT > 600) {
        blinkT = now; blinkOn = !blinkOn;
        tft.setTextSize(1);
        tft.setTextColor(blinkOn ? TFT_CYAN : COL_BG);
        tft.setCursor(80, 185);
        tft.print("Press SELECT to Start!");
      }
      bool selNow = digitalRead(BTN_SELECT);
      if (selNow == LOW && btnSelectPrev == HIGH) initGame();
      btnSelectPrev = selNow;
      break;
    }

    case STATE_PLAYING:
      if (now - lastFrame >= 16) { lastFrame = now; updateGame(); }
      break;

    case STATE_BALL_LOST: {
      bool selNow = digitalRead(BTN_SELECT);
      if (selNow == LOW && btnSelectPrev == HIGH) {
        tft.fillRect(0, 22, SCREEN_W, SCREEN_H - 22, COL_BG);
        paddleX    = (SCREEN_W - PADDLE_W) / 2;
        ballX      = paddleX + PADDLE_W / 2;
        ballY      = PADDLE_Y - BALL_SIZE - 2;
        ballSpeed  = BALL_SPEED_INIT + (level - 1) * 0.2f;
        if (ballSpeed > 7.0f) ballSpeed = 7.0f;
        ballVX     = ballSpeed;
        ballVY     = -ballSpeed;
        ballLaunched = false;
        paused       = false;
        drawBricks();
        drawPaddle(paddleX, true);
        drawBall((int)ballX, (int)ballY, true);
        drawHUD();
        gameState = STATE_PLAYING;
      }
      btnSelectPrev = selNow;
      break;
    }

    case STATE_LEVEL_CLEAR:
      if (now - msgTimer > 2500) {
        tft.fillRect(0, 22, SCREEN_W, SCREEN_H - 22, COL_BG);
        lives = MAX_LIVES;
        initLevel();
        drawBricks();
        drawPaddle(paddleX, true);
        drawBall((int)ballX, (int)ballY, true);
        drawHUD();
        gameState = STATE_PLAYING;
      }
      break;

    case STATE_GAME_OVER: {
      bool selNow = digitalRead(BTN_SELECT);
      if (selNow == LOW && btnSelectPrev == HIGH) {
        gameState = STATE_TITLE;
        drawTitle();
      }
      btnSelectPrev = selNow;
      break;
    }

    default: break;
  }

  delay(1);
}
