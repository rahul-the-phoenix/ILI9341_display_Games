#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

TFT_eSPI tft = TFT_eSPI();

// ============= BUTTON PINS =============
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_A        14
#define BTN_B        33
#define BTN_START    32
#define BTN_SELECT   25

// ============= GAME MODE ENUM =============
enum GameMode {
  GAME_MENU,
  GAME_SNAKE_SUBMENU,
  GAME_SNAKE_CLASSIC,
  GAME_SNAKE_BARRIER,
  GAME_SNAKE_NOBORDER,
  GAME_FLAPPY,
  GAME_FLAPPY_SUBMENU,
  GAME_CONNECT4,
  GAME_TICTACTOE,
  GAME_RPS,
  GAME_NINJA,
  GAME_PONG,
  GAME_SPERMRACE,
  GAME_CARRACING,
  GAME_RTEX_SUBMENU,
  GAME_RTEX,
  GAME_BRICKOUT_SUBMENU,
  GAME_BRICKOUT_CLASSIC,
  GAME_BRICKOUT_SELECT
};

GameMode currentGame = GAME_MENU;
int selectedGame     = 0;

int screenWidth  = 320;
int screenHeight = 240;

// ============= SNAKE SUBMENU =============
int snakeSubSelected = 0;  // 0=Classic, 1=Barrier, 2=No Border
int snakeMode = 0;

// ============= FLAPPY SUBMENU =============
int flappySubSelected = 0;
int flappyVersion     = 0;

// ============= RTEX SUBMENU =============
int rtexSubSelected = 0;
int rtexVersion     = 0;

// ============= BRICKOUT SUBMENU =============
int brickoutSubSelected = 0;
int brickoutSelectedLevel = 1;

// ============= MENU DESIGN CONSTANTS =============
#define MENU_BG        0x0000
#define MENU_HEADER_H  38
#define MENU_ITEMS     11
#define MENU_PER_PAGE  6
#define MENU_COLS      3
#define MENU_ROWS      2
#define CARD_W         98
#define CARD_H         86
#define CARD_GAP_X     4
#define CARD_GAP_Y     5
#define CARD_START_X   4
#define CARD_START_Y   (MENU_HEADER_H + 4)

int menuPage = 0;

const uint16_t GAME_ACCENT[11] = {
  0x07E0, 0xFFE0, 0xF81F, 0x837F, 0xFC00, 0x07FF, 0xF800, 0xFB9A, 0x02DF, 0xF7BE, 0x001F
};
const uint16_t GAME_DIM[11] = {
  0x0140, 0x2200, 0x2006, 0x1012, 0x2100, 0x020A, 0x2000, 0x300A, 0x0109, 0x2104, 0x0108
};
const char* GAME_NAMES[11] = {
  "SNAKE","FLAPPY","CONNECT 4","TIC TAC","RPS","NINJA UP","PONG","SPERM","RACING","RTEX LITE","BRICKOUT"
};
const char* GAME_SUBTITLES[11] = {
  "EAT & GROW","FLY & DODGE","CONNECT&BLOCK","THINK&PLACE",
  "CHOOSE&DUEL","DODGE RUSH","RALLY & WIN","DODGE&SURVIVE","RACE&DODGE","DINO RUN","BREAK BRICKS"
};
const char* GAME_MODE_BADGE[11] = {
  "***","1P","BOT","BOT","BOT","1P","BOT","1P","1P","1P","1P"
};

int menuHighScores[11] = {0,0,0,0,0,0,0,0,0,0,0};

// ================================================================
// =================== SNAKE GAME (Classic Wrap-Around) ==========
// ================================================================
#define SNAKE_GRID      10
#define MAX_SNAKE_LEN   200
#define COL_BG          0x0000
#define COL_SNAKE_HEAD  0xF800
#define COL_SNAKE_BODY  0xAFE5
#define COL_FOOD        0x07FF
#define COL_BONUS       0xF81F
#define COL_SCORE       0xFFFF
#define COL_GOLD        0xFD20
#define COL_BARRIER     0x8410

struct Point { int x, y; };
Point snake[MAX_SNAKE_LEN];
int snakeLen, dirX, dirY, score;
int highScoreSnake = 0;
bool gameOverSnake = false, isPausedSnake = false;
int foodCounter = 0;
unsigned long lastMoveTime, lastPulseTime;
int moveDelay = 150, pulseState = 0;
Point food, bonusFood;
bool bonusActive = false;
unsigned long bonusStartTime;
int maxGridX, maxGridY;

// Barrier system variables
#define MAX_BARRIERS 50
Point barriers[MAX_BARRIERS];
int barrierCount = 0;

// ================================================================
// =================== SNAKE COMMON FUNCTIONS ====================
// ================================================================
void snakeUpdateScoreDisplay() {
  tft.fillRect(0, 0, tft.width(), 20, COL_BG);
  tft.setTextColor(COL_SCORE); 
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.print("Score: ");
  tft.print(score);
  tft.setCursor(tft.width() - 80, 5);
  tft.print("HI: ");
  tft.print(highScoreSnake);
  tft.drawFastHLine(0, 20, tft.width(), TFT_WHITE);
}

void snakeShowPauseScreen(bool pause) {
  if (pause) {
    tft.fillRect(tft.width()/2 - 80, tft.height()/2 - 30, 160, 60, 0x001F);
    tft.drawRect(tft.width()/2 - 80, tft.height()/2 - 30, 160, 60, COL_SCORE);
    tft.setCursor(tft.width()/2 - 45, tft.height()/2 - 5);
    tft.setTextColor(COL_SCORE);
    tft.setTextSize(2);
    tft.print("PAUSED");
    tft.setTextSize(1);
  } else {
    tft.fillRect(tft.width()/2 - 80, tft.height()/2 - 30, 160, 60, COL_BG);
    snakeDrawFood(false);
    if(bonusActive) snakeDrawBonusFood(false);
  }
}

void snakeSpawnFood() {
  snakeDrawFood(true); 
  bool validPosition;
  do {
    validPosition = true;
    food.x = random(0, maxGridX);
    food.y = random(0, maxGridY);
    for (int i = 0; i < snakeLen; i++) {
      if (snake[i].x == food.x && snake[i].y == food.y) {
        validPosition = false;
        break;
      }
    }
    if(currentGame == GAME_SNAKE_BARRIER && checkCollisionWithBarriers(food.x, food.y)) {
      validPosition = false;
    }
  } while (!validPosition);
  snakeDrawFood(false);
}

void snakeSpawnBonusFood() {
  if (!bonusActive) {
    bonusActive = true;
    bonusStartTime = millis();
    bool validPosition;
    do {
      validPosition = true;
      bonusFood.x = random(0, maxGridX);
      bonusFood.y = random(0, maxGridY);
      for (int i = 0; i < snakeLen; i++) {
        if (snake[i].x == bonusFood.x && snake[i].y == bonusFood.y) {
          validPosition = false;
          break;
        }
      }
      if ((food.x == bonusFood.x && food.y == bonusFood.y)) validPosition = false;
      if(currentGame == GAME_SNAKE_BARRIER && checkCollisionWithBarriers(bonusFood.x, bonusFood.y)) {
        validPosition = false;
      }
    } while (!validPosition);
  }
}

void snakeDrawPart(int index, bool erase) {
  int x = snake[index].x * SNAKE_GRID;
  int y = snake[index].y * SNAKE_GRID + 20;
  if (erase) {
    tft.fillRect(x, y, SNAKE_GRID, SNAKE_GRID, COL_BG);
    if(currentGame == GAME_SNAKE_BARRIER) {
      for(int i = 0; i < barrierCount; i++) {
        int bx = barriers[i].x * SNAKE_GRID;
        int by = barriers[i].y * SNAKE_GRID + 20;
        if(abs(bx - x) < SNAKE_GRID && abs(by - y) < SNAKE_GRID) {
          tft.fillRect(bx, by, SNAKE_GRID, SNAKE_GRID, COL_BARRIER);
          tft.drawRect(bx, by, SNAKE_GRID, SNAKE_GRID, COL_SCORE);
        }
      }
    }
  } else {
    uint16_t color = (index == 0) ? COL_SNAKE_HEAD : COL_SNAKE_BODY;
    tft.fillRoundRect(x, y, SNAKE_GRID - 1, SNAKE_GRID - 1, 3, color);
    if (index == 0) {
      tft.fillCircle(x + 3, y + 3, 2, COL_BG);
      tft.fillCircle(x + SNAKE_GRID - 4, y + 3, 2, COL_BG);
      tft.fillCircle(x + 3, y + 3, 1, COL_SCORE);
      tft.fillCircle(x + SNAKE_GRID - 4, y + 3, 1, COL_SCORE);
    }
  }
}

void snakeDrawFood(bool erase) {
  int fx = food.x * SNAKE_GRID + SNAKE_GRID/2;
  int fy = food.y * SNAKE_GRID + 20 + SNAKE_GRID/2;
  if(erase) {
    tft.fillCircle(fx, fy, 8, COL_BG);
  } else {
    tft.fillCircle(fx, fy, 7, COL_FOOD);
    tft.fillCircle(fx, fy, 4, COL_SCORE);
    tft.fillCircle(fx, fy, 2, 0x001F);
  }
}

void snakeDrawBonusFood(bool erase) {
  if (!bonusActive && !erase) return;
  int bx = bonusFood.x * SNAKE_GRID + SNAKE_GRID/2;
  int by = bonusFood.y * SNAKE_GRID + 20 + SNAKE_GRID/2;
  if (erase) {
    tft.fillCircle(bx, by, 15, COL_BG); 
  } else {
    int rad = 8 + (pulseState % 4); 
    tft.fillCircle(bx, by, 12, COL_BG);
    tft.fillCircle(bx, by, rad, COL_BONUS);
    tft.fillCircle(bx, by, 5, COL_GOLD);
    for(int i = 0; i < 8; i++) {
      float angle = i * 45 * 3.14159 / 180;
      int x = bx + cos(angle) * 12;
      int y = by + sin(angle) * 12;
      tft.fillCircle(x, y, 2, COL_GOLD);
    }
  }
}

void snakeShowGameOver() {
  if (score > highScoreSnake) {
    highScoreSnake = score;
    menuHighScores[0] = highScoreSnake;
  }
  tft.fillScreen(COL_BG);
  tft.drawRect(5, 5, tft.width()-10, tft.height()-10, COL_GOLD);
  tft.drawRect(8, 8, tft.width()-16, tft.height()-16, COL_GOLD);
  tft.setTextSize(2);
  tft.setTextColor(COL_SNAKE_HEAD);
  tft.setCursor(tft.width()/2 - 60, tft.height()/2 - 60);
  tft.print("GAME OVER");
  tft.drawFastHLine(30, tft.height()/2 - 25, tft.width()-60, COL_SCORE);
  tft.fillRoundRect(20, tft.height()/2 - 10, tft.width()-40, 80, 10, 0x1082);
  tft.setTextSize(1);
  tft.setTextColor(COL_SCORE);
  tft.setCursor(30, tft.height()/2 + 10);
  tft.print("YOUR SCORE : ");
  tft.setTextColor(COL_GOLD);
  tft.print(score);
  tft.setTextColor(COL_SCORE);
  tft.setCursor(30, tft.height()/2 + 30);
  tft.print("HIGH SCORE : ");
  tft.setTextColor(COL_FOOD);
  tft.print(highScoreSnake);
  tft.setTextSize(1);
  tft.setTextColor(COL_SCORE);
  tft.setCursor(tft.width()/2 - 70, tft.height() - 30);
  tft.print("PRESS SELECT TO PLAY AGAIN");
  tft.setTextSize(1);
}

void snakeResetGame() {
  tft.fillScreen(COL_BG);
  snakeLen = 5; score = 0; foodCounter = 0;
  moveDelay = 150; bonusActive = false; isPausedSnake = false;
  gameOverSnake = false; pulseState = 0;
  
  int startX = maxGridX ;
  int startY = maxGridY/2 ;
  
  // Barrier mode এ snake start করার সময় barrier চেক করুন
  if(currentGame == GAME_SNAKE_BARRIER) {
    while(checkCollisionWithBarriers(startX, startY)) {
      startX = random(2, maxGridX - 2);
      startY = random(2, maxGridY - 2);
    }
  }
  
  for (int i = 0; i < snakeLen; i++) {
    snake[i] = {startX - i, startY};
    snakeDrawPart(i, false);
  }
  dirX = 1; dirY = 0;
  
  if(currentGame == GAME_SNAKE_BARRIER) {
    generateBarriers();
    drawBarriers();
  }
  
  snakeSpawnFood();
  snakeUpdateScoreDisplay();
  lastMoveTime = millis();
  lastPulseTime = millis();
}

// =================== BARRIER FUNCTIONS ====================
void generateBarriers() {
  barrierCount = 0;
  
  // Create a maze-like barrier pattern
  int centerX = maxGridX/2;
  int centerY = maxGridY/2;
  
  // Plus shape barrier
  for(int i = -3; i <= 3; i++) {
    if(i != 0 && barrierCount < MAX_BARRIERS) {
      barriers[barrierCount++] = {centerX + i, centerY};
      barriers[barrierCount++] = {centerX, centerY + i};
    }
  }
  
  // Corner square barrier
  int startX = maxGridX - 8;
  int startY = maxGridY - 8;
  for(int i = 0; i < 5 && barrierCount < MAX_BARRIERS; i++) {
    for(int j = 0; j < 5 && barrierCount < MAX_BARRIERS; j++) {
      if((i == 0 || i == 4 || j == 0 || j == 4)) {
        barriers[barrierCount++] = {startX + i, startY + j};
      }
    }
  }
  
  // Small L-shape barrier at top-left
  for(int i = 0; i < 5 && barrierCount < MAX_BARRIERS; i++) {
    barriers[barrierCount++] = {2, 3 + i};
    barriers[barrierCount++] = {2 + i, 2};
  }
}

void drawBarriers() {
  for(int i = 0; i < barrierCount; i++) {
    int x = barriers[i].x * SNAKE_GRID;
    int y = barriers[i].y * SNAKE_GRID + 20;
    tft.fillRect(x, y, SNAKE_GRID, SNAKE_GRID, COL_BARRIER);
    tft.drawRect(x, y, SNAKE_GRID, SNAKE_GRID, COL_SCORE);
  }
}

bool checkCollisionWithBarriers(int x, int y) {
  for(int i = 0; i < barrierCount; i++) {
    if(barriers[i].x == x && barriers[i].y == y) return true;
  }
  return false;
}

// =================== SNAKE GAME LOOP ====================
void runSnakeGame() {
  static unsigned long lastSelectPress = 0, lastBPress = 0;
  
  if(digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) {
    lastBPress = millis();
    showSnakeSubmenu();
    return;
  }
  
  if(digitalRead(BTN_SELECT) == LOW && millis() - lastSelectPress > 300) {
    lastSelectPress = millis();
    if(gameOverSnake) {
      snakeResetGame();
    } else {
      isPausedSnake = !isPausedSnake;
      snakeShowPauseScreen(isPausedSnake);
    }
  }
  
  if(!gameOverSnake && !isPausedSnake) {
    if(digitalRead(BTN_UP) == LOW && dirY == 0) { dirX = 0; dirY = -1; }
    else if(digitalRead(BTN_DOWN) == LOW && dirY == 0) { dirX = 0; dirY = 1; }
    else if(digitalRead(BTN_LEFT) == LOW && dirX == 0) { dirX = -1; dirY = 0; }
    else if(digitalRead(BTN_RIGHT) == LOW && dirX == 0) { dirX = 1; dirY = 0; }
    
    if(millis() - lastPulseTime > 150) {
      lastPulseTime = millis();
      pulseState++;
      if(bonusActive) {
        if(millis() - bonusStartTime > 4000) {
          snakeDrawBonusFood(true);
          bonusActive = false;
        } else {
          snakeDrawBonusFood(false);
        }
      }
      snakeDrawFood(false);
      snakeUpdateScoreDisplay();
    }
    
    if(millis() - lastMoveTime > moveDelay) {
      lastMoveTime = millis();
      int nextX = snake[0].x + dirX;
      int nextY = snake[0].y + dirY;
      
      // BARRIER MODE: শুধু WRAP-AROUND এবং ভিতরের barriers চেক করবে (বাইরের boundary থাকবে না)
      if(currentGame == GAME_SNAKE_BARRIER) {
        // WRAP-AROUND for all boundaries
        if (nextX < 0) nextX = maxGridX - 1;
        else if (nextX >= maxGridX) nextX = 0;
        if (nextY < 0) nextY = maxGridY - 1;
        else if (nextY >= maxGridY) nextY = 0;
        
        // শুধু ভিতরের barriers চেক করবে
        if(checkCollisionWithBarriers(nextX, nextY)) {
          gameOverSnake = true;
          snakeShowGameOver();
          return;
        }
      }
      // CLASSIC এবং NOBORDER মোডের জন্য (যেমন আগে ছিল)
      else if(currentGame == GAME_SNAKE_CLASSIC || currentGame == GAME_SNAKE_NOBORDER) {
        if (nextX < 0) nextX = maxGridX - 1;
        else if (nextX >= maxGridX) nextX = 0;
        if (nextY < 0) nextY = maxGridY - 1;
        else if (nextY >= maxGridY) nextY = 0;
      }
      
      Point nextHead = {nextX, nextY};
      for (int i = 0; i < snakeLen; i++) {
        if (nextHead.x == snake[i].x && nextHead.y == snake[i].y) {
          gameOverSnake = true;
          snakeShowGameOver();
          return;
        }
      }
      
      bool ateAnything = false;
      if (nextHead.x == food.x && nextHead.y == food.y) {
        score += 10; snakeLen++; foodCounter++; ateAnything = true;
        if (moveDelay > 50) moveDelay -= 2;
        snakeSpawnFood();
        if (foodCounter >= 5 && !bonusActive) { snakeSpawnBonusFood(); foodCounter = 0; }
      }
      else if (bonusActive && nextHead.x == bonusFood.x && nextHead.y == bonusFood.y) {
        unsigned long timeTaken = (millis() - bonusStartTime) / 1000;
        int dynamicBonus = 80 - (timeTaken * 10);
        if (dynamicBonus < 10) dynamicBonus = 10;
        score += dynamicBonus;
        snakeLen++; bonusActive = false; ateAnything = true;
        snakeDrawBonusFood(true);
      }
      
      if (!ateAnything) snakeDrawPart(snakeLen - 1, true);
      for (int i = snakeLen - 1; i > 0; i--) snake[i] = snake[i - 1];
      snake[0] = nextHead;
      for (int i = (ateAnything ? 0 : 1); i < snakeLen; i++) snakeDrawPart(i, false);
      if(!ateAnything) snakeDrawPart(0, false);
      snakeUpdateScoreDisplay();
    }
  }
}
// =================== SNAKE SUBMENU ====================
void showSnakeSubmenu() {
  currentGame = GAME_SNAKE_SUBMENU;
  snakeSubSelected = 0;
  
  tft.fillScreen(0x0000);
  tft.fillRect(0, 0, 320, 36, 0x0008);
  tft.drawFastHLine(0, 36, 320, 0xFFE0);
  tft.setTextSize(2); tft.setTextColor(0xFFE0);
  tft.setCursor(10, 10); tft.print("SNAKE GAME");
  tft.setTextSize(1); tft.setTextColor(0x2945);
  tft.setCursor(10, 28); tft.print("Choose your mode");
  tft.drawFastHLine(0, 224, 320, 0x1082);
  tft.fillRect(0, 225, 320, 15, 0x0008);
  tft.setTextSize(1); tft.setTextColor(0x3186);
  tft.setCursor(8, 228); tft.print("[B] Back");
  tft.setTextColor(0x07E0); tft.setCursor(110, 228); tft.print("[A/SELECT]");
  tft.setTextColor(0x2945); tft.setCursor(195, 228); tft.print("Launch");
  
  const char* mNames[] = {"CLASSIC MODE", "BARRIER MODE", "NO BORDER MODE"};
  const char* mSubtitles[] = {"Wrap-around edges", "Avoid the walls!", "Classic no borders"};
  const uint16_t mAccent[] = {0x07E0, 0xF800, 0x07FF};
  const uint16_t mDim[] = {0x0140, 0x2000, 0x020A};
  
  for (int i = 0; i < 3; i++) {
    bool sel = (i == snakeSubSelected);
    int cy = 50 + i * 58;
    if (sel) {
      tft.fillRoundRect(8, cy, 304, 52, 6, mDim[i]);
      tft.fillRoundRect(8, cy, 5, 52, 3, mAccent[i]);
      tft.drawRoundRect(8, cy, 304, 52, 6, mAccent[i]);
      tft.drawFastVLine(9, cy+1, 50, (uint16_t)(mAccent[i]>>1));
    } else {
      tft.fillRoundRect(8, cy, 304, 52, 6, 0x0821);
      tft.drawRoundRect(8, cy, 304, 52, 6, 0x1082);
    }
    tft.fillCircle(32, cy+26, 14, sel ? mAccent[i] : (uint16_t)0x1082);
    tft.setTextSize(2);
    tft.setTextColor(sel ? (uint16_t)0x0000 : (uint16_t)0x2945);
    tft.setCursor(26, cy+18); tft.print(i+1);
    tft.setTextSize(1);
    tft.setTextColor(sel ? mAccent[i] : (uint16_t)0x528A);
    tft.setCursor(56, cy+12); tft.print(mNames[i]);
    tft.setTextColor(sel ? (uint16_t)0x3186 : (uint16_t)0x2104);
    tft.setCursor(56, cy+26); tft.print(mSubtitles[i]);
    if (sel) tft.fillTriangle(298, cy+20, 298, cy+32, 304, cy+26, mAccent[i]);
  }
}

void updateSnakeSubmenu(int oldSel, int newSel) {
  const uint16_t mAccent[] = {0x07E0, 0xF800, 0x07FF};
  const uint16_t mDim[] = {0x0140, 0x2000, 0x020A};
  const char* mNames[] = {"CLASSIC MODE", "BARRIER MODE", "NO BORDER MODE"};
  const char* mSubtitles[] = {"Wrap-around edges", "Avoid the walls!", "Classic no borders"};
  
  for(int idx = 0; idx < 3; idx++) {
    if(idx != oldSel && idx != newSel) continue;
    bool sel = (idx == newSel);
    int cy = 50 + idx * 58;
    if (sel) {
      tft.fillRoundRect(8, cy, 304, 52, 6, mDim[idx]);
      tft.fillRoundRect(8, cy, 5, 52, 3, mAccent[idx]);
      tft.drawRoundRect(8, cy, 304, 52, 6, mAccent[idx]);
      tft.drawFastVLine(9, cy+1, 50, (uint16_t)(mAccent[idx]>>1));
    } else {
      tft.fillRoundRect(8, cy, 304, 52, 6, 0x0821);
      tft.drawRoundRect(8, cy, 304, 52, 6, 0x1082);
    }
    tft.fillCircle(32, cy+26, 14, sel ? mAccent[idx] : (uint16_t)0x1082);
    tft.setTextSize(2);
    tft.setTextColor(sel ? (uint16_t)0x0000 : (uint16_t)0x2945);
    tft.setCursor(26, cy+18); tft.print(idx+1);
    tft.setTextSize(1);
    tft.setTextColor(sel ? mAccent[idx] : (uint16_t)0x528A);
    tft.setCursor(56, cy+12); tft.print(mNames[idx]);
    tft.setTextColor(sel ? (uint16_t)0x3186 : (uint16_t)0x2104);
    tft.setCursor(56, cy+26); tft.print(mSubtitles[idx]);
    if (sel) tft.fillTriangle(298, cy+20, 298, cy+32, 304, cy+26, mAccent[idx]);
    else tft.fillRect(296, cy+18, 10, 16, 0x0821);
  }
}

void launchSnakeMode(int mode) {
  // Calculate grid dimensions
  maxGridX = tft.width() / SNAKE_GRID;
  maxGridY = (tft.height() - 20) / SNAKE_GRID;
  
  if(mode == 0) {
    currentGame = GAME_SNAKE_CLASSIC;
  } else if(mode == 1) {
    currentGame = GAME_SNAKE_BARRIER;
  } else {
    currentGame = GAME_SNAKE_NOBORDER;
  }
  snakeResetGame();
}

// ================================================================
// =================== FLAPPY VARIABLES ==========================
// ================================================================
#define MAX_DOTS 30
struct Dot { float x, y, speed; int brightness; };
Dot bgDots[MAX_DOTS];

struct Pipe { float x; int gapY, gapH; bool counted; };
Pipe pipes[4];

const int   PIPE_W          = 28;
const int   CAP_H           = 8;
const float SCROLL_SPEED    = 3.5;
const int   MIN_PIPE_DIST   = 120;
const int   MAX_PIPE_DIST   = 250;

float birdY = 120, prevBirdY = 120, velocity = 0;
const float gravity = 0.18, flapPower = -2.5;
int scoreFlappy = 0, highScoreFlappy = 0;
bool playing = false, isPausedFlappy = false;
bool isCountdown = false;
unsigned long countdownStartTime = 0;
int countdownValue = 3;

#define BG_COLOR          TFT_BLACK
#define PIPE_COLOR        TFT_GREEN
#define PIPE_CAP_COLOR    TFT_DARKGREEN
#define BIRD_BODY_COLOR   TFT_YELLOW
#define BIRD_DETAIL_COLOR TFT_RED

