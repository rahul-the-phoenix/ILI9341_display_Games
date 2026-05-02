/*
 * =====================================================================
 *  SOKOBAN  —  ILI9341 TFT (TFT_eSPI) — 20 Stages
 *  Display : 320x240
 * =====================================================================
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

TFT_eSPI tft = TFT_eSPI();

// ─── Button pins ──────────────────────────────────────────────────
#define BTN_UP     13
#define BTN_DOWN   12
#define BTN_LEFT   27
#define BTN_RIGHT  26
#define BTN_A      14
#define BTN_B      33
#define BTN_START  32
#define BTN_SELECT 25

// ─── Screen ───────────────────────────────────────────────────────
#define SCR_W  320
#define SCR_H  240

// ─── Tile types ───────────────────────────────────────────────────
#define T_FLOOR   0
#define T_TARGET  1
#define T_PLAYER  2
#define T_BOXON   3   // box on target (সবুজ বাক্স)
#define T_WALL    4
#define T_BOX     5
#define T_PTON    6   // player on target

// ─── Palette (RGB565) ─────────────────────────────────────────────
#define C_BLACK    0x0000
#define C_WHITE    0xFFFF
#define C_FLOOR    0x2104
#define C_FLOOR2   0x18C3
#define C_WALL     0x9C92
#define C_WALL_H   0xCE59
#define C_WALL_S   0x6B4D
#define C_TARGET   0xFD20
#define C_BOX      0xC5A0
#define C_BOX_H    0xEF1C
#define C_BOX_S    0x8340
#define C_BOXON    0x07E0
#define C_BOXON_H  0x87F0
#define C_PLAYER   0x001F
#define C_PLAYER_H 0x5CFF
#define C_PLTON    0xF81F
#define C_UI_BG    0x0010
#define C_UI_TXT   0xFFFF
#define C_UI_ACC   0xFD20

// ─── EEPROM ───────────────────────────────────────────────────────
#define EEPROM_SIZE        64
#define EEPROM_MAGIC_ADDR  0
#define EEPROM_MAGIC_VAL   0xAB
#define EEPROM_BEST_ADDR   1

// ─── Stage data ───────────────────────────────────────────────────
#define MAX_ROWS   11
#define MAX_COLS   13
#define NUM_STAGES 20

struct StageData {
  uint8_t rows;
  uint8_t cols;
  uint8_t cell[MAX_ROWS][MAX_COLS];
};

const StageData STAGES[NUM_STAGES] PROGMEM = {

// stage1  8×8
{ 8, 8, {
  {4,4,4,4,4,4,4,4},
  {4,4,4,1,4,4,4,4},
  {4,4,4,0,4,4,4,4},
  {4,4,4,5,0,5,1,4},
  {4,1,0,5,2,4,4,4},
  {4,4,4,4,5,4,4,4},
  {4,4,4,4,1,4,4,4},
  {4,4,4,4,4,4,4,4},
}},

// stage2  9×9
{ 9, 9, {
  {4,4,4,4,4,4,4,4,4},
  {4,2,0,0,4,4,4,4,4},
  {4,0,5,5,4,4,4,4,4},
  {4,0,5,0,4,4,4,1,4},
  {4,4,4,0,4,4,4,1,4},
  {4,4,4,0,0,0,0,1,4},
  {4,4,0,0,0,4,0,0,4},
  {4,4,0,0,0,4,4,4,4},
  {4,4,4,4,4,4,4,4,4},
}},

// stage3  8×10
{ 8, 6, {
  {4,4,4,4,4,4},
  {4,4,0,0,4,4},
  {4,2,5,0,4,4},
  {4,4,5,0,4,4},
  {4,4,0,5,0,4},
  {4,1,5,0,0,4},
  {4,1,1,3,1,4},
  {4,4,4,4,4,4},
}},


// stage4  8×6
{ 8, 10, {
  {4,4,4,4,4,4,4,4,4,4},
  {4,4,2,0,4,4,4,4,4,4},
  {4,4,0,5,0,0,4,4,4,4},
  {4,4,4,0,4,0,4,4,4,4},
  {4,1,4,0,4,0,0,4,4,4},
  {4,1,5,0,0,4,0,4,4,4},
  {4,1,0,0,0,5,0,4,4,4},
  {4,4,4,4,4,4,4,4,4,4},
}},

// stage5  8×8
{ 8, 8, {
  {4,4,4,4,4,4,4,4},
  {4,4,2,0,4,4,4,4},
  {4,4,0,5,0,0,4,4},
  {4,4,4,0,4,0,4,4},
  {4,1,4,0,4,0,0,4},
  {4,1,5,0,0,4,0,4},
  {4,1,0,0,0,5,0,4},
  {4,4,4,4,4,4,4,4},
}},

// stage6  11×13
{ 11, 13, {
  {4,4,4,4,4,4,4,4,4,4,4,4,4},
  {4,4,4,4,0,0,0,0,0,4,4,4,4},
  {4,0,0,0,1,4,4,4,0,4,4,4,4},
  {4,0,4,0,4,0,0,0,0,4,4,4,4},
  {4,0,4,0,5,0,5,4,1,0,4,4,4},
  {4,0,4,0,0,0,0,0,4,0,4,4,4},
  {4,0,1,4,5,0,5,0,4,0,4,4,4},
  {4,4,0,0,0,0,4,0,4,0,4,4,4},
  {4,4,0,4,4,4,1,0,0,0,0,2,4},
  {4,4,0,0,0,0,0,4,4,0,0,0,4},
  {4,4,4,4,4,4,4,4,4,4,4,4,4},
}},

// stage7  8×10
{ 8, 10, {
  {4,4,4,4,4,4,4,4,4,4},
  {4,4,4,4,0,0,4,0,2,4},
  {4,4,4,0,0,0,4,0,0,4},
  {4,4,4,5,0,5,0,5,0,4},
  {4,4,4,0,5,4,4,0,0,4},
  {4,4,4,0,5,0,4,0,4,4},
  {4,1,1,1,1,1,0,0,4,4},
  {4,4,4,4,4,4,4,4,4,4},
}},

// stage8  7×10
{ 7, 10, {
  {4,4,4,4,4,4,4,4,4,4},
  {4,4,4,4,0,0,0,0,4,4},
  {4,4,1,0,5,4,4,0,4,4},
  {4,1,1,5,0,5,0,0,2,4},
  {4,1,1,0,5,0,5,0,4,4},
  {4,4,4,4,4,4,0,0,4,4},
  {4,4,4,4,4,4,4,4,4,4},
}},

// stage9  9×11
{ 9, 11, {
  {4,4,4,4,4,4,4,4,4,4,4},
  {4,4,0,0,4,4,0,0,0,4,4},
  {4,4,0,0,0,5,0,0,0,4,4},
  {4,4,5,0,4,4,4,0,5,4,4},
  {4,4,0,4,1,1,1,4,0,4,4},
  {4,4,0,4,1,1,1,4,0,4,4},
  {4,0,5,0,0,5,0,0,5,0,4},
  {4,0,0,0,0,0,4,0,2,0,4},
  {4,4,4,4,4,4,4,4,4,4,4},
}},

// stage10  7×8
{ 7, 8, {
  {4,4,4,4,4,4,4,4},
  {4,4,4,0,0,0,0,4},
  {4,4,4,5,5,5,0,4},
  {4,2,0,5,1,1,0,4},
  {4,0,5,1,1,1,4,4},
  {4,4,4,4,0,0,4,4},
  {4,4,4,4,4,4,4,4},
}},

// stage11  6×12
{ 6, 12, {
  {4,4,4,4,4,4,4,4,4,4,4,4},
  {4,4,0,0,4,4,4,4,0,0,0,4},
  {4,0,5,0,4,4,4,4,5,0,0,4},
  {4,0,0,5,1,1,1,1,0,5,0,4},
  {4,4,0,0,0,0,4,0,2,0,4,4},
  {4,4,4,4,4,4,4,4,4,4,4,4},
}},

// stage12  7×8
{ 7, 8, {
  {4,4,4,4,4,4,4,4},
  {4,4,4,0,0,2,4,4},
  {4,0,0,5,1,0,4,4},
  {4,0,0,1,5,1,0,4},
  {4,4,4,0,0,5,0,4},
  {4,4,4,0,0,0,4,4},
  {4,4,4,4,4,4,4,4},
}},

// stage13  8×8
{ 8, 8, {
  {4,4,4,4,4,4,4,4},
  {4,4,4,1,1,4,4,4},
  {4,4,4,0,1,4,4,4},
  {4,4,0,0,5,1,4,4},
  {4,4,0,5,0,0,4,4},
  {4,0,0,4,5,5,0,4},
  {4,0,0,2,0,0,0,4},
  {4,4,4,4,4,4,4,4},
}},

// stage14  7×8
{ 7, 8, {
  {4,4,4,4,4,4,4,4},
  {4,0,0,4,0,0,0,4},
  {4,0,5,1,1,5,0,4},
  {4,2,5,1,0,0,4,4},
  {4,0,5,1,1,5,0,4},
  {4,0,0,4,0,0,0,4},
  {4,4,4,4,4,4,4,4},
}},

// stage15  7×8
{ 7, 8, {
  {4,4,4,4,4,4,4,4},
  {4,4,0,0,0,0,4,4},
  {4,0,5,0,5,5,0,4},
  {4,1,1,1,1,1,1,4},
  {4,0,5,5,0,5,0,4},
  {4,4,4,0,2,4,4,4},
  {4,4,4,4,4,4,4,4},
}},

// stage16  9×10
{ 9, 10, {
  {4,4,4,4,4,4,4,4,4,4},
  {4,4,4,0,0,0,0,4,4,4},
  {4,4,4,0,5,0,0,0,0,4},
  {4,4,4,0,5,0,4,4,0,4},
  {4,1,1,1,0,5,0,0,0,4},
  {4,1,1,1,5,4,5,0,4,4},
  {4,4,4,4,0,4,0,5,0,4},
  {4,4,4,4,0,0,2,0,0,4},
  {4,4,4,4,4,4,4,4,4,4},
}},

// stage17  7×9
{ 7, 9, {
  {4,4,4,4,4,4,4,4,4},
  {4,0,0,0,0,4,4,4,4},
  {4,0,5,5,5,4,4,4,4},
  {4,0,0,4,1,1,4,4,4},
  {4,4,0,0,1,1,5,0,4},
  {4,4,0,2,0,0,0,0,4},
  {4,4,4,4,4,4,4,4,4},
}},

// stage18  9×10
{ 9, 10, {
  {4,4,4,4,4,4,4,4,4,4},
  {4,4,4,0,0,0,4,1,0,4},
  {4,4,4,0,0,5,1,1,1,4},
  {4,4,0,0,5,0,4,0,1,4},
  {4,4,0,4,4,5,4,0,4,4},
  {4,0,0,0,5,0,0,5,0,4},
  {4,0,0,0,4,0,0,0,0,4},
  {4,4,4,4,4,4,4,2,0,4},
  {4,4,4,4,4,4,4,4,4,4},
}},

// stage19  8×10
{ 8, 10, {
  {4,4,4,4,4,4,4,4,4,4},
  {4,4,1,1,1,1,0,4,4,4},
  {4,4,4,1,1,1,5,4,4,4},
  {4,0,0,5,4,5,0,5,0,4},
  {4,0,5,5,0,0,4,5,0,4},
  {4,0,0,0,0,4,0,0,0,4},
  {4,4,4,4,0,2,0,4,4,4},
  {4,4,4,4,4,4,4,4,4,4},
}},

// stage20  8×7
{ 8, 7, {
  {4,4,4,4,4,4,4},
  {4,1,1,5,1,1,4},
  {4,1,1,4,1,1,4},
  {4,0,5,5,5,0,4},
  {4,0,0,5,0,0,4},
  {4,0,5,5,5,0,4},
  {4,0,0,4,2,0,4},
  {4,4,4,4,4,4,4},
}},

}; // end STAGES

// ─── Game state ───────────────────────────────────────────────────
#define MAX_UNDO 64

uint8_t  board[MAX_ROWS][MAX_COLS];
uint8_t  rows, cols;
int8_t   px, py;
uint16_t moves;
int      currentLevel    = 0;
int      totalTargets    = 0;
bool     levelComplete   = false;
bool     onTitleScreen   = true;
bool     onLevelSelect   = false;
int      levelSelectCursor = 0;

uint8_t  bestMoves[NUM_STAGES];

// ── Undo stack ────────────────────────────────────────────────────
struct UndoEntry {
  int8_t  oldPx, oldPy;
  bool    oldPonTarget;
  uint8_t nBox;
  int8_t  boxOldR, boxOldC; uint8_t boxOldVal;
  int8_t  boxNewR, boxNewC; uint8_t boxNewVal;
};
UndoEntry undoStack[MAX_UNDO];
int       undoTop = 0;

uint8_t TILE;
int16_t boardOffX, boardOffY;

// ─── Button debounce ──────────────────────────────────────────────
unsigned long lastBtnTime[8] = {0};
const unsigned long DEBOUNCE = 160;
bool prevState[8] = {HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH,HIGH};

// ─── Draw one tile ────────────────────────────────────────────────
void drawTile(int8_t r, int8_t c) {
  uint8_t  t  = board[r][c];
  int16_t  x  = boardOffX + c * TILE;
  int16_t  y  = boardOffY + r * TILE;
  int16_t  T  = TILE;

  switch (t) {
    case T_WALL: {
      tft.fillRect(x, y, T, T, C_WALL);
      tft.drawFastHLine(x,     y,     T, C_WALL_H);
      tft.drawFastVLine(x,     y,     T, C_WALL_H);
      tft.drawFastHLine(x,     y+T-1, T, C_WALL_S);
      tft.drawFastVLine(x+T-1, y,     T, C_WALL_S);
      if (T >= 10) {
        int mid = T/2;
        tft.drawFastHLine(x+2,   y+mid, T-4, C_WALL_S);
        tft.drawFastVLine(x+mid, y+2,   T-4, C_WALL_S);
      }
      break;
    }
    case T_FLOOR: {
      tft.fillRect(x, y, T, T, C_FLOOR2);
      break;
    }
    case T_TARGET: {
      tft.fillRect(x, y, T, T, C_FLOOR2);
      int m = max(2, T/5);
      tft.drawRect(x+m, y+m, T-2*m, T-2*m, C_TARGET);
      if (T >= 14) tft.drawRect(x+m+1, y+m+1, T-2*m-2, T-2*m-2, C_TARGET);
      break;
    }
    case T_BOX: {
      tft.fillRect(x+1, y+1, T-2, T-2, C_BOX);
      tft.drawFastHLine(x+1,   y+1,   T-2, C_BOX_H);
      tft.drawFastVLine(x+1,   y+1,   T-2, C_BOX_H);
      tft.drawFastHLine(x+1,   y+T-2, T-2, C_BOX_S);
      tft.drawFastVLine(x+T-2, y+1,   T-2, C_BOX_S);
      if (T >= 12) {
        int m = T/4;
        tft.drawRect(x+m, y+m, T-2*m, T-2*m, C_BOX_S);
      }
      tft.drawRect(x, y, T, T, C_FLOOR2);
      break;
    }
    case T_BOXON: {
      tft.fillRect(x+1, y+1, T-2, T-2, C_BOXON);
      tft.drawFastHLine(x+1, y+1, T-2, C_BOXON_H);
      tft.drawFastVLine(x+1, y+1, T-2, C_BOXON_H);
      if (T >= 12) {
        int m = T/4;
        tft.fillRect(x+m, y+m, T-2*m, T-2*m, C_BOXON_H);
      }
      tft.drawRect(x, y, T, T, C_FLOOR2);
      break;
    }
    case T_PLAYER:
    case T_PTON: {
      tft.fillRect(x, y, T, T, C_FLOOR2);
      if (t == T_PTON) {
        int m = max(2, T/5);
        tft.drawRect(x+m, y+m, T-2*m, T-2*m, C_TARGET);
        if (T >= 14) tft.drawRect(x+m+1, y+m+1, T-2*m-2, T-2*m-2, C_TARGET);
      }
      uint16_t pcol = (t == T_PTON) ? C_PLTON : C_PLAYER;
      int cx = x + T/2, cy = y + T/2;
      int hr = max(2, T/5);
      int br = max(3, T*3/8);
      tft.fillRect(cx - br/2, cy - br/4, br, T/2, pcol);
      tft.fillCircle(cx, cy - br/4 - hr, hr, pcol);
      if (T >= 12) {
        tft.fillCircle(cx - hr/3, cy - br/4 - hr, 1, C_WHITE);
        tft.fillCircle(cx + hr/3, cy - br/4 - hr, 1, C_WHITE);
      }
      if (T >= 14) {
        int legY = cy + T/4 - 2;
        tft.drawFastVLine(cx - hr/2, legY, T/5, pcol);
        tft.drawFastVLine(cx + hr/2, legY, T/5, pcol);
      }
      break;
    }
    default:
      tft.fillRect(x, y, T, T, C_FLOOR);
      break;
  }
}

// ─── Draw entire board ────────────────────────────────────────────
void drawBoard() {
  tft.fillRect(0, 20, SCR_W, SCR_H - 20, C_FLOOR);
  for (int8_t r = 0; r < rows; r++)
    for (int8_t c = 0; c < cols; c++)
      drawTile(r, c);
}

// ─── HUD ──────────────────────────────────────────────────────────
void drawHUD() {
  tft.fillRect(0, 0, SCR_W, 20, C_UI_BG);
  tft.drawFastHLine(0, 20, SCR_W, C_UI_ACC);
  char buf[32];

  tft.setTextSize(1);
  tft.setTextColor(C_UI_ACC);
  tft.setCursor(4, 6);
  sprintf(buf, "LV:%02d  MV:%03d", currentLevel + 1, moves);
  tft.print(buf);

  tft.setTextColor(0x07FF);
  tft.setCursor(160, 6);
  if (bestMoves[currentLevel] == 255)
    tft.print("BEST:---");
  else {
    sprintf(buf, "BEST:%03d", bestMoves[currentLevel]);
    tft.print(buf);
  }

  int boxesLeft = 0;
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++)
      if (board[r][c] == T_BOX) boxesLeft++;

  tft.setTextColor(boxesLeft == 0 ? C_BOXON : 0xF800);
  tft.setCursor(258, 6);
  sprintf(buf, "BOX:%d", boxesLeft);
  tft.print(buf);
}

// ─── Compute tile size & offsets ─────────────────────────────────
void computeLayout() {
  int avW = SCR_W;
  int avH = SCR_H - 20;
  int tw  = avW / cols;
  int th  = avH / rows;
  TILE = (uint8_t)min(min(tw, th), 28);
  if (TILE < 8) TILE = 8;
  boardOffX = (avW - cols * TILE) / 2;
  boardOffY = 20 + (avH - rows * TILE) / 2;
}

// ─── Load level ───────────────────────────────────────────────────
void loadLevel(int lvl) {
  StageData sd;
  memcpy_P(&sd, &STAGES[lvl], sizeof(StageData));
  rows = sd.rows;
  cols = sd.cols;
  px = -1; py = -1;
  moves      = 0;
  undoTop    = 0;
  totalTargets = 0;
  levelComplete = false;

  for (int8_t r = 0; r < rows; r++) {
    for (int8_t c = 0; c < cols; c++) {
      uint8_t v = sd.cell[r][c];

      if (v == T_PLAYER) {
        board[r][c] = T_PLAYER;
        px = c; py = r;
      } else if (v == T_PTON) {
        board[r][c] = T_PTON;
        px = c; py = r;
        totalTargets++;
      } else {
        board[r][c] = v;
        if (v == T_TARGET || v == T_BOXON) totalTargets++;
      }
    }
  }

  computeLayout();
}

// ─── Count boxes on targets ───────────────────────────────────────
int countBoxOnTarget() {
  int n = 0;
  for (int r = 0; r < rows; r++)
    for (int c = 0; c < cols; c++)
      if (board[r][c] == T_BOXON) n++;
  return n;
}

// ─── Try to move player ───────────────────────────────────────────
bool tryMove(int8_t dr, int8_t dc) {
  int8_t nr = py + dr, nc = px + dc;
  if (nr < 0 || nr >= rows || nc < 0 || nc >= cols) return false;

  uint8_t dest = board[nr][nc];
  if (dest == T_WALL) return false;

  UndoEntry ue;
  ue.oldPx = px;
  ue.oldPy = py;
  ue.oldPonTarget = (board[py][px] == T_PTON);
  ue.nBox = 0;

  if (dest == T_BOX || dest == T_BOXON) {
    int8_t br = nr + dr, bc = nc + dc;
    if (br < 0 || br >= rows || bc < 0 || bc >= cols) return false;
    uint8_t bdest = board[br][bc];
    if (bdest == T_WALL || bdest == T_BOX || bdest == T_BOXON) return false;

    ue.nBox      = 1;
    ue.boxOldR   = nr;  ue.boxOldC   = nc;  ue.boxOldVal = board[nr][nc];
    ue.boxNewR   = br;  ue.boxNewC   = bc;  ue.boxNewVal = board[br][bc];

    board[br][bc] = (bdest == T_TARGET) ? T_BOXON : T_BOX;
    board[nr][nc] = (dest  == T_BOXON)  ? T_TARGET : T_FLOOR;

    drawTile(br, bc);
    drawTile(nr, nc);
  }

  board[py][px] = ue.oldPonTarget ? T_TARGET : T_FLOOR;
  drawTile(py, px);

  uint8_t destAfterBox = board[nr][nc];
  board[nr][nc] = (destAfterBox == T_TARGET) ? T_PTON : T_PLAYER;
  px = nc; py = nr;
  drawTile(py, px);

  if (undoTop < MAX_UNDO) undoStack[undoTop++] = ue;

  moves++;
  return true;
}

// ─── Undo ─────────────────────────────────────────────────────────
void doUndo() {
  if (undoTop == 0) return;
  UndoEntry ue = undoStack[--undoTop];

  uint8_t curPlayerCell = board[py][px];
  bool destWasTarget    = (curPlayerCell == T_PTON);
  board[py][px]         = destWasTarget ? T_TARGET : T_FLOOR;
  drawTile(py, px);

  if (ue.nBox) {
    board[ue.boxOldR][ue.boxOldC] = ue.boxOldVal;
    board[ue.boxNewR][ue.boxNewC] = ue.boxNewVal;
    drawTile(ue.boxOldR, ue.boxOldC);
    drawTile(ue.boxNewR, ue.boxNewC);
  }

  px = ue.oldPx;
  py = ue.oldPy;
  board[py][px] = ue.oldPonTarget ? T_PTON : T_PLAYER;
  drawTile(py, px);

  if (moves > 0) moves--;
  drawHUD();
}

// ─── Win screen ───────────────────────────────────────────────────
void showWinScreen() {
  if (moves < bestMoves[currentLevel]) {
    bestMoves[currentLevel] = (moves > 255) ? 255 : (uint8_t)moves;
    EEPROM.write(EEPROM_BEST_ADDR + currentLevel, bestMoves[currentLevel]);
    EEPROM.commit();
  }
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(C_BOXON); delay(80);
    tft.fillScreen(C_FLOOR); delay(80);
  }
  tft.fillScreen(C_FLOOR);
  tft.drawRect(20, 40, 280, 160, C_UI_ACC);
  tft.drawRect(22, 42, 276, 156, C_BOXON);
  tft.fillRect(24, 44, 272, 152, C_UI_BG);

  tft.setTextColor(C_BOXON); tft.setTextSize(3);
  tft.setCursor(55, 60); tft.print("LEVEL CLEAR!");

  tft.setTextColor(C_UI_TXT); tft.setTextSize(2);
  tft.setCursor(50, 105);
  char buf[32];
  sprintf(buf, "Moves : %d", moves);
  tft.print(buf);

  tft.setCursor(50, 128);
  if (bestMoves[currentLevel] == 255 || moves <= bestMoves[currentLevel]) {
    tft.setTextColor(C_UI_ACC);
    tft.print("NEW BEST!");
  } else {
    tft.setTextColor(0x7BEF);
    sprintf(buf, "Best  : %d", bestMoves[currentLevel]);
    tft.print(buf);
  }

  tft.setTextSize(1); tft.setTextColor(0x07FF);
  tft.setCursor(38, 165); tft.print("[A] Next Level   [B] Level Select");

  if (currentLevel == NUM_STAGES - 1) {
    tft.setTextColor(C_UI_ACC); tft.setTextSize(2);
    tft.setCursor(60, 148); tft.print("ALL DONE!");
  }
}

// ─── Title screen ─────────────────────────────────────────────────
void showTitleScreen() {
  tft.fillScreen(C_FLOOR);
  for (int i = 0; i < SCR_W; i += 20) {
    tft.fillRect(i,      0,       10, 10, C_WALL);
    tft.drawFastHLine(i, 0,       10, C_WALL_H);
    tft.drawFastVLine(i, 0,       10, C_WALL_H);
    tft.fillRect(i + 10, SCR_H-10, 10, 10, C_WALL);
  }
  for (int i = 0; i < SCR_H; i += 20) {
    tft.fillRect(0,       i,    10, 10, C_WALL);
    tft.fillRect(SCR_W-10, i+10, 10, 10, C_WALL);
  }
  tft.setTextColor(C_UI_ACC); tft.setTextSize(4);
  tft.setCursor(42, 30); tft.print("SOKOBAN");

  tft.setTextColor(C_BOXON); tft.setTextSize(1);
  tft.setCursor(80, 80); tft.print("Push boxes onto targets!");

  int dx = 60, dy = 100;
  tft.fillRect(dx,    dy, 18, 18, C_WALL);
  tft.drawFastHLine(dx, dy, 18, C_WALL_H);
  tft.drawFastVLine(dx, dy, 18, C_WALL_H);
  tft.fillRect(dx+24, dy, 18, 18, C_BOX);
  tft.drawFastHLine(dx+24, dy, 18, C_BOX_H);
  tft.fillRect(dx+48, dy, 18, 18, C_FLOOR2);
  tft.drawRect(dx+51, dy+3, 12, 12, C_TARGET);
  tft.fillRect(dx+72, dy, 18, 18, C_BOXON);
  tft.drawFastHLine(dx+72, dy, 18, C_BOXON_H);
  tft.fillRect(dx+96, dy, 18, 18, C_FLOOR2);
  tft.fillCircle(dx+105, dy+6, 4, C_PLAYER);
  tft.fillRect(dx+102, dy+10, 6, 7, C_PLAYER);
  tft.fillRect(dx+120, dy, 18, 18, C_FLOOR2);
  tft.drawRect(dx+123, dy+3, 12, 12, C_TARGET);
  tft.fillCircle(dx+129, dy+6, 4, C_PLTON);
  tft.fillRect(dx+126, dy+10, 6, 7, C_PLTON);

  tft.setTextColor(0x7BEF); tft.setTextSize(1);
  tft.setCursor(55, 121); tft.print("Wall  Box Target Done Player P-on-T");

  tft.setTextColor(C_UI_TXT); tft.setTextSize(1);
  tft.setCursor(28, 148);
  tft.print("UP/DN/LR:Move  A:Undo  SEL:Menu  STA:Restart");
  tft.setCursor(28, 162); tft.print("20 handcrafted levels. Good luck!");

  tft.setTextColor(C_UI_ACC); tft.setTextSize(2);
  tft.setCursor(62, 185); tft.print("Press SEL to Start");
  tft.setCursor(42, 210); tft.print("From Menu Start Game");
}

// ─── Level select screen ──────────────────────────────────────────
void showLevelSelect() {
  tft.fillScreen(C_FLOOR);
  tft.fillRect(0, 0, SCR_W, 18, C_UI_BG);
  tft.drawFastHLine(0, 18, SCR_W, C_UI_ACC);
  tft.setTextColor(C_UI_ACC); tft.setTextSize(1);
  tft.setCursor(80, 5); tft.print("SELECT LEVEL  (A=Play  B=Back)");

  for (int i = 0; i < NUM_STAGES; i++) {
    int col = i % 5;
    int row = i / 5;
    int bx  = 8  + col * 62;
    int by  = 25 + row * 50;
    bool done = bestMoves[i] < 255;
    bool sel  = (i == levelSelectCursor);

    uint16_t bg     = sel ? C_UI_ACC : (done ? 0x0320 : C_UI_BG);
    uint16_t border = sel ? C_WHITE  : (done ? C_BOXON : 0x39E7);

    tft.fillRoundRect(bx, by, 56, 42, 4, bg);
    tft.drawRoundRect(bx, by, 56, 42, 4, border);
    tft.drawRoundRect(bx+1, by+1, 54, 40, 3, sel ? C_BOXON : border);

    tft.setTextColor(sel ? C_FLOOR : C_UI_TXT); tft.setTextSize(2);
    char buf[4]; sprintf(buf, "%02d", i+1);
    tft.setCursor(bx+10, by+8); tft.print(buf);

    tft.setTextSize(1);
    if (done) {
      tft.setTextColor(sel ? C_FLOOR : C_BOXON);
      sprintf(buf, "%3d", bestMoves[i]);
      tft.setCursor(bx+8, by+30); tft.print(buf);
    } else {
      tft.setTextColor(sel ? C_FLOOR : 0x39E7);
      tft.setCursor(bx+12, by+30); tft.print("---");
    }
  }
}

// ─── EEPROM ───────────────────────────────────────────────────────
void loadEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  if (EEPROM.read(EEPROM_MAGIC_ADDR) != EEPROM_MAGIC_VAL) {
    EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VAL);
    for (int i = 0; i < NUM_STAGES; i++) {
      EEPROM.write(EEPROM_BEST_ADDR + i, 255);
      bestMoves[i] = 255;
    }
    EEPROM.commit();
  } else {
    for (int i = 0; i < NUM_STAGES; i++)
      bestMoves[i] = EEPROM.read(EEPROM_BEST_ADDR + i);
  }
}

// ─── Button helpers ───────────────────────────────────────────────
const uint8_t BTN_PINS[8] = {
  BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT,
  BTN_A,  BTN_B,   BTN_START, BTN_SELECT
};
enum BtnIdx { B_UP=0,B_DOWN,B_LEFT,B_RIGHT,B_A,B_B,B_START,B_SELECT };

bool btnPressed(uint8_t idx) {
  bool cur = (digitalRead(BTN_PINS[idx]) == LOW);
  if (cur && !prevState[idx] && millis() - lastBtnTime[idx] > DEBOUNCE) {
    lastBtnTime[idx] = millis();
    prevState[idx]   = true;
    return true;
  }
  if (!cur) prevState[idx] = false;
  return false;
}

// ─── SETUP ────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 8; i++) pinMode(BTN_PINS[i], INPUT_PULLUP);
  loadEEPROM();
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(C_FLOOR);
  randomSeed(analogRead(34));
  onTitleScreen = true;
  onLevelSelect = false;
  showTitleScreen();
}

// ─── LOOP ─────────────────────────────────────────────────────────
void loop() {

  if (onTitleScreen) {
    if (btnPressed(B_SELECT)) {
      onTitleScreen      = false;
      onLevelSelect      = true;
      levelSelectCursor  = 0;
      showLevelSelect();
    }
    return;
  }

  if (onLevelSelect) {
    if (btnPressed(B_UP)    && levelSelectCursor >= 5)  { levelSelectCursor -= 5; showLevelSelect(); }
    if (btnPressed(B_DOWN)  && levelSelectCursor < 15)  { levelSelectCursor += 5; showLevelSelect(); }
    if (btnPressed(B_LEFT)  && levelSelectCursor > 0)   { levelSelectCursor--;    showLevelSelect(); }
    if (btnPressed(B_RIGHT) && levelSelectCursor < 19)  { levelSelectCursor++;    showLevelSelect(); }
    if (btnPressed(B_A)) {
      onLevelSelect = false;
      currentLevel  = levelSelectCursor;
      loadLevel(currentLevel);
      tft.fillScreen(C_FLOOR);
      drawBoard(); drawHUD();
    }
    if (btnPressed(B_B)) {
      onLevelSelect = false;
      onTitleScreen = true;
      showTitleScreen();
    }
    return;
  }

  if (levelComplete) {
    if (btnPressed(B_A)) {
      if (currentLevel < NUM_STAGES - 1) {
        currentLevel++;
        loadLevel(currentLevel);
        tft.fillScreen(C_FLOOR);
        drawBoard(); drawHUD();
        levelComplete = false;
      } else {
        onTitleScreen = true;
        showTitleScreen();
        levelComplete = false;
      }
    }
    if (btnPressed(B_B)) {
      onLevelSelect      = true;
      levelSelectCursor  = currentLevel;
      levelComplete      = false;
      showLevelSelect();
    }
    return;
  }

  // Restart current level with START button
  if (btnPressed(B_START)) {
    loadLevel(currentLevel);
    tft.fillScreen(C_FLOOR);
    drawBoard(); drawHUD();
    return;
  }

  // Reset to menu with SELECT button
  if (btnPressed(B_SELECT)) {
    onTitleScreen = true;
    onLevelSelect = false;
    levelComplete = false;
    showTitleScreen();
    return;
  }

  // Undo with A button
  if (btnPressed(B_A)) {
    doUndo();
    return;
  }

  bool moved = false;
  if (btnPressed(B_UP))    moved = tryMove(-1,  0);
  if (btnPressed(B_DOWN))  moved = tryMove( 1,  0);
  if (btnPressed(B_LEFT))  moved = tryMove( 0, -1);
  if (btnPressed(B_RIGHT)) moved = tryMove( 0,  1);

  if (moved) {
    drawHUD();
    if (countBoxOnTarget() == totalTargets && totalTargets > 0) {
      levelComplete = true;
      delay(300);
      showWinScreen();
    }
  }

  delay(10);
}
