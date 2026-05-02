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
#define COL_BARRIER     0x8410  // Grey

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

// Barrier system
#define MAX_BARRIERS 50
Point barriers[MAX_BARRIERS];
int barrierCount = 0;
bool useBarriers = true;  // Toggle barriers on/off

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
void generateBarriers();
void drawBarriers();
bool checkCollisionWithBarriers(int x, int y);

void generateBarriers() {
  barrierCount = 0;
  
  // Create a maze-like barrier pattern
  
  // Outer boundary walls (optional - can be enabled/disabled)
  // Uncomment below for boundary walls instead of wrap-around
  /*
  for(int i = 0; i < maxGridX; i++) {
    barriers[barrierCount++] = {i, 0};  // Top wall
    barriers[barrierCount++] = {i, maxGridY - 1};  // Bottom wall
  }
  for(int i = 1; i < maxGridY - 1; i++) {
    barriers[barrierCount++] = {0, i};  // Left wall
    barriers[barrierCount++] = {maxGridX - 1, i};  // Right wall
  }
  */
  
  // Create internal barriers (walls inside the play area)
  
  // Barrier 1: Horizontal line in the middle
  // for(int i = maxGridX/4; i < 3*maxGridX/4; i++) {
  //   if(i % 3 != 0) {  // Create gaps
  //     barriers[barrierCount++] = {i, maxGridY/2};
  //     if(barrierCount >= MAX_BARRIERS) break;
  //   }
  // }
  
  // Barrier 2: Vertical line
  // for(int i = maxGridY/4; i < 3*maxGridY/4; i++) {
  //   if(i % 3 != 0) {
  //     barriers[barrierCount++] = {maxGridX/2, i};
  //     if(barrierCount >= MAX_BARRIERS) break;
  //   }
  // }
  
  // Barrier 3: Small square barrier in corner
  int startX = maxGridX - 8;
  int startY = maxGridY - 8;
  for(int i = 0; i < 5; i++) {
    for(int j = 0; j < 5; j++) {
      if((i == 0 || i == 4 || j == 0 || j == 4) && barrierCount < MAX_BARRIERS) {
        barriers[barrierCount++] = {startX + i, startY + j};
      }
    }
  }
  
  // Barrier 4: Plus shape
  int centerX = maxGridX/4;
  int centerY = maxGridY/4;
  for(int i = -3; i <= 3; i++) {
    if(i != 0 && barrierCount < MAX_BARRIERS) {
      barriers[barrierCount++] = {centerX + i, centerY};
      barriers[barrierCount++] = {centerX, centerY + i};
    }
  }
  
  // Barrier 5: Diagonal line
  // for(int i = 0; i < 8; i++) {
  //   if(barrierCount < MAX_BARRIERS) {
  //     barriers[barrierCount++] = {5 + i, maxGridY - 6 - i};
  //   }
  // }
  
  // Barrier 6: L-shape barrier
  // for(int i = 0; i < 6; i++) {
  //   if(barrierCount < MAX_BARRIERS) {
  //     barriers[barrierCount++] = {2, 3 + i};
  //     barriers[barrierCount++] = {2 + i, 2};
  //   }
  // }
}

void drawBarriers() {
  for(int i = 0; i < barrierCount; i++) {
    int x = barriers[i].x * SNAKE_GRID;
    int y = barriers[i].y * SNAKE_GRID + 20;
    
    // Draw barrier with pattern
    tft.fillRect(x, y, SNAKE_GRID, SNAKE_GRID, COL_BARRIER);
    tft.drawRect(x, y, SNAKE_GRID, SNAKE_GRID, COL_SCORE);
    
    // // Add diagonal stripes
    // tft.drawLine(x, y, x + SNAKE_GRID, y + SNAKE_GRID, COL_BG);
    // tft.drawLine(x + SNAKE_GRID, y, x, y + SNAKE_GRID, COL_BG);
  }
}

bool checkCollisionWithBarriers(int x, int y) {
  for(int i = 0; i < barrierCount; i++) {
    if(barriers[i].x == x && barriers[i].y == y) {
      return true;  // Collision detected
    }
  }
  return false;
}

