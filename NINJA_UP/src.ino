#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Button Pins
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_LEFT     27      // LEFT বাটন - বাম সাইডে যাবে
#define BTN_RIGHT    26      // RIGHT বাটন - ডান সাইডে যাবে
#define BTN_A        14      
#define BTN_B        33      
#define BTN_START    32      // Play/Pause
#define BTN_SELECT   25      // Start Game

// --- Colors (ILI9341 compatible) ---
#define BG_COLOR     TFT_BLACK
#define NINJA_COLOR  TFT_CYAN   
#define WALL_COLOR   TFT_WHITE
#define SPIKE_COLOR  TFT_RED
#define BLUE_SPIKE   TFT_BLUE
#define DOT_COLOR    0x7BEF    // Light gray
#define TRAIL_COLOR  0x0410    // Dark cyan

// --- Game Settings ---
int playerSide = 0; 
float playerY = 100;
const int wallX_Left = 0;     
const int wallX_Right = 200;   
const int wallThickness = 6;   
const int playerW = 16;        
const int playerH = 24;       

// --- Ninja Trail Settings ---
const int TRAIL_SIZE = 5;
int trailX[TRAIL_SIZE];
int trailY[TRAIL_SIZE];

struct Spike {
  float y;
  int side; 
  bool isBlue;
};

struct Dot {
  float x, y;
  float speed;
};

const int MAX_DOTS = 20;
Dot dots[MAX_DOTS];
Spike spikes[2]; 

unsigned long score = 0;      
unsigned long highScore = 0;
bool playing = false;
bool isPaused = false;
float gameSpeed = 2.5; 
bool leftButtonPressed = false;
bool rightButtonPressed = false;
unsigned long lastLeftPress = 0;
unsigned long lastRightPress = 0;
const int debounceDelay = 50;

// --- Power-up Settings ---
bool isRushMode = false;
unsigned long rushStartTime = 0;
const long rushDuration = 4000; 

// --- Function Prototypes ---
void showStartScreen();
void startGame();
void updateGame();
void gameOver();
void initDots();
void screenShake();

void setup() {
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BG_COLOR);
  randomSeed(analogRead(0));
  showStartScreen();
}

void loop() {
  if (!playing) {
    if (digitalRead(BTN_SELECT) == LOW) {
      delay(200); 
      startGame();
    }
    return;
  }
  
  // Play/Pause functionality with START button
  if (digitalRead(BTN_START) == LOW) {
    delay(200);
    isPaused = !isPaused;
    
    if (isPaused) {
      tft.fillRoundRect(80, 90, 160, 60, 10, TFT_BLUE);
      tft.drawRoundRect(80, 90, 160, 60, 10, TFT_WHITE);
      tft.setTextColor(TFT_WHITE);
      tft.setTextSize(2);
      tft.setCursor(115, 110);
      tft.print("PAUSED");
      tft.setTextSize(1);
      tft.setCursor(120, 135);
      tft.print("START to Resume");
    } else {
      tft.fillRect(80, 90, 160, 60, BG_COLOR);
    }
    delay(200);
  }
  
  if (!isPaused) {
    updateGame();
  }
}

void screenShake() {
  for (int i = 0; i < 5; i++) {
    tft.invertDisplay(true);
    delay(40);
    tft.invertDisplay(false);
    delay(40);
  }
}

void initDots() {
  for (int i = 0; i < MAX_DOTS; i++) {
    dots[i].x = random(wallX_Left + wallThickness + 5, wallX_Right - 5);
    dots[i].y = random(0, 240);
    dots[i].speed = (random(5, 15) / 10.0);
  }
}

void startGame() {
  tft.fillScreen(BG_COLOR);
  playerY = 100;
  playerSide = 0;
  score = 0;
  gameSpeed = 2.5; 
  isRushMode = false;
  isPaused = false;
  initDots(); 

  for(int i=0; i<TRAIL_SIZE; i++) {
    trailX[i] = -30; 
    trailY[i] = -30;
  }

  spikes[0].y = -50;
  spikes[0].side = random(0, 2);
  spikes[0].isBlue = false;

  spikes[1].y = -200; 
  spikes[1].side = random(0, 2);
  spikes[1].isBlue = false;

  playing = true;
}

