#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Common Button Pins for both games
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_A        14      // Also used as SELECT in Snake
#define BTN_B        33      // Also used as START in Flappy
#define BTN_START    32
#define BTN_SELECT   25

// Game selection
enum GameMode {
  GAME_MENU,
  GAME_SNAKE,
  GAME_FLAPPY
};

GameMode currentGame = GAME_MENU;
int selectedGame = 0;  // 0 = Snake, 1 = Flappy Bird

// ============= SNAKE GAME VARIABLES =============
#define SNAKE_GRID      10
#define MAX_SNAKE_LEN   200

// Color definitions
#define COL_BG          0x0000  // Black
#define COL_SNAKE_HEAD  0xF800  // Red
#define COL_SNAKE_BODY  0xAFE5  // Light Green
#define COL_FOOD        0x07FF  // Cyan
#define COL_BONUS       0xF81F  // Magenta/Pink
#define COL_SCORE       0xFFFF  // White
#define COL_GOLD        0xFD20  // Gold/Yellow

struct Point { 
  int x, y; 
};

Point snake[MAX_SNAKE_LEN];
int snakeLen;
Point food, bonusFood; 
int dirX, dirY, score;
int highScoreSnake = 0; 
bool gameOverSnake = false;
bool isPausedSnake = false; 
int foodCounter = 0;   
unsigned long lastMoveTime;
unsigned long lastPulseTime;
int moveDelay = 150; 
int pulseState = 0;

bool bonusActive = false;
unsigned long bonusStartTime;

// ============= FLAPPY BIRD GAME VARIABLES =============
#define MAX_DOTS 30
struct Dot {
  float x;
  float y;
  float speed;
  int brightness;
};
Dot bgDots[MAX_DOTS];

struct Pipe {
  float x;
  int gapY;
  int gapH;
  bool counted;
};

Pipe pipes[4];
const int PIPE_W = 28;
const int CAP_H = 8;
const float SCROLL_SPEED = 1.8;
const int MIN_PIPE_DISTANCE = 120;
const int MAX_PIPE_DISTANCE = 250;

float birdY = 120, prevBirdY = 120;
float velocity = 0, gravity = 0.18, flapPower = -2.5;
int scoreFlappy = 0, highScoreFlappy = 0;
bool playing = false;
bool isPausedFlappy = false;
bool isCountdown = false;
unsigned long countdownStartTime = 0;
int countdownValue = 3;

// Flappy Colors
#define BG_COLOR     TFT_BLACK
#define PIPE_COLOR   TFT_GREEN
#define PIPE_CAP_COLOR TFT_DARKGREEN
#define BIRD_BODY_COLOR   TFT_YELLOW
#define BIRD_DETAIL_COLOR TFT_RED

// Bird Bitmaps
const unsigned char bird_body_bmp[] PROGMEM = {
  0x00, 0x7e, 0x00, 0x01, 0xff, 0x80, 0x03, 0xff, 0xc0,
  0x07, 0xff, 0xe0, 0x0f, 0xff, 0xf0, 0x1f, 0xff, 0xf8,
  0x3f, 0xff, 0xfc, 0x7f, 0xff, 0xfe, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0x7f, 0xff, 0xfe, 0x7f, 0xff, 0xfe,
  0x3f, 0xff, 0xfc, 0x1f, 0xff, 0xf8, 0x0f, 0xff, 0xf0,
  0x07, 0xff, 0xe0, 0x01, 0xff, 0x80, 0x00, 0x7e, 0x00
};

