#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Common Button Pins for all games
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
  GAME_FLAPPY,
  GAME_CONNECT4,
  GAME_TICTACTOE,
  GAME_RPS
};

GameMode currentGame = GAME_MENU;
int selectedGame = 0;  // 0 = Snake, 1 = Flappy Bird, 2 = Connect 4, 3 = Tic Tac Toe, 4 = Rock Paper Scissors

// Screen dimensions
int screenWidth = 320;
int screenHeight = 240;

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
const float SCROLL_SPEED = 3.5;
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

// ============= CONNECT 4 GAME VARIABLES =============
#define C4_ROWS 7        // 7 rows
#define C4_COLS 10       // 10 columns
#define CELL_SIZE 28     // Perfect size for 10x7 grid
#define OFFSET_X 20      // Perfect horizontal centering
#define OFFSET_Y 40      // Perfect vertical centering

// Connect 4 Colors
#define COL_BOARD   0x0000  // Pure Black background
#define COL_EMPTY   0xFFFF  // White empty cell
#define COL_PLAYER  0x07E0  // Green
#define COL_BOT     0xF800  // Red
#define COL_GLOSS   0xFFFF  // White gloss
#define COL_GOLD    0xFD20  // Gold color
#define COL_WIN     0x001F  // Blue win line
#define COL_BG_C4   0x0000  // Black screen background
#define COL_BORDER  0x7BEF  // Light gray border
#define COL_GRID    0xAD55  // Light brown/gold grid line

int c4board[C4_ROWS][C4_COLS];
int selector = 4;  // Start at middle column (0-9 -> 4 is center-left)
bool playerTurn = true, gameOverC4 = false;
int playerWins = 0, botWins = 0;
int winX[4], winY[4];

// ============= TIC TAC TOE GAME VARIABLES =============
#define TTT_SIZE 3
#define TTT_CELL_SIZE 80
#define TTT_OFFSET_X 40
#define TTT_OFFSET_Y 30

char tttBoard[TTT_SIZE][TTT_SIZE];
bool playerTurnTTT = true;
bool gameOverTTT = false;
int playerScoreTTT = 0;
int botScoreTTT = 0;
int drawScoreTTT = 0;
int selectedRow = 1, selectedCol = 1;

// Tic Tac Toe Colors
#define TTT_BG        0x0000
#define TTT_GRID      0xFFFF
#define TTT_PLAYER    0x07E0
#define TTT_BOT       0xF800
#define TTT_HIGHLIGHT 0xFD20
#define TTT_TEXT      0xFFFF

// ============= ROCK PAPER SCISSORS VARIABLES =============
#define MINT_BG      0x9F5A
#define COLOR_ROCK   0x52AA 
#define COLOR_PAPER  0xFFFF 
#define COLOR_SC_H   0xF800 
#define COLOR_TEXT   0x0000 
#define COLOR_WIN    0x07E0
#define COLOR_LOSS   0xF800
#define COLOR_GOLD_RPS 0xFD20

int pScore = 0, bScore = 0;
int currentSel = 1;

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
  tft.setCursor(80, 105);
  tft.print("1. SNAKE");
  
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(60, 135);
  tft.print("2. FLAPPY BIRD");
  
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.setCursor(70, 165);
  tft.print("3. CONNECT 4");
  
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setCursor(70, 195);
  tft.print("4. TIC TAC TOE");
  
  tft.setTextColor(0xFD20, TFT_BLACK);
  tft.setCursor(50, 225);
  tft.print("5. ROCK PAPER SCISSORS");
  
  // Draw selection indicator (arrow)
  tft.fillTriangle(45, 110 + (selectedGame * 30), 
                   45, 120 + (selectedGame * 30),
                   35, 115 + (selectedGame * 30),
                   TFT_RED);
  
  // Draw instructions
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(30, 255);
  tft.print("UP/DOWN: Select   A: Confirm");
  
  // Decorative elements
  tft.fillCircle(300, 10, 5, TFT_RED);
  tft.fillCircle(20, 265, 3, TFT_GREEN);
  tft.fillCircle(300, 265, 3, TFT_YELLOW);
}