const unsigned char bird_body_bmp[] PROGMEM = {
  0x00,0x7e,0x00,0x01,0xff,0x80,0x03,0xff,0xc0,0x07,0xff,0xe0,0x0f,0xff,0xf0,
  0x1f,0xff,0xf8,0x3f,0xff,0xfc,0x7f,0xff,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,
  0x7f,0xff,0xfe,0x7f,0xff,0xfe,0x3f,0xff,0xfc,0x1f,0xff,0xf8,0x0f,0xff,0xf0,
  0x07,0xff,0xe0,0x01,0xff,0x80,0x00,0x7e,0x00
};
const unsigned char bird_details_bmp[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,0xf8,0x00,
  0x01,0xf8,0x00,0x01,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0x7e,0x00,0x01,0xff,
  0x00,0x00,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// ================================================================
// =================== STANDALONE FLAPPY =========================
// ================================================================
#define DRAW_LOOP_INTERVAL 50
#define MAX_FALL_RATE 15
#define FLY_FORCE -6
#define FGRAVITY 1
#define FLAPPY_GROUND_Y 220

int  sa_currentWing;
int  sa_flX, sa_flY, sa_fallRate;
int  sa_pillarPos, sa_gapPosition, sa_gapHeight;
int  sa_score;
int  sa_highScore = 0;
bool sa_running   = false;
bool sa_crashed   = false;
bool sa_paused    = false;
unsigned long sa_lastDebounce  = 0;
unsigned long sa_lastPausePress = 0;
unsigned long sa_lastFlyTime   = 0;
const unsigned long SA_FLY_COOLDOWN = 150;
long sa_nextDrawTime;
int  sa_currentpcolour;

int getRandomGapHeightSA() {
  int gaps[] = {65, 70, 75, 80, 85, 90};
  return gaps[random(0, 6)];
}

void sa_drawFlappy(int x, int y) {
  tft.fillRect(x+2,  y+8,  2, 10, TFT_BLACK);
  tft.fillRect(x+4,  y+6,  2, 2,  TFT_BLACK);
  tft.fillRect(x+6,  y+4,  2, 2,  TFT_BLACK);
  tft.fillRect(x+8,  y+2,  4, 2,  TFT_BLACK);
  tft.fillRect(x+12, y,   12, 2,  TFT_BLACK);
  tft.fillRect(x+24, y+2,  2, 2,  TFT_BLACK);
  tft.fillRect(x+26, y+4,  2, 2,  TFT_BLACK);
  tft.fillRect(x+28, y+6,  2, 6,  TFT_BLACK);
  tft.fillRect(x+10, y+22,10, 2,  TFT_BLACK);
  tft.fillRect(x+4,  y+18, 2, 2,  TFT_BLACK);
  tft.fillRect(x+6,  y+20, 4, 2,  TFT_BLACK);
  tft.fillRect(x+12, y+2,  6, 2,  TFT_YELLOW);
  tft.fillRect(x+8,  y+4,  8, 2,  TFT_YELLOW);
  tft.fillRect(x+6,  y+6, 10, 2,  TFT_YELLOW);
  tft.fillRect(x+4,  y+8, 12, 2,  TFT_YELLOW);
  tft.fillRect(x+4,  y+10,14, 2,  TFT_YELLOW);
  tft.fillRect(x+4,  y+12,16, 2,  TFT_YELLOW);
  tft.fillRect(x+4,  y+14,14, 2,  TFT_YELLOW);
  tft.fillRect(x+4,  y+16,12, 2,  TFT_YELLOW);
  tft.fillRect(x+6,  y+18,12, 2,  TFT_YELLOW);
  tft.fillRect(x+10, y+20,10, 2,  TFT_YELLOW);
  tft.fillRect(x+18, y+2,  2, 2,  TFT_BLACK);
  tft.fillRect(x+16, y+4,  2, 6,  TFT_BLACK);
  tft.fillRect(x+18, y+10, 2, 2,  TFT_BLACK);
  tft.fillRect(x+18, y+4,  2, 6,  TFT_WHITE);
  tft.fillRect(x+20, y+2,  4, 10, TFT_WHITE);
  tft.fillRect(x+24, y+4,  2, 8,  TFT_WHITE);
  tft.fillRect(x+26, y+6,  2, 6,  TFT_WHITE);
  tft.fillRect(x+24, y+6,  2, 4,  TFT_BLACK);
  tft.fillRect(x+20, y+12,12, 2,  TFT_BLACK);
  tft.fillRect(x+18, y+14, 2, 2,  TFT_BLACK);
  tft.fillRect(x+20, y+14,12, 2,  TFT_RED);
  tft.fillRect(x+32, y+14, 2, 2,  TFT_BLACK);
  tft.fillRect(x+16, y+16, 2, 2,  TFT_BLACK);
  tft.fillRect(x+18, y+16, 2, 2,  TFT_RED);
  tft.fillRect(x+20, y+16,12, 2,  TFT_BLACK);
  tft.fillRect(x+18, y+18, 2, 2,  TFT_BLACK);
  tft.fillRect(x+20, y+18,10, 2,  TFT_RED);
  tft.fillRect(x+30, y+18, 2, 2,  TFT_BLACK);
  tft.fillRect(x+20, y+20,10, 2,  TFT_BLACK);
}
void sa_clearFlappy(int x, int y, uint16_t bgCol) {
  tft.fillRect(x, y, 34, 24, bgCol);
}
void sa_drawWing1(int x, int y) {
  tft.fillRect(x,    y+14, 2, 6,  TFT_BLACK);
  tft.fillRect(x+2,  y+20, 8, 2,  TFT_BLACK);
  tft.fillRect(x+2,  y+12,10, 2,  TFT_BLACK);
  tft.fillRect(x+12, y+14, 2, 2,  TFT_BLACK);
  tft.fillRect(x+10, y+16, 2, 2,  TFT_BLACK);
  tft.fillRect(x+2,  y+14, 8, 6,  TFT_WHITE);
  tft.fillRect(x+8,  y+18, 2, 2,  TFT_BLACK);
  tft.fillRect(x+10, y+14, 2, 2,  TFT_WHITE);
}
void sa_drawWing2(int x, int y) {
  tft.fillRect(x+2,  y+10,10, 2,  TFT_BLACK);
  tft.fillRect(x+2,  y+16,10, 2,  TFT_BLACK);
  tft.fillRect(x,    y+12, 2, 4,  TFT_BLACK);
  tft.fillRect(x+12, y+12, 2, 4,  TFT_BLACK);
  tft.fillRect(x+2,  y+12,10, 4,  TFT_WHITE);
}
void sa_drawWing3(int x, int y) {
  tft.fillRect(x+2,  y+6,  8, 2,  TFT_BLACK);
  tft.fillRect(x,    y+8,  2, 6,  TFT_BLACK);
  tft.fillRect(x+10, y+8,  2, 2,  TFT_BLACK);
  tft.fillRect(x+12, y+10, 2, 4,  TFT_BLACK);
  tft.fillRect(x+10, y+14, 2, 2,  TFT_BLACK);
  tft.fillRect(x+2,  y+14, 2, 2,  TFT_BLACK);
  tft.fillRect(x+4,  y+16, 6, 2,  TFT_BLACK);
  tft.fillRect(x+2,  y+8,  8, 6,  TFT_WHITE);
  tft.fillRect(x+4,  y+14, 6, 2,  TFT_WHITE);
  tft.fillRect(x+10, y+10, 2, 4,  TFT_WHITE);
}

void sa_drawPillar(int x, int gap, int gapH, uint16_t pCol, uint16_t bgCol) {
  if (gap > 0) {
    tft.fillRect(x+2, 2, 46, gap-4, pCol);
    tft.drawRect(x,   0, 50, gap,   TFT_BLACK);
    tft.drawRect(x+1, 1, 48, gap-2, TFT_BLACK);
  }
  int bStart = gap + gapH;
  int bH     = FLAPPY_GROUND_Y - bStart;
  if (bH > 0) {
    tft.fillRect(x+2, bStart+2, 46, bH-4, pCol);
    tft.drawRect(x,   bStart,   50, bH,   TFT_BLACK);
    tft.drawRect(x+1, bStart+1, 48, bH-2, TFT_BLACK);
  }
}
void sa_clearPillar(int x, int gap, int gapH, uint16_t bgCol) {
  if (gap > 0)
    tft.fillRect(x+45, 0, 5, gap, bgCol);
  int bStart = gap + gapH;
  int bH     = FLAPPY_GROUND_Y - bStart;
  if (bH > 0)
    tft.fillRect(x+45, bStart, 5, bH, bgCol);
}

void sa_drawGround(uint16_t bgCol, uint16_t g1, uint16_t g2) {
  int ty = FLAPPY_GROUND_Y;
  for (int tx = 0; tx <= 320; tx += 20) {
    tft.fillTriangle(tx,    ty,    tx+10, ty,    tx,    ty+10, g1);
    tft.fillTriangle(tx+10, ty+10, tx+10, ty,    tx,    ty+10, g2);
    tft.fillTriangle(tx+10, ty,    tx+20, ty,    tx+10, ty+10, g2);
    tft.fillTriangle(tx+20, ty+10, tx+20, ty,    tx+10, ty+10, g1);
  }
  tft.fillRect(0, FLAPPY_GROUND_Y + 8, 320, 12, TFT_GREEN);
}

void sa_startGame(int version) {
  sa_flX       = 50;
  sa_flY       = 100;
  sa_fallRate  = 0;
  sa_pillarPos    = 250;
  sa_gapPosition  = 40 + random(0, 80);
  sa_gapHeight    = getRandomGapHeightSA();
  sa_crashed   = false;
  sa_running   = false;
  sa_paused    = false;
  sa_score     = 0;
  sa_currentWing = 0;
  sa_lastFlyTime = 0;
  randomSeed(millis());

  uint16_t bgCol = (version == 0) ? (uint16_t)0x001F : (uint16_t)0x0000;
  uint16_t g1    = (version == 0) ? (uint16_t)TFT_GREEN  : (uint16_t)TFT_WHITE;
  uint16_t g2    = (version == 0) ? (uint16_t)TFT_YELLOW : (uint16_t)TFT_BLACK;
  sa_currentpcolour = (version == 0) ? (int)TFT_GREEN : (int)TFT_WHITE;

  tft.fillScreen(bgCol);
  sa_drawGround(bgCol, g1, g2);
  sa_nextDrawTime = millis() + DRAW_LOOP_INTERVAL;
}

void sa_drawPauseOverlay(uint16_t bgCol) {
  tft.fillRoundRect(60, 80, 200, 80, 10, 0x18C3);
  tft.drawRoundRect(60, 80, 200, 80, 10, TFT_WHITE);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(4);
  tft.setCursor(85, 95); tft.print("PAUSED");
  tft.setTextSize(1); tft.setTextColor(TFT_YELLOW);
  tft.setCursor(72, 145); tft.print("Press START btn to resume");
}
void sa_removePauseOverlay(int version) {
  uint16_t bgCol = (version == 0) ? (uint16_t)0x001F : (uint16_t)0x0000;
  tft.fillRoundRect(60, 80, 200, 80, 10, bgCol);
  sa_drawPillar(sa_pillarPos, sa_gapPosition, sa_gapHeight, sa_currentpcolour, bgCol);
  sa_drawFlappy(sa_flX, sa_flY);
}

void sa_drawLoop(int version) {
  uint16_t bgCol = (version == 0) ? (uint16_t)0x001F : (uint16_t)0x0000;
  sa_clearPillar(sa_pillarPos, sa_gapPosition, sa_gapHeight, bgCol);
  sa_clearFlappy(sa_flX, sa_flY, bgCol);

  if (sa_running) {
    sa_fallRate += FGRAVITY;
    if (sa_fallRate > MAX_FALL_RATE) sa_fallRate = MAX_FALL_RATE;
    sa_flY += sa_fallRate;
    if (sa_flY < 0) { sa_flY = 0; sa_fallRate = 0; }
    sa_pillarPos -= 5;
    if (sa_pillarPos <= 0 && sa_pillarPos > -5) sa_score += 5;
    else if (sa_pillarPos < -50) {
      sa_pillarPos   = 250 + random(0, 7)*10;
      sa_gapPosition = 20 + random(0, 90);
      sa_gapHeight   = getRandomGapHeightSA();
    }
  }

  sa_drawPillar(sa_pillarPos, sa_gapPosition, sa_gapHeight, sa_currentpcolour, bgCol);
  sa_drawFlappy(sa_flX, sa_flY);

  switch (sa_currentWing) {
    case 0: case 1: sa_drawWing1(sa_flX, sa_flY); break;
    case 2: case 3: sa_drawWing2(sa_flX, sa_flY); break;
    case 4: case 5: sa_drawWing3(sa_flX, sa_flY); break;
  }
  sa_currentWing++;
  if (sa_currentWing == 6) sa_currentWing = 0;

  uint16_t scoreBoxCol = (version == 0) ? (uint16_t)0x001F : (uint16_t)0x0000;
  uint16_t scoreTxtCol = (version == 0) ? (uint16_t)TFT_WHITE : (uint16_t)TFT_WHITE;
  tft.fillRect(240, 5, 70, 30, scoreBoxCol);
  tft.setTextColor(scoreTxtCol); tft.setTextSize(2);
  tft.setCursor(245, 10); tft.print("S:");
  tft.setCursor(265, 10); tft.print(sa_score);
}

void sa_checkCollision(int version) {
  if (sa_flY <= 0)             sa_crashed = true;
  if (sa_flY + 24 >= FLAPPY_GROUND_Y) sa_crashed = true;
  if (sa_flX + 34 > sa_pillarPos && sa_flX < sa_pillarPos + 50)
    if (sa_flY < sa_gapPosition || sa_flY + 24 > sa_gapPosition + sa_gapHeight)
      sa_crashed = true;

  if (sa_crashed && sa_running) {
    sa_running = false; sa_paused = false;
    if (sa_score > sa_highScore) sa_highScore = sa_score;
    if (sa_score > menuHighScores[1]) menuHighScores[1] = sa_score;
    tft.setTextColor(TFT_RED);   tft.setTextSize(5); tft.setCursor(20, 75);  tft.print("Game Over!");
    tft.setTextColor(TFT_WHITE); tft.setTextSize(4); tft.setCursor(50, 125); tft.print("Score:");
    tft.setCursor(220, 125); tft.setTextSize(5); tft.print(sa_score);
    if (sa_score >= sa_highScore && sa_score > 0) {
      tft.setCursor(50, 175); tft.setTextSize(3); tft.setTextColor(TFT_YELLOW);
      tft.print("NEW HIGH!");
    }
    delay(500);
  }
}

// ================================================================
// =================== FLAPPY BIRD (Night 1) ======================
// ================================================================
void initDots(){for(int i=0;i<MAX_DOTS;i++) bgDots[i]={(float)random(0,screenWidth),(float)random(0,screenHeight),random(3,12)/10.0f,random(40,180)};}
void updateDots(){for(int i=0;i<MAX_DOTS;i++){tft.drawPixel((int)bgDots[i].x,(int)bgDots[i].y,BG_COLOR);bgDots[i].x-=bgDots[i].speed;if(bgDots[i].x<0)bgDots[i]={(float)screenWidth,(float)random(0,screenHeight),random(3,12)/10.0f,random(40,180)};tft.drawPixel((int)bgDots[i].x,(int)bgDots[i].y,tft.color565(bgDots[i].brightness,bgDots[i].brightness,bgDots[i].brightness));}}
void startCountdown(){isCountdown=true;countdownValue=3;countdownStartTime=millis();}
void updateCountdown(){if(!isCountdown)return;int cx=screenWidth/2,cy=screenHeight/2;unsigned long e=millis()-countdownStartTime;int nv=4-(e/1000);if(nv!=countdownValue&&nv>=0){countdownValue=nv;tft.fillRect(cx-40,cy-30,40,30,BG_COLOR);tft.setTextColor(TFT_YELLOW);tft.setTextSize(4);if(countdownValue>0){tft.setCursor(cx-12,cy-20);tft.print(countdownValue);}}if(e>=3000){tft.fillRect(cx-40,cy-30,80,60,BG_COLOR);isCountdown=false;isPausedFlappy=false;}}
int getRandomPipeDist(){return random(MIN_PIPE_DIST,MAX_PIPE_DIST+1);}
void createPipe(int idx,float sx){pipes[idx]={sx,random(50,screenHeight-130),random(70,110),false};}
void startFlappyGame(){tft.fillScreen(BG_COLOR);initDots();scoreFlappy=0;isPausedFlappy=false;isCountdown=false;birdY=screenHeight/2;velocity=0;float cx=screenWidth+50;for(int i=0;i<4;i++){createPipe(i,cx);cx+=getRandomPipeDist();}playing=true;}
void updateFlappyScreen(){updateDots();tft.fillRect(60,(int)prevBirdY,28,22,BG_COLOR);for(int i=0;i<4;i++){int ope=pipes[i].x+PIPE_W+5;if(ope>0&&ope<screenWidth)tft.fillRect(ope,0,10,screenHeight,BG_COLOR);if(pipes[i].x<screenWidth&&pipes[i].x+PIPE_W>0){tft.fillRect(pipes[i].x,0,PIPE_W,pipes[i].gapY,PIPE_COLOR);tft.fillRect(pipes[i].x-3,pipes[i].gapY-CAP_H,PIPE_W+6,CAP_H,PIPE_CAP_COLOR);int by=pipes[i].gapY+pipes[i].gapH;tft.fillRect(pipes[i].x,by,PIPE_W,screenHeight-by,PIPE_COLOR);tft.fillRect(pipes[i].x-3,by,PIPE_W+6,CAP_H,PIPE_CAP_COLOR);}}tft.drawBitmap(60,(int)birdY,bird_body_bmp,24,18,BIRD_BODY_COLOR);tft.drawBitmap(60,(int)birdY,bird_details_bmp,24,18,BIRD_DETAIL_COLOR);tft.setTextColor(TFT_RED);tft.setTextSize(2);tft.setCursor(screenWidth-75,8);tft.print(scoreFlappy);}
void showStartScreenFlappy(){tft.fillScreen(BG_COLOR);for(int i=0;i<MAX_DOTS;i++){uint16_t dc=tft.color565(bgDots[i].brightness,bgDots[i].brightness,bgDots[i].brightness);tft.drawPixel((int)bgDots[i].x,(int)bgDots[i].y,dc);}tft.fillRoundRect(40,25,240,70,15,PIPE_COLOR);tft.setTextColor(BG_COLOR);tft.setTextSize(2);tft.setCursor(60,40);tft.print("FLAPPY BIRD");tft.setCursor(55,62);tft.print("--- NIGHT 1 ---");tft.fillRoundRect(30,110,260,115,10,PIPE_CAP_COLOR);tft.setTextColor(PIPE_COLOR);tft.setTextSize(1);tft.setCursor(55,125);tft.print("SELECT:Flap  START:Pause  B:Menu");tft.setCursor(55,140);tft.print("Avoid pipes to score!");tft.setTextColor(PIPE_COLOR);tft.setCursor(40,160);tft.print("High Score: ");tft.print(highScoreFlappy);}
void flappyGameOver(){playing=false;isPausedFlappy=false;isCountdown=false;if(scoreFlappy>highScoreFlappy){highScoreFlappy=scoreFlappy;menuHighScores[1]=highScoreFlappy;}int cx=screenWidth/2;for(int i=0;i<3;i++){tft.fillRoundRect(cx-100,45,200,130,15,PIPE_COLOR);delay(100);tft.fillRoundRect(cx-100,45,200,130,15,PIPE_CAP_COLOR);delay(100);}tft.fillRoundRect(cx-100,45,200,130,15,PIPE_COLOR);tft.setTextColor(BG_COLOR);tft.setTextSize(2);tft.setCursor(cx-88,68);tft.print("GAME OVER");tft.fillRect(cx-70,95,140,50,BG_COLOR);tft.setTextColor(PIPE_COLOR);tft.setTextSize(2);tft.setCursor(cx-60,105);tft.print("SCORE: ");tft.print(scoreFlappy);tft.setTextSize(1);tft.setCursor(cx-45,130);tft.print("BEST: ");tft.print(highScoreFlappy);tft.setTextColor(BG_COLOR);tft.setCursor(cx-75,162);tft.print("PRESS SELECT BUTTON");delay(500);while(digitalRead(BTN_SELECT)==HIGH)delay(50);delay(200);showFlappySubmenu();}
void runFlappyNight1(){static unsigned long lastStartPress=0,lastSelectPress=0,lastBPress=0;if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){lastBPress=millis();playing=false;showFlappySubmenu();return;}if(!playing){updateDots();static int blinkCtr=0;if(millis()-lastStartPress>500){lastStartPress=millis();blinkCtr++;tft.fillRect(40,175,220,20,BG_COLOR);tft.setTextSize(1);tft.setTextColor(PIPE_COLOR);tft.setCursor(40,180);tft.print(blinkCtr%2==0?"> Press SELECT to Start <":"  Press SELECT to Start  ");}delay(30);if(digitalRead(BTN_SELECT)==LOW){delay(100);startFlappyGame();}return;}if(!isPausedFlappy&&!isCountdown&&digitalRead(BTN_START)==LOW&&millis()-lastSelectPress>200){lastSelectPress=millis();isPausedFlappy=true;tft.setTextColor(TFT_WHITE);tft.setTextSize(3);tft.setCursor(screenWidth/2-55,screenHeight/2-25);tft.print("PAUSED!");delay(200);return;}if(isPausedFlappy&&!isCountdown&&digitalRead(BTN_SELECT)==LOW&&millis()-lastSelectPress>200){lastSelectPress=millis();tft.fillRect(screenWidth/2-80,screenHeight/2-40,160,50,BG_COLOR);startCountdown();delay(200);return;}if(isCountdown){updateCountdown();delay(50);return;}if(isPausedFlappy){delay(50);return;}if(digitalRead(BTN_SELECT)==LOW&&millis()-lastSelectPress>50){lastSelectPress=millis();velocity=flapPower;}velocity+=gravity;prevBirdY=birdY;birdY+=velocity;for(int i=0;i<4;i++){pipes[i].x-=SCROLL_SPEED;if(!pipes[i].counted&&pipes[i].x+PIPE_W<60){pipes[i].counted=true;scoreFlappy+=5;}if(pipes[i].x<-PIPE_W-20){float fx=-999;for(int j=0;j<4;j++)if(pipes[j].x>fx)fx=pipes[j].x;createPipe(i,fx+getRandomPipeDist());}if(pipes[i].x<60+24&&pipes[i].x+PIPE_W>60)if(birdY<pipes[i].gapY||birdY+18>pipes[i].gapY+pipes[i].gapH){flappyGameOver();return;}}if(birdY<0||birdY>screenHeight-18){flappyGameOver();return;}updateFlappyScreen();delay(12);}
void runFlappySA(int version){static unsigned long lastBPress=0;if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){lastBPress=millis();showFlappySubmenu();return;}if(sa_running&&!sa_crashed){if(digitalRead(BTN_START)==LOW&&millis()-sa_lastPausePress>300){sa_lastPausePress=millis();sa_paused=!sa_paused;uint16_t bgCol=(version==0)?(uint16_t)0x001F:(uint16_t)0x0000;if(sa_paused){sa_drawPauseOverlay(bgCol);sa_nextDrawTime=millis()+DRAW_LOOP_INTERVAL;}else{sa_removePauseOverlay(version);sa_nextDrawTime=millis()+DRAW_LOOP_INTERVAL;}}}if(digitalRead(BTN_SELECT)==LOW){if(millis()-sa_lastDebounce>50){sa_lastDebounce=millis();if(sa_crashed){sa_startGame(version);}else if(!sa_running){sa_running=true;sa_fallRate=FLY_FORCE;sa_lastFlyTime=millis();}else if(!sa_paused&&millis()-sa_lastFlyTime>SA_FLY_COOLDOWN){sa_fallRate=FLY_FORCE;sa_lastFlyTime=millis();}}}uint16_t bgCol=(version==0)?(uint16_t)0x001F:(uint16_t)0x0000;if(!sa_paused){if(millis()>sa_nextDrawTime&&!sa_crashed){sa_drawLoop(version);sa_checkCollision(version);sa_nextDrawTime+=DRAW_LOOP_INTERVAL;}if(!sa_running&&!sa_crashed){static unsigned long hintT=0;static bool hintV=true;if(millis()-hintT>500){hintT=millis();hintV=!hintV;tft.setTextSize(1);tft.setTextColor(hintV?TFT_WHITE:bgCol);tft.setCursor(60,5);tft.print("> PRESS SELECT TO FLY <");}}if(sa_crashed){static unsigned long goWait=0;if(goWait==0)goWait=millis();if(millis()-goWait>300&&digitalRead(BTN_SELECT)==LOW){goWait=0;sa_startGame(version);}}delay(10);}delay(5);}

// ================================================================
// =================== FLAPPY SUBMENU =============================
// ================================================================
void showFlappySubmenu() {
  currentGame=GAME_FLAPPY_SUBMENU; flappySubSelected=0;
  tft.fillScreen(0x0000);
  tft.fillRect(0,0,320,36,0x0008); tft.drawFastHLine(0,36,320,0xFFE0);
  tft.setTextSize(2); tft.setTextColor(0xFFE0);
  tft.setCursor(10,10); tft.print("FLAPPY BIRD");
  tft.setTextSize(1); tft.setTextColor(0x2945);
  tft.setCursor(10,28); tft.print("Choose your version");
  tft.drawFastHLine(0,224,320,0x1082);
  tft.fillRect(0,225,320,15,0x0008);
  tft.setTextSize(1); tft.setTextColor(0x3186);
  tft.setCursor(8,228); tft.print("[B] Back");
  tft.setTextColor(0x07E0); tft.setCursor(110,228); tft.print("[A/SELECT]");
  tft.setTextColor(0x2945); tft.setCursor(195,228); tft.print("Launch");
  const char* vNames[]={"Flappy Bird Day 1","Flappy Bird Night 1","Flappy Bird Night 2"};
  const char* vSubtitles[]={"Blue sky, green pipes","Dark sky, bitmap bird","Black sky, white pipes"};
  const uint16_t vAccent[]={0xFFE0,0x07FF,0x7BEF};
  const uint16_t vDim[]={0x2200,0x020A,0x1082};
  for(int i=0;i<3;i++) {
    bool sel=(i==flappySubSelected); int cy=46+i*58;
    if(sel) { tft.fillRoundRect(8,cy,304,52,6,vDim[i]); tft.fillRoundRect(8,cy,5,52,3,vAccent[i]); tft.drawRoundRect(8,cy,304,52,6,vAccent[i]); tft.drawFastVLine(9,cy+1,50,(uint16_t)(vAccent[i]>>1)); }
    else    { tft.fillRoundRect(8,cy,304,52,6,0x0821);  tft.drawRoundRect(8,cy,304,52,6,0x1082); }
    tft.fillCircle(32,cy+26,14,sel?vAccent[i]:(uint16_t)0x1082);
    tft.setTextSize(2); tft.setTextColor(sel?(uint16_t)0x0000:(uint16_t)0x2945);
    tft.setCursor(26,cy+18); tft.print(i+1);
    tft.setTextSize(1); tft.setTextColor(sel?vAccent[i]:(uint16_t)0x528A);
    tft.setCursor(56,cy+12); tft.print(vNames[i]);
    tft.setTextColor(sel?(uint16_t)0x3186:(uint16_t)0x2104);
    tft.setCursor(56,cy+26); tft.print(vSubtitles[i]);
    if(sel) tft.fillTriangle(298,cy+20,298,cy+32,304,cy+26,vAccent[i]);
  }
}

void updateFlappySubmenu(int oldSel, int newSel) {
  const uint16_t vAccent[]={0xFFE0,0x07FF,0x7BEF};
  const uint16_t vDim[]={0x2200,0x020A,0x1082};
  const char* vNames[]={"Flappy Bird Day 1","Flappy Bird Night 1","Flappy Bird Night 2"};
  const char* vSubtitles[]={"Blue sky, green pipes","Dark sky, bitmap bird","Black sky, white pipes"};
  for(int idx=0;idx<3;idx++) {
    if(idx!=oldSel&&idx!=newSel) continue;
    bool sel=(idx==newSel); int cy=46+idx*58;
    if(sel) { tft.fillRoundRect(8,cy,304,52,6,vDim[idx]); tft.fillRoundRect(8,cy,5,52,3,vAccent[idx]); tft.drawRoundRect(8,cy,304,52,6,vAccent[idx]); tft.drawFastVLine(9,cy+1,50,(uint16_t)(vAccent[idx]>>1)); }
    else    { tft.fillRoundRect(8,cy,304,52,6,0x0821); tft.drawRoundRect(8,cy,304,52,6,0x1082); }
    tft.fillCircle(32,cy+26,14,sel?vAccent[idx]:(uint16_t)0x1082);
    tft.setTextSize(2); tft.setTextColor(sel?(uint16_t)0x0000:(uint16_t)0x2945);
    tft.setCursor(26,cy+18); tft.print(idx+1);
    tft.setTextSize(1); tft.setTextColor(sel?vAccent[idx]:(uint16_t)0x528A);
    tft.setCursor(56,cy+12); tft.print(vNames[idx]);
    tft.setTextColor(sel?(uint16_t)0x3186:(uint16_t)0x2104);
    tft.setCursor(56,cy+26); tft.print(vSubtitles[idx]);
    if(sel) tft.fillTriangle(298,cy+20,298,cy+32,304,cy+26,vAccent[idx]);
    else    tft.fillRect(296,cy+18,10,16,0x0821);
  }
}