void updateScoreDisplay() {
  // Show score at the top of the screen
  tft.fillRect(0, 0, tft.width(), 20, COL_BG);
  tft.setTextColor(COL_SCORE); 
  tft.setTextSize(1);
  tft.setCursor(5, 5);
  tft.print("Score: ");
  tft.print(score);
  
  tft.setCursor(tft.width() - 70, 5);
  tft.print("HI: ");
  tft.print(highScore);
  
  // Show barrier status
  if(useBarriers) {
    tft.setCursor(tft.width()/2 - 30, 5);
    tft.setTextColor(COL_BARRIER);
    tft.print("WALLS");
  }
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
    drawBarriers();  // Redraw barriers
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
    food.y = random(0, maxGridY) * SNAKE_GRID + 20;
    
    // Check if food spawns on snake
    for (int i = 0; i < snakeLen; i++) {
      int snakePxX = snake[i].x * SNAKE_GRID;
      int snakePxY = snake[i].y * SNAKE_GRID + 20;
      if (abs(snakePxX - food.x) < SNAKE_GRID && abs(snakePxY - food.y) < SNAKE_GRID) {
        validPosition = false;
        break;
      }
    }
    
    // Check if food spawns on barriers
    int gridX = food.x / SNAKE_GRID;
    int gridY = (food.y - 20) / SNAKE_GRID;
    if(checkCollisionWithBarriers(gridX, gridY)) {
      validPosition = false;
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
      
      // Check if bonus spawns on barriers
      int gridX = bonusFood.x / SNAKE_GRID;
      int gridY = (bonusFood.y - 20) / SNAKE_GRID;
      if(checkCollisionWithBarriers(gridX, gridY)) {
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
    // Redraw any barriers that might be underneath
    if(useBarriers) {
      for(int i = 0; i < barrierCount; i++) {
        int bx = barriers[i].x * SNAKE_GRID;
        int by = barriers[i].y * SNAKE_GRID + 20;
        if(abs(bx - x) < SNAKE_GRID && abs(by - y) < SNAKE_GRID) {
          tft.fillRect(bx, by, SNAKE_GRID, SNAKE_GRID, COL_BARRIER);
          tft.drawRect(bx, by, SNAKE_GRID, SNAKE_GRID, COL_SCORE);
          tft.drawLine(bx, by, bx + SNAKE_GRID, by + SNAKE_GRID, COL_BG);
          tft.drawLine(bx + SNAKE_GRID, by, bx, by + SNAKE_GRID, COL_BG);
        }
      }
    }
  } else {
    uint16_t color = (index == 0) ? COL_SNAKE_HEAD : COL_SNAKE_BODY;
    tft.fillRoundRect(x, y, SNAKE_GRID - 1, SNAKE_GRID - 1, 3, color);
    
    // Add eyes for head
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
    tft.fillCircle(food.x + 5, food.y + 5, 8, COL_BG);
  } else {
    tft.fillCircle(food.x + 5, food.y + 5, 7, COL_FOOD);
    tft.fillCircle(food.x + 5, food.y + 5, 4, COL_SCORE);
    tft.fillCircle(food.x + 5, food.y + 5, 2, 0x001F);
  }
}

void drawBonusFood(bool erase) {
  if (!bonusActive && !erase) return;
  if (erase) {
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, 15, COL_BG); 
  } else {
    int rad = 8 + (pulseState % 4); 
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, 12, COL_BG);
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, rad, COL_BONUS);
    tft.fillCircle(bonusFood.x + 5, bonusFood.y + 5, 5, COL_GOLD);
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
  tft.setCursor(tft.width()/2 - 70, tft.height() - 40);
  tft.print("PRESS SELECT TO START");
  
  tft.setTextColor(COL_BARRIER);
  tft.setCursor(tft.width()/2 - 50, tft.height() - 25);
  tft.print("AVOID THE WALLS!");
  
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
  
  // Make sure snake doesn't start inside barriers
  while(checkCollisionWithBarriers(startX, startY)) {
    startX = random(2, maxGridX - 2);
    startY = random(2, maxGridY - 2);
  }
  
  for (int i = 0; i < snakeLen; i++) {
    snake[i] = {startX - i, startY};
    drawSnakePart(i, false);
  }
  
  dirX = 1; 
  dirY = 0; 
  
  if(useBarriers) {
    generateBarriers();
    drawBarriers();
  }
  
  spawnFood();
  updateScoreDisplay();
  
  lastMoveTime = millis();
  lastPulseTime = millis();
}

void setup() {
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COL_BG);
  
  maxGridX = tft.width() / SNAKE_GRID;
  maxGridY = (tft.height() - 20) / SNAKE_GRID;
  
  randomSeed(analogRead(34));
  
  resetGame();
}

void loop() {
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

    if (millis() - lastMoveTime > moveDelay) {
      lastMoveTime = millis();
      
      int nextX = snake[0].x + dirX;
      int nextY = snake[0].y + dirY;

      // WRAP-AROUND logic (barriers override this)
      if (!useBarriers) {
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
      } else {
        // With barriers enabled, check boundaries first
        if (nextX < 0 || nextX >= maxGridX || nextY < 0 || nextY >= maxGridY) {
          gameOver = true;
          showGameOver();
          return;
        }
      }

      Point nextHead = {nextX, nextY};
      
      // Check collision with barriers
      if(useBarriers && checkCollisionWithBarriers(nextHead.x, nextHead.y)) {
        gameOver = true;
        showGameOver();
        return;
      }
      
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
      
      if (abs(headPxX - (food.x + 5)) < SNAKE_GRID && abs(headPxY - (food.y + 5)) < SNAKE_GRID) {
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
      else if (bonusActive && abs(headPxX - (bonusFood.x + 5)) < SNAKE_GRID+2 && abs(headPxY - (bonusFood.y + 5)) < SNAKE_GRID+2) {
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
      
      for (int i = (ateAnything ? 0 : 1); i < snakeLen; i++) {
        drawSnakePart(i, false);
      }
      if(!ateAnything) drawSnakePart(0, false);
      
      updateScoreDisplay(); 
    }
  }
}