void updateMenuSelection() {
  // Clear old triangles
  for(int i = 0; i < 5; i++) {
    tft.fillTriangle(45, 110 + (i * 30), 45, 120 + (i * 30), 35, 115 + (i * 30), TFT_BLACK);
  }
  
  // Draw new triangle
  tft.fillTriangle(45, 110 + (selectedGame * 30), 
                   45, 120 + (selectedGame * 30),
                   35, 115 + (selectedGame * 30),
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

// ============= CONNECT 4 GAME FUNCTIONS =============
void drawGlossyCircle(int x, int y, int r, uint16_t color) {
  if (color == COL_EMPTY) {
    tft.fillCircle(x, y, r, COL_BOARD);
    tft.drawCircle(x, y, r, COL_GRID);
  } else {
    tft.fillCircle(x, y, r, color);
    tft.fillCircle(x - r/3, y - r/3, r/4, COL_GLOSS);
  }
}

void updateSelectorC4() {
  tft.fillRect(0, 0, 320, 38, COL_BG_C4);
  if(!gameOverC4) {
    int tx = OFFSET_X + selector * CELL_SIZE + CELL_SIZE/2;
    int ty = 30;
    for(int i = 1; i <= 3; i++) {
      tft.fillTriangle(tx - (6 + i/2), ty - i,
                       tx + (6 + i/2), ty - i,
                       tx, ty + 10,
                       playerTurn ? 0x03E0 : 0xB800);
    }
    tft.fillTriangle(tx - 6, ty, tx + 6, ty, tx, ty + 12, playerTurn ? COL_PLAYER : COL_BOT);
    tft.fillRoundRect(10, 8, 100, 22, 4, COL_BORDER);
    tft.setTextSize(1);
    tft.setTextColor(playerTurn ? COL_PLAYER : COL_BOT, COL_BORDER);
    tft.setCursor(15, 13);
    tft.print(playerTurn ? "YOUR TURN" : "BOT TURN");
    tft.fillRoundRect(210, 8, 100, 22, 4, COL_BORDER);
    tft.setTextColor(COL_GLOSS, COL_BORDER);
    tft.setCursor(215, 13);
    tft.print("YOU:");
    tft.print(playerWins);
    tft.print(" BOT:");
    tft.print(botWins);
  }
}

void drawBoardC4() {
  int boardWidth = C4_COLS * CELL_SIZE + 12;
  int boardHeight = C4_ROWS * CELL_SIZE + 12;
  tft.fillRoundRect(OFFSET_X - 6, OFFSET_Y - 6, boardWidth, boardHeight, 10, COL_BORDER);
  tft.fillRoundRect(OFFSET_X - 4, OFFSET_Y - 4, boardWidth - 4, boardHeight - 4, 8, COL_BOARD);
  tft.drawRoundRect(OFFSET_X - 5, OFFSET_Y - 5, boardWidth - 2, boardHeight - 2, 9, COL_GOLD);
  for(int c = 0; c <= C4_COLS; c++) {
    tft.drawLine(OFFSET_X + c * CELL_SIZE, OFFSET_Y, OFFSET_X + c * CELL_SIZE, OFFSET_Y + C4_ROWS * CELL_SIZE, COL_GRID);
  }
  for(int r = 0; r <= C4_ROWS; r++) {
    tft.drawLine(OFFSET_X, OFFSET_Y + r * CELL_SIZE, OFFSET_X + C4_COLS * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, COL_GRID);
  }
  for (int r = 0; r < C4_ROWS; r++) {
    for (int c = 0; c < C4_COLS; c++) {
      uint16_t color;
      if (c4board[r][c] == 1) color = COL_PLAYER;
      else if (c4board[r][c] == 2) color = COL_BOT;
      else color = COL_EMPTY;
      int ballRadius = CELL_SIZE/2 - 4;
      drawGlossyCircle(OFFSET_X + c * CELL_SIZE + CELL_SIZE/2, OFFSET_Y + r * CELL_SIZE + CELL_SIZE/2, ballRadius, color);
    }
  }
  updateSelectorC4();
}

void drawWinLineC4() {
  for (int i = 0; i < 3; i++) {
    int x1 = OFFSET_X + winX[i] * CELL_SIZE + CELL_SIZE/2;
    int y1 = OFFSET_Y + winY[i] * CELL_SIZE + CELL_SIZE/2;
    int x2 = OFFSET_X + winX[i+1] * CELL_SIZE + CELL_SIZE/2;
    int y2 = OFFSET_Y + winY[i+1] * CELL_SIZE + CELL_SIZE/2;
    for (int thickness = -2; thickness <= 2; thickness++) {
      for (int ox = -thickness; ox <= thickness; ox++) {
        for (int oy = -thickness; oy <= thickness; oy++) {
          if(abs(ox) + abs(oy) <= thickness * 2) {
            tft.drawLine(x1 + ox, y1 + oy, x2 + ox, y2 + oy, COL_WIN);
          }
        }
      }
    }
    delay(100);
  }
  for(int flash = 0; flash < 3; flash++) {
    for (int i = 0; i < 3; i++) {
      int x1 = OFFSET_X + winX[i] * CELL_SIZE + CELL_SIZE/2;
      int y1 = OFFSET_Y + winY[i] * CELL_SIZE + CELL_SIZE/2;
      int x2 = OFFSET_X + winX[i+1] * CELL_SIZE + CELL_SIZE/2;
      int y2 = OFFSET_Y + winY[i+1] * CELL_SIZE + CELL_SIZE/2;
      tft.drawLine(x1, y1, x2, y2, COL_GLOSS);
    }
    delay(80);
    for (int i = 0; i < 3; i++) {
      int x1 = OFFSET_X + winX[i] * CELL_SIZE + CELL_SIZE/2;
      int y1 = OFFSET_Y + winY[i] * CELL_SIZE + CELL_SIZE/2;
      int x2 = OFFSET_X + winX[i+1] * CELL_SIZE + CELL_SIZE/2;
      int y2 = OFFSET_Y + winY[i+1] * CELL_SIZE + CELL_SIZE/2;
      tft.drawLine(x1, y1, x2, y2, COL_WIN);
    }
    delay(80);
  }
  delay(300);
}

void animateDiscC4(int col, int targetRow, uint16_t color) {
  int x = OFFSET_X + col * CELL_SIZE + CELL_SIZE/2;
  int ballRadius = CELL_SIZE/2 - 4;
  for (int row = 0; row <= targetRow; row++) {
    int y = OFFSET_Y + row * CELL_SIZE + CELL_SIZE/2;
    if (row > 0) {
      drawGlossyCircle(x, OFFSET_Y + (row - 1) * CELL_SIZE + CELL_SIZE/2, ballRadius, COL_EMPTY);
    }
    tft.fillCircle(x + 2, y + 2, ballRadius, 0x0000);
    drawGlossyCircle(x, y, ballRadius, color);
    delay(45);
  }
}

bool checkWinC4(int p) {
  for (int r = 0; r < C4_ROWS; r++) {
    for (int c = 0; c < C4_COLS - 3; c++) {
      if (c4board[r][c] == p && c4board[r][c+1] == p && c4board[r][c+2] == p && c4board[r][c+3] == p) {
        for (int i = 0; i < 4; i++) { winY[i] = r; winX[i] = c + i; }
        return true;
      }
    }
  }
  for (int r = 0; r < C4_ROWS - 3; r++) {
    for (int c = 0; c < C4_COLS; c++) {
      if (c4board[r][c] == p && c4board[r+1][c] == p && c4board[r+2][c] == p && c4board[r+3][c] == p) {
        for (int i = 0; i < 4; i++) { winY[i] = r + i; winX[i] = c; }
        return true;
      }
    }
  }
  for (int r = 3; r < C4_ROWS; r++) {
    for (int c = 0; c < C4_COLS - 3; c++) {
      if (c4board[r][c] == p && c4board[r-1][c+1] == p && c4board[r-2][c+2] == p && c4board[r-3][c+3] == p) {
        for (int i = 0; i < 4; i++) { winY[i] = r - i; winX[i] = c + i; }
        return true;
      }
    }
  }
  for (int r = 0; r < C4_ROWS - 3; r++) {
    for (int c = 0; c < C4_COLS - 3; c++) {
      if (c4board[r][c] == p && c4board[r+1][c+1] == p && c4board[r+2][c+2] == p && c4board[r+3][c+3] == p) {
        for (int i = 0; i < 4; i++) { winY[i] = r + i; winX[i] = c + i; }
        return true;
      }
    }
  }
  return false;
}

bool isDrawC4() {
  for (int c = 0; c < C4_COLS; c++) {
    if (c4board[0][c] == 0) return false;
  }
  return true;
}

void showGameOverC4(String msg, uint16_t txtCol) {
  delay(600);
  tft.fillRect(40, 50, 240, 140, 0x0000);
  tft.drawRoundRect(40, 50, 240, 140, 10, COL_GOLD);
  tft.drawRoundRect(42, 52, 236, 136, 8, txtCol);
  tft.setTextSize(2);
  tft.setTextColor(txtCol, 0x0000);
  if (msg == "YOU WIN!") tft.setCursor(108, 75);
  else if (msg == "BOT WINS!") tft.setCursor(98, 75);
  else if (msg == "DRAW!") tft.setCursor(128, 75);
  tft.print(msg);
  tft.drawFastHLine(70, 105, 180, COL_GOLD);
  tft.setTextSize(2);
  tft.setTextColor(COL_GLOSS, 0x0000);
  tft.setCursor(85, 120);
  tft.print("YOU: ");
  tft.print(playerWins);
  tft.setCursor(85, 145);
  tft.print("BOT: ");
  tft.print(botWins);
  tft.setTextSize(1);
  tft.setTextColor(COL_GOLD, 0x0000);
  tft.setCursor(72, 175);
  tft.print("PRESS SELECT TO PLAY AGAIN");
}

bool dropDiscC4(int col, int p) {
  for (int r = C4_ROWS - 1; r >= 0; r--) {
    if (c4board[r][col] == 0) {
      animateDiscC4(col, r, (p == 1) ? COL_PLAYER : COL_BOT);
      c4board[r][col] = p;
      return true;
    }
  }
  return false;
}

void botMoveC4() {
  delay(300);
  int mc = -1;
  for (int c = 0; c < C4_COLS; c++) {
    if (c4board[0][c] == 0) {
      int r;
      for (r = C4_ROWS - 1; r >= 0; r--) if (c4board[r][c] == 0) break;
      c4board[r][c] = 2;
      if (checkWinC4(2)) mc = c;
      c4board[r][c] = 0;
      if (mc != -1) break;
    }
  }
  if (mc == -1) {
    for (int c = 0; c < C4_COLS; c++) {
      if (c4board[0][c] == 0) {
        int r;
        for (r = C4_ROWS - 1; r >= 0; r--) if (c4board[r][c] == 0) break;
        c4board[r][c] = 1;
        if (checkWinC4(1)) mc = c;
        c4board[r][c] = 0;
        if (mc != -1) break;
      }
    }
  }
  if (mc == -1) {
    do { mc = random(C4_COLS); } while (c4board[0][mc] != 0);
  }
  dropDiscC4(mc, 2);
  if (checkWinC4(2)) { botWins++; drawWinLineC4(); showGameOverC4("BOT WINS!", COL_BOT); gameOverC4 = true; } 
  else if (isDrawC4()) { showGameOverC4("DRAW!", COL_GOLD); gameOverC4 = true; }
  else { playerTurn = true; updateSelectorC4(); }
}

void resetGameC4() {
  tft.fillScreen(COL_BG_C4);
  for (int r = 0; r < C4_ROWS; r++) for (int c = 0; c < C4_COLS; c++) c4board[r][c] = 0;
  selector = 4;
  gameOverC4 = false;
  playerTurn = (random(2) == 0);
  drawBoardC4();
  if(!playerTurn) { delay(500); botMoveC4(); }
}

void runConnect4Game() {
  static unsigned long lastLeftPress = 0, lastRightPress = 0, lastSelectPress = 0, lastBPress = 0;
  if (digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) {
    lastBPress = millis();
    gameOverC4 = true;
    returnToMenu();
    return;
  }
  if (!gameOverC4) {
    if (playerTurn) {
      if (digitalRead(BTN_LEFT) == LOW && selector > 0 && millis() - lastLeftPress > 120) { lastLeftPress = millis(); selector--; updateSelectorC4(); }
      if (digitalRead(BTN_RIGHT) == LOW && selector < C4_COLS - 1 && millis() - lastRightPress > 120) { lastRightPress = millis(); selector++; updateSelectorC4(); }
      if (digitalRead(BTN_SELECT) == LOW && millis() - lastSelectPress > 200) {
        lastSelectPress = millis();
        if (dropDiscC4(selector, 1)) {
          if (checkWinC4(1)) { playerWins++; drawWinLineC4(); showGameOverC4("YOU WIN!", COL_PLAYER); gameOverC4 = true; } 
          else if (isDrawC4()) { showGameOverC4("DRAW!", COL_GOLD); gameOverC4 = true; }
          else { playerTurn = false; updateSelectorC4(); delay(300); botMoveC4(); }
        }
      }
    }
  } else if (digitalRead(BTN_SELECT) == LOW && millis() - lastSelectPress > 300) { lastSelectPress = millis(); resetGameC4(); }
  delay(10);
}

// ============= TIC TAC TOE GAME FUNCTIONS =============
void drawTTTBoard() {
  for (int i = 1; i < TTT_SIZE; i++) {
    tft.drawLine(TTT_OFFSET_X + i * TTT_CELL_SIZE, TTT_OFFSET_Y, TTT_OFFSET_X + i * TTT_CELL_SIZE, TTT_OFFSET_Y + TTT_SIZE * TTT_CELL_SIZE, TTT_GRID);
    tft.drawLine(TTT_OFFSET_X, TTT_OFFSET_Y + i * TTT_CELL_SIZE, TTT_OFFSET_X + TTT_SIZE * TTT_CELL_SIZE, TTT_OFFSET_Y + i * TTT_CELL_SIZE, TTT_GRID);
  }
  for (int row = 0; row < TTT_SIZE; row++) {
    for (int col = 0; col < TTT_SIZE; col++) {
      int x = TTT_OFFSET_X + col * TTT_CELL_SIZE;
      int y = TTT_OFFSET_Y + row * TTT_CELL_SIZE;
      if (tttBoard[row][col] == 'X') {
        tft.drawLine(x + 15, y + 15, x + TTT_CELL_SIZE - 15, y + TTT_CELL_SIZE - 15, TTT_BOT);
        tft.drawLine(x + TTT_CELL_SIZE - 15, y + 15, x + 15, y + TTT_CELL_SIZE - 15, TTT_BOT);
      } else if (tttBoard[row][col] == 'O') {
        tft.drawCircle(x + TTT_CELL_SIZE/2, y + TTT_CELL_SIZE/2, TTT_CELL_SIZE/2 - 15, TTT_PLAYER);
        tft.fillCircle(x + TTT_CELL_SIZE/2, y + TTT_CELL_SIZE/2, TTT_CELL_SIZE/2 - 18, TTT_BG);
        tft.drawCircle(x + TTT_CELL_SIZE/2, y + TTT_CELL_SIZE/2, TTT_CELL_SIZE/2 - 16, TTT_PLAYER);
      }
    }
  }
}

void drawTTTSelector() {
  int x = TTT_OFFSET_X + selectedCol * TTT_CELL_SIZE;
  int y = TTT_OFFSET_Y + selectedRow * TTT_CELL_SIZE;
  for (int i = 0; i < 3; i++) {
    tft.drawRect(x - i, y - i, TTT_CELL_SIZE + i*2, TTT_CELL_SIZE + i*2, TTT_HIGHLIGHT);
  }
}

void updateTTTScore() {
  tft.fillRect(0, 0, 320, 28, TTT_BG);
  tft.drawRect(0, 0, 320, 28, TTT_GRID);
  tft.setTextSize(1);
  tft.setTextColor(TTT_TEXT, TTT_BG);
  tft.setCursor(10, 8); tft.print("YOU: "); tft.print(playerScoreTTT);
  tft.setCursor(120, 8); tft.print("DRAW: "); tft.print(drawScoreTTT);
  tft.setCursor(230, 8); tft.print("BOT: "); tft.print(botScoreTTT);
  if (!gameOverTTT && playerTurnTTT) { tft.setCursor(10, 20); tft.print("YOUR TURN (O)"); }
  else if (!gameOverTTT && !playerTurnTTT) { tft.setCursor(10, 20); tft.print("BOT TURN (X)..."); }
}

bool checkWinTTT(char player, int &winRow1, int &winCol1, int &winRow2, int &winCol2, int &winRow3, int &winCol3) {
  for (int i = 0; i < TTT_SIZE; i++) {
    if (tttBoard[i][0] == player && tttBoard[i][1] == player && tttBoard[i][2] == player) {
      winRow1 = i; winCol1 = 0; winRow2 = i; winCol2 = 1; winRow3 = i; winCol3 = 2;
      return true;
    }
  }
  for (int i = 0; i < TTT_SIZE; i++) {
    if (tttBoard[0][i] == player && tttBoard[1][i] == player && tttBoard[2][i] == player) {
      winRow1 = 0; winCol1 = i; winRow2 = 1; winCol2 = i; winRow3 = 2; winCol3 = i;
      return true;
    }
  }
  if (tttBoard[0][0] == player && tttBoard[1][1] == player && tttBoard[2][2] == player) {
    winRow1 = 0; winCol1 = 0; winRow2 = 1; winCol2 = 1; winRow3 = 2; winCol3 = 2;
    return true;
  }
  if (tttBoard[0][2] == player && tttBoard[1][1] == player && tttBoard[2][0] == player) {
    winRow1 = 0; winCol1 = 2; winRow2 = 1; winCol2 = 1; winRow3 = 2; winCol3 = 0;
    return true;
  }
  return false;
}

void drawWinningLineTTT(int row1, int col1, int row2, int col2, int row3, int col3) {
  int x1 = TTT_OFFSET_X + col1 * TTT_CELL_SIZE + TTT_CELL_SIZE/2;
  int y1 = TTT_OFFSET_Y + row1 * TTT_CELL_SIZE + TTT_CELL_SIZE/2;
  int x3 = TTT_OFFSET_X + col3 * TTT_CELL_SIZE + TTT_CELL_SIZE/2;
  int y3 = TTT_OFFSET_Y + row3 * TTT_CELL_SIZE + TTT_CELL_SIZE/2;
  for (int step = 0; step <= 10; step++) {
    int currentX = x1 + (x3 - x1) * step / 10;
    int currentY = y1 + (y3 - y1) * step / 10;
    for (int thickness = -3; thickness <= 3; thickness++) {
      for (int offset = -2; offset <= 2; offset++) {
        if (abs(thickness) + abs(offset) <= 3) {
          tft.drawLine(x1 + offset, y1 + thickness, currentX + offset, currentY + thickness, TFT_WHITE);
        }
      }
    }
    delay(50);
  }
  for (int thickness = -3; thickness <= 3; thickness++) {
    for (int offset = -2; offset <= 2; offset++) {
      if (abs(thickness) + abs(offset) <= 3) {
        tft.drawLine(x1 + offset, y1 + thickness, x3 + offset, y3 + thickness, TFT_WHITE);
      }
    }
  }
  for (int flash = 0; flash < 5; flash++) {
    for (int thickness = -3; thickness <= 3; thickness++) {
      for (int offset = -2; offset <= 2; offset++) {
        if (abs(thickness) + abs(offset) <= 3) {
          tft.drawLine(x1 + offset, y1 + thickness, x3 + offset, y3 + thickness, TTT_BG);
        }
      }
    }
    delay(80);
    for (int thickness = -3; thickness <= 3; thickness++) {
      for (int offset = -2; offset <= 2; offset++) {
        if (abs(thickness) + abs(offset) <= 3) {
          tft.drawLine(x1 + offset, y1 + thickness, x3 + offset, y3 + thickness, TFT_WHITE);
        }
      }
    }
    delay(80);
  }
  delay(500);
}

bool isDrawTTT() {
  for (int i = 0; i < TTT_SIZE; i++) {
    for (int j = 0; j < TTT_SIZE; j++) {
      if (tttBoard[i][j] == ' ') return false;
    }
  }
  return true;
}

void showGameOverTTT(String msg) {
  delay(500);
  tft.fillRect(40, 90, 240, 80, 0x0000);
  tft.drawRect(40, 90, 240, 80, TTT_GRID);
  tft.drawRect(43, 93, 234, 74, TTT_GRID);
  tft.setTextSize(2);
  if (msg == "YOU WIN!") { tft.setTextColor(TTT_PLAYER); tft.setCursor(110, 115); }
  else if (msg == "BOT WINS!") { tft.setTextColor(TTT_BOT); tft.setCursor(100, 115); }
  else { tft.setTextColor(TTT_GRID); tft.setCursor(120, 115); }
  tft.print(msg);
  tft.setTextSize(1);
  tft.setTextColor(TTT_TEXT);
  tft.setCursor(80, 145);
  tft.print("PRESS SELECT TO CONTINUE");
  while(digitalRead(BTN_SELECT) == HIGH) { delay(50); }
  delay(300);
}

void resetTTTGame() {
  for (int i = 0; i < TTT_SIZE; i++) {
    for (int j = 0; j < TTT_SIZE; j++) {
      tttBoard[i][j] = ' ';
    }
  }
  playerTurnTTT = (random(2) == 0);
  gameOverTTT = false;
  selectedRow = 1;
  selectedCol = 1;
  tft.fillScreen(TTT_BG);
  drawTTTBoard();
  updateTTTScore();
  drawTTTSelector();
  if (!playerTurnTTT) { delay(500); botMoveTTT(); }
}

void botMoveTTT() {
  if (gameOverTTT || playerTurnTTT) return;
  delay(400);
  int bestRow = -1, bestCol = -1;
  int tempRow1, tempCol1, tempRow2, tempCol2, tempRow3, tempCol3;
  for (int i = 0; i < TTT_SIZE; i++) {
    for (int j = 0; j < TTT_SIZE; j++) {
      if (tttBoard[i][j] == ' ') {
        tttBoard[i][j] = 'X';
        if (checkWinTTT('X', tempRow1, tempCol1, tempRow2, tempCol2, tempRow3, tempCol3)) {
          bestRow = i; bestCol = j;
        }
        tttBoard[i][j] = ' ';
        if (bestRow != -1) break;
      }
    }
    if (bestRow != -1) break;
  }
  if (bestRow == -1) {
    for (int i = 0; i < TTT_SIZE; i++) {
      for (int j = 0; j < TTT_SIZE; j++) {
        if (tttBoard[i][j] == ' ') {
          tttBoard[i][j] = 'O';
          if (checkWinTTT('O', tempRow1, tempCol1, tempRow2, tempCol2, tempRow3, tempCol3)) {
            bestRow = i; bestCol = j;
          }
          tttBoard[i][j] = ' ';
          if (bestRow != -1) break;
        }
      }
      if (bestRow != -1) break;
    }
  }
  if (bestRow == -1 && tttBoard[1][1] == ' ') { bestRow = 1; bestCol = 1; }
  if (bestRow == -1) {
    int corners[4][2] = {{0,0}, {0,2}, {2,0}, {2,2}};
    for (int i = 0; i < 4; i++) {
      if (tttBoard[corners[i][0]][corners[i][1]] == ' ') {
        bestRow = corners[i][0]; bestCol = corners[i][1]; break;
      }
    }
  }
  if (bestRow == -1) {
    do { bestRow = random(TTT_SIZE); bestCol = random(TTT_SIZE); } while (tttBoard[bestRow][bestCol] != ' ');
  }
  tttBoard[bestRow][bestCol] = 'X';
  drawTTTBoard();
  int winRow1, winCol1, winRow2, winCol2, winRow3, winCol3;
  if (checkWinTTT('X', winRow1, winCol1, winRow2, winCol2, winRow3, winCol3)) {
    drawWinningLineTTT(winRow1, winCol1, winRow2, winCol2, winRow3, winCol3);
    botScoreTTT++; gameOverTTT = true; updateTTTScore(); showGameOverTTT("BOT WINS!"); resetTTTGame();
  } else if (isDrawTTT()) { drawScoreTTT++; gameOverTTT = true; updateTTTScore(); showGameOverTTT("DRAW!"); resetTTTGame();
  } else { playerTurnTTT = true; updateTTTScore(); drawTTTSelector(); }
}

void runTicTacToe() {
  static unsigned long lastMoveTime = 0, lastBPress = 0;
  const unsigned long debounceDelay = 200;
  if (digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) { lastBPress = millis(); returnToMenu(); return; }
  if (!gameOverTTT && playerTurnTTT && millis() - lastMoveTime > debounceDelay) {
    bool moved = false;
    if (digitalRead(BTN_UP) == LOW && selectedRow > 0) { selectedRow--; moved = true; }
    else if (digitalRead(BTN_DOWN) == LOW && selectedRow < TTT_SIZE - 1) { selectedRow++; moved = true; }
    else if (digitalRead(BTN_LEFT) == LOW && selectedCol > 0) { selectedCol--; moved = true; }
    else if (digitalRead(BTN_RIGHT) == LOW && selectedCol < TTT_SIZE - 1) { selectedCol++; moved = true; }
    else if (digitalRead(BTN_SELECT) == LOW) {
      if (tttBoard[selectedRow][selectedCol] == ' ') {
        tttBoard[selectedRow][selectedCol] = 'O';
        drawTTTBoard();
        int winRow1, winCol1, winRow2, winCol2, winRow3, winCol3;
        if (checkWinTTT('O', winRow1, winCol1, winRow2, winCol2, winRow3, winCol3)) {
          drawWinningLineTTT(winRow1, winCol1, winRow2, winCol2, winRow3, winCol3);
          playerScoreTTT++; gameOverTTT = true; updateTTTScore(); showGameOverTTT("YOU WIN!"); resetTTTGame();
        } else if (isDrawTTT()) { drawScoreTTT++; gameOverTTT = true; updateTTTScore(); showGameOverTTT("DRAW!"); resetTTTGame();
        } else { playerTurnTTT = false; updateTTTScore(); botMoveTTT(); }
        moved = true;
      }
    }
    if (moved) { lastMoveTime = millis(); tft.fillScreen(TTT_BG); drawTTTBoard(); updateTTTScore(); drawTTTSelector(); }
  }
  delay(10);
}

// ============= ROCK PAPER SCISSORS FUNCTIONS =============
void drawMintBackground() { tft.fillScreen(MINT_BG); }

void drawRealRock(int x, int y, int s) {
  uint16_t rockDark = 0x3DEF, rockLight = 0x7BEF, rockMid = 0x5ACB;
  tft.fillRoundRect(x+2, y+4, s-4, s-8, 10, rockMid);
  tft.fillRoundRect(x+2, y+s-12, s-4, 8, 6, rockDark);
  tft.fillRect(x+s-10, y+8, 6, s-16, rockDark);
  tft.fillRoundRect(x+2, y+4, 8, s-8, 4, rockLight);
  tft.fillRect(x+6, y+4, s-12, 6, rockLight);
  for(int i=0; i<4; i++) {
    int startX = x+random(8, s-8), startY = y+random(10, s-10);
    tft.drawLine(startX, startY, startX+random(-5, 5), startY+random(8, 15), rockDark);
    tft.drawLine(startX, startY, startX+random(5, 10), startY+random(-3, 3), rockDark);
  }
  for(int i=0; i<8; i++) tft.fillCircle(x+random(8, s-8), y+random(10, s-10), random(1, 3), rockDark);
  for(int i=0; i<40; i++) tft.drawPixel(x+random(5, s-5), y+random(8, s-8), 0x2104);
  tft.drawPixel(x+random(6, s-6), y+random(8, s-8), 0xFFFF);
  tft.drawPixel(x+random(6, s-6), y+random(8, s-8), 0xFFFF);
  tft.drawRoundRect(x+2, y+4, s-4, s-8, 10, 0x2945);
  for(int i=0; i<15; i++) tft.drawPixel(x+random(5, s-5), y+random(10, s-10), 0x2104);
}

void drawProPaper(int x, int y, int s) {
  tft.fillRect(x, y, s-4, s, COLOR_PAPER);
  tft.drawRect(x, y, s-4, s, 0x0000);
  for(int i=6; i<s; i+=6) tft.drawFastHLine(x+2, y+i, s-8, 0xAD7F);
  tft.fillRect(x+2, y+2, 3, s-4, 0x94B2);
}

void drawProScissor(int x, int y, int s) {
  tft.drawCircle(x+8, y+s-10, 7, COLOR_SC_H);
  tft.drawCircle(x+s-12, y+s-10, 7, COLOR_SC_H);
  tft.fillTriangle(x+10, y+s-15, x+s-5, y+5, x+s-10, y+2, 0x0000);
  tft.fillTriangle(x+s-14, y+s-15, x+5, y+5, x+10, y+2, 0x0000);
  tft.drawLine(x+12, y+s-18, x+s-12, y+8, 0xFFFF);
}

void updateScoreRPS() {
  tft.fillRoundRect(0, 0, screenWidth, 40, 8, 0x0000);
  tft.drawRoundRect(0, 0, screenWidth, 40, 8, COLOR_GOLD_RPS);
  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(30, 12); tft.print("PLAYER: "); tft.print(pScore);
  tft.setCursor(200, 12); tft.print("BOT: "); tft.print(bScore);
  tft.drawFastVLine(160, 5, 30, COLOR_GOLD_RPS);
}

void drawSelectionBoxes() {
  tft.fillRect(0, 100, screenWidth, 80, MINT_BG);
  for (int i = 1; i <= 3; i++) {
    int x = 40 + (i - 1) * 95;
    uint16_t border = (currentSel == i) ? 0xF800 : 0x0000; 
    tft.drawRect(x-3, 147, 70, 70, border); 
    tft.drawRect(x-4, 146, 72, 72, border);
    tft.drawRect(x-5, 145, 74, 74, border);
    if(currentSel == i) {
      for(int g=1; g<=3; g++) tft.drawRect(x-3-g, 147-g, 70+(g*2), 70+(g*2), 0xF820);
    }
    if(i==1) drawRealRock(x+18, 153, 50);
    else if(i==2) drawProPaper(x+18, 153, 50);
    else if(i==3) drawProScissor(x+18, 153, 50);
  }
  tft.setTextSize(1);
  tft.setTextColor(0xFFFF);
  tft.setCursor(20, 225);
  tft.print("LEFT/RIGHT: Select   SELECT: Play | B: Menu");
}

void drawVSAnimation() { delay(800); }

void showResultAnimationRPS(String result, uint16_t color) {
  int boxX = 60, boxY = 90, boxW = 200, boxH = 60;
  for(int i=0; i<10; i++) { tft.drawRoundRect(boxX-i, boxY-i, boxW+(i*2), boxH+(i*2), 10, color); delay(20); }
  tft.fillRoundRect(boxX, boxY, boxW, boxH, 10, 0x0000);
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 10, color);
  tft.setTextSize(3);
  tft.setTextColor(color);
  int textX = boxX + (boxW/2) - (result.length() * 9);
  tft.setCursor(textX, boxY + 20);
  tft.print(result);
  delay(1500);
}

void checkUltimateWinner() {
  if (pScore >= 5 || bScore >= 5) {
    delay(500);
    for(int i=0; i<screenWidth; i+=10) { tft.drawFastVLine(i, 0, screenHeight, 0x0000); delay(8); }
    tft.drawRect(5, 5, screenWidth-10, screenHeight-10, COLOR_GOLD_RPS);
    tft.drawRect(8, 8, screenWidth-16, screenHeight-16, COLOR_GOLD_RPS);
    tft.drawRect(11, 11, screenWidth-22, screenHeight-22, COLOR_GOLD_RPS);
    tft.setTextSize(3);
    if (pScore >= 5) {
      tft.fillScreen(0x0000);
      tft.setTextColor(COLOR_WIN);
      tft.setCursor(65, 50);
      tft.println("VICTORY!");
      tft.setTextSize(2);
      tft.setTextColor(0xFFFF);
      tft.setCursor(45, 90);
      tft.println("YOU ARE SUPREME!");
      tft.fillRect(135, 130, 50, 30, COLOR_GOLD_RPS);
      tft.fillRect(155, 160, 10, 20, COLOR_GOLD_RPS);
      tft.fillCircle(160, 135, 25, COLOR_GOLD_RPS);
      tft.fillCircle(160, 135, 20, 0x0000);
      tft.fillRect(130, 165, 60, 5, COLOR_GOLD_RPS);
    } else {
      tft.fillScreen(0x0000);
      tft.setTextColor(COLOR_LOSS);
      tft.setCursor(55, 50);
      tft.println("GAME OVER");
      tft.setTextSize(2);
      tft.setTextColor(0xFFFF);
      tft.setCursor(55, 90);
      tft.println("BOT IS SUPREME!");
      tft.drawLine(130, 130, 190, 170, COLOR_LOSS);
      tft.drawLine(190, 130, 130, 170, COLOR_LOSS);
      tft.drawLine(140, 130, 180, 170, COLOR_LOSS);
      tft.drawLine(180, 130, 140, 170, COLOR_LOSS);
    }
    unsigned long blinkStart = millis();
    while (digitalRead(BTN_SELECT) == HIGH) {
      if((millis() - blinkStart) > 500) {
        tft.setTextSize(1);
        tft.setTextColor(0xAD7F);
        tft.setCursor(85, 210);
        tft.print("PRESS SELECT");
        delay(100);
        tft.fillRect(85, 210, 150, 15, 0x0000);
        blinkStart = millis();
      }
      delay(10);
    }
    tft.fillScreen(0xFFFF);
    delay(100);
    pScore = 0; bScore = 0;
    drawMintBackground();
    updateScoreRPS();
    drawSelectionBoxes();
    delay(500);
  }
}

void runRPSGame() {
  static unsigned long lastButtonTime = 0;
  static unsigned long lastBPress = 0;
  
  if (digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) {
    lastBPress = millis();
    returnToMenu();
    return;
  }
  
  if (digitalRead(BTN_LEFT) == LOW && millis() - lastButtonTime > 200) {
    lastButtonTime = millis();
    currentSel--;
    if(currentSel < 1) currentSel = 3;
    drawSelectionBoxes(); 
  }
  
  if (digitalRead(BTN_RIGHT) == LOW && millis() - lastButtonTime > 200) {
    lastButtonTime = millis();
    currentSel++;
    if(currentSel > 3) currentSel = 1;
    drawSelectionBoxes();
  }
  
  if (digitalRead(BTN_SELECT) == LOW && millis() - lastButtonTime > 300) {
    lastButtonTime = millis();
    
    tft.fillRect(0, 45, screenWidth, 55, MINT_BG);
    drawVSAnimation();
    tft.fillRect(0, 45, screenWidth, 80, MINT_BG);
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(40, 55);
    tft.print("YOU");
    
    if(currentSel == 1) drawRealRock(35, 75, 60);
    else if(currentSel == 2) drawProPaper(35, 75, 60);
    else drawProScissor(35, 75, 60);
    
    int botPick = random(1, 4);
    
    tft.setTextSize(2);
    tft.setCursor(200, 55);
    tft.print("BOT");
    
    for(int i = 0; i < 5; i++) {
        tft.setCursor(170, 90);
        tft.setTextColor(0xF800);
        tft.print("THINKING");
        delay(200);
        tft.fillRect(170, 90, 100, 15, MINT_BG);
        delay(100);
    }
    
    tft.fillRect(140, 70, 130, 50, MINT_BG);
    
    if(botPick == 1) drawRealRock(185, 75, 60);
    else if(botPick == 2) drawProPaper(185, 75, 60);
    else drawProScissor(185, 75, 60);
    
    delay(1000);
    
    String result;
    uint16_t resultColor;
    
    if (currentSel == botPick) {
        result = "DRAW!";
        resultColor = 0xFFFF;
    } else if ((currentSel == 1 && botPick == 3) || 
               (currentSel == 2 && botPick == 1) || 
               (currentSel == 3 && botPick == 2)) {
        pScore++;
        result = "YOU WIN!";
        resultColor = COLOR_WIN;
    } else {
        bScore++;
        result = "YOU LOST!";
        resultColor = COLOR_LOSS;
    }
    
    updateScoreRPS();
    showResultAnimationRPS(result, resultColor);
    
    if (pScore >= 5 || bScore >= 5) {
      checkUltimateWinner();
    } else {
      drawMintBackground();
      updateScoreRPS();
      drawSelectionBoxes();
    }
  }        
  delay(10);
}

// ============= MAIN SETUP AND LOOP =============
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
  tft.fillScreen(TFT_BLACK);
  
  screenWidth = tft.width();
  screenHeight = tft.height();
  
  randomSeed(analogRead(34));
  
  initDots();
  showMenu();
}