void launchFlappyVersion(int version) {
  flappyVersion=version;
  if(version==1) { currentGame=GAME_FLAPPY; showStartScreenFlappy(); }
  else           { currentGame=GAME_FLAPPY; sa_startGame(version); }
}

// ================================================================
// =================== CONNECT 4 ==================================
// ================================================================
#define C4_ROWS    7
#define C4_COLS    10
#define CELL_SIZE  28
#define OFFSET_X   20
#define OFFSET_Y   40
#define COL_BOARD  0x0000
#define COL_EMPTY  0xFFFF
#define COL_PLAYER 0x07E0
#define COL_BOT    0xF800
#define COL_GLOSS  0xFFFF
#define COL_WIN    0x001F
#define COL_BG_C4  0x0000
#define COL_BORDER 0x7BEF
#define COL_GRID   0xAD55

int c4board[C4_ROWS][C4_COLS];
int selector = 4;
bool playerTurn = true, gameOverC4 = false;
int playerWins = 0, botWins = 0;
int winX[4], winY[4];

void drawGlossyCircle(int x,int y,int r,uint16_t color){if(color==COL_EMPTY){tft.fillCircle(x,y,r,COL_BOARD);tft.drawCircle(x,y,r,COL_GRID);}else{tft.fillCircle(x,y,r,color);tft.fillCircle(x-r/3,y-r/3,r/4,COL_GLOSS);}}
void updateSelectorC4(){tft.fillRect(0,0,320,38,COL_BG_C4);if(!gameOverC4){int tx=OFFSET_X+selector*CELL_SIZE+CELL_SIZE/2,ty=30;for(int i=1;i<=3;i++)tft.fillTriangle(tx-(6+i/2),ty-i,tx+(6+i/2),ty-i,tx,ty+10,playerTurn?0x03E0:0xB800);tft.fillTriangle(tx-6,ty,tx+6,ty,tx,ty+12,playerTurn?COL_PLAYER:COL_BOT);tft.fillRoundRect(10,8,100,22,4,COL_BORDER);tft.setTextSize(1);tft.setTextColor(playerTurn?COL_PLAYER:COL_BOT,COL_BORDER);tft.setCursor(15,13);tft.print(playerTurn?"YOUR TURN":"BOT TURN");tft.fillRoundRect(210,8,100,22,4,COL_BORDER);tft.setTextColor(COL_GLOSS,COL_BORDER);tft.setCursor(215,13);tft.print("YOU:");tft.print(playerWins);tft.print(" BOT:");tft.print(botWins);}}
void drawBoardC4(){int bw=C4_COLS*CELL_SIZE+12,bh=C4_ROWS*CELL_SIZE+12;tft.fillRoundRect(OFFSET_X-6,OFFSET_Y-6,bw,bh,10,COL_BORDER);tft.fillRoundRect(OFFSET_X-4,OFFSET_Y-4,bw-4,bh-4,8,COL_BOARD);tft.drawRoundRect(OFFSET_X-5,OFFSET_Y-5,bw-2,bh-2,9,COL_GOLD);for(int c=0;c<=C4_COLS;c++)tft.drawLine(OFFSET_X+c*CELL_SIZE,OFFSET_Y,OFFSET_X+c*CELL_SIZE,OFFSET_Y+C4_ROWS*CELL_SIZE,COL_GRID);for(int r=0;r<=C4_ROWS;r++)tft.drawLine(OFFSET_X,OFFSET_Y+r*CELL_SIZE,OFFSET_X+C4_COLS*CELL_SIZE,OFFSET_Y+r*CELL_SIZE,COL_GRID);for(int r=0;r<C4_ROWS;r++)for(int c=0;c<C4_COLS;c++){uint16_t col=(c4board[r][c]==1)?COL_PLAYER:(c4board[r][c]==2)?COL_BOT:COL_EMPTY;drawGlossyCircle(OFFSET_X+c*CELL_SIZE+CELL_SIZE/2,OFFSET_Y+r*CELL_SIZE+CELL_SIZE/2,CELL_SIZE/2-4,col);}updateSelectorC4();}
void animateDiscC4(int col,int tr,uint16_t color){int x=OFFSET_X+col*CELL_SIZE+CELL_SIZE/2,br=CELL_SIZE/2-4;for(int r=0;r<=tr;r++){int y=OFFSET_Y+r*CELL_SIZE+CELL_SIZE/2;if(r>0)drawGlossyCircle(x,OFFSET_Y+(r-1)*CELL_SIZE+CELL_SIZE/2,br,COL_EMPTY);tft.fillCircle(x+2,y+2,br,0x0000);drawGlossyCircle(x,y,br,color);delay(45);}}
bool checkWinC4(int p){for(int r=0;r<C4_ROWS;r++)for(int c=0;c<C4_COLS-3;c++)if(c4board[r][c]==p&&c4board[r][c+1]==p&&c4board[r][c+2]==p&&c4board[r][c+3]==p){for(int i=0;i<4;i++){winY[i]=r;winX[i]=c+i;}return true;}for(int r=0;r<C4_ROWS-3;r++)for(int c=0;c<C4_COLS;c++)if(c4board[r][c]==p&&c4board[r+1][c]==p&&c4board[r+2][c]==p&&c4board[r+3][c]==p){for(int i=0;i<4;i++){winY[i]=r+i;winX[i]=c;}return true;}for(int r=3;r<C4_ROWS;r++)for(int c=0;c<C4_COLS-3;c++)if(c4board[r][c]==p&&c4board[r-1][c+1]==p&&c4board[r-2][c+2]==p&&c4board[r-3][c+3]==p){for(int i=0;i<4;i++){winY[i]=r-i;winX[i]=c+i;}return true;}for(int r=0;r<C4_ROWS-3;r++)for(int c=0;c<C4_COLS-3;c++)if(c4board[r][c]==p&&c4board[r+1][c+1]==p&&c4board[r+2][c+2]==p&&c4board[r+3][c+3]==p){for(int i=0;i<4;i++){winY[i]=r+i;winX[i]=c+i;}return true;}return false;}
bool isDrawC4(){for(int c=0;c<C4_COLS;c++)if(c4board[0][c]==0)return false;return true;}
void drawWinLineC4(){for(int i=0;i<3;i++){int x1=OFFSET_X+winX[i]*CELL_SIZE+CELL_SIZE/2,y1=OFFSET_Y+winY[i]*CELL_SIZE+CELL_SIZE/2;int x2=OFFSET_X+winX[i+1]*CELL_SIZE+CELL_SIZE/2,y2=OFFSET_Y+winY[i+1]*CELL_SIZE+CELL_SIZE/2;for(int t=-2;t<=2;t++)for(int o=-2;o<=2;o++)if(abs(o)+abs(t)<=4)tft.drawLine(x1+o,y1+t,x2+o,y2+t,COL_WIN);delay(100);}}
void showGameOverC4(String msg,uint16_t tc){delay(600);tft.fillRect(40,50,240,140,0x0000);tft.drawRoundRect(40,50,240,140,10,COL_GOLD);tft.drawRoundRect(42,52,236,136,8,tc);tft.setTextSize(2);tft.setTextColor(tc,0x0000);if(msg=="YOU WIN!")tft.setCursor(108,75);else if(msg=="BOT WINS!")tft.setCursor(98,75);else tft.setCursor(128,75);tft.print(msg);tft.drawFastHLine(70,105,180,COL_GOLD);tft.setTextSize(2);tft.setTextColor(COL_GLOSS,0x0000);tft.setCursor(85,120);tft.print("YOU: ");tft.print(playerWins);tft.setCursor(85,145);tft.print("BOT: ");tft.print(botWins);tft.setTextSize(1);tft.setTextColor(COL_GOLD,0x0000);tft.setCursor(72,175);tft.print("PRESS SELECT TO PLAY AGAIN");}
bool dropDiscC4(int col,int p){for(int r=C4_ROWS-1;r>=0;r--)if(c4board[r][col]==0){animateDiscC4(col,r,(p==1)?COL_PLAYER:COL_BOT);c4board[r][col]=p;return true;}return false;}
void botMoveC4(){delay(300);int mc=-1;for(int c=0;c<C4_COLS&&mc==-1;c++)if(c4board[0][c]==0){int r;for(r=C4_ROWS-1;r>=0;r--)if(c4board[r][c]==0)break;c4board[r][c]=2;if(checkWinC4(2))mc=c;c4board[r][c]=0;}if(mc==-1)for(int c=0;c<C4_COLS&&mc==-1;c++)if(c4board[0][c]==0){int r;for(r=C4_ROWS-1;r>=0;r--)if(c4board[r][c]==0)break;c4board[r][c]=1;if(checkWinC4(1))mc=c;c4board[r][c]=0;}if(mc==-1){do{mc=random(C4_COLS);}while(c4board[0][mc]!=0);}dropDiscC4(mc,2);if(checkWinC4(2)){botWins++;menuHighScores[2]=playerWins;drawWinLineC4();showGameOverC4("BOT WINS!",COL_BOT);gameOverC4=true;}else if(isDrawC4()){showGameOverC4("DRAW!",COL_GOLD);gameOverC4=true;}else{playerTurn=true;updateSelectorC4();}}
void resetGameC4(){tft.fillScreen(COL_BG_C4);for(int r=0;r<C4_ROWS;r++)for(int c=0;c<C4_COLS;c++)c4board[r][c]=0;selector=4;gameOverC4=false;playerTurn=(random(2)==0);drawBoardC4();if(!playerTurn){delay(500);botMoveC4();}}
void runConnect4Game(){static unsigned long lastL=0,lastR=0,lastSel=0,lastB=0;if(digitalRead(BTN_B)==LOW&&millis()-lastB>300){lastB=millis();gameOverC4=true;returnToMenu();return;}if(!gameOverC4&&playerTurn){if(digitalRead(BTN_LEFT)==LOW&&selector>0&&millis()-lastL>120){lastL=millis();selector--;updateSelectorC4();}if(digitalRead(BTN_RIGHT)==LOW&&selector<C4_COLS-1&&millis()-lastR>120){lastR=millis();selector++;updateSelectorC4();}if(digitalRead(BTN_SELECT)==LOW&&millis()-lastSel>200){lastSel=millis();if(dropDiscC4(selector,1)){if(checkWinC4(1)){playerWins++;menuHighScores[2]=playerWins;drawWinLineC4();showGameOverC4("YOU WIN!",COL_PLAYER);gameOverC4=true;}else if(isDrawC4()){showGameOverC4("DRAW!",COL_GOLD);gameOverC4=true;}else{playerTurn=false;updateSelectorC4();delay(300);botMoveC4();}}}}else if(gameOverC4&&digitalRead(BTN_SELECT)==LOW&&millis()-lastSel>300){lastSel=millis();resetGameC4();}delay(10);}

// ================================================================
// =================== TIC TAC TOE ================================
// ================================================================
#define TTT_SIZE      3
#define TTT_CELL_SIZE 80
#define TTT_OFFSET_X  40
#define TTT_OFFSET_Y  30
#define TTT_BG        0x0000
#define TTT_GRID      0xFFFF
#define TTT_PLAYER    0x07E0
#define TTT_BOT       0xF800
#define TTT_HIGHLIGHT 0xFD20
#define TTT_TEXT      0xFFFF

char tttBoard[TTT_SIZE][TTT_SIZE];
bool playerTurnTTT = true, gameOverTTT = false;
int playerScoreTTT = 0, botScoreTTT = 0, drawScoreTTT = 0;
int selectedRow = 1, selectedCol = 1;

void drawTTTBoard(){for(int i=1;i<TTT_SIZE;i++){tft.drawLine(TTT_OFFSET_X+i*TTT_CELL_SIZE,TTT_OFFSET_Y,TTT_OFFSET_X+i*TTT_CELL_SIZE,TTT_OFFSET_Y+TTT_SIZE*TTT_CELL_SIZE,TTT_GRID);tft.drawLine(TTT_OFFSET_X,TTT_OFFSET_Y+i*TTT_CELL_SIZE,TTT_OFFSET_X+TTT_SIZE*TTT_CELL_SIZE,TTT_OFFSET_Y+i*TTT_CELL_SIZE,TTT_GRID);}for(int r=0;r<TTT_SIZE;r++)for(int c=0;c<TTT_SIZE;c++){int x=TTT_OFFSET_X+c*TTT_CELL_SIZE,y=TTT_OFFSET_Y+r*TTT_CELL_SIZE;if(tttBoard[r][c]=='X'){tft.drawLine(x+15,y+15,x+TTT_CELL_SIZE-15,y+TTT_CELL_SIZE-15,TTT_BOT);tft.drawLine(x+TTT_CELL_SIZE-15,y+15,x+15,y+TTT_CELL_SIZE-15,TTT_BOT);}else if(tttBoard[r][c]=='O'){tft.drawCircle(x+TTT_CELL_SIZE/2,y+TTT_CELL_SIZE/2,TTT_CELL_SIZE/2-15,TTT_PLAYER);tft.fillCircle(x+TTT_CELL_SIZE/2,y+TTT_CELL_SIZE/2,TTT_CELL_SIZE/2-18,TTT_BG);tft.drawCircle(x+TTT_CELL_SIZE/2,y+TTT_CELL_SIZE/2,TTT_CELL_SIZE/2-16,TTT_PLAYER);}}}
void drawTTTSelector(){int x=TTT_OFFSET_X+selectedCol*TTT_CELL_SIZE,y=TTT_OFFSET_Y+selectedRow*TTT_CELL_SIZE;for(int i=0;i<3;i++)tft.drawRect(x-i,y-i,TTT_CELL_SIZE+i*2,TTT_CELL_SIZE+i*2,TTT_HIGHLIGHT);}
void updateTTTScore(){tft.fillRect(0,0,320,28,TTT_BG);tft.drawRect(0,0,320,28,TTT_GRID);tft.setTextSize(1);tft.setTextColor(TTT_TEXT,TTT_BG);tft.setCursor(10,8);tft.print("YOU: ");tft.print(playerScoreTTT);tft.setCursor(120,8);tft.print("DRAW: ");tft.print(drawScoreTTT);tft.setCursor(230,8);tft.print("BOT: ");tft.print(botScoreTTT);if(!gameOverTTT&&playerTurnTTT){tft.setCursor(10,20);tft.print("YOUR TURN (O)");}else if(!gameOverTTT&&!playerTurnTTT){tft.setCursor(10,20);tft.print("BOT TURN (X)...");}}
bool checkWinTTT(char p,int&r1,int&c1,int&r2,int&c2,int&r3,int&c3){for(int i=0;i<TTT_SIZE;i++)if(tttBoard[i][0]==p&&tttBoard[i][1]==p&&tttBoard[i][2]==p){r1=i;c1=0;r2=i;c2=1;r3=i;c3=2;return true;}for(int i=0;i<TTT_SIZE;i++)if(tttBoard[0][i]==p&&tttBoard[1][i]==p&&tttBoard[2][i]==p){r1=0;c1=i;r2=1;c2=i;r3=2;c3=i;return true;}if(tttBoard[0][0]==p&&tttBoard[1][1]==p&&tttBoard[2][2]==p){r1=0;c1=0;r2=1;c2=1;r3=2;c3=2;return true;}if(tttBoard[0][2]==p&&tttBoard[1][1]==p&&tttBoard[2][0]==p){r1=0;c1=2;r2=1;c2=1;r3=2;c3=0;return true;}return false;}
void drawWinningLineTTT(int r1,int c1,int r2,int c2,int r3,int c3){int x1=TTT_OFFSET_X+c1*TTT_CELL_SIZE+TTT_CELL_SIZE/2,y1=TTT_OFFSET_Y+r1*TTT_CELL_SIZE+TTT_CELL_SIZE/2;int x3=TTT_OFFSET_X+c3*TTT_CELL_SIZE+TTT_CELL_SIZE/2,y3=TTT_OFFSET_Y+r3*TTT_CELL_SIZE+TTT_CELL_SIZE/2;for(int s=0;s<=10;s++){int cx=x1+(x3-x1)*s/10,cy=y1+(y3-y1)*s/10;for(int t=-3;t<=3;t++)for(int o=-2;o<=2;o++)if(abs(t)+abs(o)<=3)tft.drawLine(x1+o,y1+t,cx+o,cy+t,TFT_WHITE);delay(50);}for(int fl=0;fl<5;fl++){for(int t=-3;t<=3;t++)for(int o=-2;o<=2;o++)if(abs(t)+abs(o)<=3)tft.drawLine(x1+o,y1+t,x3+o,y3+t,TTT_BG);delay(80);for(int t=-3;t<=3;t++)for(int o=-2;o<=2;o++)if(abs(t)+abs(o)<=3)tft.drawLine(x1+o,y1+t,x3+o,y3+t,TFT_WHITE);delay(80);}delay(500);}
bool isDrawTTT(){for(int i=0;i<TTT_SIZE;i++)for(int j=0;j<TTT_SIZE;j++)if(tttBoard[i][j]==' ')return false;return true;}
void showGameOverTTT(String msg){delay(500);tft.fillRect(40,90,240,80,0x0000);tft.drawRect(40,90,240,80,TTT_GRID);tft.drawRect(43,93,234,74,TTT_GRID);tft.setTextSize(2);if(msg=="YOU WIN!"){tft.setTextColor(TTT_PLAYER);tft.setCursor(110,115);}else if(msg=="BOT WINS!"){tft.setTextColor(TTT_BOT);tft.setCursor(100,115);}else{tft.setTextColor(TTT_GRID);tft.setCursor(120,115);}tft.print(msg);tft.setTextSize(1);tft.setTextColor(TTT_TEXT);tft.setCursor(80,145);tft.print("PRESS SELECT TO CONTINUE");while(digitalRead(BTN_SELECT)==HIGH)delay(50);delay(300);}
void botMoveTTT(){if(gameOverTTT||playerTurnTTT)return;delay(400);int br=-1,bc=-1,t1,t2,t3,t4,t5,t6;for(int i=0;i<TTT_SIZE&&br==-1;i++)for(int j=0;j<TTT_SIZE&&br==-1;j++)if(tttBoard[i][j]==' '){tttBoard[i][j]='X';if(checkWinTTT('X',t1,t2,t3,t4,t5,t6)){br=i;bc=j;}tttBoard[i][j]=' ';}if(br==-1)for(int i=0;i<TTT_SIZE&&br==-1;i++)for(int j=0;j<TTT_SIZE&&br==-1;j++)if(tttBoard[i][j]==' '){tttBoard[i][j]='O';if(checkWinTTT('O',t1,t2,t3,t4,t5,t6)){br=i;bc=j;}tttBoard[i][j]=' ';}if(br==-1&&tttBoard[1][1]==' '){br=1;bc=1;}if(br==-1){int cn[4][2]={{0,0},{0,2},{2,0},{2,2}};for(int i=0;i<4&&br==-1;i++)if(tttBoard[cn[i][0]][cn[i][1]]==' '){br=cn[i][0];bc=cn[i][1];}}if(br==-1){do{br=random(TTT_SIZE);bc=random(TTT_SIZE);}while(tttBoard[br][bc]!=' ');}tttBoard[br][bc]='X';drawTTTBoard();int wr1,wc1,wr2,wc2,wr3,wc3;if(checkWinTTT('X',wr1,wc1,wr2,wc2,wr3,wc3)){drawWinningLineTTT(wr1,wc1,wr2,wc2,wr3,wc3);botScoreTTT++;menuHighScores[3]=playerScoreTTT;gameOverTTT=true;updateTTTScore();showGameOverTTT("BOT WINS!");resetTTTGame();}else if(isDrawTTT()){drawScoreTTT++;gameOverTTT=true;updateTTTScore();showGameOverTTT("DRAW!");resetTTTGame();}else{playerTurnTTT=true;updateTTTScore();drawTTTSelector();}}
void resetTTTGame(){for(int i=0;i<TTT_SIZE;i++)for(int j=0;j<TTT_SIZE;j++)tttBoard[i][j]=' ';playerTurnTTT=(random(2)==0);gameOverTTT=false;selectedRow=1;selectedCol=1;tft.fillScreen(TTT_BG);drawTTTBoard();updateTTTScore();drawTTTSelector();if(!playerTurnTTT){delay(500);botMoveTTT();}}
void runTicTacToe(){static unsigned long lastMv=0,lastB=0;if(digitalRead(BTN_B)==LOW&&millis()-lastB>300){lastB=millis();returnToMenu();return;}if(!gameOverTTT&&playerTurnTTT&&millis()-lastMv>200){bool mv=false;if(digitalRead(BTN_UP)==LOW&&selectedRow>0){selectedRow--;mv=true;}else if(digitalRead(BTN_DOWN)==LOW&&selectedRow<TTT_SIZE-1){selectedRow++;mv=true;}else if(digitalRead(BTN_LEFT)==LOW&&selectedCol>0){selectedCol--;mv=true;}else if(digitalRead(BTN_RIGHT)==LOW&&selectedCol<TTT_SIZE-1){selectedCol++;mv=true;}else if(digitalRead(BTN_SELECT)==LOW&&tttBoard[selectedRow][selectedCol]==' '){tttBoard[selectedRow][selectedCol]='O';drawTTTBoard();int r1,c1,r2,c2,r3,c3;if(checkWinTTT('O',r1,c1,r2,c2,r3,c3)){drawWinningLineTTT(r1,c1,r2,c2,r3,c3);playerScoreTTT++;menuHighScores[3]=playerScoreTTT;gameOverTTT=true;updateTTTScore();showGameOverTTT("YOU WIN!");resetTTTGame();}else if(isDrawTTT()){drawScoreTTT++;gameOverTTT=true;updateTTTScore();showGameOverTTT("DRAW!");resetTTTGame();}else{playerTurnTTT=false;updateTTTScore();botMoveTTT();}mv=true;}if(mv){lastMv=millis();tft.fillScreen(TTT_BG);drawTTTBoard();updateTTTScore();drawTTTSelector();}}delay(10);}

// ================================================================
// =================== RPS GAME ===================================
// ================================================================
#define MINT_BG       0x9F5A
#define COLOR_ROCK    0x52AA
#define COLOR_PAPER   0xFFFF
#define COLOR_SC_H    0xF800
#define COLOR_TEXT    0x0000
#define COLOR_WIN_RPS 0x07E0
#define COLOR_LOSS    0xF800
#define COLOR_GOLD_RPS 0xFD20

int pScore = 0, bScore = 0;
int currentSel = 1;

void drawMintBackground(){tft.fillScreen(MINT_BG);}
void drawRealRock(int x,int y,int s){uint16_t rd=0x3DEF,rl=0x7BEF,rm=0x5ACB;tft.fillRoundRect(x+2,y+4,s-4,s-8,10,rm);tft.fillRoundRect(x+2,y+s-12,s-4,8,6,rd);tft.fillRect(x+s-10,y+8,6,s-16,rd);tft.fillRoundRect(x+2,y+4,8,s-8,4,rl);tft.fillRect(x+6,y+4,s-12,6,rl);for(int i=0;i<4;i++){int sx=x+random(8,s-8),sy=y+random(10,s-10);tft.drawLine(sx,sy,sx+random(-5,5),sy+random(8,15),rd);tft.drawLine(sx,sy,sx+random(5,10),sy+random(-3,3),rd);}for(int i=0;i<8;i++)tft.fillCircle(x+random(8,s-8),y+random(10,s-10),random(1,3),rd);tft.drawRoundRect(x+2,y+4,s-4,s-8,10,0x2945);}
void drawProPaper(int x,int y,int s){tft.fillRect(x,y,s-4,s,COLOR_PAPER);tft.drawRect(x,y,s-4,s,0x0000);for(int i=6;i<s;i+=6)tft.drawFastHLine(x+2,y+i,s-8,0xAD7F);tft.fillRect(x+2,y+2,3,s-4,0x94B2);}
void drawProScissor(int x,int y,int s){tft.drawCircle(x+8,y+s-10,7,COLOR_SC_H);tft.drawCircle(x+s-12,y+s-10,7,COLOR_SC_H);tft.fillTriangle(x+10,y+s-15,x+s-5,y+5,x+s-10,y+2,0x0000);tft.fillTriangle(x+s-14,y+s-15,x+5,y+5,x+10,y+2,0x0000);tft.drawLine(x+12,y+s-18,x+s-12,y+8,0xFFFF);}
void updateScoreRPS(){tft.fillRoundRect(0,0,screenWidth,40,8,0x0000);tft.drawRoundRect(0,0,screenWidth,40,8,COLOR_GOLD_RPS);tft.setTextColor(0xFFFF);tft.setTextSize(2);tft.setCursor(30,12);tft.print("PLAYER: ");tft.print(pScore);tft.setCursor(200,12);tft.print("BOT: ");tft.print(bScore);tft.drawFastVLine(160,5,30,COLOR_GOLD_RPS);}
void drawSelectionBoxes(){tft.fillRect(0,100,screenWidth,80,MINT_BG);for(int i=1;i<=3;i++){int x=40+(i-1)*95;uint16_t border=(currentSel==i)?0xF800:0x0000;tft.drawRect(x-3,147,70,70,border);tft.drawRect(x-4,146,72,72,border);tft.drawRect(x-5,145,74,74,border);if(currentSel==i)for(int g=1;g<=3;g++)tft.drawRect(x-3-g,147-g,70+g*2,70+g*2,0xF820);if(i==1)drawRealRock(x+18,153,50);else if(i==2)drawProPaper(x+18,153,50);else drawProScissor(x+18,153,50);}tft.setTextSize(1);tft.setTextColor(0xFFFF);tft.setCursor(20,225);tft.print("LEFT/RIGHT: Select   SELECT: Play | B: Menu");}
void showResultAnimationRPS(String result,uint16_t color){int bx=60,by=90,bw=200,bh=60;for(int i=0;i<10;i++){tft.drawRoundRect(bx-i,by-i,bw+i*2,bh+i*2,10,color);delay(20);}tft.fillRoundRect(bx,by,bw,bh,10,0x0000);tft.drawRoundRect(bx,by,bw,bh,10,color);tft.setTextSize(3);tft.setTextColor(color);tft.setCursor(bx+(bw/2)-(result.length()*9),by+20);tft.print(result);delay(1500);}
void checkUltimateWinner(){if(pScore<5&&bScore<5)return;delay(500);for(int i=0;i<screenWidth;i+=10){tft.drawFastVLine(i,0,screenHeight,0x0000);delay(8);}tft.setTextSize(3);if(pScore>=5){tft.fillScreen(0x0000);tft.setTextColor(COLOR_WIN_RPS);tft.setCursor(65,50);tft.println("VICTORY!");tft.setTextSize(2);tft.setTextColor(0xFFFF);tft.setCursor(45,90);tft.println("YOU ARE SUPREME!");menuHighScores[4]=pScore;}else{tft.fillScreen(0x0000);tft.setTextColor(COLOR_LOSS);tft.setCursor(55,50);tft.println("GAME OVER");tft.setTextSize(2);tft.setTextColor(0xFFFF);tft.setCursor(55,90);tft.println("BOT IS SUPREME!");}unsigned long bs=millis();while(digitalRead(BTN_SELECT)==HIGH){if(millis()-bs>500){tft.setTextSize(1);tft.setTextColor(0xAD7F);tft.setCursor(85,210);tft.print("PRESS SELECT");delay(100);tft.fillRect(85,210,150,15,0x0000);bs=millis();}delay(10);}pScore=0;bScore=0;drawMintBackground();updateScoreRPS();drawSelectionBoxes();delay(500);}
void runRPSGame(){static unsigned long lastBtn=0,lastB=0;if(digitalRead(BTN_B)==LOW&&millis()-lastB>300){lastB=millis();returnToMenu();return;}if(digitalRead(BTN_LEFT)==LOW&&millis()-lastBtn>200){lastBtn=millis();currentSel--;if(currentSel<1)currentSel=3;drawSelectionBoxes();}if(digitalRead(BTN_RIGHT)==LOW&&millis()-lastBtn>200){lastBtn=millis();currentSel++;if(currentSel>3)currentSel=1;drawSelectionBoxes();}if(digitalRead(BTN_SELECT)==LOW&&millis()-lastBtn>300){lastBtn=millis();tft.fillRect(0,45,screenWidth,55,MINT_BG);delay(800);tft.fillRect(0,45,screenWidth,80,MINT_BG);tft.setTextSize(2);tft.setTextColor(COLOR_TEXT);tft.setCursor(40,55);tft.print("YOU");if(currentSel==1)drawRealRock(35,75,60);else if(currentSel==2)drawProPaper(35,75,60);else drawProScissor(35,75,60);int bp=random(1,4);tft.setCursor(200,55);tft.print("BOT");for(int i=0;i<5;i++){tft.setCursor(170,90);tft.setTextColor(0xF800);tft.print("THINKING");delay(200);tft.fillRect(170,90,100,15,MINT_BG);delay(100);}tft.fillRect(140,70,130,50,MINT_BG);if(bp==1)drawRealRock(185,75,60);else if(bp==2)drawProPaper(185,75,60);else drawProScissor(185,75,60);delay(1000);String result;uint16_t rc;if(currentSel==bp){result="DRAW!";rc=0xFFFF;}else if((currentSel==1&&bp==3)||(currentSel==2&&bp==1)||(currentSel==3&&bp==2)){pScore++;result="YOU WIN!";rc=COLOR_WIN_RPS;}else{bScore++;result="YOU LOST!";rc=COLOR_LOSS;}updateScoreRPS();showResultAnimationRPS(result,rc);if(pScore>=5||bScore>=5){checkUltimateWinner();}else{drawMintBackground();updateScoreRPS();drawSelectionBoxes();}}delay(10);}