const unsigned char bird_details_bmp[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x70, 0x00, 0x00, 0xf8, 0x00, 0x01, 0xf8, 0x00,
  0x01, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e,
  0x00, 0x01, 0xff, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

int screenWidth, screenHeight;

// ============= UTILITY FUNCTIONS =============
void showMenu() {
  tft.fillScreen(TFT_BLACK);
  
  // Draw title border
  tft.drawRect(20, 15, 280, 60, TFT_CYAN);
  tft.drawRect(22, 17, 276, 56, TFT_CYAN);
  
  // Draw title
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(50, 30);
  tft.print("GAMING");
  tft.setCursor(70, 60);
  tft.print("CONSOLE");
  
  // Draw menu options
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(80, 120);
  tft.print("1. SNAKE");
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(60, 170);
  tft.print("2. FLAPPY BIRD");
  
  // Draw selection indicator (arrow)
  tft.fillTriangle(45, 125 + (selectedGame * 50), 
                   45, 135 + (selectedGame * 50),
                   35, 130 + (selectedGame * 50),
                   TFT_RED);
  
  // Draw instructions
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(30, 220);
  tft.print("UP/DOWN: Select   A: Confirm");
  
  // Decorative elements
  tft.fillCircle(300, 10, 5, TFT_RED);
  tft.fillCircle(20, 230, 3, TFT_GREEN);
  tft.fillCircle(300, 230, 3, TFT_YELLOW);
}

void updateMenuSelection() {
  // Clear old triangle
  tft.fillTriangle(45, 125, 45, 135, 35, 130, TFT_BLACK);
  tft.fillTriangle(45, 175, 45, 185, 35, 180, TFT_BLACK);
  
  // Draw new triangle
  tft.fillTriangle(45, 125 + (selectedGame * 50), 
                   45, 135 + (selectedGame * 50),
                   35, 130 + (selectedGame * 50),
                   TFT_RED);
}

void returnToMenu() {
  currentGame = GAME_MENU;
  showMenu();
}

// ============= SNAKE GAME FUNCTIONS =============
void resetSnakeGame();
void drawSnakePart(int index, bool erase);
void drawFood(bool erase);
void drawBonusFood(bool erase);
void spawnFood();
void spawnBonusFood();
void updateSnakeScoreDisplay();
void showSnakeGameOver();
void showSnakePauseScreen(bool pause);

void updateSnakeScoreDisplay() {
  tft.fillRect(0, 0, tft.width(), 25, COL_BG);
  tft.setTextColor(COL_SCORE); 
  tft.setTextSize(2);
  tft.setCursor(10, 5);
  tft.print("Score: ");
  tft.print(score);
  
  tft.setCursor(tft.width() - 150, 5);
  tft.print("HIGH: ");
  tft.print(highScoreSnake);
}

void showSnakePauseScreen(bool pause) {
  if (pause) {
    tft.fillRect(60, 80, 200, 60, 0x001F);
    tft.drawRect(60, 80, 200, 60, COL_SCORE);
    tft.setCursor(120, 105);
    tft.setTextColor(COL_SCORE);
    tft.setTextSize(3);
    tft.print("PAUSED");
    tft.setTextSize(2);
  } else {
    tft.fillRect(60, 80, 200, 60, COL_BG);
    drawFood(false);
    if(bonusActive) drawBonusFood(false);
  }
}

void spawnFood() {
  drawFood(true); 
  int maxGridX = (tft.width() - 20) / SNAKE_GRID;
  int maxGridY = (tft.height() - 50) / SNAKE_GRID;
  
  bool validPosition;
  do {
    validPosition = true;
    food.x = random(2, maxGridX - 2) * SNAKE_GRID + 10;
    food.y = random(4, maxGridY - 2) * SNAKE_GRID + 40;
    
    for (int i = 0; i < snakeLen; i++) {
      int snakePxX = snake[i].x * SNAKE_GRID + 10;
      int snakePxY = snake[i].y * SNAKE_GRID + 40;
      if (abs(snakePxX - food.x) < SNAKE_GRID && abs(snakePxY - food.y) < SNAKE_GRID) {
        validPosition = false;
        break;
      }
    }
  } while (!validPosition);
  
  drawFood(false);
}

void spawnBonusFood() {
  if (!bonusActive) {
    bonusActive = true;
    bonusStartTime = millis();
    int maxGridX = (tft.width() - 20) / SNAKE_GRID;
    int maxGridY = (tft.height() - 50) / SNAKE_GRID;
    
    bool validPosition;
    do {
      validPosition = true;
      bonusFood.x = random(2, maxGridX - 2) * SNAKE_GRID + 10;
      bonusFood.y = random(4, maxGridY - 2) * SNAKE_GRID + 40;
      
      for (int i = 0; i < snakeLen; i++) {
        int snakePxX = snake[i].x * SNAKE_GRID + 10;
        int snakePxY = snake[i].y * SNAKE_GRID + 40;
        if (abs(snakePxX - bonusFood.x) < SNAKE_GRID && abs(snakePxY - bonusFood.y) < SNAKE_GRID) {
          validPosition = false;
          break;
        }
      }
      
      if (abs(food.x - bonusFood.x) < SNAKE_GRID && abs(food.y - bonusFood.y) < SNAKE_GRID) {
        validPosition = false;
      }
    } while (!validPosition);
    
    drawBonusFood(false);
  }
}

void drawSnakePart(int index, bool erase) {
  int x = snake[index].x * SNAKE_GRID + 10;
  int y = snake[index].y * SNAKE_GRID + 40;
  
  if (erase) {
    tft.fillRect(x, y, SNAKE_GRID, SNAKE_GRID, COL_BG);
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

void drawFood(bool erase) {
  if(erase) {
    tft.fillCircle(food.x, food.y, 8, COL_BG);
  } else {
    tft.fillCircle(food.x, food.y, 7, COL_FOOD);
    tft.fillCircle(food.x, food.y, 4, COL_SCORE);
    tft.fillCircle(food.x, food.y, 2, 0x001F);
  }
}

void drawBonusFood(bool erase) {
  if (!bonusActive && !erase) return;
  if (erase) {
    tft.fillCircle(bonusFood.x, bonusFood.y, 15, COL_BG); 
  } else {
    int rad = 8 + (pulseState % 4); 
    tft.fillCircle(bonusFood.x, bonusFood.y, 12, COL_BG);
    tft.fillCircle(bonusFood.x, bonusFood.y, rad, COL_BONUS);
    tft.fillCircle(bonusFood.x, bonusFood.y, 5, COL_GOLD);
    for(int i = 0; i < 8; i++) {
      float angle = i * 45 * 3.14159 / 180;
      int x = bonusFood.x + cos(angle) * 12;
      int y = bonusFood.y + sin(angle) * 12;
      tft.fillCircle(x, y, 2, COL_GOLD);
    }
  }
}

void showSnakeGameOver() {
  if (score > highScoreSnake) highScoreSnake = score;

  tft.fillScreen(COL_BG);
  
  tft.drawRect(5, 5, tft.width()-10, tft.height()-10, COL_GOLD);
  tft.drawRect(8, 8, tft.width()-16, tft.height()-16, COL_GOLD);
  
  tft.setTextSize(3);
  tft.setTextColor(COL_SNAKE_HEAD);
  tft.setCursor(tft.width()/2 - 80, 40);
  tft.print("GAME OVER");
  
  tft.drawFastHLine(30, 85, tft.width()-60, COL_SCORE);
  
  tft.fillRoundRect(20, 100, tft.width()-40, 90, 10, 0x1082);
  
  tft.setTextSize(2);
  tft.setTextColor(COL_SCORE);
  tft.setCursor(40, 120);
  tft.print("YOUR SCORE : ");
  tft.setTextColor(COL_GOLD);
  tft.print(score);
  
  tft.setTextColor(COL_SCORE);
  tft.setCursor(40, 150);
  tft.print("HIGH SCORE : ");
  tft.setTextColor(COL_FOOD);
  tft.print(highScoreSnake);
  
  tft.setTextSize(1);
  tft.setTextColor(COL_SCORE);
  tft.setCursor(50, 200);
  tft.setTextSize(2);
  tft.print("Press SELECT to Menu");
  
  while(digitalRead(BTN_SELECT) == HIGH && digitalRead(BTN_A) == HIGH) {
    delay(50);
  }
  delay(300);
  returnToMenu();
}

void resetSnakeGame() {
  tft.fillScreen(COL_BG);
  
  snakeLen = 5; 
  score = 0; 
  foodCounter = 0;
  moveDelay = 150; 
  bonusActive = false; 
  isPausedSnake = false;
  gameOverSnake = false;
  pulseState = 0;
  
  int maxGridX = (tft.width() - 20) / SNAKE_GRID;
  int maxGridY = (tft.height() - 50) / SNAKE_GRID;
  int startX = maxGridX / 2;
  int startY = maxGridY / 2;
  
  for (int i = 0; i < snakeLen; i++) {
    snake[i] = {startX - i, startY};
    drawSnakePart(i, false);
  }
  
  dirX = 1; 
  dirY = 0; 
  
  spawnFood();
  updateSnakeScoreDisplay();
  
  lastMoveTime = millis();
  lastPulseTime = millis();
}

void runSnakeGame() {
  static unsigned long lastSelectPress = 0;
  static unsigned long lastBPress = 0;
  
  // Check for menu exit (B button press)
  if (digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) {
    lastBPress = millis();
    returnToMenu();
    return;
  }
  
  if (digitalRead(BTN_SELECT) == LOW && millis() - lastSelectPress > 300) {
    lastSelectPress = millis();
    if (gameOverSnake) {
      resetSnakeGame();
    } else {
      isPausedSnake = !isPausedSnake; 
      showSnakePauseScreen(isPausedSnake);
    }
  }

  if (!gameOverSnake && !isPausedSnake) {
    if (digitalRead(BTN_UP) == LOW && dirY == 0) {
      dirX = 0; dirY = -1;
    }
    else if (digitalRead(BTN_DOWN) == LOW && dirY == 0) {
      dirX = 0; dirY = 1;
    }
    else if (digitalRead(BTN_LEFT) == LOW && dirX == 0) {
      dirX = -1; dirY = 0;
    }
    else if (digitalRead(BTN_RIGHT) == LOW && dirX == 0) {
      dirX = 1; dirY = 0;
    }

    if (millis() - lastPulseTime > 150) { 
      lastPulseTime = millis();
      pulseState++;
      if (bonusActive) {
        if (millis() - bonusStartTime > 4000) { 
          drawBonusFood(true);
          bonusActive = false;
        } else { 
          drawBonusFood(false); 
        }
      }
      drawFood(false);
      updateSnakeScoreDisplay();
    }

    if (millis() - lastMoveTime > moveDelay) {
      lastMoveTime = millis();
      
      int nextX = snake[0].x + dirX;
      int nextY = snake[0].y + dirY;
      int maxX = (tft.width() - 20) / SNAKE_GRID;
      int maxY = (tft.height() - 50) / SNAKE_GRID;

      if (nextX < 0 || nextX >= maxX || nextY < 0 || nextY >= maxY) {
        gameOverSnake = true;
        showSnakeGameOver();
        return;
      }

      Point nextHead = {nextX, nextY};
      
      for (int i = 0; i < snakeLen; i++) {
        if (nextHead.x == snake[i].x && nextHead.y == snake[i].y) {
          gameOverSnake = true; 
          showSnakeGameOver(); 
          return;
        }
      }

      bool ateAnything = false;
      int headPxX = nextHead.x * SNAKE_GRID + 10 + (SNAKE_GRID/2);
      int headPxY = nextHead.y * SNAKE_GRID + 40 + (SNAKE_GRID/2);
      
      if (abs(headPxX - food.x) < SNAKE_GRID && abs(headPxY - food.y) < SNAKE_GRID) {
        score += 10; 
        snakeLen++; 
        foodCounter++;
        ateAnything = true;
        
        if (moveDelay > 50) moveDelay -= 2;
        
        spawnFood();
        if (foodCounter >= 5 && !bonusActive) { 
          spawnBonusFood(); 
          foodCounter = 0; 
        }
      } 
      else if (bonusActive && abs(headPxX - bonusFood.x) < SNAKE_GRID+2 && abs(headPxY - bonusFood.y) < SNAKE_GRID+2) {
        unsigned long timeTaken = (millis() - bonusStartTime) / 1000;
        int dynamicBonus = 80 - (timeTaken * 10);
        if (dynamicBonus < 10) dynamicBonus = 10;
        
        score += dynamicBonus;
        snakeLen++; 
        bonusActive = false; 
        ateAnything = true;
        drawBonusFood(true); 
      }

      if (!ateAnything) {
        drawSnakePart(snakeLen - 1, true);
      }
      
      for (int i = snakeLen - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
      }
      snake[0] = nextHead;
      
      for (int i = (ateAnything ? 0 : 1); i < snakeLen; i++) {
        drawSnakePart(i, false);
      }
      if(!ateAnything) drawSnakePart(0, false);
      
      updateSnakeScoreDisplay(); 
    }
  }
}

// ============= FLAPPY BIRD GAME FUNCTIONS =============
void initDots() {
  for (int i = 0; i < MAX_DOTS; i++) {
    bgDots[i].x = random(0, screenWidth);
    bgDots[i].y = random(0, screenHeight);
    bgDots[i].speed = random(3, 12) / 10.0;
    bgDots[i].brightness = random(40, 180);
  }
}

void updateDots() {
  for (int i = 0; i < MAX_DOTS; i++) {
    tft.drawPixel((int)bgDots[i].x, (int)bgDots[i].y, BG_COLOR);
    
    bgDots[i].x -= bgDots[i].speed;
    
    if (bgDots[i].x < 0) {
      bgDots[i].x = screenWidth;
      bgDots[i].y = random(0, screenHeight);
      bgDots[i].speed = random(3, 12) / 10.0;
      bgDots[i].brightness = random(40, 180);
    }
    
    uint16_t dotColor = tft.color565(bgDots[i].brightness, bgDots[i].brightness, bgDots[i].brightness);
    tft.drawPixel((int)bgDots[i].x, (int)bgDots[i].y, dotColor);
  }
}

void showPauseOverlayFlappy() {
  int centerX = screenWidth / 2;
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(centerX - 55, screenHeight/2 - 25);
  tft.print("PAUSED!");
}

void hidePauseOverlayFlappy() {
  int centerX = screenWidth / 2;
  tft.fillRect(centerX - 80, screenHeight/2 - 40, 160, 50, BG_COLOR);
}

void startCountdown() {
  isCountdown = true;
  countdownValue = 3;
  countdownStartTime = millis();
}

void updateCountdown() {
  if (!isCountdown) return;
  
  int centerX = screenWidth / 2;
  int centerY = screenHeight / 2;
  
  unsigned long elapsed = millis() - countdownStartTime;
  int newCountdown = 4 - (elapsed / 1000);
  
  if (newCountdown != countdownValue && newCountdown >= 0) {
    countdownValue = newCountdown;
    tft.fillRect(centerX - 40, centerY - 30, 40, 30, BG_COLOR);
    
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(4);
    if (countdownValue > 0) {
      tft.setCursor(centerX - 12, centerY - 20);
      tft.print(countdownValue);
    }
  }
  
  if (elapsed >= 3000) {
    tft.fillRect(centerX - 40, centerY - 30, 80, 60, BG_COLOR);
    isCountdown = false;
    isPausedFlappy = false;
  }
}

int getRandomDistance() {
  return random(MIN_PIPE_DISTANCE, MAX_PIPE_DISTANCE + 1);
}

void createPipe(int index, float startX) {
  pipes[index].x = startX;
  pipes[index].gapY = random(50, screenHeight - 130);
  pipes[index].gapH = random(70, 110);
  pipes[index].counted = false;
}

void startFlappyGame() {
  tft.fillScreen(BG_COLOR);
  initDots();
  scoreFlappy = 0; 
  isPausedFlappy = false;
  isCountdown = false;
  birdY = screenHeight / 2;
  velocity = 0;
  
  float currentX = screenWidth + 50;
  for (int i = 0; i < 4; i++) {
    createPipe(i, currentX);
    currentX += getRandomDistance();
  }
  
  playing = true;
}

void updateFlappyScreen() {
  updateDots();
  
  tft.fillRect(60, (int)prevBirdY, 28, 22, BG_COLOR);
  
  for (int i = 0; i < 4; i++) {
    int oldPipeEnd = pipes[i].x + PIPE_W + 5;
    if (oldPipeEnd > 0 && oldPipeEnd < screenWidth) {
      tft.fillRect(oldPipeEnd, 0, 10, screenHeight, BG_COLOR);
    }
    
    if (pipes[i].x < screenWidth && pipes[i].x + PIPE_W > 0) {
      tft.fillRect(pipes[i].x, 0, PIPE_W, pipes[i].gapY, PIPE_COLOR);
      tft.fillRect(pipes[i].x - 3, pipes[i].gapY - CAP_H, PIPE_W + 6, CAP_H, PIPE_CAP_COLOR);
      
      int bottomY = pipes[i].gapY + pipes[i].gapH;
      tft.fillRect(pipes[i].x, bottomY, PIPE_W, screenHeight - bottomY, PIPE_COLOR);
      tft.fillRect(pipes[i].x - 3, bottomY, PIPE_W + 6, CAP_H, PIPE_CAP_COLOR);
    }
  }
  
  tft.drawBitmap(60, (int)birdY, bird_body_bmp, 24, 18, BIRD_BODY_COLOR);
  tft.drawBitmap(60, (int)birdY, bird_details_bmp, 24, 18, BIRD_DETAIL_COLOR);
  
  tft.setTextColor(TFT_RED);
  tft.setTextSize(2);
  tft.setCursor(screenWidth - 75, 8);
  tft.print(scoreFlappy);
  
  tft.setTextSize(1);
}

void showStartScreenFlappy() {
  tft.fillScreen(BG_COLOR);
  
  for (int i = 0; i < MAX_DOTS; i++) {
    uint16_t dotColor = tft.color565(bgDots[i].brightness, bgDots[i].brightness, bgDots[i].brightness);
    tft.drawPixel((int)bgDots[i].x, (int)bgDots[i].y, dotColor);
  }
  
  tft.fillRoundRect(40, 25, 240, 70, 15, PIPE_COLOR);
  tft.setTextColor(BG_COLOR);
  tft.setTextSize(3);
  tft.setCursor(85, 48);
  tft.print("FLAPPY");
  tft.setCursor(100, 78);
  tft.print("BIRD");
  
  tft.fillRoundRect(30, 110, 260, 130, 10, PIPE_CAP_COLOR);
  tft.setTextColor(PIPE_COLOR);
  tft.setTextSize(1);
  tft.setCursor(55, 128);
  tft.print("HOW TO PLAY:");
  tft.setCursor(40, 145);
  tft.print("• Press SELECT to flap");
  tft.setCursor(40, 160);
  tft.print("• Press START to Pause");
  tft.setCursor(40, 175);
  tft.print("• Press SELECT to Resume");
  tft.setCursor(40, 190);
  tft.print("• 3-2-1-GO! countdown");
  tft.setCursor(40, 205);
  tft.print("• Press B to Menu");
  
  tft.setTextSize(1);
  tft.setTextColor(PIPE_COLOR);
  tft.setCursor(40, 225);
  tft.print("High Score: ");
  tft.print(highScoreFlappy);
}

void flappyGameOver() {
  playing = false;
  isPausedFlappy = false;
  isCountdown = false;
  if (scoreFlappy > highScoreFlappy) highScoreFlappy = scoreFlappy;
  
  int centerX = screenWidth / 2;
  
  for (int i = 0; i < 3; i++) {
    tft.fillRoundRect(centerX - 100, 45, 200, 130, 15, PIPE_COLOR);
    delay(100);
    tft.fillRoundRect(centerX - 100, 45, 200, 130, 15, PIPE_CAP_COLOR);
    delay(100);
  }
  
  tft.fillRoundRect(centerX - 100, 45, 200, 130, 15, PIPE_COLOR);
  tft.setTextColor(BG_COLOR);
  
  tft.setTextSize(2);
  tft.setCursor(centerX - 88, 68);
  tft.print("GAME OVER");
  
  tft.fillRect(centerX - 70, 95, 140, 50, BG_COLOR);
  tft.setTextColor(PIPE_COLOR);
  tft.setTextSize(2);
  tft.setCursor(centerX - 60, 105);
  tft.print("SCORE: ");
  tft.print(scoreFlappy);
  
  tft.setTextSize(1);
  tft.setCursor(centerX - 45, 130);
  tft.print("BEST: ");
  tft.print(highScoreFlappy);
  
  tft.setTextSize(1);
  tft.setTextColor(BG_COLOR);
  tft.setCursor(centerX - 75, 162);
  tft.print("PRESS SELECT BUTTON");
  
  delay(500);
  while(digitalRead(BTN_SELECT) == HIGH) {
    delay(50);
  }
  delay(200);
  showStartScreenFlappy();
}

void runFlappyGame() {
  static unsigned long lastStartPress = 0;
  static unsigned long lastSelectPress = 0;
  static unsigned long lastBPress = 0;
  
  // Check for menu exit (B button press)
  if (digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) {
    lastBPress = millis();
    playing = false;
    returnToMenu();
    return;
  }
  
  if (!playing) {
    updateDots();
    
    static int blinkCounter = 0;
    if (millis() - lastStartPress > 500) {
      lastStartPress = millis();
      blinkCounter++;
      
      tft.fillRect(40, 120, 220, 30, BG_COLOR);
      tft.setTextSize(1);
      tft.setTextColor(PIPE_COLOR);
      tft.setCursor(40, 120);
      if (blinkCounter % 2 == 0) {
        tft.print("> Press SELECT to Start <");
      } else {
        tft.print("  Press SELECT to Start  ");
      }
    }
    
    delay(30);
    
    if (digitalRead(BTN_SELECT) == LOW) {
      delay(100);
      startFlappyGame();
    }
    return;
  }

  if (!isPausedFlappy && !isCountdown && digitalRead(BTN_START) == LOW && (millis() - lastSelectPress > 200)) {
    lastSelectPress = millis();
    isPausedFlappy = true;
    showPauseOverlayFlappy();
    delay(200);
    return;
  }
  
  if (isPausedFlappy && !isCountdown && digitalRead(BTN_SELECT) == LOW && (millis() - lastSelectPress > 200)) {
    lastSelectPress = millis();
    hidePauseOverlayFlappy();
    startCountdown();
    delay(200);
    return;
  }
  
  if (isCountdown) {
    updateCountdown();
    delay(50);
    return;
  }
  
  if (isPausedFlappy) {
    delay(50);
    return;
  }

  if (digitalRead(BTN_SELECT) == LOW && (millis() - lastSelectPress > 50)) {
    lastSelectPress = millis();
    velocity = flapPower;
  }

  velocity += gravity;
  prevBirdY = birdY;
  birdY += velocity;

  for (int i = 0; i < 4; i++) {
    pipes[i].x -= SCROLL_SPEED;
    
    if (!pipes[i].counted && pipes[i].x + PIPE_W < 60) {
      pipes[i].counted = true;
      scoreFlappy += 5;
    }
    
    if (pipes[i].x < -PIPE_W - 20) {
      float farthestX = -999;
      for (int j = 0; j < 4; j++) {
        if (pipes[j].x > farthestX) {
          farthestX = pipes[j].x;
        }
      }
      int randomDistance = getRandomDistance();
      createPipe(i, farthestX + randomDistance);
    }
    
    int birdX = 60;
    int birdWidth = 24;
    int birdHeight = 18;
    
    if (pipes[i].x < birdX + birdWidth && (pipes[i].x + PIPE_W) > birdX) {
      if (birdY < pipes[i].gapY || (birdY + birdHeight) > (pipes[i].gapY + pipes[i].gapH)) {
        flappyGameOver(); 
        return;
      }
    }
  }
  
  if (birdY < 0 || birdY > screenHeight - 18) { 
    flappyGameOver(); 
    return; 
  }
  
  updateFlappyScreen();
  delay(12);
}

// ============= MAIN SETUP AND LOOP =============
void setup() {
  Serial.begin(115200);
  
  // Initialize button pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  // Seed random
  randomSeed(analogRead(34));
  
  // Initialize Flappy dimensions
  screenWidth = tft.width();
  screenHeight = tft.height();
  initDots();
  
  // Show menu
  showMenu();
}

void loop() {
  static unsigned long lastButtonTime = 0;
  static bool upPressed = false;
  static bool downPressed = false;
  
  switch (currentGame) {
    case GAME_MENU:
      // Handle UP button with debounce
      if (digitalRead(BTN_UP) == LOW && !upPressed && millis() - lastButtonTime > 200) {
        lastButtonTime = millis();
        upPressed = true;
        selectedGame = (selectedGame == 0) ? 1 : 0;
        updateMenuSelection();
      } 
      else if (digitalRead(BTN_UP) == HIGH) {
        upPressed = false;
      }
      
      // Handle DOWN button with debounce
      if (digitalRead(BTN_DOWN) == LOW && !downPressed && millis() - lastButtonTime > 200) {
        lastButtonTime = millis();
        downPressed = true;
        selectedGame = (selectedGame == 0) ? 1 : 0;
        updateMenuSelection();
      }
      else if (digitalRead(BTN_DOWN) == HIGH) {
        downPressed = false;
      }
      
      // Handle A button to confirm selection
      if (digitalRead(BTN_SELECT) == LOW && millis() - lastButtonTime > 200) {
        lastButtonTime = millis();
        if (selectedGame == 0) {
          currentGame = GAME_SNAKE;
          resetSnakeGame();
        } else {
          currentGame = GAME_FLAPPY;
          showStartScreenFlappy();
        }
      }
      break;
      
    case GAME_SNAKE:
      runSnakeGame();
      break;
      
    case GAME_FLAPPY:
      runFlappyGame();
      break;
  }
  
  delay(10);
}