void updateGame() {
  // ===== LEFT এবং RIGHT বাটন দিয়ে সাইড কন্ট্রোল =====
  bool leftState = (digitalRead(BTN_LEFT) == LOW);
  bool rightState = (digitalRead(BTN_RIGHT) == LOW);
  
  // LEFT বাটন চাপলে বাম সাইডে যাবে
  if (leftState && !leftButtonPressed && (millis() - lastLeftPress > debounceDelay)) {
    playerSide = 0;
    leftButtonPressed = true;
    lastLeftPress = millis();
    
    tft.fillRect(10, 200, 80, 20, BG_COLOR);
    tft.setTextColor(TFT_GREEN, BG_COLOR);
    tft.setCursor(10, 200);
    tft.print("LEFT SIDE");
  } else if (!leftState) {
    leftButtonPressed = false;
  }
  
  // RIGHT বাটন চাপলে ডান সাইডে যাবে
  if (rightState && !rightButtonPressed && (millis() - lastRightPress > debounceDelay)) {
    playerSide = 1;
    rightButtonPressed = true;
    lastRightPress = millis();
    
    tft.fillRect(10, 200, 80, 20, BG_COLOR);
    tft.setTextColor(TFT_GREEN, BG_COLOR);
    tft.setCursor(10, 200);
    tft.print("RIGHT SIDE");
  } else if (!rightState) {
    rightButtonPressed = false;
  }

  float currentEffectiveSpeed = gameSpeed;
  int scoreMultiplier = 1;
  bool showPlayer = true;
  uint16_t currentPlayerColor = NINJA_COLOR;

  if (!isRushMode) {
    gameSpeed += 0.0024; 
    if (gameSpeed > 6.0) gameSpeed = 6.0; 
  }

  if (isRushMode) {
    unsigned long elapsed = millis() - rushStartTime;
    if (elapsed < rushDuration) {
      if (elapsed <= 6000) {
        currentEffectiveSpeed = gameSpeed * 4.0;
        scoreMultiplier = 5;
        currentPlayerColor = BLUE_SPIKE;
      } else {
        currentEffectiveSpeed = gameSpeed; 
        scoreMultiplier = 1; 
        currentPlayerColor = NINJA_COLOR;
        if ((elapsed / 120) % 2 == 0) showPlayer = false; 
      }
    } else {
      isRushMode = false; 
    }
  }

  score += scoreMultiplier; 

  // Clear play area
  tft.fillRect(wallX_Left + wallThickness, 0, (wallX_Right - wallX_Left) - wallThickness, 240, BG_COLOR);
  
  // Draw dots
  for (int i = 0; i < MAX_DOTS; i++) {
    dots[i].y += dots[i].speed + (currentEffectiveSpeed * 0.3);
    if (dots[i].y > 240) {
      dots[i].y = 0;
      dots[i].x = random(wallX_Left + wallThickness + 5, wallX_Right - 5);
    }
    tft.drawPixel((int)dots[i].x, (int)dots[i].y, DOT_COLOR);
  }

  // Update trail positions
  for (int i = TRAIL_SIZE - 1; i > 0; i--) {
    trailX[i] = trailX[i - 1];
    trailY[i] = trailY[i - 1];
  }
  int currentNinjaX = (playerSide == 0) ? (wallX_Left + wallThickness + 2) : (wallX_Right - playerW - 2);
  trailX[0] = currentNinjaX;
  trailY[0] = (int)playerY;

  // Draw trail
  for (int i = 1; i < TRAIL_SIZE; i++) {
    if (trailX[i] > 0) {
       tft.drawRect(trailX[i], trailY[i], playerW, playerH, TRAIL_COLOR);
    }
  }

  // Draw walls
  tft.fillRect(wallX_Left, 0, wallThickness, 240, WALL_COLOR);
  tft.fillRect(wallX_Right, 0, wallThickness, 240, WALL_COLOR);

  tft.drawRect(wallX_Left - 1, 0, wallThickness + 2, 240, TFT_DARKGREY);
  tft.drawRect(wallX_Right - 1, 0, wallThickness + 2, 240, TFT_DARKGREY);

  // Draw spikes
  for (int i = 0; i < 2; i++) {
    spikes[i].y += currentEffectiveSpeed;
    int spikeSize = 28;
    uint16_t sColor = spikes[i].isBlue ? BLUE_SPIKE : SPIKE_COLOR;
    
    if (spikes[i].side == 0) {
      tft.fillTriangle(wallX_Left + wallThickness, (int)spikes[i].y, 
                       wallX_Left + wallThickness, (int)spikes[i].y + spikeSize, 
                       wallX_Left + wallThickness + spikeSize, (int)spikes[i].y + (spikeSize/2), 
                       sColor);
    } else {
      tft.fillTriangle(wallX_Right, (int)spikes[i].y, 
                       wallX_Right, (int)spikes[i].y + spikeSize, 
                       wallX_Right - spikeSize, (int)spikes[i].y + (spikeSize/2), 
                       sColor);
    }

    // Collision detection
    if (spikes[i].side == playerSide) {
      if (spikes[i].y + spikeSize > playerY && spikes[i].y < playerY + playerH) {
        if (spikes[i].isBlue) {
          isRushMode = true;
          rushStartTime = millis();
          spikes[i].y = 300;
        } else if (!isRushMode) {
          delay(1000);
          gameOver();
          return;
        }
      }
    }

    // Reset spike when off screen
    if (spikes[i].y > 240) {
      int otherSpike = (i == 0) ? 1 : 0;
      spikes[i].y = spikes[otherSpike].y - random(120, 180); 
      spikes[i].side = random(0, 2);
      spikes[i].isBlue = (random(0, 12) == 1); 
    }
  }

  // Draw player ninja
  if (showPlayer) {
    tft.fillRect(currentNinjaX, (int)playerY, playerW, playerH, currentPlayerColor);
    tft.fillCircle(currentNinjaX + playerW - 4, (int)playerY + 6, 2, TFT_WHITE);
    tft.fillCircle(currentNinjaX + playerW - 4, (int)playerY + playerH - 6, 2, TFT_WHITE);
  }

  // ===== স্কোর এবং হাই স্কোর ডিসপ্লে =====
  tft.fillRect(220, 0, 100, 60, BG_COLOR);
  
  tft.setTextColor(TFT_GREEN, BG_COLOR);
  tft.setTextSize(1);
  tft.setCursor(250, 10);
  tft.print("SCORE");
  
  tft.setTextColor(TFT_YELLOW, BG_COLOR);
  tft.setTextSize(2);
  tft.setCursor(244, 22);
  char scoreBuf[15];
  unsigned long displayScore = score / 10;
  sprintf(scoreBuf, "%04lu", displayScore);
  tft.print(scoreBuf);
  
  tft.setTextColor(TFT_CYAN, BG_COLOR);
  tft.setTextSize(1);
  tft.setCursor(250, 50);
  tft.print("BEST:");
  
  tft.setTextColor(TFT_WHITE, BG_COLOR);
  tft.setTextSize(2);
  tft.setCursor(244, 62);
  sprintf(scoreBuf, "%04lu", highScore);
  tft.print(scoreBuf);
  


    if (isRushMode && (millis() - rushStartTime) < rushDuration) {
    // রাশ মোড টেক্সট
    tft.setTextSize(1);
    tft.setCursor(225, 200);  // 65 থেকে পরিবর্তন করে 200
    tft.setTextColor(TFT_YELLOW, BG_COLOR);
    tft.print("RUSH!");
    
    // রাশ মোড টাইমার বার
    int barWidth = 80;
    int barX = 220;
    int barY = 210;  // 75 থেকে পরিবর্তন করে 210
    int elapsedPercent = ((millis() - rushStartTime) * 100) / rushDuration;
    
    // ব্যাকগ্রাউন্ড বার
    tft.fillRect(barX, barY, barWidth, 6, TFT_DARKGREY);
    // রাশ মোড প্রগ্রেস বার (বাম থেকে ডানে কমবে)
    tft.fillRect(barX, barY, barWidth - (barWidth * elapsedPercent / 100), 6, TFT_RED);
    
    // রাশ মোড বাকী সময় দেখান
    unsigned long remainingTime = (rushDuration - (millis() - rushStartTime)) / 1000;
    tft.setTextColor(TFT_WHITE, BG_COLOR);
    tft.setCursor(225, 220);  // 85 থেকে পরিবর্তন করে 220
    tft.print(remainingTime);
    tft.print("s");
  } else {
    // রাশ মোড না থাকলে জায়গা পরিষ্কার করুন
    tft.fillRect(220, 190, 100, 50, BG_COLOR);  // ক্লিয়ার এলাকা আপডেট
  }
  

  
  delay(10); 
}