// ================================================================
// =================== NINJA UP ===================================
// ================================================================
#define NINJA_COLOR  TFT_CYAN
#define WALL_COLOR   TFT_WHITE
#define SPIKE_COLOR  TFT_RED
#define BLUE_SPIKE   TFT_BLUE
#define NDOT_COLOR   0x7BEF
#define TRAIL_COLOR  0x0410

#define NINJA_WALL_LEFT   0
#define NINJA_WALL_RIGHT  200
#define NINJA_WALL_THICK  6
#define NINJA_PW          16
#define NINJA_PH          24

const int TRAIL_SIZE = 5;
int trailX[TRAIL_SIZE], trailY[TRAIL_SIZE];

struct Spike { float y; int side; bool isBlue; };
struct NinjaDot { float x, y, speed; };

const int MAX_NINJA_DOTS = 20;
NinjaDot ninjaDots[MAX_NINJA_DOTS];
Spike spikes[2];

int ninjaPlayerSide = 0;
float ninjaPlayerY = 100;
unsigned long ninjaScore = 0;
unsigned long ninjaHighScore = 0;
bool ninjaPlaying = false;
bool ninjaPaused = false;
float ninjaGameSpeed = 2.5;
bool ninjaLeftPressed = false, ninjaRightPressed = false;
unsigned long lastNinjaLeft = 0, lastNinjaRight = 0;
const int NINJA_DEBOUNCE = 50;

bool isRushMode = false;
unsigned long rushStartTime = 0;
const long rushDuration = 4000;

void initNinjaDots(){for(int i=0;i<MAX_NINJA_DOTS;i++){ninjaDots[i].x=random(NINJA_WALL_LEFT+NINJA_WALL_THICK+5,NINJA_WALL_RIGHT-5);ninjaDots[i].y=random(0,240);ninjaDots[i].speed=(random(5,15)/10.0);}}
void showStartScreenNinja(){tft.fillScreen(BG_COLOR);tft.setTextColor(NINJA_COLOR);tft.setTextSize(3);tft.setCursor(60,30);tft.print("NINJA UP!");tft.setTextColor(WALL_COLOR);tft.setTextSize(1);tft.setCursor(30,80);tft.print("SELECT : Start Game");tft.setCursor(30,98);tft.print("LEFT   : Move to Left Wall");tft.setCursor(30,116);tft.print("RIGHT  : Move to Right Wall");tft.setCursor(30,134);tft.print("START  : Pause / Resume");tft.setCursor(30,152);tft.print("B      : Back to Menu");tft.setCursor(30,170);tft.print("Avoid RED spikes!");tft.setTextColor(TFT_BLUE);tft.setCursor(30,188);tft.print("Catch BLUE spikes = RUSH MODE!");tft.setTextColor(TFT_YELLOW);tft.setCursor(60,210);tft.print("Press SELECT to begin");tft.setTextColor(TFT_CYAN);tft.setCursor(30,228);tft.print("Best: ");tft.print(ninjaHighScore);}
void startNinjaGame(){tft.fillScreen(BG_COLOR);ninjaPlayerY=100;ninjaPlayerSide=0;ninjaScore=0;ninjaGameSpeed=2.5;isRushMode=false;ninjaPaused=false;initNinjaDots();for(int i=0;i<TRAIL_SIZE;i++){trailX[i]=-30;trailY[i]=-30;}spikes[0]={-50.0f,(int)random(0,2),false};spikes[1]={-200.0f,(int)random(0,2),false};ninjaPlaying=true;}
void updateNinjaScoreDisplay(){tft.fillRect(220,0,100,60,BG_COLOR);tft.setTextColor(TFT_GREEN,BG_COLOR);tft.setTextSize(1);tft.setCursor(250,10);tft.print("SCORE");tft.setTextColor(TFT_YELLOW,BG_COLOR);tft.setTextSize(2);tft.setCursor(244,22);char sb[15];sprintf(sb,"%04lu",ninjaScore/10);tft.print(sb);tft.setTextColor(TFT_CYAN,BG_COLOR);tft.setTextSize(1);tft.setCursor(250,50);tft.print("BEST:");tft.setTextColor(TFT_WHITE,BG_COLOR);tft.setTextSize(2);tft.setCursor(244,62);sprintf(sb,"%04lu",ninjaHighScore);tft.print(sb);}
void ninjaGameOver(){for(int i=0;i<5;i++){tft.invertDisplay(true);delay(40);tft.invertDisplay(false);delay(40);}ninjaPlaying=false;ninjaPaused=false;unsigned long cs=ninjaScore/10;if(cs>ninjaHighScore){ninjaHighScore=cs;menuHighScores[5]=(int)ninjaHighScore;}tft.fillRoundRect(40,70,240,100,10,TFT_RED);tft.drawRoundRect(40,70,240,100,10,TFT_WHITE);tft.setTextColor(TFT_WHITE);tft.setTextSize(2);tft.setCursor(90,90);tft.print("GAME OVER!");tft.setTextSize(1);tft.setCursor(90,115);tft.print("Score: ");tft.print(cs);tft.setCursor(90,135);tft.print("Best:  ");tft.print(ninjaHighScore);tft.setTextSize(1);tft.setCursor(65,180);tft.setTextColor(TFT_YELLOW);tft.print("SELECT: Play Again   B: Menu");}
void updateNinjaGame(){bool leftState=(digitalRead(BTN_LEFT)==LOW);bool rightState=(digitalRead(BTN_RIGHT)==LOW);if(leftState&&!ninjaLeftPressed&&(millis()-lastNinjaLeft>NINJA_DEBOUNCE)){ninjaPlayerSide=0;ninjaLeftPressed=true;lastNinjaLeft=millis();}else if(!leftState)ninjaLeftPressed=false;if(rightState&&!ninjaRightPressed&&(millis()-lastNinjaRight>NINJA_DEBOUNCE)){ninjaPlayerSide=1;ninjaRightPressed=true;lastNinjaRight=millis();}else if(!rightState)ninjaRightPressed=false;float effectiveSpeed=ninjaGameSpeed;int scoreMul=1;bool showPlayer=true;uint16_t playerColor=NINJA_COLOR;if(!isRushMode){ninjaGameSpeed+=0.0024;if(ninjaGameSpeed>6.0)ninjaGameSpeed=6.0;}if(isRushMode){unsigned long el=millis()-rushStartTime;if(el<rushDuration){if(el<=6000){effectiveSpeed=ninjaGameSpeed*4.0;scoreMul=5;playerColor=BLUE_SPIKE;}else{effectiveSpeed=ninjaGameSpeed;scoreMul=1;playerColor=NINJA_COLOR;if((el/120)%2==0)showPlayer=false;}}else isRushMode=false;}ninjaScore+=scoreMul;tft.fillRect(NINJA_WALL_LEFT+NINJA_WALL_THICK,0,(NINJA_WALL_RIGHT-NINJA_WALL_LEFT)-NINJA_WALL_THICK,240,BG_COLOR);for(int i=0;i<MAX_NINJA_DOTS;i++){ninjaDots[i].y+=ninjaDots[i].speed+(effectiveSpeed*0.3);if(ninjaDots[i].y>240){ninjaDots[i].y=0;ninjaDots[i].x=random(NINJA_WALL_LEFT+NINJA_WALL_THICK+5,NINJA_WALL_RIGHT-5);}tft.drawPixel((int)ninjaDots[i].x,(int)ninjaDots[i].y,NDOT_COLOR);}for(int i=TRAIL_SIZE-1;i>0;i--){trailX[i]=trailX[i-1];trailY[i]=trailY[i-1];}int ninjaX=(ninjaPlayerSide==0)?(NINJA_WALL_LEFT+NINJA_WALL_THICK+2):(NINJA_WALL_RIGHT-NINJA_PW-2);trailX[0]=ninjaX;trailY[0]=(int)ninjaPlayerY;for(int i=1;i<TRAIL_SIZE;i++)if(trailX[i]>0)tft.drawRect(trailX[i],trailY[i],NINJA_PW,NINJA_PH,TRAIL_COLOR);tft.fillRect(NINJA_WALL_LEFT,0,NINJA_WALL_THICK,240,WALL_COLOR);tft.fillRect(NINJA_WALL_RIGHT,0,NINJA_WALL_THICK,240,WALL_COLOR);tft.drawRect(NINJA_WALL_LEFT-1,0,NINJA_WALL_THICK+2,240,TFT_DARKGREY);tft.drawRect(NINJA_WALL_RIGHT-1,0,NINJA_WALL_THICK+2,240,TFT_DARKGREY);for(int i=0;i<2;i++){spikes[i].y+=effectiveSpeed;int ss=28;uint16_t sc=spikes[i].isBlue?BLUE_SPIKE:SPIKE_COLOR;if(spikes[i].side==0){tft.fillTriangle(NINJA_WALL_LEFT+NINJA_WALL_THICK,(int)spikes[i].y,NINJA_WALL_LEFT+NINJA_WALL_THICK,(int)spikes[i].y+ss,NINJA_WALL_LEFT+NINJA_WALL_THICK+ss,(int)spikes[i].y+ss/2,sc);}else{tft.fillTriangle(NINJA_WALL_RIGHT,(int)spikes[i].y,NINJA_WALL_RIGHT,(int)spikes[i].y+ss,NINJA_WALL_RIGHT-ss,(int)spikes[i].y+ss/2,sc);}if(spikes[i].side==ninjaPlayerSide){if(spikes[i].y+ss>ninjaPlayerY&&spikes[i].y<ninjaPlayerY+NINJA_PH){if(spikes[i].isBlue){isRushMode=true;rushStartTime=millis();spikes[i].y=300;}else if(!isRushMode){delay(1000);ninjaGameOver();return;}}}if(spikes[i].y>240){int other=(i==0)?1:0;spikes[i].y=spikes[other].y-random(120,180);spikes[i].side=random(0,2);spikes[i].isBlue=(random(0,12)==1);}}if(showPlayer){tft.fillRect(ninjaX,(int)ninjaPlayerY,NINJA_PW,NINJA_PH,playerColor);tft.fillCircle(ninjaX+NINJA_PW-4,(int)ninjaPlayerY+6,2,TFT_BLACK);tft.fillCircle(ninjaX+NINJA_PW-4,(int)ninjaPlayerY+NINJA_PH-6,2,TFT_BLACK);}updateNinjaScoreDisplay();if(isRushMode&&(millis()-rushStartTime)<rushDuration){tft.setTextSize(1);tft.setCursor(225,200);tft.setTextColor(TFT_YELLOW,BG_COLOR);tft.print("RUSH!");int bw=80,bx=220,by=210;int ep=((millis()-rushStartTime)*100)/rushDuration;tft.fillRect(bx,by,bw,6,TFT_DARKGREY);tft.fillRect(bx,by,bw-(bw*ep/100),6,TFT_RED);unsigned long rt=(rushDuration-(millis()-rushStartTime))/1000;tft.setTextColor(TFT_WHITE,BG_COLOR);tft.setCursor(225,220);tft.print(rt);tft.print("s");}else{tft.fillRect(220,190,100,50,BG_COLOR);}delay(10);}
void runNinjaGame(){static unsigned long lastBPress=0;if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){lastBPress=millis();ninjaPlaying=false;returnToMenu();return;}if(!ninjaPlaying){if(digitalRead(BTN_SELECT)==LOW){delay(200);startNinjaGame();}return;}if(digitalRead(BTN_START)==LOW){delay(200);ninjaPaused=!ninjaPaused;if(ninjaPaused){tft.fillRoundRect(50,90,160,50,10,TFT_BLUE);tft.drawRoundRect(50,90,160,50,10,TFT_WHITE);tft.setTextColor(TFT_WHITE);tft.setTextSize(2);tft.setCursor(90,105);tft.print("PAUSED");tft.setTextSize(1);tft.setCursor(65,125);tft.print("START to Resume");}else{tft.fillRect(50,90,160,50,BG_COLOR);}delay(200);}if(ninjaPaused)return;if(!ninjaPlaying){if(digitalRead(BTN_SELECT)==LOW){delay(200);startNinjaGame();}return;}updateNinjaGame();}

// ================================================================
// =================== PONG =======================================
// ================================================================
#define PONG_TOP     25
#define PONG_BOTTOM  232
#define PONG_LEFT    8
#define PONG_RIGHT   312
#define PADDLE_W     5
#define PADDLE_H     45
#define BALL_SZ      6
#define COLOR_BOT_P  TFT_RED
#define COLOR_YOU_P  TFT_CYAN
#define COLOR_BALL_P TFT_GREEN
#define COLOR_GRID_P 0x4208
#define COLOR_SNOW_P 0x7BEF

struct Snow { float x, y, speed; };
Snow snowflakes[40];

float ballX, ballY, ballDX, ballDY;
float botPaddleY, youPaddleY;
unsigned long pongScore = 0;
unsigned long pongHighScore = 0;
unsigned long lastPongScoreUpdate = 0;
unsigned long lastSnowUpdate = 0;
float pongBallSpeed = 2.0;
bool pongActive = false;
bool pongPaused = false;

void drawPongSnow(){if(millis()-lastSnowUpdate<40)return;lastSnowUpdate=millis();for(int i=0;i<40;i++){tft.drawPixel((int)snowflakes[i].x,(int)snowflakes[i].y,TFT_BLACK);snowflakes[i].y+=snowflakes[i].speed;if(snowflakes[i].y>=screenHeight){snowflakes[i].y=0;snowflakes[i].x=random(0,screenWidth);}tft.drawPixel((int)snowflakes[i].x,(int)snowflakes[i].y,COLOR_SNOW_P);}}
void drawPongElements(){tft.drawRect(PONG_LEFT-2,PONG_TOP-2,PONG_RIGHT-PONG_LEFT+4,PONG_BOTTOM-PONG_TOP+4,TFT_WHITE);for(int y=PONG_TOP;y<PONG_BOTTOM;y+=15)tft.fillRect(screenWidth/2-2,y,4,8,COLOR_GRID_P);tft.fillRect(PONG_LEFT,(int)botPaddleY,PADDLE_W,PADDLE_H,COLOR_BOT_P);tft.fillRect(PONG_RIGHT-PADDLE_W,(int)youPaddleY,PADDLE_W,PADDLE_H,COLOR_YOU_P);tft.fillCircle((int)ballX,(int)ballY,BALL_SZ,COLOR_BALL_P);tft.fillRect(screenWidth/2-1,PONG_TOP,2,PONG_BOTTOM-PONG_TOP,0x39E7);}
void updatePongScoreUI(){tft.fillRect(0,0,screenWidth,22,TFT_BLACK);tft.setTextSize(2);tft.setTextColor(TFT_ORANGE);tft.setCursor(10,2);tft.print("HI:");tft.print(pongHighScore);tft.setTextColor(TFT_WHITE);tft.setCursor(screenWidth-120,2);tft.print("SC:");tft.print(pongScore);}
void resetPongBall(){ballX=screenWidth/2;ballY=(PONG_TOP+PONG_BOTTOM)/2;ballDX=(random(0,2)==0)?pongBallSpeed:-pongBallSpeed;ballDY=((random(-12,13))/10.0);if(abs(ballDY)<0.4)ballDY=(ballDY>0)?0.4:-0.4;pongScore=0;botPaddleY=PONG_TOP+(PONG_BOTTOM-PONG_TOP-PADDLE_H)/2;youPaddleY=botPaddleY;}
void pongGameOver(){pongActive=false;if(pongScore>pongHighScore){pongHighScore=pongScore;menuHighScores[6]=(int)pongHighScore;}tft.fillScreen(TFT_BLACK);int px=(screenWidth-260)/2,py=(screenHeight-130)/2;tft.fillRoundRect(px,py,260,130,15,TFT_WHITE);tft.fillRoundRect(px+4,py+4,252,122,12,TFT_BLACK);tft.setTextSize(3);tft.setTextColor(COLOR_BOT_P);tft.setCursor(px+35,py+30);tft.print("GAME OVER");tft.setTextSize(2);tft.setTextColor(TFT_WHITE);tft.setCursor(px+40,py+65);tft.print("Score: ");tft.print(pongScore);tft.setCursor(px+40,py+90);tft.print("Best:  ");tft.print(pongHighScore);tft.setTextColor(TFT_YELLOW);tft.setTextSize(1);tft.setCursor(px+40,py+115);tft.print("SELECT: Play Again   B: Menu");}
void showStartScreenPong(){tft.fillScreen(TFT_BLACK);tft.setTextSize(5);tft.setTextColor(COLOR_YOU_P);tft.setCursor(screenWidth/2-90,screenHeight/2-50);tft.print("DINO");tft.setCursor(screenWidth/2-85,screenHeight/2+5);tft.print("PONG");tft.setTextSize(2);tft.setTextColor(TFT_WHITE);tft.setCursor(screenWidth/2-95,screenHeight/2+55);tft.print("SELECT: Start");tft.setTextSize(1);tft.setTextColor(TFT_GREEN);tft.setCursor(screenWidth/2-80,screenHeight-35);tft.print("UP/DN: Move  |  B: Back");tft.setCursor(screenWidth/2-65,screenHeight-22);tft.print("Best: ");tft.print(pongHighScore);for(int i=0;i<40;i++){snowflakes[i].x=random(0,screenWidth);snowflakes[i].y=random(0,screenHeight);snowflakes[i].speed=random(1,6)/10.0;}for(int f=0;f<80;f++){drawPongSnow();delay(12);}}
void runPongGame(){static unsigned long lastBPress=0;if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){lastBPress=millis();pongActive=false;returnToMenu();return;}if(!pongActive){drawPongSnow();if(digitalRead(BTN_SELECT)==LOW){delay(60);pongActive=true;pongPaused=false;resetPongBall();tft.fillScreen(TFT_BLACK);updatePongScoreUI();drawPongElements();}return;}if(digitalRead(BTN_SELECT)==LOW){delay(60);pongPaused=!pongPaused;if(pongPaused){tft.setTextSize(3);tft.setTextColor(TFT_YELLOW);tft.setCursor(screenWidth/2-50,screenHeight/2-15);tft.print("PAUSED");}else{tft.fillRect(screenWidth/2-60,screenHeight/2-25,120,50,TFT_BLACK);}while(digitalRead(BTN_SELECT)==LOW)delay(10);}if(pongPaused){delay(10);return;}if(millis()-lastPongScoreUpdate>250){pongScore++;lastPongScoreUpdate=millis();updatePongScoreUI();}int oldBX=(int)ballX,oldBY=(int)ballY,oldBotY=(int)botPaddleY,oldYouY=(int)youPaddleY;if(digitalRead(BTN_UP)==LOW&&youPaddleY>PONG_TOP)youPaddleY-=4.5;if(digitalRead(BTN_DOWN)==LOW&&youPaddleY<PONG_BOTTOM-PADDLE_H)youPaddleY+=4.5;float targetBot=ballY-(PADDLE_H/2);if(targetBot<PONG_TOP)targetBot=PONG_TOP;if(targetBot>PONG_BOTTOM-PADDLE_H)targetBot=PONG_BOTTOM-PADDLE_H;botPaddleY=targetBot;if(youPaddleY<PONG_TOP)youPaddleY=PONG_TOP;if(youPaddleY>PONG_BOTTOM-PADDLE_H)youPaddleY=PONG_BOTTOM-PADDLE_H;ballX+=ballDX;ballY+=ballDY;if(ballY-BALL_SZ<=PONG_TOP){ballY=PONG_TOP+BALL_SZ;ballDY=abs(ballDY);}if(ballY+BALL_SZ>=PONG_BOTTOM){ballY=PONG_BOTTOM-BALL_SZ;ballDY=-abs(ballDY);}if(ballX-BALL_SZ<=PONG_LEFT+PADDLE_W&&ballX+BALL_SZ>=PONG_LEFT&&ballY+BALL_SZ>=botPaddleY&&ballY-BALL_SZ<=botPaddleY+PADDLE_H){float hp=(ballY-botPaddleY)/PADDLE_H;ballDY=(hp-0.5)*4.5;ballDX=abs(ballDX)+0.05;if(abs(ballDY)>3.8)ballDY=(ballDY>0)?3.8:-3.8;if(ballDX>3.0)ballDX=3.0;ballX=PONG_LEFT+PADDLE_W+BALL_SZ;}if(ballX+BALL_SZ>=PONG_RIGHT-PADDLE_W&&ballX-BALL_SZ<=PONG_RIGHT&&ballY+BALL_SZ>=youPaddleY&&ballY-BALL_SZ<=youPaddleY+PADDLE_H){float hp=(ballY-youPaddleY)/PADDLE_H;ballDY=(hp-0.5)*4.5;ballDX=-abs(ballDX)-0.05;if(abs(ballDY)>3.8)ballDY=(ballDY>0)?3.8:-3.8;if(abs(ballDX)>3.0)ballDX=(ballDX>0)?3.0:-3.0;ballX=PONG_RIGHT-PADDLE_W-BALL_SZ;}if(ballX+BALL_SZ<=PONG_LEFT||ballX-BALL_SZ>=PONG_RIGHT){pongGameOver();return;}tft.fillRect(PONG_LEFT,oldBotY,PADDLE_W,PADDLE_H,TFT_BLACK);tft.fillRect(PONG_RIGHT-PADDLE_W,oldYouY,PADDLE_W,PADDLE_H,TFT_BLACK);tft.fillCircle(oldBX,oldBY,BALL_SZ,TFT_BLACK);drawPongSnow();drawPongElements();delay(12);}

// ================================================================
// =================== SPERM RACE =================================
// ================================================================
#define SR_BG_COLOR    0xFDCF
#define SR_SPERM_WHITE 0xFFFF
#define SR_ENEMY_RED   0xF800
#define SR_HEAL_BLUE   0x001F
#define SR_BAR_COLOR   0x0000
#define SR_HEART_RED   0xF800
#define SR_WASTE_RED   0xB820

float srPlayerX = 50, srPlayerY = 150;
float srPrevX = 50, srPrevY = 150;
float srMoveSpeed = 5.5;
float srGameSpeed = 2.5;
unsigned long srScore = 0;
unsigned long srHighScore = 0;
int srLives = 3;
bool srDead = false;
bool srPaused = false;
unsigned long srLastScoreUpdate = 0;
unsigned long srLastButtonPress = 0;

struct SREnemyS { float x, y, prevX, prevY; };
SREnemyS srEnemies[6];

struct SRParticle { float x, y, prevX, prevY; float speed; int size; };
SRParticle srWaste[15];

float srBlueX, srBlueY, srPrevBlueX, srPrevBlueY;
bool srBlueActive = false;
unsigned long srNextBlueSpawn = 5000;
float srTailPhase = 0;

bool srIsBlinking = false;
bool srIsHealing = false;
unsigned long srBlinkTimer = 0;
int srBlinkCount = 0;

float srGetSafeY(int index) {
  float newY; bool safe; int attempts = 0;
  do {
    safe = true; newY = random(35, 220); attempts++;
    for (int i = 0; i < 6; i++)
      if (i != index && abs(newY - srEnemies[i].y) < 28) { safe = false; break; }
  } while (!safe && attempts < 15);
  return newY;
}

