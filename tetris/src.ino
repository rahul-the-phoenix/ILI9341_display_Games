#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// ============= BUTTON PINS =============
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_A        14
#define BTN_B        33
#define BTN_START    32
#define BTN_SELECT   25

// ============= GAME SETTINGS =============
#define SCREEN_W  320
#define SCREEN_H  240

// Optimized board size for 320x240 display
#define FIELD_W   12   // Board width (12 blocks)
#define FIELD_H   18   // Board height (18 blocks)

// Block size calculation for optimal screen usage
#define BLOCK_SIZE  13  // 13 pixels per block
#define GAME_AREA_W  (FIELD_W * BLOCK_SIZE)   // 12*13 = 156 pixels
#define GAME_AREA_H  (FIELD_H * BLOCK_SIZE)   // 18*13 = 234 pixels

// Layout Calculations - Left aligned with small margin
#define OFFSET_X   8                    // Small left margin (8px)
#define OFFSET_Y   ((SCREEN_H - GAME_AREA_H) / 2)  // (240-234)/2 = 3px vertical center

// UI Panel Settings (Right Side - Now with more space)
#define UI_X       (OFFSET_X + GAME_AREA_W + 8)    // 8 + 156 + 8 = 172
#define UI_W       (SCREEN_W - UI_X - 5)           // 320 - 172 - 5 = 143 pixels (much better!)

// Colors
#define C_BLACK    0x0000
#define C_WHITE    0xFFFF
#define C_GRAY     0x528A
#define C_DARKGRAY 0x2124
#define C_BORDER   0xAD55
#define C_CYAN     0x07FF
#define C_BLUE     0x001F
#define C_ORANGE   0xFD20
#define C_YELLOW   0xFFE0
#define C_GREEN    0x07E0
#define C_PURPLE   0xF81F
#define C_RED      0xF800

const byte figures[7][4][4] = {
  {{0,0,0,0}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}},  // I
  {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},  // J
  {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},  // L
  {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},  // O
  {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}},  // S
  {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}},  // T
  {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}   // Z
};

uint16_t colors[8] = {C_BLACK, C_CYAN, C_BLUE, C_ORANGE, C_YELLOW, C_GREEN, C_PURPLE, C_RED};

// Game Variables
int field[FIELD_H][FIELD_W];
int currentX, currentY;
int currentType, currentRot;
int nextType;
unsigned long lastDropTime = 0;
int dropInterval = 500;
int score = 0;
bool gameOver = false;
bool isPaused = false;

unsigned long lastButtonTime = 0;
const int buttonDelay = 120;

void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  
  spr.setColorDepth(8);
  spr.createSprite(SCREEN_W, SCREEN_H);

  randomSeed(analogRead(34));
  drawStartScreen();
  
  while(digitalRead(BTN_SELECT) == HIGH) {
    delay(10);
  }
  delay(300);
  resetGame();
}

void drawStartScreen() {
  spr.fillSprite(C_BLACK);
  spr.setTextColor(C_YELLOW, C_BLACK);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("TETRIS", SCREEN_W / 2, 40, 4);
  
  spr.setTextColor(C_WHITE, C_BLACK);
  spr.setTextSize(1);
  spr.drawString("Controls:", SCREEN_W / 2 - 110, 80, 2);
  spr.drawString("LEFT/RIGHT: Move", SCREEN_W / 2 - 110, 105, 1);
  spr.drawString("UP: Rotate", SCREEN_W / 2 - 110, 125, 1);
  spr.drawString("DOWN: Soft Drop", SCREEN_W / 2 - 110, 145, 1);
  spr.drawString("A: Change Shape", SCREEN_W / 2 - 110, 165, 1);
  spr.drawString("SELECT: Hard Drop", SCREEN_W / 2 - 110, 185, 1);
  spr.drawString("START: Pause", SCREEN_W / 2 - 110, 205, 1);
  
  spr.setTextColor(C_GREEN, C_BLACK);
  spr.drawString("PRESS START", SCREEN_W / 2, 235, 2);
  
  spr.pushSprite(0, 0);
}

void loop() {
  if (!gameOver) {
    if (!isPaused) {
      handleInput();
      
      if (millis() - lastDropTime > dropInterval) {
        if (!move(0, 1)) {
          lockPiece();
          clearLines();
          spawnPiece();
          
          if (checkCollision(currentX, currentY, currentRot)) {
            gameOver = true;
          }
        }
        lastDropTime = millis();
      }
      
      drawGame();
    } else {
      drawPauseScreen();
      if (digitalRead(BTN_SELECT) == LOW) {
        isPaused = false;
        lastDropTime = millis();
        delay(300);
      }
    }
  } else {
    drawGameOver();
    if (digitalRead(BTN_SELECT) == LOW) {
      resetGame();
      delay(300);
    }
  }
}

