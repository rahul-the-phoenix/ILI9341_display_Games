#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Button pins
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_SELECT   25

// Color definitions
#define COL_BG          0x0000  // Black
#define COL_SNAKE_HEAD  0xF800  // Red
#define COL_SNAKE_BODY  0xAFE5  // Light Green
#define COL_FOOD        0x07FF  // Cyan
#define COL_BONUS       0xF81F  // Magenta/Pink
#define COL_SCORE       0xFFFF  // White
#define COL_GOLD        0xFD20  // Gold/Yellow

#define SNAKE_GRID      10      // 10px per cell
#define MAX_SNAKE_LEN   200

struct Point { 
  int x, y; 
};

Point snake[MAX_SNAKE_LEN];
int snakeLen;
Point food, bonusFood; 
int dirX, dirY, score;
int highScore = 0; 
bool gameOver;
bool isPaused = false; 
int foodCounter = 0;   
unsigned long lastMoveTime;
unsigned long lastPulseTime;
int moveDelay = 150; 
int pulseState = 0;

bool bonusActive = false;
unsigned long bonusStartTime;

// Grid dimensions for full screen
int maxGridX;
int maxGridY;

// Function prototypes
void resetGame();
void drawSnakePart(int index, bool erase);
void drawFood(bool erase);
void drawBonusFood(bool erase);
void spawnFood();
void spawnBonusFood();
void updateScoreDisplay();
void showGameOver();
void showPauseScreen(bool pause);

void updateScoreDisplay() {
  // Show score at the top of the screen
  tft.fillRect(0, 0, tft.width(), 20, COL_BG);
  tft.setTextColor(COL_SCORE); 
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.print("HI: ");
  tft.print(highScore);
  tft.setCursor(90, 5);
  tft.setTextColor(TFT_RED); 
  tft.print("NO BORDER MODE");
  tft.setTextColor(COL_SCORE); 
  tft.setCursor(tft.width() - 90, 5);
  tft.print("SCORE: ");
  tft.print(score);
    tft.drawFastHLine(0, 20, tft.width(), TFT_WHITE); 
  
}

void showPauseScreen(bool pause) {
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
    drawFood(false);
    if(bonusActive) drawBonusFood(false);
  }
}

void spawnFood() {
  drawFood(true); 
  
  bool validPosition;
  do {
    validPosition = true;
    food.x = random(0, maxGridX) * SNAKE_GRID;
    food.y = random(0, maxGridY) * SNAKE_GRID + 20; // Start below score area
    
    // Check if food spawns on snake
    for (int i = 0; i < snakeLen; i++) {
      int snakePxX = snake[i].x * SNAKE_GRID;
      int snakePxY = snake[i].y * SNAKE_GRID + 20;
      if (abs(snakePxX - food.x) < SNAKE_GRID && abs(snakePxY - food.y) < SNAKE_GRID) {
        validPosition = false;
        break;
      }
    }
  } while (!validPosition);
}

void spawnBonusFood() {
  if (!bonusActive) {
    bonusActive = true;
    bonusStartTime = millis();
    
    bool validPosition;
    do {
      validPosition = true;
      bonusFood.x = random(0, maxGridX) * SNAKE_GRID;
      bonusFood.y = random(0, maxGridY) * SNAKE_GRID + 20;
      
      // Check if bonus spawns on snake or regular food
      for (int i = 0; i < snakeLen; i++) {
        int snakePxX = snake[i].x * SNAKE_GRID;
        int snakePxY = snake[i].y * SNAKE_GRID + 20;
        if (abs(snakePxX - bonusFood.x) < SNAKE_GRID && abs(snakePxY - bonusFood.y) < SNAKE_GRID) {
          validPosition = false;
          break;
        }
      }
      
      if (abs(food.x - bonusFood.x) < SNAKE_GRID && abs(food.y - bonusFood.y) < SNAKE_GRID) {
        validPosition = false;
      }
    } while (!validPosition);
  }
}

void drawSnakePart(int index, bool erase) {
  int x = snake[index].x * SNAKE_GRID;
  int y = snake[index].y * SNAKE_GRID + 20;
  
  if (erase) {
    tft.fillRect(x, y, SNAKE_GRID, SNAKE_GRID, COL_BG);
  } else {
    uint16_t color = (index == 0) ? COL_SNAKE_HEAD : COL_SNAKE_BODY;
    tft.fillRoundRect(x, y, SNAKE_GRID - 1, SNAKE_GRID - 1, 3, color);
    
    // Add eyes for head
    if (index == 0) {
      tft.fillCircle(x + 3, y + 3, 2, COL_BG);
      tft.fillCircle(x + SNAKE_GRID - 4, y + 3, 2, COL_BG);
      // Pupils
      tft.fillCircle(x + 3, y + 3, 1, COL_SCORE);
      tft.fillCircle(x + SNAKE_GRID - 4, y + 3, 1, COL_SCORE);
    }
  }
}

void drawFood(bool erase) {
  if(erase) {
    tft.fillCircle(food.x + 5, food.y + 5, 8, COL_BG);
  } else {
    // Food with glow effect
    tft.fillCircle(food.x + 5, food.y + 5, 7, COL_FOOD);
    tft.fillCircle(food.x + 5, food.y + 5, 4, COL_SCORE);
    tft.fillCircle(food.x + 5, food.y + 5, 2, 0x001F); // Blue center
  }
}

void drawBonusFood(bool erase) {
  if (!bonusActive && !erase) return;
  if (erase) {
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, 15, COL_BG); 
  } else {
    int rad = 8 + (pulseState % 4); 
    // Bonus with star effect
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, 12, COL_BG);
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, rad, COL_BONUS);
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, 5, COL_GOLD);
    // Draw star shape
    for(int i = 0; i < 8; i++) {
      float angle = i * 45 * 3.14159 / 180;
      int x = bonusFood.x + 5 + cos(angle) * 12;
      int y = bonusFood.y + 5 + sin(angle) * 12;
      tft.fillCircle(x, y, 2, COL_GOLD);
    }
  }
}