void loop() {
  static unsigned long lastButtonTime = 0;
  static bool upPressed = false;
  static bool downPressed = false;
  
  switch (currentGame) {
    case GAME_MENU:
      if (digitalRead(BTN_UP) == LOW && !upPressed && millis() - lastButtonTime > 200) {
        lastButtonTime = millis();
        upPressed = true;
        selectedGame = (selectedGame > 0) ? selectedGame - 1 : 4;
        updateMenuSelection();
      } 
      else if (digitalRead(BTN_UP) == HIGH) upPressed = false;
      
      if (digitalRead(BTN_DOWN) == LOW && !downPressed && millis() - lastButtonTime > 200) {
        lastButtonTime = millis();
        downPressed = true;
        selectedGame = (selectedGame < 4) ? selectedGame + 1 : 0;
        updateMenuSelection();
      }
      else if (digitalRead(BTN_DOWN) == HIGH) downPressed = false;
      
      if (digitalRead(BTN_A) == LOW && millis() - lastButtonTime > 200) {
        lastButtonTime = millis();
        if (selectedGame == 0) { currentGame = GAME_SNAKE; resetSnakeGame(); }
        else if (selectedGame == 1) { currentGame = GAME_FLAPPY; showStartScreenFlappy(); }
        else if (selectedGame == 2) { currentGame = GAME_CONNECT4; resetGameC4(); }
        else if (selectedGame == 3) { currentGame = GAME_TICTACTOE; resetTTTGame(); }
        else { currentGame = GAME_RPS; drawMintBackground(); updateScoreRPS(); drawSelectionBoxes(); }
      }
      break;
      
    case GAME_SNAKE: runSnakeGame(); break;
    case GAME_FLAPPY: runFlappyGame(); break;
    case GAME_CONNECT4: runConnect4Game(); break;
    case GAME_TICTACTOE: runTicTacToe(); break;
    case GAME_RPS: runRPSGame(); break;
  }
  
  delay(10);
}