void srDrawPixelHeart(int x,int y,uint16_t color){tft.fillRect(x+1,y,3,1,color);tft.fillRect(x+5,y,3,1,color);tft.fillRect(x,y+1,9,3,color);tft.fillRect(x+1,y+4,7,1,color);tft.fillRect(x+2,y+5,5,1,color);tft.fillRect(x+3,y+6,3,1,color);tft.drawPixel(x+4,y+7,color);}
void srUpdateHUD(){tft.fillRect(0,0,320,25,SR_BAR_COLOR);tft.setTextSize(2);tft.setTextColor(TFT_WHITE);tft.setCursor(5,5);tft.print("HI:");tft.print(srHighScore);for(int i=0;i<srLives;i++)srDrawPixelHeart(90+(i*15),8,SR_HEART_RED);tft.setTextColor(TFT_YELLOW);tft.setCursor(250,5);tft.print("S:");tft.print(srScore);}
void srDrawWasteParticle(int x,int y,int size,uint16_t color){if(size==1)tft.drawPixel(x,y,color);else if(size==2)tft.fillRect(x-1,y-1,2,2,color);else tft.fillCircle(x,y,size-1,color);}
void srDrawSperm(int x,int y,uint16_t color){tft.fillCircle(x,y,8,color);if(color!=SR_BG_COLOR){tft.fillCircle(x+2,y-4,2,TFT_BLACK);tft.fillCircle(x+2,y+4,2,TFT_BLACK);tft.fillCircle(x+3,y-5,1,TFT_WHITE);tft.fillCircle(x+3,y+3,1,TFT_WHITE);}for(int i=0;i<18;i++){int sY=y+(int)(sin(srTailPhase+(i*0.5))*(i*1.2));int sX=x-7-(i*1);if(i<12)tft.fillCircle(sX,sY,2,color);else tft.drawPixel(sX,sY,color);}}
void srResetGame(){if(srScore>srHighScore)srHighScore=srScore;tft.fillScreen(SR_BG_COLOR);srScore=0;srLives=3;srGameSpeed=2.5;srPlayerX=50;srPlayerY=150;srDead=false;srPaused=false;srIsBlinking=false;srBlueActive=false;for(int i=0;i<6;i++){srEnemies[i].x=320+(i*55);srEnemies[i].y=srGetSafeY(i);srEnemies[i].prevX=srEnemies[i].x;srEnemies[i].prevY=srEnemies[i].y;}for(int i=0;i<15;i++){srWaste[i].x=random(0,320);srWaste[i].y=random(25,230);srWaste[i].speed=random(8,25)/10.0;srWaste[i].size=random(1,3);}srUpdateHUD();}
void srGameOver(){delay(500);srDead=true;if(srScore>srHighScore){srHighScore=srScore;menuHighScores[7]=(int)srHighScore;}tft.fillRoundRect(40,70,240,100,10,TFT_BLACK);tft.drawRoundRect(40,70,240,100,10,TFT_WHITE);tft.setTextColor(TFT_RED);tft.setTextSize(3);tft.setCursor(70,85);tft.print("GAME OVER");tft.setTextSize(2);tft.setTextColor(TFT_YELLOW);tft.setCursor(70,115);tft.print("HI: ");tft.print(srHighScore);tft.setTextColor(TFT_WHITE);tft.setCursor(70,140);tft.print("SC: ");tft.print(srScore);tft.setTextColor(0x07FF);tft.setTextSize(1);tft.setCursor(85,160);tft.print("[ SELECT: RETRY  B: MENU ]");}
void showStartScreenSperm(){tft.fillScreen(SR_BG_COLOR);tft.setTextColor(TFT_RED);tft.setTextSize(2);tft.setCursor(60,30);tft.print("SPERM RACE");tft.setTextColor(TFT_BLACK);tft.setTextSize(1);tft.setCursor(30,70);tft.print("Dodge RED enemies!");tft.setCursor(30,85);tft.print("Catch BLUE balls for extra life!");tft.setCursor(30,100);tft.print("UP/DOWN/LEFT/RIGHT: Move");tft.setCursor(30,115);tft.print("SELECT: Pause  B: Menu");tft.setTextColor(TFT_RED);tft.setCursor(60,145);tft.print("Press SELECT to start");while(digitalRead(BTN_SELECT)==HIGH&&digitalRead(BTN_B)==HIGH)delay(10);if(digitalRead(BTN_B)==LOW){delay(200);returnToMenu();return;}delay(300);srResetGame();}
void runSpermRaceGame(){static unsigned long lastBPress=0;if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){lastBPress=millis();returnToMenu();return;}if(digitalRead(BTN_SELECT)==LOW&&millis()-srLastButtonPress>300){srLastButtonPress=millis();if(srDead)srResetGame();else{srPaused=!srPaused;if(srPaused){tft.fillRoundRect(85,100,150,40,5,TFT_BLACK);tft.drawRoundRect(85,100,150,40,5,TFT_WHITE);tft.setTextColor(TFT_YELLOW);tft.setTextSize(2);tft.setCursor(115,115);tft.print("PAUSED");}else{tft.fillScreen(SR_BG_COLOR);srUpdateHUD();}}}if(srPaused||srDead)return;if(millis()-srLastScoreUpdate>250){srScore++;srLastScoreUpdate=millis();srUpdateHUD();if(srGameSpeed<7.0)srGameSpeed+=0.001;}for(int i=0;i<15;i++){srDrawWasteParticle((int)srWaste[i].prevX,(int)srWaste[i].prevY,srWaste[i].size,SR_BG_COLOR);srWaste[i].prevX=srWaste[i].x;srWaste[i].prevY=srWaste[i].y;srWaste[i].x-=srWaste[i].speed;if(srWaste[i].x<0){srWaste[i].x=320;srWaste[i].y=random(25,230);srWaste[i].speed=random(8,25)/10.0;}srDrawWasteParticle((int)srWaste[i].x,(int)srWaste[i].y,srWaste[i].size,SR_WASTE_RED);}srPrevX=srPlayerX;srPrevY=srPlayerY;if(digitalRead(BTN_UP)==LOW)srPlayerY-=srMoveSpeed;if(digitalRead(BTN_DOWN)==LOW)srPlayerY+=srMoveSpeed;if(digitalRead(BTN_LEFT)==LOW)srPlayerX-=srMoveSpeed;if(digitalRead(BTN_RIGHT)==LOW)srPlayerX+=srMoveSpeed;if(srPlayerX<25)srPlayerX=25;if(srPlayerX>295)srPlayerX=295;if(srPlayerY<35)srPlayerY=35;if(srPlayerY>225)srPlayerY=225;srDrawSperm((int)srPrevX,(int)srPrevY,SR_BG_COLOR);for(int i=0;i<6;i++){tft.fillCircle((int)srEnemies[i].prevX,(int)srEnemies[i].prevY,10,SR_BG_COLOR);srEnemies[i].prevX=srEnemies[i].x;srEnemies[i].prevY=srEnemies[i].y;srEnemies[i].x-=srGameSpeed;if(srEnemies[i].x<-15){srEnemies[i].x=335;srEnemies[i].y=srGetSafeY(i);}tft.fillCircle((int)srEnemies[i].x,(int)srEnemies[i].y,10,SR_ENEMY_RED);tft.fillCircle((int)srEnemies[i].x-3,(int)srEnemies[i].y-3,2,TFT_BLACK);tft.fillCircle((int)srEnemies[i].x+3,(int)srEnemies[i].y-3,2,TFT_BLACK);tft.fillCircle((int)srEnemies[i].x,(int)srEnemies[i].y+2,2,TFT_BLACK);float dx=srPlayerX-srEnemies[i].x,dy=srPlayerY-srEnemies[i].y;if(dx*dx+dy*dy<196&&!srIsBlinking){srLives--;srUpdateHUD();tft.fillCircle((int)srEnemies[i].x,(int)srEnemies[i].y,12,SR_BG_COLOR);srEnemies[i].x=335;srEnemies[i].y=srGetSafeY(i);if(srLives<=0){srGameOver();return;}else{srIsBlinking=true;srIsHealing=false;srBlinkCount=0;srBlinkTimer=millis();}}}if(!srBlueActive&&millis()>srNextBlueSpawn){srBlueActive=true;srBlueX=335;srBlueY=random(35,210);srPrevBlueX=srBlueX;srPrevBlueY=srBlueY;}if(srBlueActive){tft.fillCircle((int)srPrevBlueX,(int)srPrevBlueY,10,SR_BG_COLOR);srPrevBlueX=srBlueX;srPrevBlueY=srBlueY;srBlueX-=(srGameSpeed+0.5);if(srBlueX<-15){srBlueActive=false;srNextBlueSpawn=millis()+random(5000,10000);}else{tft.fillCircle((int)srBlueX,(int)srBlueY,10,SR_HEAL_BLUE);tft.fillCircle((int)srBlueX,(int)srBlueY,5,TFT_CYAN);tft.fillCircle((int)srBlueX,(int)srBlueY,2,TFT_WHITE);float bdx=srPlayerX-srBlueX,bdy=srPlayerY-srBlueY;if(bdx*bdx+bdy*bdy<196&&!srIsBlinking){if(srLives<5)srLives++;srUpdateHUD();srBlueActive=false;tft.fillCircle((int)srBlueX,(int)srBlueY,12,SR_BG_COLOR);srNextBlueSpawn=millis()+random(8000,12000);srIsBlinking=true;srIsHealing=true;srBlinkCount=0;srBlinkTimer=millis();}}}srTailPhase+=0.5;if(srIsBlinking){if(millis()-srBlinkTimer>80){srBlinkTimer=millis();srBlinkCount++;}if(srBlinkCount<6){uint16_t fc=srIsHealing?((srBlinkCount%2==0)?SR_HEAL_BLUE:SR_SPERM_WHITE):((srBlinkCount%2==0)?SR_ENEMY_RED:SR_SPERM_WHITE);srDrawSperm((int)srPlayerX,(int)srPlayerY,fc);}else{srIsBlinking=false;srDrawSperm((int)srPlayerX,(int)srPlayerY,SR_SPERM_WHITE);}}else srDrawSperm((int)srPlayerX,(int)srPlayerY,SR_SPERM_WHITE);delay(16);}

// ================================================================
// =================== CAR RACING =================================
// ================================================================
#define CR_ROAD_GRAY   TFT_DARKGREY
#define CR_GRASS_BLACK TFT_BLACK
#define CR_LINE_YELLOW TFT_YELLOW
#define CR_CURB_RED    TFT_RED
#define CR_WINDOW_BLUE TFT_CYAN
#define CR_RACER_RED   TFT_RED
#define CR_TEXT_COLOR  TFT_WHITE

const int crLaneX[] = {80, 160, 240};
int crPlayerLane = 1;
float crCurrentPlayerX = 160;
int crTargetX = 160;
float crSlideSpeed = 6.0;
int crScore = 0;
bool crGameOver = false;
bool crIsPaused = false;
int crLineOffset = 0;
int crHighScore = 0;
float crEnemyY = -80;
int crEnemyLane = 1;
int crEnemyType = 0;
float crGameSpeed = 4.5;

int crVehiclePoints[]  = {3, 4, 8, 10, 12};
int crVehicleHeights[] = {32, 32, 32, 40, 58};

unsigned long crLastButtonPress = 0;
const int crDebounceDelay = 200;
unsigned long crLeftPressStart = 0;
unsigned long crRightPressStart = 0;
bool crLeftProcessing = false;
bool crRightProcessing = false;
const int crLongPressTime = 170;
bool crLastLeftState = HIGH, crLastRightState = HIGH;
bool crLastSelectState = HIGH, crLastStartState = HIGH;

void crDrawRoadBase(){tft.fillRect(30,0,260,240,CR_ROAD_GRAY);for(int i=0;i<240;i+=20){tft.fillRect(30,i,5,10,TFT_WHITE);tft.fillRect(30,i+10,5,10,CR_CURB_RED);tft.fillRect(285,i,5,10,TFT_WHITE);tft.fillRect(285,i+10,5,10,CR_CURB_RED);}}
void crDrawPlayer(float xPos){static float crLastX=160;int x=(int)xPos-15,lx=(int)crLastX-15,y=198;if(lx!=x){tft.fillRect(lx-5,y-5,40,50,CR_ROAD_GRAY);if(lx<=35){for(int i=0;i<240;i+=20){tft.fillRect(30,i,5,10,TFT_WHITE);tft.fillRect(30,i+10,5,10,CR_CURB_RED);}}if(lx+35>=285){for(int i=0;i<240;i+=20){tft.fillRect(285,i,5,10,TFT_WHITE);tft.fillRect(285,i+10,5,10,CR_CURB_RED);}}}tft.fillRoundRect(x+4,y+4,30,40,5,TFT_DARKGREY);tft.fillRoundRect(x,y,30,40,5,CR_RACER_RED);tft.fillRect(x+6,y+8,18,8,CR_WINDOW_BLUE);tft.fillRect(x+3,y+18,24,2,CR_WINDOW_BLUE);tft.fillRect(x+2,y+2,5,4,TFT_YELLOW);tft.fillRect(x+23,y+2,5,4,TFT_YELLOW);tft.fillRect(x+2,y+34,5,4,TFT_RED);tft.fillRect(x+23,y+34,5,4,TFT_RED);tft.fillRect(x-2,y+8,4,12,TFT_BLACK);tft.fillRect(x+28,y+8,4,12,TFT_BLACK);tft.fillRect(x-2,y+24,4,12,TFT_BLACK);tft.fillRect(x+28,y+24,4,12,TFT_BLACK);crLastX=xPos;}
void crDrawEnemy(int lane,int y,int type){int x=crLaneX[lane]-18;switch(type){case 0:tft.fillRoundRect(x+4,y,28,40,5,TFT_BLUE);tft.fillRect(x+8,y+8,20,10,TFT_BLACK);tft.fillRect(x+8,y+28,20,4,TFT_BLACK);tft.fillRect(x+6,y,6,4,TFT_WHITE);tft.fillRect(x+24,y,6,4,TFT_WHITE);tft.fillRect(x+6,y+38,6,4,TFT_RED);tft.fillRect(x+24,y+38,6,4,TFT_RED);break;case 1:tft.fillRoundRect(x+4,y,28,40,5,TFT_GREEN);tft.fillRect(x+16,y,6,40,TFT_BLACK);tft.fillRect(x+7,y+10,22,8,TFT_BLACK);tft.fillRect(x+7,y+28,22,3,TFT_BLACK);tft.fillRect(x+6,y,5,4,TFT_WHITE);tft.fillRect(x+25,y,5,4,TFT_WHITE);break;case 2:tft.fillRect(x+10,y,16,12,TFT_BLACK);tft.fillRect(x+10,y+36,16,12,TFT_BLACK);tft.fillRoundRect(x+11,y+8,14,32,4,TFT_RED);tft.fillRect(x+13,y+12,10,24,TFT_DARKGREY);tft.fillCircle(x+17,y+18,8,TFT_BLACK);tft.fillRect(x+5,y+20,26,8,TFT_OLIVE);tft.fillCircle(x+17,y+36,6,TFT_BLACK);break;case 3:tft.fillRoundRect(x+4,y,28,44,5,TFT_WHITE);tft.fillRect(x+8,y+2,6,3,TFT_RED);tft.fillRect(x+18,y+2,6,3,TFT_BLUE);tft.fillRect(x+7,y+8,22,8,TFT_BLACK);tft.fillRect(x+11,y+24,14,3,TFT_RED);tft.fillRect(x+16,y+19,3,13,TFT_RED);tft.fillRect(x+6,y+18,2,10,TFT_CYAN);tft.fillRect(x+28,y+18,2,10,TFT_CYAN);break;case 4:tft.fillRoundRect(x,y,36,60,3,TFT_LIGHTGREY);tft.fillRect(x,y,36,40,TFT_ORANGE);tft.fillRect(x+5,y+44,26,12,TFT_BLACK);tft.fillRect(x+3,y+58,6,3,TFT_YELLOW);tft.fillRect(x+27,y+58,6,3,TFT_YELLOW);tft.fillRect(x-3,y+44,4,12,TFT_BLACK);tft.fillRect(x+35,y+44,4,12,TFT_BLACK);break;}}
void crTriggerCrash(int xPos){int yPos=210;for(int i=0;i<360;i+=45){float a=i*0.017453;tft.fillTriangle(xPos,yPos,xPos+(int)(cos(a)*35),yPos+(int)(sin(a)*35),xPos+8,yPos+8,TFT_ORANGE);}delay(100);for(int i=22;i<382;i+=45){float a=i*0.017453;tft.fillTriangle(xPos,yPos,xPos+(int)(cos(a)*25),yPos+(int)(sin(a)*25),xPos-5,yPos+5,TFT_YELLOW);}delay(100);tft.fillCircle(xPos,yPos,12,TFT_WHITE);delay(50);for(int i=0;i<30;i++)tft.drawPixel(xPos+random(-30,30),yPos+random(-30,30),TFT_WHITE);delay(50);for(int i=0;i<5;i++){tft.invertDisplay(true);delay(30);tft.invertDisplay(false);delay(30);}}
void crShowGameOver(){if(crScore>crHighScore){crHighScore=crScore;menuHighScores[8]=crHighScore;EEPROM.write(1,crHighScore);EEPROM.commit();}delay(500);tft.fillScreen(TFT_BLACK);tft.drawRect(10,10,300,220,TFT_YELLOW);tft.drawRect(12,12,296,216,TFT_ORANGE);tft.setTextColor(TFT_RED);tft.setTextSize(3);tft.setCursor(65,50);tft.print("GAME OVER!");tft.setTextSize(2);tft.setTextColor(TFT_GREEN);tft.setCursor(90,100);tft.print("SCORE: ");tft.setTextColor(TFT_WHITE);tft.print(crScore);tft.setTextColor(TFT_YELLOW);tft.setCursor(90,130);tft.print("HIGH:  ");tft.setTextColor(TFT_WHITE);tft.print(crHighScore);tft.setTextSize(1);tft.setTextColor(TFT_CYAN);tft.setCursor(70,180);tft.print("PRESS SELECT TO PLAY");}
void crHandleLaneChange(){bool leftPressed=(digitalRead(BTN_LEFT)==LOW);if(leftPressed&&!crLeftProcessing&&millis()-crLastButtonPress>crDebounceDelay){crLeftPressStart=millis();crLeftProcessing=true;}if(crLeftProcessing&&leftPressed&&millis()-crLeftPressStart>=crLongPressTime){crPlayerLane=0;crTargetX=crLaneX[crPlayerLane];crLeftProcessing=false;crLastButtonPress=millis();}if(crLeftProcessing&&!leftPressed){if(millis()-crLeftPressStart<crLongPressTime&&crPlayerLane>0){crPlayerLane--;crTargetX=crLaneX[crPlayerLane];}crLeftProcessing=false;crLeftPressStart=0;}bool rightPressed=(digitalRead(BTN_RIGHT)==LOW);if(rightPressed&&!crRightProcessing&&millis()-crLastButtonPress>crDebounceDelay){crRightPressStart=millis();crRightProcessing=true;}if(crRightProcessing&&rightPressed&&millis()-crRightPressStart>=crLongPressTime){crPlayerLane=2;crTargetX=crLaneX[crPlayerLane];crRightProcessing=false;crLastButtonPress=millis();}if(crRightProcessing&&!rightPressed){if(millis()-crRightPressStart<crLongPressTime&&crPlayerLane<2){crPlayerLane++;crTargetX=crLaneX[crPlayerLane];}crRightProcessing=false;crRightPressStart=0;}}
void crResetGame(){tft.fillScreen(CR_GRASS_BLACK);crDrawRoadBase();crPlayerLane=1;crCurrentPlayerX=crLaneX[crPlayerLane];crTargetX=crLaneX[crPlayerLane];crEnemyY=-80;crEnemyType=random(0,5);crEnemyLane=random(0,3);crScore=0;crGameSpeed=3.8;crGameOver=false;crIsPaused=false;crLineOffset=0;crLeftProcessing=false;crRightProcessing=false;crLeftPressStart=0;crRightPressStart=0;}
void showStartScreenCar(){tft.fillScreen(CR_GRASS_BLACK);tft.setTextColor(CR_LINE_YELLOW);tft.setTextSize(3);tft.setCursor(70,60);tft.print("CAR RACING");tft.setTextSize(1);tft.setTextColor(CR_TEXT_COLOR);tft.setCursor(80,110);tft.print("PRESS SELECT TO START");tft.setCursor(50,130);tft.print("LEFT/RIGHT: MOVE LANES");tft.setCursor(70,150);tft.print("SELECT: PAUSE/RESUME");tft.setCursor(70,170);tft.print("B: BACK TO MENU");while(digitalRead(BTN_SELECT)==HIGH&&digitalRead(BTN_B)==HIGH)delay(10);if(digitalRead(BTN_B)==LOW){delay(200);returnToMenu();return;}delay(300);crResetGame();}
void runCarRacingGame(){if(crGameOver){if(digitalRead(BTN_SELECT)==LOW){delay(200);crResetGame();}if(digitalRead(BTN_B)==LOW){delay(200);returnToMenu();}return;}bool selPress=digitalRead(BTN_SELECT)==LOW&&millis()-crLastButtonPress>crDebounceDelay;bool staPress=digitalRead(BTN_START)==LOW&&millis()-crLastButtonPress>crDebounceDelay;if(selPress||staPress){crLastButtonPress=millis();crIsPaused=!crIsPaused;if(crIsPaused){tft.fillRoundRect(80,90,160,50,10,TFT_BLUE);tft.drawRoundRect(80,90,160,50,10,TFT_WHITE);tft.setTextColor(TFT_WHITE);tft.setTextSize(2);tft.setCursor(125,105);tft.print("PAUSED");tft.setTextSize(1);tft.setCursor(115,125);tft.print("PRESS START");}else{tft.fillRect(80,90,160,50,CR_ROAD_GRAY);crDrawRoadBase();}}if(digitalRead(BTN_B)==LOW&&millis()-crLastButtonPress>crDebounceDelay){crLastButtonPress=millis();crGameOver=true;returnToMenu();return;}if(crIsPaused){delay(10);return;}crHandleLaneChange();if(abs(crCurrentPlayerX-crTargetX)>2){if(crCurrentPlayerX<crTargetX)crCurrentPlayerX+=crSlideSpeed;else crCurrentPlayerX-=crSlideSpeed;}else crCurrentPlayerX=crTargetX;tft.fillRect(crLaneX[crEnemyLane]-25,(int)crEnemyY-5,50,(int)crGameSpeed+8,CR_ROAD_GRAY);crEnemyY+=crGameSpeed;if(crEnemyY>250){tft.fillRect(crLaneX[crEnemyLane]-25,200,50,40,CR_ROAD_GRAY);crScore+=crVehiclePoints[crEnemyType];crEnemyY=-80;crEnemyLane=random(0,3);crEnemyType=random(0,5);crGameSpeed+=0.05;if(crGameSpeed>8.0)crGameSpeed=8.0;}if(abs(crCurrentPlayerX-crLaneX[crEnemyLane])<25&&(crEnemyY+crVehicleHeights[crEnemyType])>195&&crEnemyY<225){crTriggerCrash((int)crCurrentPlayerX);crGameOver=true;crShowGameOver();return;}crLineOffset=(crLineOffset+(int)crGameSpeed)%48;for(int i=-48;i<240;i+=48){tft.fillRect(115,i+crLineOffset,3,24,CR_LINE_YELLOW);tft.fillRect(202,i+crLineOffset,3,24,CR_LINE_YELLOW);tft.fillRect(115,i+crLineOffset-6,3,6,CR_ROAD_GRAY);tft.fillRect(202,i+crLineOffset-6,3,6,CR_ROAD_GRAY);}crDrawPlayer(crCurrentPlayerX);crDrawEnemy(crEnemyLane,(int)crEnemyY,crEnemyType);tft.fillRect(250,5,65,15,CR_GRASS_BLACK);tft.setTextColor(TFT_WHITE);tft.setTextSize(1);tft.setCursor(255,8);tft.print("S:");tft.setTextColor(CR_LINE_YELLOW);tft.print(crScore);delay(16);}

// ================================================================
// =================== RTEX (DINO RUN) ============================
// ================================================================
const unsigned char rtex_dino_run1[] PROGMEM = {
  0x00,0x00,0x07,0xf0,0x0f,0xf8,0x0f,0xf8,0x0f,0xf8,0x0f,0x80,0x0f,0xf8,0x0e,0x00,
  0x0f,0xe0,0x0f,0xe0,0x9f,0xe0,0xdf,0xe0,0xff,0xe0,0xff,0xe0,0xff,0xc0,0xff,0x80,
  0x7f,0x00,0x3e,0x00,0x22,0x00,0x22,0x00,0x62,0x00
};
const unsigned char rtex_dino_run2[] PROGMEM = {
  0x00,0x00,0x07,0xf0,0x0f,0xf8,0x0f,0xf8,0x0f,0xf8,0x0f,0x80,0x0f,0xf8,0x0e,0x00,
  0x0f,0xe0,0x0f,0xe0,0x9f,0xe0,0xdf,0xe0,0xff,0xe0,0xff,0xe0,0xff,0xc0,0xff,0x80,
  0x7f,0x00,0x3e,0x00,0x3c,0x00,0x06,0x00,0x06,0x00
};
const unsigned char rtex_cactus_small[] PROGMEM = {
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0xdf,0x00, 0xdf,0x00, 0xdf,0x00, 0xdf,0x00, 0xff,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00
};
const unsigned char rtex_cactus_large[] PROGMEM = {
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0xdd,0x00, 0xdd,0x00, 0xdd,0x00, 0xdd,0x00, 0xdd,0x00,
  0xff,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00
};
const unsigned char rtex_cactus_double[] PROGMEM = {
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00,
  0xdf,0xfb,0x00, 0xdf,0xfb,0x00, 0xdf,0xfb,0x00, 0xff,0xff,0x00, 0x1c,0x38,0x00,
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00,
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00
};
const unsigned char rtex_cloud_big[] PROGMEM = {
  0x00,0xfe,0x00, 0x07,0xff,0x80, 0x1f,0xff,0xe0, 0x3f,0xff,0xf0,
  0x7f,0xff,0xf8, 0x7f,0xff,0xf8, 0x7f,0xff,0xf8, 0x3f,0xff,0xf0,
  0x1f,0xff,0xe0, 0x00,0x00,0x00
};

#define RTEX_DINO_SCALE    2
#define RTEX_CACTUS_SCALE  3
#define RTEX_DINO_W   16
#define RTEX_DINO_H   21
#define RTEX_CS_W     12
#define RTEX_CS_H     18
#define RTEX_CL_W     12
#define RTEX_CL_H     24
#define RTEX_CD_W     24
#define RTEX_CD_H     19
#define RTEX_CLOUD_W  24
#define RTEX_CLOUD_H  10
#define RTEX_DINO_SW  (RTEX_DINO_W * RTEX_DINO_SCALE)
#define RTEX_DINO_SH  (RTEX_DINO_H * RTEX_DINO_SCALE)
#define RTEX_CS_SW    (RTEX_CS_W * RTEX_CACTUS_SCALE)
#define RTEX_CS_SH    (RTEX_CS_H * RTEX_CACTUS_SCALE)
#define RTEX_CL_SW    (RTEX_CL_W * RTEX_CACTUS_SCALE)
#define RTEX_CL_SH    (RTEX_CL_H * RTEX_CACTUS_SCALE)
#define RTEX_CD_SW    (RTEX_CD_W * RTEX_CACTUS_SCALE)
#define RTEX_CD_SH    (RTEX_CD_H * RTEX_CACTUS_SCALE)
#define RTEX_DINO_X   40
#define RTEX_GROUND_Y 210
#define RTEX_SCR_W    320
#define RTEX_SCR_H    240

#define RTEX_DAY_BG    0xFFFF
#define RTEX_DAY_FG    0x0000
#define RTEX_NIGHT_BG  0x000F
#define RTEX_NIGHT_FG  0xFFFF
#define RTEX_COL_STAR  0xC618
#define RTEX_COL_MOON  0xFFE0

int   rtexDinoY, rtexPrevDinoY;
float rtexJumpVel;
const float RTEX_GRAVITY = 1.35;
float rtexCactusX, rtexPrevCactusX;
float rtexGameSpeed;
float rtexScoreF;
int   rtexHighScore = 0;
bool  rtexIsJumping, rtexPlaying;
bool  rtexIsPaused = false;
int   rtexCactusType;
uint16_t rtexBgColor, rtexFgColor;

float rtexCloud1X, rtexCloud1Y, rtexCloud2X, rtexCloud2Y;
float rtexPrevCloud1X, rtexPrevCloud2X;

#define RTEX_NUM_DOTS 25
float rtexDotX[RTEX_NUM_DOTS];
int   rtexPrevDotX[RTEX_NUM_DOTS];
int   rtexDotY[RTEX_NUM_DOTS];

float rtexBumpX[3];
int   rtexPrevBumpX[3];

int   rtexDinoFrame = 0;
unsigned long rtexLastFrameTime = 0;
const unsigned long RTEX_JUMP_DEBOUNCE = 80;