void showGameOver() {
  if (score > highScore) highScore = score;

  tft.fillScreen(COL_BG);
  
  // Decorative borders
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
  tft.print(highScore);
  
  tft.setTextSize(1);
  tft.setTextColor(COL_SCORE);
  tft.setCursor(tft.width()/2 - 60, tft.height() - 30);
  tft.print("PRESS SELECT TO START");
  
  tft.setTextSize(1);
}

void resetGame() {
  tft.fillScreen(COL_BG);
  
  snakeLen = 5; 
  score = 0; 
  foodCounter = 0;
  moveDelay = 150; 
  bonusActive = false; 
  isPaused = false;
  gameOver = false;
  pulseState = 0;
  
  int startX = maxGridX / 2;
  int startY = maxGridY / 2;
  
  for (int i = 0; i < snakeLen; i++) {
    snake[i] = {startX - i, startY};
    drawSnakePart(i, false);
  }
  
  dirX = 1; 
  dirY = 0; 
  
  spawnFood();
  updateScoreDisplay();
  
  lastMoveTime = millis();
  lastPulseTime = millis();
}

void setup() {
  // Initialize button pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  // Initialize display for ILI9341
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COL_BG);
  
  // Calculate full screen grid dimensions
  maxGridX = tft.width() / SNAKE_GRID;
  maxGridY = (tft.height() - 20) / SNAKE_GRID; // Leave 20px for score
  
  // Seed random
  randomSeed(analogRead(34));
  
  resetGame();
}

void loop() {
  // Handle SELECT button for pause/reset
  static unsigned long lastSelectPress = 0;
  if (digitalRead(BTN_SELECT) == LOW && millis() - lastSelectPress > 300) {
    lastSelectPress = millis();
    if (gameOver) {
      resetGame();
    } else {
      isPaused = !isPaused; 
      showPauseScreen(isPaused);
    }
  }

  if (!gameOver && !isPaused) {
    // Handle direction buttons
    if (digitalRead(BTN_UP) == LOW && dirY == 0) {
      dirX = 0; 
      dirY = -1;
    }
    else if (digitalRead(BTN_DOWN) == LOW && dirY == 0) {
      dirX = 0; 
      dirY = 1;
    }
    else if (digitalRead(BTN_LEFT) == LOW && dirX == 0) {
      dirX = -1; 
      dirY = 0;
    }
    else if (digitalRead(BTN_RIGHT) == LOW && dirX == 0) {
      dirX = 1; 
      dirY = 0;
    }

    // Handle pulsing animation for bonus food
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
      updateScoreDisplay();
    }

    // Game movement logic with WRAP-AROUND boundaries
    if (millis() - lastMoveTime > moveDelay) {
      lastMoveTime = millis();
      
      int nextX = snake[0].x + dirX;
      int nextY = snake[0].y + dirY;

      // WRAP-AROUND logic - snake appears on opposite side
      if (nextX < 0) {
        nextX = maxGridX - 1;
      } else if (nextX >= maxGridX) {
        nextX = 0;
      }
      
      if (nextY < 0) {
        nextY = maxGridY - 1;
      } else if (nextY >= maxGridY) {
        nextY = 0;
      }

      Point nextHead = {nextX, nextY};
      
      // Check collision with self
      for (int i = 0; i < snakeLen; i++) {
        if (nextHead.x == snake[i].x && nextHead.y == snake[i].y) {
          gameOver = true; 
          showGameOver(); 
          return;
        }
      }

      bool ateAnything = false;
      int headPxX = nextHead.x * SNAKE_GRID + (SNAKE_GRID/2);
      int headPxY = nextHead.y * SNAKE_GRID + 20 + (SNAKE_GRID/2);
      
      // Check collision with regular food
      if (abs(headPxX - (food.x + 5)) < SNAKE_GRID && abs(headPxY - (food.y + 5)) < SNAKE_GRID) {
        score += 10; 
        snakeLen++; 
        foodCounter++;
        ateAnything = true;
        
        // Increase speed
        if (moveDelay > 50) moveDelay -= 2;
        
        spawnFood();
        if (foodCounter >= 5 && !bonusActive) { 
          spawnBonusFood(); 
          foodCounter = 0; 
        }
      } 
      // Check collision with bonus food
      else if (bonusActive && abs(headPxX - (bonusFood.x + 5)) < SNAKE_GRID+2 && abs(headPxY - (bonusFood.y + 5)) < SNAKE_GRID+2) {
        // Dynamic Scoring Logic based on time taken to get bonus
        unsigned long timeTaken = (millis() - bonusStartTime) / 1000;
        int dynamicBonus = 80 - (timeTaken * 10);
        if (dynamicBonus < 10) dynamicBonus = 10;
        
        score += dynamicBonus;
        snakeLen++; 
        bonusActive = false; 
        ateAnything = true;
        drawBonusFood(true); 
      }

      // Move snake
      if (!ateAnything) {
        drawSnakePart(snakeLen - 1, true);
      }
      
      for (int i = snakeLen - 1; i > 0; i--) {
        snake[i] = snake[i - 1];
      }
      snake[0] = nextHead;
      
      // Draw snake parts
      for (int i = (ateAnything ? 0 : 1); i < snakeLen; i++) {
        drawSnakePart(i, false);
      }
      if(!ateAnything) drawSnakePart(0, false);
      
      updateScoreDisplay(); 
    }
  }
}
