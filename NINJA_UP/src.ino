#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Button Pins
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_A        14      
#define BTN_B        33      
#define BTN_START    32      // Play/Pause
#define BTN_SELECT   25      // Start Game & Side Change

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
bool isPaused = false;        // NEW: Pause state
float gameSpeed = 1.5; 
bool buttonPressed = false; 
unsigned long lastButtonPress = 0;
const int debounceDelay = 50;

// --- Power-up Settings ---
bool isRushMode = false;
unsigned long rushStartTime = 0;
const long rushDuration = 8000; 

// --- Function Prototypes ---
void showStartScreen();
void startGame();
void updateGame();
void gameOver();
void initDots();
void screenShake();

void setup() {
  // Initialize button pins
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1);  // Landscape orientation (320x240)
  tft.fillScreen(BG_COLOR);
  randomSeed(analogRead(0));
  showStartScreen();
}

void loop() {
  if (!playing) {
    // SELECT button to start game
    if (digitalRead(BTN_SELECT) == LOW) {
      delay(200); 
      startGame();
    }
    return;
  }
  
  // Play/Pause functionality with START button
  if (digitalRead(BTN_START) == LOW) {
    delay(200); // Debounce
    isPaused = !isPaused;
    
    // Show pause message
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
      // Clear pause message by redrawing game area
      tft.fillRect(80, 90, 160, 60, BG_COLOR);
    }
    delay(200);
  }
  
  if (!isPaused) {
    updateGame();
  }
}

// --- Screen Shake Function ---
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
  gameSpeed = 1.5; 
  isRushMode = false;
  isPaused = false;  // Reset pause state
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
  // SELECT button for side change (instead of BTN_A)
  bool buttonState = (digitalRead(BTN_SELECT) == LOW);
  
  if (buttonState && !buttonPressed && (millis() - lastButtonPress > debounceDelay)) {
    playerSide = 1 - playerSide; 
    buttonPressed = true;
    lastButtonPress = millis();
  } else if (!buttonState) {
    buttonPressed = false; 
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
        currentEffectiveSpeed = gameSpeed * 3.5;
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

  // Draw boundary lines for style
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
    // Add ninja details (eyes)
    tft.fillCircle(currentNinjaX + playerW - 4, (int)playerY + 6, 2, TFT_WHITE);
    tft.fillCircle(currentNinjaX + playerW - 4, (int)playerY + playerH - 6, 2, TFT_WHITE);
  }

  // Display score
  tft.setTextColor(WALL_COLOR, BG_COLOR);
  tft.setTextSize(1);
  tft.setCursor(240, 10);
  char scoreBuf[15];
  sprintf(scoreBuf, "S: %04lu", score / 10);
  tft.print(scoreBuf);
  
  // Display high score
  tft.setCursor(240, 35);
  sprintf(scoreBuf, "BEST: %04lu", highScore);
  tft.print(scoreBuf);
  
  // Display rush mode indicator
  // if (isRushMode && (millis() - rushStartTime) < rushDuration) {
  //   tft.setTextSize(1);
  //   tft.setCursor(10, 10);
  //   tft.setTextColor(TFT_YELLOW, BG_COLOR);
  //   tft.print("RUSH MODE!");
    
  //   // Draw rush mode timer bar
  //   int barWidth = 80;
  //   int elapsedPercent = ((millis() - rushStartTime) * 100) / rushDuration;
  //   tft.fillRect(10, 25, barWidth, 5, TFT_DARKGREY);
  //   tft.fillRect(10, 25, barWidth - (barWidth * elapsedPercent / 100), 5, TFT_YELLOW);
  // }

  // // Show control hints
  // tft.setTextSize(1);
  // tft.setCursor(10, 230);
  // tft.setTextColor(TFT_GREEN, BG_COLOR);
  // tft.print("SELECT:Switch  START:Pause");
  
  delay(10); 
}

void showStartScreen() {
  tft.fillScreen(BG_COLOR);
  
  // Title
  tft.setTextColor(NINJA_COLOR);
  tft.setTextSize(3);
  tft.setCursor(80, 50);
  tft.print("NINJA UP");
  
  // Instructions
  tft.setTextColor(WALL_COLOR);
  tft.setTextSize(1);
  tft.setCursor(50, 100);
  tft.print("SELECT button to START");
  
  tft.setCursor(50, 120);
  tft.print("SELECT also to SWITCH sides");
  
  tft.setCursor(50, 140);
  tft.print("START button to PAUSE/RESUME");
  
  tft.setCursor(50, 160);
  tft.print("Avoid RED spikes!");
  
  tft.setCursor(50, 180);
  tft.print("Catch BLUE spikes for RUSH MODE!");
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(80, 210);
  tft.print("Press SELECT to begin");
  
  // Draw decorative ninja star
  tft.fillCircle(160, 230, 8, TFT_CYAN);
  tft.fillCircle(160, 230, 4, BG_COLOR);
}

void gameOver() {
  screenShake();
  playing = false;
  isPaused = false;
  unsigned long currentScore = score / 10;
  if (currentScore > highScore) highScore = currentScore;
  
  // Draw game over panel
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