#define RTEX_NUM_STARS 40
float rtexStarX[RTEX_NUM_STARS];
float rtexStarY[RTEX_NUM_STARS];
int   rtexStarTwinkle[RTEX_NUM_STARS];
unsigned long rtexLastStarTwinkle[RTEX_NUM_STARS];
int rtexMoonX = 280, rtexMoonY = 35;

float rtexSavedSpeed, rtexSavedCactusX, rtexSavedScore;
float rtexSavedCloud1X, rtexSavedCloud2X;
float rtexSavedDotX[RTEX_NUM_DOTS];
float rtexSavedBumpX[3];
int   rtexSavedCactusType;

#define RTEX_EEPROM_ADDR  10
#define RTEX_EEPROM_MAGIC 12

void rtexDrawBitmapScaled(int x, int y, const uint8_t* bmp,
                           int bmpW, int bmpH, uint16_t color, uint16_t bg,
                           int scale, bool transparent = true)
{
  int bytesPerRow = (bmpW + 7) / 8;
  for (int row = 0; row < bmpH; row++) {
    for (int col = 0; col < bmpW; col++) {
      int byteIndex = row * bytesPerRow + col / 8;
      uint8_t b = pgm_read_byte(&bmp[byteIndex]);
      bool set = (b >> (7 - (col % 8))) & 1;
      if (set)
        tft.fillRect(x + col * scale, y + row * scale, scale, scale, color);
      else if (!transparent)
        tft.fillRect(x + col * scale, y + row * scale, scale, scale, bg);
    }
  }
}

void rtexDrawDino(int x, int y, const uint8_t* bmp) {
  rtexDrawBitmapScaled(x, y, bmp, RTEX_DINO_W, RTEX_DINO_H, rtexFgColor, rtexBgColor, RTEX_DINO_SCALE);
}
void rtexDrawCactus(int x, int y, const uint8_t* bmp, int w, int h) {
  rtexDrawBitmapScaled(x, y, bmp, w, h, rtexFgColor, rtexBgColor, RTEX_CACTUS_SCALE);
}
void rtexDrawCloud(int x, int y) {
  uint16_t cloudCol = (rtexVersion == 0) ? (uint16_t)0x4208 : (uint16_t)0x4208;
  rtexDrawBitmapScaled(x, y, rtex_cloud_big, RTEX_CLOUD_W, RTEX_CLOUD_H, cloudCol, rtexBgColor, RTEX_DINO_SCALE);
}
int rtexCactusDrawY(int origH) { return RTEX_GROUND_Y - (origH * RTEX_CACTUS_SCALE); }
void rtexDrawBigBump(int x) {
  tft.fillRect(x,     RTEX_GROUND_Y - 4, 16, 2, rtexFgColor);
  tft.fillRect(x - 2, RTEX_GROUND_Y - 2, 20, 2, rtexFgColor);
}
void rtexEraseDino()    { tft.fillRect(RTEX_DINO_X, rtexPrevDinoY, RTEX_DINO_SW, RTEX_DINO_SH, rtexBgColor); }
void rtexEraseCactus()  { tft.fillRect((int)rtexPrevCactusX, RTEX_GROUND_Y - RTEX_CL_SH, RTEX_CD_SW + 10, RTEX_CL_SH + 5, rtexBgColor); }
void rtexEraseClouds()  {
  tft.fillRect((int)rtexPrevCloud1X, (int)rtexCloud1Y, RTEX_CLOUD_W * RTEX_DINO_SCALE, RTEX_CLOUD_H * RTEX_DINO_SCALE, rtexBgColor);
  tft.fillRect((int)rtexPrevCloud2X, (int)rtexCloud2Y, RTEX_CLOUD_W * RTEX_DINO_SCALE, RTEX_CLOUD_H * RTEX_DINO_SCALE, rtexBgColor);
}
void rtexEraseBumps()   { for(int i=0;i<3;i++) tft.fillRect(rtexPrevBumpX[i]-4, RTEX_GROUND_Y-5, 26, 6, rtexBgColor); }
void rtexEraseDots()    { for(int i=0;i<RTEX_NUM_DOTS;i++) tft.fillRect(rtexPrevDotX[i], rtexDotY[i], 2, 2, rtexBgColor); }

void rtexInitStars() {
  for (int i = 0; i < RTEX_NUM_STARS; i++) {
    rtexStarX[i] = random(0, RTEX_SCR_W);
    rtexStarY[i] = random(5, 180);
    rtexStarTwinkle[i] = random(0, 3);
    rtexLastStarTwinkle[i] = millis();
  }
}

void rtexDrawStars() {
  for (int i = 0; i < RTEX_NUM_STARS; i++) {
    if (millis() - rtexLastStarTwinkle[i] > (unsigned long)random(1000, 5000)) {
      rtexStarTwinkle[i] = random(0, 3);
      rtexLastStarTwinkle[i] = millis();
    }
    uint16_t sc = (rtexStarTwinkle[i] == 1) ? (uint16_t)0xFFFF : (uint16_t)RTEX_COL_STAR;
    if (rtexStarTwinkle[i] == 2) sc = 0x8410;
    tft.drawPixel((int)rtexStarX[i], (int)rtexStarY[i], sc);
  }
}

void rtexDrawMoon() {
  tft.fillCircle(rtexMoonX, rtexMoonY, 14, RTEX_COL_MOON);
  tft.fillCircle(rtexMoonX + 5, rtexMoonY - 3, 11, rtexBgColor);
}

void rtexSaveState() {
  rtexSavedSpeed    = rtexGameSpeed;
  rtexSavedCactusX  = rtexCactusX;
  rtexSavedScore    = rtexScoreF;
  rtexSavedCloud1X  = rtexCloud1X;
  rtexSavedCloud2X  = rtexCloud2X;
  rtexSavedCactusType = rtexCactusType;
  for(int i=0;i<RTEX_NUM_DOTS;i++) { rtexSavedDotX[i] = rtexDotX[i]; }
  for(int i=0;i<3;i++)             { rtexSavedBumpX[i] = rtexBumpX[i]; }
}

void rtexRestoreState() {
  rtexGameSpeed   = rtexSavedSpeed;
  rtexCactusX     = rtexSavedCactusX;
  rtexScoreF      = rtexSavedScore;
  rtexCloud1X     = rtexSavedCloud1X;
  rtexCloud2X     = rtexSavedCloud2X;
  rtexCactusType  = rtexSavedCactusType;
  for(int i=0;i<RTEX_NUM_DOTS;i++) { rtexDotX[i] = rtexSavedDotX[i]; rtexPrevDotX[i] = (int)rtexSavedDotX[i]; }
  for(int i=0;i<3;i++)             { rtexBumpX[i] = rtexSavedBumpX[i]; rtexPrevBumpX[i] = (int)rtexSavedBumpX[i]; }
}

void rtexSaveHighScore() {
  EEPROM.write(RTEX_EEPROM_MAGIC, 0xBB);
  EEPROM.write(RTEX_EEPROM_ADDR,     rtexHighScore & 0xFF);
  EEPROM.write(RTEX_EEPROM_ADDR + 1, (rtexHighScore >> 8) & 0xFF);
  EEPROM.commit();
}

void rtexLoadHighScore() {
  if (EEPROM.read(RTEX_EEPROM_MAGIC) == 0xBB) {
    int lo = EEPROM.read(RTEX_EEPROM_ADDR);
    int hi = EEPROM.read(RTEX_EEPROM_ADDR + 1);
    rtexHighScore = lo | (hi << 8);
    if (rtexHighScore > 9999 || rtexHighScore < 0) { rtexHighScore = 0; rtexSaveHighScore(); }
  } else {
    rtexHighScore = 0; rtexSaveHighScore();
  }
}

void rtexStartGame(int version) {
  rtexPlaying    = true;
  rtexIsPaused   = false;
  rtexScoreF     = 0;
  rtexGameSpeed  = 6.5;
  rtexCactusX    = RTEX_SCR_W;
  rtexDinoY      = RTEX_GROUND_Y - RTEX_DINO_SH;
  rtexPrevDinoY  = rtexDinoY;
  rtexJumpVel    = 0;
  rtexIsJumping  = false;
  rtexDinoFrame  = 0;
  rtexCactusType = random(0, 3);

  if (version == 0) { rtexBgColor = RTEX_DAY_BG;   rtexFgColor = RTEX_DAY_FG; }
  else              { rtexBgColor = RTEX_NIGHT_BG;  rtexFgColor = RTEX_NIGHT_FG; }

  rtexCloud1X = 100; rtexCloud1Y = 40;
  rtexCloud2X = 260; rtexCloud2Y = 70;
  rtexPrevCloud1X = rtexCloud1X; rtexPrevCloud2X = rtexCloud2X;

  for(int i=0;i<3;i++) { rtexBumpX[i] = RTEX_SCR_W + i*200.0; rtexPrevBumpX[i]=(int)rtexBumpX[i]; }
  for(int i=0;i<RTEX_NUM_DOTS;i++) {
    rtexDotX[i]    = random(0, RTEX_SCR_W);
    rtexDotY[i]    = random(RTEX_GROUND_Y+2, RTEX_GROUND_Y+12);
    rtexPrevDotX[i]= (int)rtexDotX[i];
  }

  if (version == 1) rtexInitStars();

  tft.fillScreen(rtexBgColor);
  tft.drawFastHLine(0, RTEX_GROUND_Y, RTEX_SCR_W, rtexFgColor);
  if (version == 1) rtexDrawMoon();
}

void rtexDrawFrame() {
  rtexEraseDino(); rtexEraseCactus(); rtexEraseClouds(); rtexEraseBumps(); rtexEraseDots();
  tft.fillRect(0, 0, RTEX_SCR_W, 22, rtexBgColor);
  tft.drawFastHLine(0, RTEX_GROUND_Y, RTEX_SCR_W, rtexFgColor);

  if (rtexVersion == 1) { rtexDrawStars(); rtexDrawMoon(); }

  for(int i=0;i<RTEX_NUM_DOTS;i++) {
    int dx=(int)rtexDotX[i];
    if(dx>0&&dx<RTEX_SCR_W-2) tft.fillRect(dx, rtexDotY[i], 2, 2, rtexFgColor);
  }
  for(int i=0;i<3;i++) {
    int bx=(int)rtexBumpX[i];
    if(bx>0&&bx<RTEX_SCR_W-20) rtexDrawBigBump(bx);
  }

  rtexDrawCloud((int)rtexCloud1X,(int)rtexCloud1Y);
  rtexDrawCloud((int)rtexCloud2X,(int)rtexCloud2Y);

  const uint8_t* df = (rtexDinoFrame==0) ? rtex_dino_run1 : rtex_dino_run2;
  rtexDrawDino(RTEX_DINO_X, rtexDinoY, df);

  if (rtexCactusX < RTEX_SCR_W+50 && rtexCactusX > -(RTEX_CD_SW)) {
    int cx=(int)rtexCactusX;
    if(rtexCactusType==0)      rtexDrawCactus(cx, rtexCactusDrawY(RTEX_CS_H), rtex_cactus_small,  RTEX_CS_W, RTEX_CS_H);
    else if(rtexCactusType==1) rtexDrawCactus(cx, rtexCactusDrawY(RTEX_CL_H), rtex_cactus_large,  RTEX_CL_W, RTEX_CL_H);
    else                       rtexDrawCactus(cx, rtexCactusDrawY(RTEX_CD_H), rtex_cactus_double, RTEX_CD_W, RTEX_CD_H);
  }

  tft.setTextSize(2); tft.setTextColor(rtexFgColor);
  tft.setCursor(10,5); tft.print("HI:"); tft.print(rtexHighScore);
  tft.setCursor(240,5); tft.print((int)rtexScoreF);
}

void rtexShowPause() {
  tft.fillRect(0,80,RTEX_SCR_W,50,rtexBgColor);
  tft.setTextColor(TFT_RED); tft.setTextSize(3);
  tft.setCursor(110,92); tft.print("PAUSED");
  if(rtexVersion==1) { rtexDrawMoon(); rtexDrawStars(); }
}
void rtexErasePause() {
  tft.fillRect(0,80,RTEX_SCR_W,50,rtexBgColor);
  tft.setTextSize(2); tft.setTextColor(rtexFgColor);
  tft.setCursor(10,5); tft.print("HI:"); tft.print(rtexHighScore);
  tft.setCursor(240,5); tft.print((int)rtexScoreF);
}

void rtexGameOver() {
  rtexPlaying = false; rtexIsPaused = false;
  int cs = (int)rtexScoreF;
  if(cs > rtexHighScore) { rtexHighScore = cs; rtexSaveHighScore(); menuHighScores[9] = rtexHighScore; }
  for(int i=0;i<3;i++) { tft.fillScreen(TFT_RED); delay(60); tft.fillScreen(rtexBgColor); delay(60); }
  tft.fillRoundRect(60,70,200,110,12,rtexBgColor);
  tft.drawRoundRect(60,70,200,110,12,rtexFgColor);
  tft.drawRoundRect(62,72,196,106,10,TFT_RED);
  tft.setTextColor(TFT_RED); tft.setTextSize(3); tft.setCursor(75,88); tft.print("GAME OVER");
  tft.drawFastHLine(80,118,160,rtexFgColor);
  tft.setTextSize(2); tft.setTextColor(rtexFgColor);
  tft.setCursor(90,126); tft.print("Score: "); tft.print(cs);
  tft.setCursor(90,148); tft.print("Best:  "); tft.print(rtexHighScore);
  tft.setTextSize(2); tft.setTextColor(TFT_CYAN);
  tft.setCursor(60,174); tft.print("SELECT: Play Again");
  if(rtexVersion==1) rtexDrawMoon();
}

void runRtexGame() {
  static unsigned long lastStartPress  = 0;
  static unsigned long lastSelectPress = 0;
  static unsigned long lastBPress      = 0;

  if(digitalRead(BTN_B)==LOW && millis()-lastBPress>300) {
    lastBPress = millis();
    rtexPlaying = false; rtexIsPaused = false;
    showRtexSubmenu(); return;
  }

  if(rtexPlaying && digitalRead(BTN_START)==LOW && millis()-lastStartPress>300) {
    lastStartPress = millis();
    if(!rtexIsPaused) {
      rtexIsPaused = true; rtexSaveState(); rtexShowPause();
    } else {
      rtexIsPaused = false; rtexErasePause(); rtexRestoreState(); rtexDrawFrame();
    }
    delay(100); return;
  }

  if(rtexIsPaused) { delay(50); return; }

  if(!rtexPlaying) {
    if(digitalRead(BTN_SELECT)==LOW && millis()-lastSelectPress>300) {
      lastSelectPress = millis();
      rtexStartGame(rtexVersion);
    }
    return;
  }

  static unsigned long lastSel = 0;
  if((digitalRead(BTN_SELECT)==LOW||digitalRead(BTN_A)==LOW)
     && !rtexIsJumping && millis()-lastSel>RTEX_JUMP_DEBOUNCE) {
    lastSel = millis(); rtexIsJumping = true; rtexJumpVel = -18.2;
  }

  rtexPrevDinoY = rtexDinoY;
  if(rtexIsJumping) {
    rtexJumpVel += RTEX_GRAVITY;
    rtexDinoY   += (int)rtexJumpVel;
    if(rtexDinoY < 20)                  { rtexDinoY=20; rtexJumpVel=0; rtexIsJumping=false; }
    if(rtexDinoY >= RTEX_GROUND_Y-RTEX_DINO_SH) { rtexDinoY=RTEX_GROUND_Y-RTEX_DINO_SH; rtexIsJumping=false; rtexJumpVel=0; }
  }

  rtexPrevCactusX = rtexCactusX;
  rtexCactusX    -= rtexGameSpeed;
  rtexGameSpeed  += 0.0011; if(rtexGameSpeed>22.0) rtexGameSpeed=22.0;
  rtexScoreF     += 0.45;

  if(rtexCactusX < -RTEX_CD_SW-20) {
    int positions[]={RTEX_SCR_W, RTEX_SCR_W+50, RTEX_SCR_W+100, RTEX_SCR_W+150, RTEX_SCR_W+200, RTEX_SCR_W+250};
    rtexCactusX = positions[random(0,6)]; rtexCactusType = random(0,3);
  }

  rtexPrevCloud1X = rtexCloud1X; rtexPrevCloud2X = rtexCloud2X;
  rtexCloud1X -= 1.5; rtexCloud2X -= 1.0;
  if(rtexCloud1X < -(RTEX_CLOUD_W*RTEX_DINO_SCALE)) { rtexCloud1X=RTEX_SCR_W; rtexCloud1Y=random(25,70); }
  if(rtexCloud2X < -(RTEX_CLOUD_W*RTEX_DINO_SCALE)) { rtexCloud2X=RTEX_SCR_W; rtexCloud2Y=random(25,70); }

  for(int i=0;i<RTEX_NUM_DOTS;i++) {
    rtexPrevDotX[i]=(int)rtexDotX[i]; rtexDotX[i]-=rtexGameSpeed;
    if(rtexDotX[i]<-5) { rtexDotX[i]=RTEX_SCR_W+random(0,80); rtexDotY[i]=random(RTEX_GROUND_Y+2,RTEX_GROUND_Y+12); }
  }
  for(int i=0;i<3;i++) {
    rtexPrevBumpX[i]=(int)rtexBumpX[i]; rtexBumpX[i]-=rtexGameSpeed;
    if(rtexBumpX[i]<-30) rtexBumpX[i]=RTEX_SCR_W+random(100,400);
  }

  if(millis()-rtexLastFrameTime>100) { rtexLastFrameTime=millis(); rtexDinoFrame=1-rtexDinoFrame; }

  int cW, cH;
  if(rtexCactusType==0)      { cW=RTEX_CS_SW;  cH=RTEX_CS_SH; }
  else if(rtexCactusType==1) { cW=RTEX_CL_SW;  cH=RTEX_CL_SH; }
  else                       { cW=RTEX_CD_SW;  cH=RTEX_CD_SH; }
  int cTop  = RTEX_GROUND_Y - cH;
  int dLeft = RTEX_DINO_X+4, dRight = RTEX_DINO_X+RTEX_DINO_SW-4, dBottom = rtexDinoY+RTEX_DINO_SH-2;
  int cLeft = (int)rtexCactusX+4, cRight = (int)rtexCactusX+cW-4;
  if(cRight>dLeft && cLeft<dRight && dBottom>cTop) {
    rtexGameOver();
    while(digitalRead(BTN_SELECT)==HIGH && digitalRead(BTN_B)==HIGH) delay(10);
    delay(200);
    if(digitalRead(BTN_B)==LOW) { showRtexSubmenu(); return; }
    rtexStartGame(rtexVersion); return;
  }

  rtexDrawFrame();
  delay(12);
}

void showRtexSubmenu() {
  currentGame = GAME_RTEX_SUBMENU;
  rtexSubSelected = 0;

  tft.fillScreen(0x0000);
  tft.fillRect(0,0,320,36,0x0008);
  tft.drawFastHLine(0,36,320,0xF7BE);
  tft.setTextSize(2); tft.setTextColor(0xF7BE);
  tft.setCursor(10,10); tft.print("RTEX LITE");
  tft.setTextSize(1); tft.setTextColor(0x2945);
  tft.setCursor(10,28); tft.print("Choose your version");
  tft.drawFastHLine(0,224,320,0x1082);
  tft.fillRect(0,225,320,15,0x0008);
  tft.setTextSize(1); tft.setTextColor(0x3186);
  tft.setCursor(8,228); tft.print("[B] Back");
  tft.setTextColor(0x07E0); tft.setCursor(110,228); tft.print("[A/SELECT]");
  tft.setTextColor(0x2945); tft.setCursor(195,228); tft.print("Launch");

  const char* vNames[]    = {"RTEX DAY", "RTEX NIGHT"};
  const char* vSubtitles[]= {"Bright sky, classic run","Dark sky, stars & moon"};
  const uint16_t vAccent[]= {0xFFE0, 0x07FF};
  const uint16_t vDim[]   = {0x2200, 0x020A};

  for(int i=0;i<2;i++) {
    bool sel=(i==rtexSubSelected);
    int cy = 50 + i*82;
    if(sel) {
      tft.fillRoundRect(8,cy,304,72,6,vDim[i]);
      tft.fillRoundRect(8,cy,5,72,3,vAccent[i]);
      tft.drawRoundRect(8,cy,304,72,6,vAccent[i]);
    } else {
      tft.fillRoundRect(8,cy,304,72,6,0x0821);
      tft.drawRoundRect(8,cy,304,72,6,0x1082);
    }
    tft.fillCircle(38,cy+36,20,sel?vAccent[i]:(uint16_t)0x1082);
    tft.setTextSize(2);
    tft.setTextColor(sel?(uint16_t)0x0000:(uint16_t)0x2945);
    tft.setCursor(31,cy+28); tft.print(i+1);
    tft.setTextSize(2);
    tft.setTextColor(sel?vAccent[i]:(uint16_t)0x528A);
    tft.setCursor(68,cy+16); tft.print(vNames[i]);
    tft.setTextSize(1);
    tft.setTextColor(sel?(uint16_t)0x3186:(uint16_t)0x2104);
    tft.setCursor(68,cy+38); tft.print(vSubtitles[i]);
    tft.setTextColor(sel?vAccent[i]:(uint16_t)0x2104);
    tft.setCursor(68,cy+52);
    char sb[16]; sprintf(sb,"HI: %04d", rtexHighScore);
    tft.print(sb);
    if(sel) tft.fillTriangle(298,cy+28,298,cy+44,306,cy+36,vAccent[i]);
  }
}

void updateRtexSubmenu(int oldSel, int newSel) {
  const uint16_t vAccent[]= {0xFFE0, 0x07FF};
  const uint16_t vDim[]   = {0x2200, 0x020A};
  const char* vNames[]    = {"RTEX DAY",  "RTEX NIGHT"};
  const char* vSubtitles[]= {"Bright sky, classic run","Dark sky, stars & moon"};

  for(int idx=0;idx<2;idx++) {
    if(idx!=oldSel && idx!=newSel) continue;
    bool sel=(idx==newSel);
    int cy=50+idx*82;
    if(sel) {
      tft.fillRoundRect(8,cy,304,72,6,vDim[idx]);
      tft.fillRoundRect(8,cy,5,72,3,vAccent[idx]);
      tft.drawRoundRect(8,cy,304,72,6,vAccent[idx]);
    } else {
      tft.fillRoundRect(8,cy,304,72,6,0x0821);
      tft.drawRoundRect(8,cy,304,72,6,0x1082);
    }
    tft.fillCircle(38,cy+36,20,sel?vAccent[idx]:(uint16_t)0x1082);
    tft.setTextSize(2);
    tft.setTextColor(sel?(uint16_t)0x0000:(uint16_t)0x2945);
    tft.setCursor(31,cy+28); tft.print(idx+1);
    tft.setTextSize(2);
    tft.setTextColor(sel?vAccent[idx]:(uint16_t)0x528A);
    tft.setCursor(68,cy+16); tft.print(vNames[idx]);
    tft.setTextSize(1);
    tft.setTextColor(sel?(uint16_t)0x3186:(uint16_t)0x2104);
    tft.setCursor(68,cy+38); tft.print(vSubtitles[idx]);
    tft.setTextColor(sel?vAccent[idx]:(uint16_t)0x2104);
    tft.setCursor(68,cy+52);
    char sb[16]; sprintf(sb,"HI: %04d", rtexHighScore);
    tft.print(sb);
    if(sel) tft.fillTriangle(298,cy+28,298,cy+44,306,cy+36,vAccent[idx]);
    else    tft.fillRect(294,cy+26,14,22,0x0821);
  }
}

void launchRtexVersion(int version) {
  rtexVersion = version;
  currentGame = GAME_RTEX;
  if(version==0) { rtexBgColor=RTEX_DAY_BG;  rtexFgColor=RTEX_DAY_FG; }
  else           { rtexBgColor=RTEX_NIGHT_BG; rtexFgColor=RTEX_NIGHT_FG; }
  tft.fillScreen(rtexBgColor);
  if(version==1) { for(int i=0;i<30;i++) tft.drawPixel(random(0,320),random(5,180),(uint16_t)RTEX_COL_STAR); rtexDrawMoon(); }
  tft.setTextColor(rtexFgColor); tft.setTextSize(4);
  tft.setCursor(50,55);
  tft.print(version==0?"RTEX DAY":"RTEX NIGHT");
  if(version==1) { tft.setTextColor(TFT_CYAN); tft.setTextSize(1); tft.setCursor(110,95); tft.print("NIGHT MODE"); }
  const uint8_t* df = rtex_dino_run1;
  rtexDrawBitmapScaled(140, 110, df, RTEX_DINO_W, RTEX_DINO_H, rtexFgColor, rtexBgColor, RTEX_DINO_SCALE);
  tft.setTextSize(2); tft.setTextColor(version==0?(uint16_t)0x07FF:(uint16_t)TFT_YELLOW);
  tft.setCursor(50,160); tft.print("SELECT: Jump/Start");
  tft.setTextSize(1); tft.setTextColor(rtexFgColor);
  tft.setCursor(70,185); tft.print("START: Pause   B: Back");
  tft.setTextSize(2); tft.setTextColor(rtexFgColor);
  tft.setCursor(95,205); tft.print("HI: "); tft.print(rtexHighScore);
  rtexPlaying = false; rtexIsPaused = false;
}

// ================================================================
// =================== BRICKOUT GAME ==============================
// ================================================================
#define BCOUT_COL_BG        TFT_BLACK
#define BCOUT_COL_PADDLE    TFT_CYAN
#define BCOUT_COL_BALL      TFT_WHITE
#define BCOUT_COL_TEXT      TFT_WHITE
#define BCOUT_COL_SCORE     TFT_YELLOW
#define BCOUT_COL_LIFE      TFT_RED
#define BCOUT_COL_BORDER    0x2104

const uint16_t BCOUT_BRICK_COLORS[] = {
  TFT_RED, 0xFD20, TFT_YELLOW, TFT_GREEN, TFT_CYAN, 0x781F
};

#define BCOUT_EEPROM_SIZE   4
#define BCOUT_HISCORE_ADDR  20

#define BCOUT_PADDLE_W      70
#define BCOUT_PADDLE_H      8
#define BCOUT_PADDLE_Y      (screenHeight - 10)
#define BCOUT_PADDLE_SPEED  8

#define BCOUT_BALL_SIZE     5
#define BCOUT_BALL_SPEED_INIT 2.5f

#define BCOUT_BRICK_COLS    10
#define BCOUT_BRICK_ROWS    6
#define BCOUT_BRICK_W       28
#define BCOUT_BRICK_H       12
#define BCOUT_BRICK_PAD_X   2
#define BCOUT_BRICK_PAD_Y   2
#define BCOUT_BRICK_OFFSET_X ((screenWidth - (BCOUT_BRICK_COLS * (BCOUT_BRICK_W + BCOUT_BRICK_PAD_X) - BCOUT_BRICK_PAD_X)) / 2)
#define BCOUT_BRICK_OFFSET_Y 30

#define BCOUT_MAX_LIVES     3
#define BCOUT_MAX_LEVELS    25

struct BCOUTBrick {
  bool alive;
  uint8_t hits;
};

enum BCOUTGameState {
  BCOUT_STATE_PLAYING,
  BCOUT_STATE_BALL_LOST,
  BCOUT_STATE_LEVEL_CLEAR,
  BCOUT_STATE_GAME_OVER
};