void handleInput() {
  if (millis() - lastButtonTime < buttonDelay) return;

  if (digitalRead(BTN_SELECT) == LOW) {
    isPaused = !isPaused;
    lastButtonTime = millis();
    return;
  }

  if (isPaused) return;

  if (digitalRead(BTN_LEFT) == LOW) {
    move(-1, 0);
    lastButtonTime = millis();
  }
  else if (digitalRead(BTN_RIGHT) == LOW) {
    move(1, 0);
    lastButtonTime = millis();
  }
  else if (digitalRead(BTN_UP) == LOW) {
    rotate();
    lastButtonTime = millis();
  }
  // else if (digitalRead(BTN_DOWN) == LOW) {
  //   if (move(0, 1)) {
  //     lastDropTime = millis();
  //   }
  //   lastButtonTime = millis();
  // }
  else if (digitalRead(BTN_DOWN) == LOW) {
    // Hard drop
    while (move(0, 1)) {}
    lockPiece();
    clearLines();
    spawnPiece();
    lastDropTime = millis();
    lastButtonTime = millis();
  }
  // else if (digitalRead(BTN_A) == LOW) {
  //   // Change current shape (cheat/debug feature)
  //   int newType = random(0, 7);
  //   if (!checkCollision(currentX, currentY, currentRot)) {
  //     currentType = newType;
  //   }
  //   lastButtonTime = millis();
  // }
}

bool move(int dx, int dy) {
  if (!checkCollision(currentX + dx, currentY + dy, currentRot)) {
    currentX += dx;
    currentY += dy;
    return true;
  }
  return false;
}

void rotate() {
  int nextRot = (currentRot + 1) % 4;
  if (!checkCollision(currentX, currentY, nextRot)) {
    currentRot = nextRot;
  }
}

bool checkCollision(int x, int y, int rot) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (getBlock(currentType, rot, j, i)) {
        int fieldX = x + j;
        int fieldY = y + i;

        if (fieldX < 0 || fieldX >= FIELD_W) return true;
        if (fieldY >= FIELD_H) return true;
        if (fieldY >= 0 && field[fieldY][fieldX] != 0) return true;
      }
    }
  }
  return false;
}

int getBlock(int type, int rot, int x, int y) {
  switch (rot) {
    case 0: return figures[type][y][x];
    case 1: return figures[type][3-x][y];
    case 2: return figures[type][3-y][3-x];
    case 3: return figures[type][x][3-y];
  }
  return 0;
}

void lockPiece() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (getBlock(currentType, currentRot, j, i)) {
        int fieldX = currentX + j;
        int fieldY = currentY + i;
        if (fieldY >= 0 && fieldY < FIELD_H && fieldX >= 0 && fieldX < FIELD_W) {
          field[fieldY][fieldX] = currentType + 1;
        }
      }
    }
  }
}

void clearLines() {
  int linesCleared = 0;
  
  for (int y = FIELD_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < FIELD_W; x++) {
      if (field[y][x] == 0) {
        full = false;
        break;
      }
    }
    
    if (full) {
      for (int k = y; k > 0; k--) {
        for (int x = 0; x < FIELD_W; x++) {
          field[k][x] = field[k-1][x];
        }
      }
      for (int x = 0; x < FIELD_W; x++) field[0][x] = 0;
      
      y++;
      linesCleared++;
    }
  }

  if (linesCleared > 0) {
    // Score system: 100, 300, 500, 800 for 1,2,3,4 lines
    int points[] = {0, 100, 300, 500, 800};
    score += points[linesCleared];
    dropInterval = max(100, 500 - (score / 400) * 25);
  }
}

void spawnPiece() {
  currentType = nextType;
  nextType = random(0, 7);
  currentRot = 0;
  currentX = FIELD_W / 2 - 2;  // Center of board
  currentY = 0;
}

void resetGame() {
  for (int y = 0; y < FIELD_H; y++) {
    for (int x = 0; x < FIELD_W; x++) {
      field[y][x] = 0;
    }
  }
  score = 0;
  dropInterval = 500;
  gameOver = false;
  isPaused = false;
  
  nextType = random(0, 7);
  spawnPiece();
  lastDropTime = millis();
}