void showStartScreen() {
  tft.fillScreen(BG_COLOR);
  
  tft.setTextColor(NINJA_COLOR);
  tft.setTextSize(3);
  tft.setCursor(80, 40);
  tft.print("NINJA UP");
  
  tft.setTextColor(WALL_COLOR);
  tft.setTextSize(1);
  tft.setCursor(40, 90);
  tft.print("SELECT button to START");
  
  tft.setCursor(40, 110);
  tft.print("LEFT/RIGHT to SWITCH sides");
  
  tft.setCursor(40, 130);
  tft.print("START button to PAUSE/RESUME");
  
  tft.setCursor(40, 150);
  tft.print("Avoid RED spikes!");
  
  tft.setCursor(40, 170);
  tft.print("Catch BLUE spikes for RUSH!");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(80, 200);
  tft.print("Press SELECT to begin");
  
  tft.fillCircle(160, 225, 8, TFT_CYAN);
  tft.fillCircle(160, 225, 4, BG_COLOR);
}

void gameOver() {
  screenShake();
  playing = false;
  isPaused = false;
  unsigned long currentScore = score / 10;
  if (currentScore > highScore) highScore = currentScore;
  
  tft.fillRoundRect(40, 70, 240, 100, 10, TFT_RED);
  tft.drawRoundRect(40, 70, 240, 100, 10, TFT_WHITE);
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(100, 90);
  tft.print("GAME OVER!");
  
  tft.setTextSize(1);
  tft.setCursor(100, 115);
  tft.print("Score: ");
  tft.print(currentScore);
  
  tft.setCursor(100, 135);
  tft.print("Best: ");
  tft.print(highScore);
  
  tft.setTextSize(1);
  tft.setCursor(75, 180);
  tft.setTextColor(TFT_YELLOW);
  tft.print("Press SELECT to play again");
  
  delay(10);
}