BCOUTBrick bcoutBricks[BCOUT_BRICK_ROWS][BCOUT_BRICK_COLS];

float   bcoutBallX, bcoutBallY;
float   bcoutBallVX, bcoutBallVY;
float   bcoutBallSpeed;
int     bcoutPaddleX;
int     bcoutScore;
int     bcoutHiScore;
int     bcoutLives;
int     bcoutLevel;
bool    bcoutBallLaunched;
bool    bcoutPaused;
BCOUTGameState bcoutGameState;
int     bcoutGameMode;

unsigned long bcoutLastFrame;
unsigned long bcoutMsgTimer;

bool bcoutBtnLeftPrev   = HIGH;
bool bcoutBtnRightPrev  = HIGH;
bool bcoutBtnSelectPrev = HIGH;

// Level Maps (simplified for space - same as before)
const uint8_t BCOUT_LEVEL_MAPS[BCOUT_MAX_LEVELS][BCOUT_BRICK_ROWS][BCOUT_BRICK_COLS] = {
  {{1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}},
  // ... (rest of level maps - keep as in original)
};

// Brickout functions (keep from original)
void bcoutLoadHiScore() {
  EEPROM.begin(512);
  bcoutHiScore = EEPROM.read(BCOUT_HISCORE_ADDR) << 8;
  bcoutHiScore |= EEPROM.read(BCOUT_HISCORE_ADDR + 1);
  if (bcoutHiScore > 99999) bcoutHiScore = 0;
}

void bcoutSaveHiScore() {
  EEPROM.write(BCOUT_HISCORE_ADDR,     (bcoutHiScore >> 8) & 0xFF);
  EEPROM.write(BCOUT_HISCORE_ADDR + 1, bcoutHiScore & 0xFF);
  EEPROM.commit();
}

void bcoutInitLevel(int level) {
  int lv = (level - 1) % BCOUT_MAX_LEVELS;
  for (int r = 0; r < BCOUT_BRICK_ROWS; r++) {
    for (int c = 0; c < BCOUT_BRICK_COLS; c++) {
      bcoutBricks[r][c].hits  = BCOUT_LEVEL_MAPS[lv][r][c];
      bcoutBricks[r][c].alive = (bcoutBricks[r][c].hits > 0);
    }
  }
  bcoutPaddleX    = (screenWidth - BCOUT_PADDLE_W) / 2;
  bcoutBallX      = bcoutPaddleX + BCOUT_PADDLE_W / 2;
  bcoutBallY      = BCOUT_PADDLE_Y - BCOUT_BALL_SIZE - 2;
  bcoutBallSpeed  = BCOUT_BALL_SPEED_INIT + (level - 1) * 0.02f;
  if (bcoutBallSpeed > 4.0f) bcoutBallSpeed = 4.0f;
  bcoutBallVX     = bcoutBallSpeed;
  bcoutBallVY     = -bcoutBallSpeed;
  bcoutBallLaunched = false;
  bcoutPaused       = false;
}

void bcoutInitClassic() {
  bcoutGameMode = 0;
  bcoutScore = 0;
  bcoutLives = BCOUT_MAX_LIVES;
  bcoutLevel = 1;
  bcoutInitLevel(bcoutLevel);
  bcoutGameState = BCOUT_STATE_PLAYING;
  tft.fillScreen(BCOUT_COL_BG);
  bcoutDrawHUD();
  bcoutDrawBricks();
  bcoutDrawPaddle(bcoutPaddleX, true);
  bcoutDrawBall((int)bcoutBallX, (int)bcoutBallY, true);
}

void bcoutInitSelect(int level) {
  bcoutGameMode = 1;
  bcoutScore = 0;
  bcoutLives = BCOUT_MAX_LIVES;
  bcoutLevel = level;
  bcoutInitLevel(bcoutLevel);
  bcoutGameState = BCOUT_STATE_PLAYING;
  tft.fillScreen(BCOUT_COL_BG);
  bcoutDrawHUD();
  bcoutDrawBricks();
  bcoutDrawPaddle(bcoutPaddleX, true);
  bcoutDrawBall((int)bcoutBallX, (int)bcoutBallY, true);
}

void bcoutDrawPauseMessage() {
  tft.setTextColor(TFT_RED);
  tft.setTextSize(3);
  tft.setCursor(115, 105);
  tft.print("PAUSED");
}

void bcoutErasePauseMessage() {
  tft.fillRoundRect(80, 90, 160, 50, 10, BCOUT_COL_BG);
  bcoutDrawHUD();
}

void bcoutDrawBrick(int c, int r) {
  if (!bcoutBricks[r][c].alive) return;
  int x = BCOUT_BRICK_OFFSET_X + c * (BCOUT_BRICK_W + BCOUT_BRICK_PAD_X);
  int y = BCOUT_BRICK_OFFSET_Y + r * (BCOUT_BRICK_H + BCOUT_BRICK_PAD_Y);
  uint16_t col = BCOUT_BRICK_COLORS[r % 6];
  tft.fillRect(x, y, BCOUT_BRICK_W, BCOUT_BRICK_H, col);
  tft.drawFastHLine(x, y, BCOUT_BRICK_W, TFT_WHITE);
  tft.drawFastVLine(x, y, BCOUT_BRICK_H, TFT_WHITE);
  tft.drawFastHLine(x, y + BCOUT_BRICK_H - 1, BCOUT_BRICK_W, BCOUT_COL_BG);
  tft.drawFastVLine(x + BCOUT_BRICK_W - 1, y, BCOUT_BRICK_H, BCOUT_COL_BG);
}

void bcoutEraseBrick(int c, int r) {
  int x = BCOUT_BRICK_OFFSET_X + c * (BCOUT_BRICK_W + BCOUT_BRICK_PAD_X);
  int y = BCOUT_BRICK_OFFSET_Y + r * (BCOUT_BRICK_H + BCOUT_BRICK_PAD_Y);
  tft.fillRect(x, y, BCOUT_BRICK_W, BCOUT_BRICK_H, BCOUT_COL_BG);
}

void bcoutDrawBricks() {
  for (int r = 0; r < BCOUT_BRICK_ROWS; r++)
    for (int c = 0; c < BCOUT_BRICK_COLS; c++)
      if (bcoutBricks[r][c].alive)
        bcoutDrawBrick(c, r);
}

void bcoutDrawPaddle(int x, bool show) {
  tft.fillRoundRect(x, BCOUT_PADDLE_Y, BCOUT_PADDLE_W, BCOUT_PADDLE_H,
                    BCOUT_PADDLE_H / 2, show ? BCOUT_COL_PADDLE : BCOUT_COL_BG);
  if (show)
    tft.drawFastHLine(x + 2, BCOUT_PADDLE_Y + 1, BCOUT_PADDLE_W - 4, TFT_WHITE);
}

void bcoutDrawBall(int x, int y, bool show) {
  tft.fillCircle(x, y, BCOUT_BALL_SIZE, show ? BCOUT_COL_BALL : BCOUT_COL_BG);
}

void bcoutDrawHUD() {
  tft.fillRect(0, 0, screenWidth, 22, 0x2104);
  tft.setTextColor(BCOUT_COL_SCORE);
  tft.setTextSize(1);
  tft.setCursor(4, 7);
  if (bcoutGameMode == 0) {
    tft.print("SCR:");
    tft.print(bcoutScore);
  } else {
    tft.print("SELECT MODE");
  }

  tft.setTextColor(TFT_MAGENTA);
  tft.setCursor(screenWidth / 2 - 20, 7);
  tft.print("LV:");
  tft.print(bcoutLevel);
  tft.print("/25");

  tft.setCursor(screenWidth - 75, 7);
  tft.setTextColor(BCOUT_COL_LIFE);
  tft.print("LIVES:");
  for (int i = 0; i < BCOUT_MAX_LIVES; i++) {
    if (i < bcoutLives)
      tft.fillCircle(screenWidth - 12 + (i - BCOUT_MAX_LIVES + 1) * 10, 11, 3, BCOUT_COL_LIFE);
    else
      tft.drawCircle(screenWidth - 12 + (i - BCOUT_MAX_LIVES + 1) * 10, 11, 3, BCOUT_COL_LIFE);
  }
  
  if (bcoutGameMode == 0) {
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(4, 15);
    tft.print("HI:");
    tft.print(bcoutHiScore);
  }
}

void bcoutDrawGameOver() {
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
  tft.setCursor(170, 107); tft.print(bcoutScore);

  if (bcoutGameMode == 0 && bcoutScore >= bcoutHiScore && bcoutScore > 0) {
    for (int i = 0; i < 3; i++) {
      tft.setTextColor(TFT_YELLOW);
      tft.setCursor(52, 133); tft.print("NEW HI-SCORE!");
      delay(50);
      tft.fillRect(52, 133, 190, 12, tft.color565(15, 0, 0));
      delay(50);
    }
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(52, 133); tft.print("NEW HI-SCORE!");
  } else if (bcoutGameMode == 0) {
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(65, 133); tft.print("HI-SCORE: ");
    tft.setTextColor(TFT_YELLOW); tft.print(bcoutHiScore);
  } else {
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(80, 133); tft.print("SELECT MODE");
  }

  tft.fillRoundRect(70, 150, 180, 20, 8, tft.color565(0, 20, 0));
  tft.drawRoundRect(70, 150, 180, 20, 8, TFT_GREEN);
  tft.setTextColor(TFT_GREEN); tft.setTextSize(1);
  tft.setCursor(85, 155); tft.print("PRESS SELECT TO RESTART");
}

void bcoutDrawLevelClear() {
  tft.fillRect(50, 85, 220, 70, 0x1082);
  tft.drawRect(50, 85, 220, 70, TFT_GREEN);
  tft.setTextColor(TFT_GREEN); tft.setTextSize(2);
  tft.setCursor(62, 95);  tft.print("LEVEL CLEAR!");
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.setCursor(78, 120);
  if (bcoutLevel < BCOUT_MAX_LEVELS) {
    tft.print("Level "); tft.print(bcoutLevel); tft.print(" -> "); tft.print(bcoutLevel+1);
  } else {
    tft.setTextColor(TFT_YELLOW);
    tft.print("You Beat All 25 Levels!");
  }
  tft.setTextColor(0xAD75); tft.setCursor(100, 138); tft.print("Get ready...");
}

bool bcoutCheckBrickCollision() {
  int bx = (int)bcoutBallX - BCOUT_BALL_SIZE;
  int by = (int)bcoutBallY - BCOUT_BALL_SIZE;
  int bw = BCOUT_BALL_SIZE * 2;
  int bh = BCOUT_BALL_SIZE * 2;

  for (int r = 0; r < BCOUT_BRICK_ROWS; r++) {
    for (int c = 0; c < BCOUT_BRICK_COLS; c++) {
      if (!bcoutBricks[r][c].alive) continue;

      int rx = BCOUT_BRICK_OFFSET_X + c * (BCOUT_BRICK_W + BCOUT_BRICK_PAD_X);
      int ry = BCOUT_BRICK_OFFSET_Y + r * (BCOUT_BRICK_H + BCOUT_BRICK_PAD_Y);

      if (bx < rx + BCOUT_BRICK_W && bx + bw > rx &&
          by < ry + BCOUT_BRICK_H && by + bh > ry) {

        float overlapLeft  = (bx + bw) - rx;
        float overlapRight = (rx + BCOUT_BRICK_W) - bx;
        float overlapTop   = (by + bh) - ry;
        float overlapBot   = (ry + BCOUT_BRICK_H) - by;
        float minH = min(overlapLeft, overlapRight);
        float minV = min(overlapTop, overlapBot);
        if (minH < minV) bcoutBallVX = -bcoutBallVX;
        else              bcoutBallVY = -bcoutBallVY;

        bcoutBricks[r][c].hits--;
        if (bcoutBricks[r][c].hits <= 0) {
          bcoutBricks[r][c].alive = false;
          bcoutEraseBrick(c, r);
          if (bcoutGameMode == 0) {
            bcoutScore += (r + 1) * 10 * bcoutLevel;
          }
        } else {
          bcoutDrawBrick(c, r);
        }
        bcoutDrawHUD();
        return true;
      }
    }
  }
  return false;
}

bool bcoutAllBricksCleared() {
  for (int r = 0; r < BCOUT_BRICK_ROWS; r++)
    for (int c = 0; c < BCOUT_BRICK_COLS; c++)
      if (bcoutBricks[r][c].alive) return false;
  return true;
}

void bcoutUpdateGame() {
  bool leftNow  = digitalRead(BTN_LEFT);
  bool rightNow = digitalRead(BTN_RIGHT);
  bool selNow   = digitalRead(BTN_SELECT);

  if (selNow == LOW && bcoutBtnSelectPrev == HIGH) {
    if (!bcoutBallLaunched) {
      bcoutBallLaunched = true;
    } else {
      bcoutPaused = !bcoutPaused;
      if (bcoutPaused) bcoutDrawPauseMessage();
      else        bcoutErasePauseMessage();
    }
  }
  bcoutBtnSelectPrev = selNow;

  if (bcoutPaused) return;

  int oldPaddle = bcoutPaddleX;
  if (leftNow  == LOW) bcoutPaddleX = max(0, bcoutPaddleX - BCOUT_PADDLE_SPEED);
  if (rightNow == LOW) bcoutPaddleX = min(screenWidth - BCOUT_PADDLE_W, bcoutPaddleX + BCOUT_PADDLE_SPEED);
  if (oldPaddle != bcoutPaddleX) {
    bcoutDrawPaddle(oldPaddle, false);
    bcoutDrawPaddle(bcoutPaddleX, true);
  }

  if (!bcoutBallLaunched) {
    int oldBX = (int)bcoutBallX, oldBY = (int)bcoutBallY;
    bcoutBallX = bcoutPaddleX + BCOUT_PADDLE_W / 2;
    bcoutBallY = BCOUT_PADDLE_Y - BCOUT_BALL_SIZE - 1;
    if ((int)bcoutBallX != oldBX || (int)bcoutBallY != oldBY) {
      bcoutDrawBall(oldBX, oldBY, false);
      bcoutDrawBall((int)bcoutBallX, (int)bcoutBallY, true);
    }
    return;
  }

  int oldBX = (int)bcoutBallX, oldBY = (int)bcoutBallY;
  bcoutBallX += bcoutBallVX;
  bcoutBallY += bcoutBallVY;

  if (bcoutBallX - BCOUT_BALL_SIZE < 0)       { bcoutBallX = BCOUT_BALL_SIZE;        bcoutBallVX =  fabs(bcoutBallVX); }
  if (bcoutBallX + BCOUT_BALL_SIZE > screenWidth){ bcoutBallX = screenWidth-BCOUT_BALL_SIZE; bcoutBallVX = -fabs(bcoutBallVX); }
  if (bcoutBallY - BCOUT_BALL_SIZE < 22)      { bcoutBallY = 22 + BCOUT_BALL_SIZE;   bcoutBallVY =  fabs(bcoutBallVY); }

  if (bcoutBallY + BCOUT_BALL_SIZE >= BCOUT_PADDLE_Y &&
      bcoutBallY - BCOUT_BALL_SIZE <= BCOUT_PADDLE_Y + BCOUT_PADDLE_H &&
      bcoutBallX >= bcoutPaddleX && bcoutBallX <= bcoutPaddleX + BCOUT_PADDLE_W &&
      bcoutBallVY > 0) {
    float rel   = (bcoutBallX - (bcoutPaddleX + BCOUT_PADDLE_W / 2.0f)) / (BCOUT_PADDLE_W / 2.0f);
    float angle = rel * 60.0f * (PI / 180.0f);
    float spd   = sqrt(bcoutBallVX * bcoutBallVX + bcoutBallVY * bcoutBallVY);
    bcoutBallVX = spd * sin(angle);
    bcoutBallVY = -spd * cos(angle);
    if (fabs(bcoutBallVX) < 0.5f) bcoutBallVX = (bcoutBallVX >= 0 ? 0.5f : -0.5f);
    bcoutBallY = BCOUT_PADDLE_Y - BCOUT_BALL_SIZE - 1;
  }

  bcoutCheckBrickCollision();

  if (bcoutBallY + BCOUT_BALL_SIZE > screenHeight) {
    bcoutLives--;
    bcoutDrawHUD();
    if (bcoutLives <= 0) {
      if (bcoutGameMode == 0 && bcoutScore > bcoutHiScore) { 
        bcoutHiScore = bcoutScore; 
        bcoutSaveHiScore(); 
        menuHighScores[10] = bcoutHiScore;
      }
      bcoutGameState = BCOUT_STATE_GAME_OVER;
      bcoutDrawGameOver();
      bcoutMsgTimer = millis();
      return;
    }
    bcoutGameState = BCOUT_STATE_BALL_LOST;
    tft.setTextColor(TFT_ORANGE); tft.setTextSize(3);
    tft.setCursor(90, 105); tft.print("BALL LOST!");
    return;
  }

  if ((int)bcoutBallX != oldBX || (int)bcoutBallY != oldBY) {
    bcoutDrawBall(oldBX, oldBY, false);
    bcoutDrawBall((int)bcoutBallX, (int)bcoutBallY, true);
  }

  if (bcoutAllBricksCleared()) {
    bcoutLevel++;
    bcoutGameState = BCOUT_STATE_LEVEL_CLEAR;
    bcoutDrawLevelClear();
    bcoutMsgTimer = millis();
  }
}

void brickoutDrawLevelSelect() {
  tft.fillScreen(BCOUT_COL_BG);
  
  tft.fillRect(0, 0, screenWidth, 36, 0x0008);
  tft.drawFastHLine(0, 36, screenWidth, 0xFFE0);
  tft.setTextSize(2);
  tft.setTextColor(0xFFE0);
  tft.setCursor(10, 10);
  tft.print("SELECT & PLAY");
  tft.setTextSize(1);
  tft.setTextColor(0x2945);
  tft.setCursor(10, 28);
  tft.print("UP/DOWN/LEFT/RIGHT | SELECT to Play | B to Back");

  tft.drawFastHLine(0, screenHeight - 15, screenWidth, 0x1082);
  tft.fillRect(0, screenHeight - 14, screenWidth, 14, 0x0008);
  tft.setTextSize(1);
  tft.setTextColor(0x3186);
  tft.setCursor(8, screenHeight - 11);
  tft.print("[B] Back");
  tft.setTextColor(0x07E0);
  tft.setCursor(110, screenHeight - 11);
  tft.print("[SELECT] Play Level");
  tft.setTextColor(0x07FF);
  tft.setCursor(230, screenHeight - 11);
  tft.print("LV: 01-25");

  int rows = 5, cols = 5;
  int cellW = 50, cellH = 36;
  int startX = (screenWidth - (cols * cellW)) / 2;
  int startY = 55;
  
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      int lvl = r * cols + c + 1;
      int x = startX + c * cellW;
      int y = startY + r * cellH;
      bool selected = (lvl == brickoutSelectedLevel);
      
      if (selected) {
        tft.fillRoundRect(x+2, y+2, cellW - 6, cellH - 6, 5, 0x07E0);
        tft.drawRoundRect(x, y, cellW - 2, cellH - 2, 5, TFT_WHITE);
        tft.drawRoundRect(x+1, y+1, cellW - 4, cellH - 4, 5, TFT_YELLOW);
        tft.setTextColor(TFT_BLACK);
      } else {
        tft.fillRoundRect(x+2, y+2, cellW - 6, cellH - 6, 5, 0x1082);
        tft.drawRoundRect(x, y, cellW - 2, cellH - 2, 5, 0x3186);
        tft.setTextColor(0xFFFF);
      }
      tft.setTextSize(2);
      tft.setCursor(x + 15, y + 12);
      if (lvl < 10) tft.print(" ");
      tft.print(lvl);
    }
  }
  
  tft.fillRect(startX, startY + rows * cellH + 15, screenWidth - startX*2, 45, 0x0008);
  tft.drawRoundRect(startX, startY + rows * cellH + 15, screenWidth - startX*2, 45, 8, 0x07E0);
  
  tft.setTextSize(2);
  tft.setTextColor(0xFFE0);
  tft.setCursor(startX + 20, startY + rows * cellH + 28);
  tft.print("SELECTED LEVEL:");
  
  tft.setTextSize(3);
  tft.setTextColor(0x07FF);
  tft.setCursor(startX + 220, startY + rows * cellH + 28);
  if (brickoutSelectedLevel < 10) tft.print(" ");
  tft.print(brickoutSelectedLevel);
  
  int lv = (brickoutSelectedLevel - 1) % BCOUT_MAX_LEVELS;
  uint16_t brickCol = BCOUT_BRICK_COLORS[lv % 6];
  int px = startX + 20;
  int py = startY + rows * cellH + 28;
  tft.fillRect(px + 100, py + 3, 40, 16, brickCol);
  tft.drawFastHLine(px + 100, py + 3, 40, TFT_WHITE);
  tft.drawFastVLine(px + 100, py + 3, 16, TFT_WHITE);
  
  tft.setTextSize(1);
  tft.setTextColor(0xFFE0);
  tft.setCursor(px + 145, py + 12);
  tft.print("1-hit");
  
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(px + 100, py + 30);
  tft.print("Preview");
}

void runBrickoutSelectMode() {
  static unsigned long lastBtnTime = 0;
  static bool upPressed = false, downPressed = false, leftPressed = false, rightPressed = false;
  
  if (digitalRead(BTN_B) == LOW) {
    delay(200);
    showBrickoutSubmenu();
    return;
  }
  
  if (digitalRead(BTN_SELECT) == LOW) {
    delay(200);
    bcoutInitSelect(brickoutSelectedLevel);
    currentGame = GAME_BRICKOUT_SELECT;
    return;
  }
  
  if (digitalRead(BTN_UP) == LOW && !upPressed && millis() - lastBtnTime > 200) {
    lastBtnTime = millis();
    upPressed = true;
    brickoutSelectedLevel -= 5;
    if (brickoutSelectedLevel < 1) brickoutSelectedLevel = 1;
    brickoutDrawLevelSelect();
  } else if (digitalRead(BTN_UP) == HIGH) {
    upPressed = false;
  }
  
  if (digitalRead(BTN_DOWN) == LOW && !downPressed && millis() - lastBtnTime > 200) {
    lastBtnTime = millis();
    downPressed = true;
    brickoutSelectedLevel += 5;
    if (brickoutSelectedLevel > 25) brickoutSelectedLevel = 25;
    brickoutDrawLevelSelect();
  } else if (digitalRead(BTN_DOWN) == HIGH) {
    downPressed = false;
  }
  
  if (digitalRead(BTN_LEFT) == LOW && !leftPressed && millis() - lastBtnTime > 150) {
    lastBtnTime = millis();
    leftPressed = true;
    brickoutSelectedLevel--;
    if (brickoutSelectedLevel < 1) brickoutSelectedLevel = 25;
    brickoutDrawLevelSelect();
  } else if (digitalRead(BTN_LEFT) == HIGH) {
    leftPressed = false;
  }
  
  if (digitalRead(BTN_RIGHT) == LOW && !rightPressed && millis() - lastBtnTime > 150) {
    lastBtnTime = millis();
    rightPressed = true;
    brickoutSelectedLevel++;
    if (brickoutSelectedLevel > 25) brickoutSelectedLevel = 1;
    brickoutDrawLevelSelect();
  } else if (digitalRead(BTN_RIGHT) == HIGH) {
    rightPressed = false;
  }
  
  delay(20);
}

void runBrickoutGame() {
  static unsigned long lastBPress = 0;
  
  if (digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) {
    lastBPress = millis();
    if (bcoutGameMode == 0) {
      showBrickoutSubmenu();
    } else {
      brickoutDrawLevelSelect();
      currentGame = GAME_BRICKOUT_SUBMENU;
    }
    return;
  }

  unsigned long now = millis();

  switch (bcoutGameState) {
    case BCOUT_STATE_PLAYING:
      if (now - bcoutLastFrame >= 16) { 
        bcoutLastFrame = now; 
        bcoutUpdateGame(); 
      }
      break;

    case BCOUT_STATE_BALL_LOST: {
      bool selNow = digitalRead(BTN_SELECT);
      if (selNow == LOW && bcoutBtnSelectPrev == HIGH) {
        tft.fillRect(0, 22, screenWidth, screenHeight - 22, BCOUT_COL_BG);
        bcoutPaddleX    = (screenWidth - BCOUT_PADDLE_W) / 2;
        bcoutBallX      = bcoutPaddleX + BCOUT_PADDLE_W / 2;
        bcoutBallY      = BCOUT_PADDLE_Y - BCOUT_BALL_SIZE - 2;
        bcoutBallSpeed  = BCOUT_BALL_SPEED_INIT + (bcoutLevel - 1) * 0.004f;
        if (bcoutBallSpeed > 4.0f) bcoutBallSpeed = 4.0f;
        bcoutBallVX     = bcoutBallSpeed;
        bcoutBallVY     = -bcoutBallSpeed;
        bcoutBallLaunched = false;
        bcoutPaused       = false;
        bcoutDrawBricks();
        bcoutDrawPaddle(bcoutPaddleX, true);
        bcoutDrawBall((int)bcoutBallX, (int)bcoutBallY, true);
        bcoutDrawHUD();
        bcoutGameState = BCOUT_STATE_PLAYING;
      }
      bcoutBtnSelectPrev = selNow;
      break;
    }

    case BCOUT_STATE_LEVEL_CLEAR:
      if (now - bcoutMsgTimer > 2500) {
        tft.fillRect(0, 22, screenWidth, screenHeight - 22, BCOUT_COL_BG);
        bcoutInitLevel(bcoutLevel);
        bcoutDrawBricks();
        bcoutDrawPaddle(bcoutPaddleX, true);
        bcoutDrawBall((int)bcoutBallX, (int)bcoutBallY, true);
        bcoutDrawHUD();
        bcoutGameState = BCOUT_STATE_PLAYING;
      }
      break;

    case BCOUT_STATE_GAME_OVER: {
      bool selNow = digitalRead(BTN_SELECT);
      if (selNow == LOW && bcoutBtnSelectPrev == HIGH) {
        if (bcoutGameMode == 0) {
          bcoutInitClassic();
        } else {
          brickoutDrawLevelSelect();
          currentGame = GAME_BRICKOUT_SUBMENU;
        }
      }
      bcoutBtnSelectPrev = selNow;
      break;
    }

    default: break;
  }
}