void drawGame() {
  spr.fillSprite(C_BLACK);
  
  // ========== RIGHT PANEL UI ==========
  spr.fillRect(UI_X, 0, UI_W, SCREEN_H, C_DARKGRAY);
  spr.drawRect(UI_X, 0, UI_W, SCREEN_H, C_GRAY);
  
  // Next piece label
  spr.setTextColor(C_WHITE, C_DARKGRAY);
  spr.setTextDatum(MC_DATUM);
  spr.setTextSize(1);
  spr.drawString("NEXT", UI_X + UI_W/2, 25, 2);
  
  // Next piece box - bigger now
  int boxSize = min(UI_W - 20, 100);
  int boxX = UI_X + (UI_W - boxSize)/2;
  int boxY = 45;
  spr.drawRect(boxX, boxY, boxSize, boxSize, C_WHITE);
  
  // Draw next piece centered
  int blockSizeUI = boxSize / 4;
  int startX = boxX + (boxSize - (4 * blockSizeUI))/2;
  int startY = boxY + (boxSize - (4 * blockSizeUI))/2;
  
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (figures[nextType][i][j]) {
        spr.fillRect(startX + j * blockSizeUI, startY + i * blockSizeUI, 
                     blockSizeUI - 1, blockSizeUI - 1, colors[nextType + 1]);
        spr.drawRect(startX + j * blockSizeUI, startY + i * blockSizeUI, 
                     blockSizeUI - 1, blockSizeUI - 1, C_WHITE);
      }
    }
  }
  
  // Score section
  spr.setTextFont(2);
  spr.drawString("SCORE", UI_X + UI_W/2, boxY + boxSize + 20, 2);
  spr.setTextFont(4);
  spr.setTextSize(1);
  char scoreStr[10];
  sprintf(scoreStr, "%d", score);
  spr.drawString(scoreStr, UI_X + UI_W/2, boxY + boxSize + 50);
  spr.setTextFont(1);
  
  // Level display
  // int level = (500 - dropInterval) / 25;
  // spr.setTextFont(2);
  // spr.drawString("LEVEL", UI_X + UI_W/2, boxY + boxSize + 90, 2);
  // spr.setTextFont(4);
  // char levelStr[5];
  // sprintf(levelStr, "%d", level);
  // spr.drawString(levelStr, UI_X + UI_W/2, boxY + boxSize + 115);
  // spr.setTextFont(1);
  
  // // Game Title at bottom
  // spr.setTextColor(C_GRAY, C_DARKGRAY);
  // spr.drawString("TETRIS", UI_X + UI_W/2, SCREEN_H - 20, 1);
  
  // ========== GAME AREA ==========
  // Game border
  spr.drawRect(OFFSET_X - 2, OFFSET_Y - 2, GAME_AREA_W + 4, GAME_AREA_H + 4, C_BORDER);
  spr.drawRect(OFFSET_X - 1, OFFSET_Y - 1, GAME_AREA_W + 2, GAME_AREA_H + 2, C_GRAY);
  
  // Draw field (locked pieces)
  for (int y = 0; y < FIELD_H; y++) {
    for (int x = 0; x < FIELD_W; x++) {
      if (field[y][x] != 0) {
        drawBlock(OFFSET_X + x * BLOCK_SIZE, OFFSET_Y + y * BLOCK_SIZE, colors[field[y][x]]);
      }
    }
  }
  
  // Draw current piece
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (getBlock(currentType, currentRot, j, i)) {
        int drawX = OFFSET_X + (currentX + j) * BLOCK_SIZE;
        int drawY = OFFSET_Y + (currentY + i) * BLOCK_SIZE;
        
        if (drawY + BLOCK_SIZE > OFFSET_Y && drawY < OFFSET_Y + GAME_AREA_H) {
          drawBlock(drawX, drawY, colors[currentType + 1]);
        }
      }
    }
  }

  spr.pushSprite(0, 0);
}

void drawBlock(int x, int y, uint16_t color) {
  spr.fillRect(x, y, BLOCK_SIZE - 1, BLOCK_SIZE - 1, color);
  spr.drawRect(x, y, BLOCK_SIZE - 1, BLOCK_SIZE - 1, C_WHITE);
  // Add 3D effect for better visibility
  if (BLOCK_SIZE > 4) {
    spr.drawLine(x + 1, y + 1, x + BLOCK_SIZE - 3, y + 1, 0xFFFF);
    spr.drawLine(x + 1, y + 1, x + 1, y + BLOCK_SIZE - 3, 0xFFFF);
  }
}

void drawPauseScreen() {
  spr.fillSprite(C_BLACK);
  spr.setTextColor(C_YELLOW, C_BLACK);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("PAUSED", SCREEN_W / 2, SCREEN_H / 2 - 20, 4);
  spr.setTextColor(C_WHITE, C_BLACK);
  spr.drawString("Press START to Resume", SCREEN_W / 2, SCREEN_H / 2 + 30, 2);
  spr.pushSprite(0, 0);
}

void drawGameOver() {
  spr.fillSprite(C_BLACK);
  spr.setTextColor(C_RED, C_BLACK);
  spr.setTextDatum(MC_DATUM);
  spr.drawString("GAME OVER", SCREEN_W / 2, SCREEN_H / 2 - 40, 4);
  spr.setTextColor(C_WHITE, C_BLACK);
  spr.drawString("Final Score", SCREEN_W / 2, SCREEN_H / 2, 2);
  spr.setTextFont(4);
  spr.drawString(String(score), SCREEN_W / 2, SCREEN_H / 2 + 30);
  spr.setTextFont(1);
  spr.setTextColor(C_GREEN, C_BLACK);
  spr.drawString("Press START to Restart", SCREEN_W / 2, SCREEN_H - 30, 2);
  spr.pushSprite(0, 0);
}
