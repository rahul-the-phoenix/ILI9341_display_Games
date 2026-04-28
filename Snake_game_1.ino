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

#define SNAKE_GRID      10      // 10px per cell (bigger)
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
  tft.fillRect(0, 0, tft.width(), 25, COL_BG);
  tft.setTextColor(COL_SCORE); 
  tft.setTextSize(2);
  tft.setCursor(10, 5);
  tft.print("Score: ");
  tft.print(score);
  
  tft.setCursor(tft.width() - 150, 5);
  tft.print("HIGH: ");
  tft.print(highScore);
  
  // if(bonusActive) {
  //   int remaining = 8 - (millis() - bonusStartTime) / 1000;
  //   if(remaining < 0) remaining = 0;
  //   tft.setCursor(tft.width()/2 - 50, 5);
  //   tft.setTextColor(COL_BONUS);
  //   tft.print("Bonus: ");
  //   tft.print(remaining);
  //   tft.print("s");
  // }
}

void showPauseScreen(bool pause) {
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
    
    // Check if food spawns on snake
    for (int i = 0; i < snakeLen; i++) {
      int snakePxX = snake[i].x * SNAKE_GRID + 10;
      int snakePxY = snake[i].y * SNAKE_GRID + 40;
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
    int maxGridX = (tft.width() - 20) / SNAKE_GRID;
    int maxGridY = (tft.height() - 50) / SNAKE_GRID;
    
    bool validPosition;
    do {
      validPosition = true;
      bonusFood.x = random(2, maxGridX - 2) * SNAKE_GRID + 10;
      bonusFood.y = random(4, maxGridY - 2) * SNAKE_GRID + 40;
      
      // Check if bonus spawns on snake or regular food
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
    
    // Add eyes for head (bigger)
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
    tft.fillCircle(food.x, food.y, 8, COL_BG);
  } else {
    // Bigger food with glow effect
    tft.fillCircle(food.x, food.y, 7, COL_FOOD);
    tft.fillCircle(food.x, food.y, 4, COL_SCORE);
    tft.fillCircle(food.x, food.y, 2, 0x001F); // Blue center
  }
}

void drawBonusFood(bool erase) {
  if (!bonusActive && !erase) return;
  if (erase) {
    tft.fillCircle(bonusFood.x, bonusFood.y, 15, COL_BG); 
  } else {
    int rad = 8 + (pulseState % 4); 
    // Bigger bonus with star effect
    tft.fillCircle(bonusFood.x, bonusFood.y, 12, COL_BG);
    tft.fillCircle(bonusFood.x, bonusFood.y, rad, COL_BONUS);
    tft.fillCircle(bonusFood.x, bonusFood.y, 5, COL_GOLD);
    // Draw star shape
    for(int i = 0; i < 8; i++) {
      float angle = i * 45 * 3.14159 / 180;
      int x = bonusFood.x + cos(angle) * 12;
      int y = bonusFood.y + sin(angle) * 12;
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
  tft.print(highScore);
  
  tft.setTextSize(1);
  tft.setTextColor(COL_SCORE);
  // tft.setCursor(30, 185);
  // tft.print("PRESS SELECT TO START");
  
  tft.setTextSize(2);
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
      updateScoreDisplay(); // Update timer display
    }

    // Game movement logic
    if (millis() - lastMoveTime > moveDelay) {
      lastMoveTime = millis();
      
      int nextX = snake[0].x + dirX;
      int nextY = snake[0].y + dirY;
      int maxX = (tft.width() - 20) / SNAKE_GRID;
      int maxY = (tft.height() - 50) / SNAKE_GRID;

      // Wall collision (no wrap around - classic mode)
      if (nextX < 0 || nextX >= maxX || nextY < 0 || nextY >= maxY) {
        gameOver = true;
        showGameOver();
        return;
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
      int headPxX = nextHead.x * SNAKE_GRID + 10 + (SNAKE_GRID/2);
      int headPxY = nextHead.y * SNAKE_GRID + 40 + (SNAKE_GRID/2);
      
      // Check collision with regular food
      if (abs(headPxX - food.x) < SNAKE_GRID && abs(headPxY - food.y) < SNAKE_GRID) {
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
      else if (bonusActive && abs(headPxX - bonusFood.x) < SNAKE_GRID+2 && abs(headPxY - bonusFood.y) < SNAKE_GRID+2) {
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