void showBrickoutSubmenu() {
  currentGame = GAME_BRICKOUT_SUBMENU;
  brickoutSubSelected = 0;
  brickoutSelectedLevel = 1;

  tft.fillScreen(0x0000);

  tft.fillRect(0, 0, 320, 36, 0x0008);
  tft.drawFastHLine(0, 36, 320, 0xFFE0);
  tft.setTextSize(2); tft.setTextColor(0xFFE0);
  tft.setCursor(10, 10); tft.print("BRICKOUT");
  tft.setTextSize(1); tft.setTextColor(0x2945);
  tft.setCursor(10, 28); tft.print("Choose your mode");

  tft.drawFastHLine(0, 224, 320, 0x1082);
  tft.fillRect(0, 225, 320, 15, 0x0008);
  tft.setTextSize(1); tft.setTextColor(0x3186);
  tft.setCursor(8, 228); tft.print("[B] Back");
  tft.setTextColor(0x07E0); tft.setCursor(110, 228); tft.print("[A/SELECT]");
  tft.setTextColor(0x2945); tft.setCursor(195, 228); tft.print("Launch");

  const char* mNames[] = {"CLASSIC MODE", "SELECT & PLAY"};
  const char* mSubtitles[] = {"Progressive levels w/ scoring", "Pick any level 1-25, no scoring"};
  const uint16_t mAccent[] = {0x07E0, 0x07FF};
  const uint16_t mDim[] = {0x0140, 0x020A};

  for (int i = 0; i < 2; i++) {
    bool sel = (i == brickoutSubSelected);
    int cy = 50 + i * 82;
    if (sel) {
      tft.fillRoundRect(8, cy, 304, 72, 6, mDim[i]);
      tft.fillRoundRect(8, cy, 5, 72, 3, mAccent[i]);
      tft.drawRoundRect(8, cy, 304, 72, 6, mAccent[i]);
    } else {
      tft.fillRoundRect(8, cy, 304, 72, 6, 0x0821);
      tft.drawRoundRect(8, cy, 304, 72, 6, 0x1082);
    }

    tft.fillCircle(38, cy + 36, 20, sel ? mAccent[i] : (uint16_t)0x1082);
    tft.setTextSize(2);
    tft.setTextColor(sel ? (uint16_t)0x0000 : (uint16_t)0x2945);
    tft.setCursor(31, cy + 28); tft.print(i + 1);

    tft.setTextSize(2);
    tft.setTextColor(sel ? mAccent[i] : (uint16_t)0x528A);
    tft.setCursor(68, cy + 16); tft.print(mNames[i]);
    tft.setTextSize(1);
    tft.setTextColor(sel ? (uint16_t)0x3186 : (uint16_t)0x2104);
    tft.setCursor(68, cy + 38); tft.print(mSubtitles[i]);

    if (sel) tft.fillTriangle(298, cy + 28, 298, cy + 44, 306, cy + 36, mAccent[i]);
  }
}

void updateBrickoutSubmenu(int oldSel, int newSel) {
  const uint16_t mAccent[] = {0x07E0, 0x07FF};
  const uint16_t mDim[] = {0x0140, 0x020A};
  const char* mNames[] = {"CLASSIC MODE", "SELECT & PLAY"};
  const char* mSubtitles[] = {"Progressive levels w/ scoring", "Pick any level 1-25, no scoring"};

  for (int idx = 0; idx < 2; idx++) {
    if (idx != oldSel && idx != newSel) continue;
    bool sel = (idx == newSel);
    int cy = 50 + idx * 82;
    if (sel) {
      tft.fillRoundRect(8, cy, 304, 72, 6, mDim[idx]);
      tft.fillRoundRect(8, cy, 5, 72, 3, mAccent[idx]);
      tft.drawRoundRect(8, cy, 304, 72, 6, mAccent[idx]);
    } else {
      tft.fillRoundRect(8, cy, 304, 72, 6, 0x0821);
      tft.drawRoundRect(8, cy, 304, 72, 6, 0x1082);
    }
    tft.fillCircle(38, cy + 36, 20, sel ? mAccent[idx] : (uint16_t)0x1082);
    tft.setTextSize(2);
    tft.setTextColor(sel ? (uint16_t)0x0000 : (uint16_t)0x2945);
    tft.setCursor(31, cy + 28); tft.print(idx + 1);
    tft.setTextSize(2);
    tft.setTextColor(sel ? mAccent[idx] : (uint16_t)0x528A);
    tft.setCursor(68, cy + 16); tft.print(mNames[idx]);
    tft.setTextSize(1);
    tft.setTextColor(sel ? (uint16_t)0x3186 : (uint16_t)0x2104);
    tft.setCursor(68, cy + 38); tft.print(mSubtitles[idx]);
    if (sel) {
      tft.fillTriangle(298, cy + 28, 298, cy + 44, 306, cy + 36, mAccent[idx]);
    } else {
      tft.fillRect(296, cy + 26, 14, 22, 0x0821);
    }
  }
}

void launchBrickoutMode(int mode) {
  if (mode == 0) {
    bcoutLoadHiScore();
    bcoutInitClassic();
    currentGame = GAME_BRICKOUT_CLASSIC;
  } else {
    brickoutSelectedLevel = 1;
    brickoutDrawLevelSelect();
    currentGame = GAME_BRICKOUT_SELECT;
  }
}

// ================================================================
// =================== PAGE-BASED MENU SYSTEM =====================
// ================================================================
int getGamePage(int idx) { return idx / MENU_PER_PAGE; }
int getGamePosInPage(int idx) { return idx % MENU_PER_PAGE; }
int getTotalPages() { return (MENU_ITEMS + MENU_PER_PAGE - 1) / MENU_PER_PAGE; }

void getCardXY(int pos, int &cx, int &cy) {
  int col = pos % MENU_COLS;
  int row = pos / MENU_COLS;
  cx = CARD_START_X + col * (CARD_W + CARD_GAP_X);
  cy = CARD_START_Y + row * (CARD_H + CARD_GAP_Y);
}

void drawMenuChrome() {
  tft.fillScreen(MENU_BG);
  tft.fillRect(0,0,screenWidth,MENU_HEADER_H,0x0008);
  tft.drawFastHLine(0,MENU_HEADER_H,screenWidth,0x1082);
  tft.drawFastVLine(6,4,28,0x2124);
  tft.drawFastHLine(6,4,4,0x2124); tft.drawFastHLine(6,31,4,0x2124);
  tft.setTextSize(2); tft.setTextColor(0x07FF);
  tft.setCursor(18,8); tft.print("ARCADE");
  tft.setTextSize(1); tft.setTextColor(0x2945);
  tft.setCursor(18,26); tft.print("11 GAMES  v5.0  ESP32");
  for(int i=0;i<4;i++)
    tft.fillCircle(screenWidth-16-i*9,19,3,(i==0)?0xF800:(i==1)?0xFFE0:(i==2)?0x07E0:0x07FF);
  tft.drawFastVLine(screenWidth-6,4,28,0x2124);
  tft.drawFastHLine(screenWidth-10,4,4,0x2124); tft.drawFastHLine(screenWidth-10,31,4,0x2124);
  int totalPages=getTotalPages();
  int dotY=screenHeight-8, dotSpacing=14;
  int dotStartX=screenWidth/2-(totalPages*dotSpacing)/2;
  for(int p=0;p<totalPages;p++) {
    uint16_t dc=(p==menuPage)?0x07FF:(uint16_t)0x2104;
    int dx=dotStartX+p*dotSpacing;
    if(p==menuPage) tft.fillCircle(dx,dotY,4,dc); else tft.drawCircle(dx,dotY,3,dc);
  }
  tft.drawFastHLine(0,screenHeight-15,screenWidth,0x1082);
  tft.fillRect(0,screenHeight-14,screenWidth,14,0x0008);
  tft.setTextSize(1);
  tft.setTextColor(0x3186); tft.setCursor(6,screenHeight-11); tft.print("[UP/DN/LR]");
  tft.setTextColor(0x2945); tft.setCursor(70,screenHeight-11); tft.print("Nav");
  tft.setTextColor(0x07E0); tft.setCursor(100,screenHeight-11); tft.print("[A]");
  tft.setTextColor(0x2945); tft.setCursor(116,screenHeight-11); tft.print("Launch");
  tft.setTextColor(0x07FF); tft.setCursor(170,screenHeight-11); tft.print("LR");
  tft.setTextColor(0x2945); tft.setCursor(184,screenHeight-11); tft.print("PgFlip");
  char pbuf[8]; sprintf(pbuf,"P%d/%d",menuPage+1,getTotalPages());
  tft.setTextColor(0xFFE0); tft.setCursor(screenWidth-50,screenHeight-11); tft.print(pbuf);
}

void drawMenuCard(int idx, bool active) {
  if(idx<0||idx>=MENU_ITEMS) return;
  int pos=getGamePosInPage(idx);
  int cx,cy; getCardXY(pos,cx,cy);
  uint16_t accent=GAME_ACCENT[idx];
  uint16_t dim=GAME_DIM[idx];

  if(active) {
    tft.fillRoundRect(cx,cy,CARD_W,CARD_H,5,dim);
    tft.fillRoundRect(cx,cy,4,CARD_H,2,accent);
    tft.drawFastHLine(cx,cy,CARD_W,accent);
    tft.drawFastHLine(cx,cy+1,CARD_W,accent);
    tft.drawRoundRect(cx+1,cy+1,CARD_W-2,CARD_H-2,4,(uint16_t)(accent>>1));
  } else {
    tft.fillRoundRect(cx,cy,CARD_W,CARD_H,5,MENU_BG);
    tft.drawRoundRect(cx,cy,CARD_W,CARD_H,5,0x1082);
  }

  int iconBgH=38;
  uint16_t iconBg=active?(uint16_t)(dim|0x0820):(uint16_t)0x0821;
  tft.fillRect(cx+4,cy+3,CARD_W-8,iconBgH,iconBg);
  tft.drawFastHLine(cx+4,cy+3+iconBgH,CARD_W-8,active?accent:(uint16_t)0x1082);

  int iconCX=cx+CARD_W/2, iconCY=cy+3+iconBgH/2;
  uint16_t ic=active?accent:(uint16_t)0x2104;

  switch(idx) {
    case 0:
      tft.drawFastHLine(iconCX-8,iconCY,10,ic);
      tft.drawFastVLine(iconCX+2,iconCY,8,ic);
      tft.drawFastHLine(iconCX-4,iconCY+8,7,ic);
      tft.fillRect(iconCX-7,iconCY-4,6,5,ic);
      tft.fillCircle(iconCX-4,iconCY-5,1,active?(uint16_t)0x0000:ic); break;
    case 1:
      tft.fillTriangle(iconCX-10,iconCY+2,iconCX+10,iconCY-4,iconCX+10,iconCY+8,ic);
      tft.fillCircle(iconCX+8,iconCY-4,3,ic);
      tft.fillRect(iconCX-9,iconCY-10,4,22,ic);
      tft.fillRect(iconCX+9,iconCY-10,4,22,ic); break;
    case 2:
      for(int r=0;r<2;r++) for(int c=0;c<4;c++)
        tft.fillCircle(iconCX-9+c*6,iconCY-3+r*7,2,ic); break;
    case 3:
      tft.drawFastVLine(iconCX-3,iconCY-8,16,ic);
      tft.drawFastVLine(iconCX+3,iconCY-8,16,ic);
      tft.drawFastHLine(iconCX-8,iconCY-3,16,ic);
      tft.drawFastHLine(iconCX-8,iconCY+3,16,ic); break;
    case 4:
      tft.fillRoundRect(iconCX-6,iconCY,12,8,2,ic);
      for(int f=0;f<4;f++) tft.fillRoundRect(iconCX-6+f*3,iconCY-6,2,7,1,ic); break;
    case 5:
      tft.fillRect(iconCX-8,iconCY-6,4,14,ic);
      tft.fillRect(iconCX+4,iconCY-6,4,14,ic);
      tft.fillTriangle(iconCX,iconCY+2,iconCX-6,iconCY+2,iconCX-3,iconCY-1,ic); break;
    case 6:
      tft.fillRect(iconCX-10,iconCY-6,4,12,ic);
      tft.fillRect(iconCX+6,iconCY-6,4,12,ic);
      tft.fillCircle(iconCX,iconCY,3,ic); break;
    case 7:
      tft.fillCircle(iconCX+6,iconCY,5,ic);
      tft.drawFastHLine(iconCX-8,iconCY,10,ic);
      tft.drawPixel(iconCX-6,iconCY-1,ic);
      tft.drawPixel(iconCX-4,iconCY+1,ic);
      tft.drawPixel(iconCX-2,iconCY-1,ic); break;
    case 8:
      tft.fillRoundRect(iconCX-5,iconCY-8,10,16,2,ic);
      tft.fillRect(iconCX-7,iconCY-4,2,5,ic);
      tft.fillRect(iconCX+5,iconCY-4,2,5,ic);
      tft.fillRect(iconCX-3,iconCY-4,6,5,active?iconBg:(uint16_t)MENU_BG); break;
    case 9:
      tft.fillRect(iconCX-5,iconCY-8,8,10,ic);
      tft.fillRect(iconCX-7,iconCY-6,3,4,ic);
      tft.fillRect(iconCX+3,iconCY-2,5,4,ic);
      tft.fillRect(iconCX-5,iconCY+2,10,6,ic);
      tft.fillRect(iconCX-7,iconCY+4,3,2,ic);
      tft.fillRect(iconCX+3,iconCY+4,3,2,ic);
      tft.drawPixel(iconCX+1,iconCY-7,active?iconBg:(uint16_t)MENU_BG); break;
    case 10:
      tft.fillRect(iconCX-9,iconCY-6,18,4,ic);
      tft.fillRect(iconCX-9,iconCY,18,4,ic);
      tft.fillRect(iconCX-9,iconCY+6,18,4,ic);
      for(int i=0;i<3;i++) {
        tft.fillRect(iconCX-9+i*6,iconCY-2,2,12,ic);
      }
      tft.fillCircle(iconCX,iconCY+12,5,ic);
      tft.fillCircle(iconCX,iconCY+12,2,TFT_WHITE);
      break;
  }

  tft.setTextSize(1);
  tft.setTextColor(active?accent:(uint16_t)0x528A);
  tft.setCursor(cx+5,cy+iconBgH+7); tft.print(GAME_NAMES[idx]);

  tft.setTextColor(active?(uint16_t)0x3186:(uint16_t)0x2104);
  tft.setCursor(cx+5,cy+iconBgH+18); tft.print(GAME_SUBTITLES[idx]);

  int badgeX=cx+CARD_W-22, badgeY=cy+iconBgH+5;
  tft.fillRoundRect(badgeX,badgeY,18,10,2,active?accent:(uint16_t)0x1082);
  tft.setTextSize(1);
  tft.setTextColor(active?(uint16_t)MENU_BG:(uint16_t)0x2945);
  tft.setCursor(badgeX+1,badgeY+2); tft.print(GAME_MODE_BADGE[idx]);

  tft.setTextColor(active?accent:(uint16_t)0x2104);
  tft.setCursor(cx+5,cy+iconBgH+30);
  char sbuf[10]; sprintf(sbuf,"HI:%04d",menuHighScores[idx]);
  tft.print(sbuf);

  if(active)
    tft.fillTriangle(cx+CARD_W-8,cy+CARD_H-10,cx+CARD_W-8,cy+CARD_H-4,cx+CARD_W-3,cy+CARD_H-7,accent);
}

void showMenu() {
  drawMenuChrome();
  int startIdx=menuPage*MENU_PER_PAGE;
  int endIdx=min(startIdx+MENU_PER_PAGE,MENU_ITEMS);
  for(int i=startIdx;i<endIdx;i++) drawMenuCard(i,i==selectedGame);
}

void updateMenuSelection(int oldSel, int newSel) {
  int oldPage=getGamePage(oldSel), newPage=getGamePage(newSel);
  if(oldPage!=newPage) { menuPage=newPage; showMenu(); }
  else { drawMenuCard(oldSel,false); drawMenuCard(newSel,true); }
}

void returnToMenu() { currentGame=GAME_MENU; showMenu(); }

// ================================================================
// ===================== SETUP & LOOP =============================
// ================================================================
void setup(){
  Serial.begin(115200);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A,     INPUT_PULLUP);
  pinMode(BTN_B,     INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT,INPUT_PULLUP);

  EEPROM.begin(512);
  int storedCR = EEPROM.read(1);
  if(storedCR != 255) { crHighScore = storedCR; menuHighScores[8] = crHighScore; }
  rtexLoadHighScore();
  menuHighScores[9] = rtexHighScore;
  bcoutLoadHiScore();
  menuHighScores[10] = bcoutHiScore;

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  screenWidth  = tft.width();
  screenHeight = tft.height();

  randomSeed(analogRead(34));
  initDots();

  tft.setTextColor(0x07FF); tft.setTextSize(3);
  tft.setCursor(70,90);  tft.print("ARCADE");
  tft.setTextColor(0xFFE0); tft.setTextSize(2);
  tft.setCursor(85,125); tft.print("CONSOLE");
  tft.setTextColor(0x2945); tft.setTextSize(1);
  tft.setCursor(85,155); tft.print("11 GAMES  v5.0");
  for(int i=0;i<4;i++)
    tft.fillCircle(130+i*16,175,3,(i==0)?0xF800:(i==1)?0xFFE0:(i==2)?0x07E0:0x07FF);
  delay(1800);
  showMenu();
}

void loop(){
  static unsigned long lastBtnTime=0;
  static bool upPressed=false,downPressed=false,leftPressed=false,rightPressed=false;

  switch(currentGame){

    case GAME_MENU:{
      if(digitalRead(BTN_UP)==LOW&&!upPressed&&millis()-lastBtnTime>180){
        lastBtnTime=millis();upPressed=true;
        int old=selectedGame,posInPage=getGamePosInPage(selectedGame);
        if(posInPage<MENU_COLS){int prevPage=menuPage-1;if(prevPage<0)prevPage=getTotalPages()-1;int newIdx=prevPage*MENU_PER_PAGE+(MENU_ROWS-1)*MENU_COLS+(posInPage%MENU_COLS);if(newIdx>=MENU_ITEMS)newIdx=MENU_ITEMS-1;selectedGame=newIdx;}
        else selectedGame-=MENU_COLS;
        updateMenuSelection(old,selectedGame);
      } else if(digitalRead(BTN_UP)==HIGH) upPressed=false;

      if(digitalRead(BTN_DOWN)==LOW&&!downPressed&&millis()-lastBtnTime>180){
        lastBtnTime=millis();downPressed=true;
        int old=selectedGame,posInPage=getGamePosInPage(selectedGame);
        if(posInPage>=(MENU_ROWS-1)*MENU_COLS){int nextPage=menuPage+1;if(nextPage>=getTotalPages())nextPage=0;int col=posInPage%MENU_COLS;int newIdx=nextPage*MENU_PER_PAGE+col;if(newIdx>=MENU_ITEMS)newIdx=nextPage*MENU_PER_PAGE;selectedGame=newIdx;}
        else{selectedGame+=MENU_COLS;if(selectedGame>=MENU_ITEMS)selectedGame=MENU_ITEMS-1;}
        updateMenuSelection(old,selectedGame);
      } else if(digitalRead(BTN_DOWN)==HIGH) downPressed=false;

      if(digitalRead(BTN_LEFT)==LOW&&!leftPressed&&millis()-lastBtnTime>180){
        lastBtnTime=millis();leftPressed=true;
        int old=selectedGame,posInPage=getGamePosInPage(selectedGame),col=posInPage%MENU_COLS;
        if(col==0){int row=posInPage/MENU_COLS;int prevPage=menuPage-1;if(prevPage<0)prevPage=getTotalPages()-1;int newIdx=prevPage*MENU_PER_PAGE+row*MENU_COLS+(MENU_COLS-1);if(newIdx>=MENU_ITEMS)newIdx=MENU_ITEMS-1;selectedGame=newIdx;}
        else selectedGame--;
        updateMenuSelection(old,selectedGame);
      } else if(digitalRead(BTN_LEFT)==HIGH) leftPressed=false;

      if(digitalRead(BTN_RIGHT)==LOW&&!rightPressed&&millis()-lastBtnTime>180){
        lastBtnTime=millis();rightPressed=true;
        int old=selectedGame,posInPage=getGamePosInPage(selectedGame),col=posInPage%MENU_COLS,row=posInPage/MENU_COLS;
        if(col==MENU_COLS-1){int nextPage=menuPage+1;if(nextPage>=getTotalPages())nextPage=0;int newIdx=nextPage*MENU_PER_PAGE+row*MENU_COLS;if(newIdx>=MENU_ITEMS)newIdx=nextPage*MENU_PER_PAGE;selectedGame=newIdx;}
        else{int newIdx=selectedGame+1;if(newIdx>=MENU_ITEMS)newIdx=menuPage*MENU_PER_PAGE;selectedGame=newIdx;}
        updateMenuSelection(old,selectedGame);
      } else if(digitalRead(BTN_RIGHT)==HIGH) rightPressed=false;

      if(digitalRead(BTN_A)==LOW&&millis()-lastBtnTime>200){
        lastBtnTime=millis();
        switch(selectedGame){
          case 0: showSnakeSubmenu(); break;
          case 1: showFlappySubmenu(); break;
          case 2: currentGame=GAME_CONNECT4;  resetGameC4();            break;
          case 3: currentGame=GAME_TICTACTOE; resetTTTGame();           break;
          case 4: currentGame=GAME_RPS; drawMintBackground();updateScoreRPS();drawSelectionBoxes(); break;
          case 5: currentGame=GAME_NINJA;     showStartScreenNinja();   break;
          case 6: currentGame=GAME_PONG;      showStartScreenPong();    break;
          case 7: currentGame=GAME_SPERMRACE; showStartScreenSperm();   break;
          case 8: currentGame=GAME_CARRACING; showStartScreenCar();     break;
          case 9: showRtexSubmenu();                                     break;
          case 10: showBrickoutSubmenu();                                break;
        }
      }
      break;
    }

    case GAME_SNAKE_SUBMENU:{
      static unsigned long lastSubBtn=0;
      if(digitalRead(BTN_UP)==LOW&&!upPressed&&millis()-lastSubBtn>180){
        lastSubBtn=millis();upPressed=true;
        int old=snakeSubSelected; snakeSubSelected--;
        if(snakeSubSelected<0)snakeSubSelected=2;
        updateSnakeSubmenu(old,snakeSubSelected);
      }else if(digitalRead(BTN_UP)==HIGH)upPressed=false;
      
      if(digitalRead(BTN_DOWN)==LOW&&!downPressed&&millis()-lastSubBtn>180){
        lastSubBtn=millis();downPressed=true;
        int old=snakeSubSelected; snakeSubSelected++;
        if(snakeSubSelected>2)snakeSubSelected=0;
        updateSnakeSubmenu(old,snakeSubSelected);
      }else if(digitalRead(BTN_DOWN)==HIGH)downPressed=false;
      
      if((digitalRead(BTN_A)==LOW||digitalRead(BTN_SELECT)==LOW)&&millis()-lastSubBtn>300){
        lastSubBtn=millis(); launchSnakeMode(snakeSubSelected);
      }
      if(digitalRead(BTN_B)==LOW&&millis()-lastSubBtn>300){
        lastSubBtn=millis(); returnToMenu();
      }
      break;
    }

    case GAME_SNAKE_CLASSIC:
    case GAME_SNAKE_BARRIER:
    case GAME_SNAKE_NOBORDER:
      runSnakeGame();
      break;

    case GAME_FLAPPY_SUBMENU:{
      static unsigned long lastSubBtn=0;
      if(digitalRead(BTN_UP)==LOW&&!upPressed&&millis()-lastSubBtn>180){lastSubBtn=millis();upPressed=true;int old=flappySubSelected;flappySubSelected--;if(flappySubSelected<0)flappySubSelected=2;updateFlappySubmenu(old,flappySubSelected);}else if(digitalRead(BTN_UP)==HIGH)upPressed=false;
      if(digitalRead(BTN_DOWN)==LOW&&!downPressed&&millis()-lastSubBtn>180){lastSubBtn=millis();downPressed=true;int old=flappySubSelected;flappySubSelected++;if(flappySubSelected>2)flappySubSelected=0;updateFlappySubmenu(old,flappySubSelected);}else if(digitalRead(BTN_DOWN)==HIGH)downPressed=false;
      if((digitalRead(BTN_A)==LOW||digitalRead(BTN_SELECT)==LOW)&&millis()-lastSubBtn>300){lastSubBtn=millis();launchFlappyVersion(flappySubSelected);}
      if(digitalRead(BTN_B)==LOW&&millis()-lastSubBtn>300){lastSubBtn=millis();returnToMenu();}
      break;
    }

    case GAME_FLAPPY:{
      if(flappyVersion==1) runFlappyNight1();
      else                 runFlappySA(flappyVersion);
      break;
    }

    case GAME_RTEX_SUBMENU:{
      static unsigned long lastRtexSubBtn=0;
      if(digitalRead(BTN_UP)==LOW&&!upPressed&&millis()-lastRtexSubBtn>180){
        lastRtexSubBtn=millis();upPressed=true;
        int old=rtexSubSelected; rtexSubSelected=1-rtexSubSelected;
        updateRtexSubmenu(old,rtexSubSelected);
      } else if(digitalRead(BTN_UP)==HIGH) upPressed=false;
      if(digitalRead(BTN_DOWN)==LOW&&!downPressed&&millis()-lastRtexSubBtn>180){
        lastRtexSubBtn=millis();downPressed=true;
        int old=rtexSubSelected; rtexSubSelected=1-rtexSubSelected;
        updateRtexSubmenu(old,rtexSubSelected);
      } else if(digitalRead(BTN_DOWN)==HIGH) downPressed=false;
      if((digitalRead(BTN_A)==LOW||digitalRead(BTN_SELECT)==LOW)&&millis()-lastRtexSubBtn>300){
        lastRtexSubBtn=millis(); launchRtexVersion(rtexSubSelected);
      }
      if(digitalRead(BTN_B)==LOW&&millis()-lastRtexSubBtn>300){
        lastRtexSubBtn=millis(); returnToMenu();
      }
      break;
    }

    case GAME_RTEX: runRtexGame(); break;
    
    case GAME_BRICKOUT_SUBMENU:{
      static unsigned long lastBrickoutBtn=0;
      if (brickoutSubSelected == 1) {
        runBrickoutSelectMode();
        break;
      }
      
      if(digitalRead(BTN_UP)==LOW&&!upPressed&&millis()-lastBrickoutBtn>180){
        lastBrickoutBtn=millis(); upPressed=true;
        int old=brickoutSubSelected; brickoutSubSelected=1-brickoutSubSelected;
        updateBrickoutSubmenu(old,brickoutSubSelected);
      } else if(digitalRead(BTN_UP)==HIGH) upPressed=false;
      
      if(digitalRead(BTN_DOWN)==LOW&&!downPressed&&millis()-lastBrickoutBtn>180){
        lastBrickoutBtn=millis(); downPressed=true;
        int old=brickoutSubSelected; brickoutSubSelected=1-brickoutSubSelected;
        updateBrickoutSubmenu(old,brickoutSubSelected);
      } else if(digitalRead(BTN_DOWN)==HIGH) downPressed=false;
      
      if((digitalRead(BTN_A)==LOW||digitalRead(BTN_SELECT)==LOW)&&millis()-lastBrickoutBtn>300){
        lastBrickoutBtn=millis(); 
        launchBrickoutMode(brickoutSubSelected);
      }
      if(digitalRead(BTN_B)==LOW&&millis()-lastBrickoutBtn>300){
        lastBrickoutBtn=millis(); returnToMenu();
      }
      break;
    }
    
    case GAME_BRICKOUT_CLASSIC: runBrickoutGame(); break;
    case GAME_BRICKOUT_SELECT: runBrickoutGame(); break;

    case GAME_CONNECT4:  runConnect4Game();   break;
    case GAME_TICTACTOE: runTicTacToe();      break;
    case GAME_RPS:       runRPSGame();        break;
    case GAME_NINJA:     runNinjaGame();      break;
    case GAME_PONG:      runPongGame();       break;
    case GAME_SPERMRACE: runSpermRaceGame();  break;
    case GAME_CARRACING: runCarRacingGame();  break;
  }

  delay(10);
}
