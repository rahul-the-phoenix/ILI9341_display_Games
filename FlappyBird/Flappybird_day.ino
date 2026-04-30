#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

TFT_eSPI tft = TFT_eSPI();

/*______Button Definitions_______*/
#define BTN_START    32  // Fly / Start game (active LOW)
#define BTN_SELECT   25  // Select / Confirm / Reset (active LOW)

/*______Game Constants_______*/
#define DRAW_LOOP_INTERVAL 50
#define MAX_FALL_RATE 15     // Maximum falling speed
#define FLY_FORCE -6         // Reduced fly force (was -8)
#define GRAVITY 1            // Gravity constant

// EEPROM address for highscore
#define addr 0

/*______Game Variables_______*/
int currentpage;
int currentWing;
int flX, flY, fallRate;
int pillarPos, gapPosition;
int score;
int highScore = 0;
bool running = false;
bool crashed = false;
bool startButtonPressed = false;
bool selectButtonPressed = false;
unsigned long lastDebounceTime = 0;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 50;
const unsigned long gameOverDelay = 500;
unsigned long lastFlyTime = 0;     // To prevent multiple fly inputs
const unsigned long flyCooldown = 150; // Cooldown between fly actions

long nextDrawLoopRunTime;

// Colors
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define LIME    0x07E0

int currentpcolour = GREEN;

/*______Function Prototypes_______*/
void drawHome();
void startGame();
void drawLoop();
void checkCollision();
void drawPillar(int x, int gap);
void clearPillar(int x, int gap);
void drawFlappy(int x, int y);
void clearFlappy(int x, int y);
void drawWing1(int x, int y);
void drawWing2(int x, int y);
void drawWing3(int x, int y);
void resetScore();
void setupButtons();
bool readStartButton();
bool readSelectButton();

/*______Setup Function_______*/
void setup() {
  Serial.begin(9600);
  
  // Initialize display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  
  // Setup buttons
  setupButtons();
  
  // Initialize EEPROM
  EEPROM.begin(512);
  highScore = EEPROM.read(addr);
  
  // Show loading screen
  tft.setTextSize(3);
  tft.setTextColor(WHITE);
  tft.setCursor(50, 10);
  tft.print("This Project");
  tft.setCursor(90, 50);
  tft.print("Done By");
  tft.setTextSize(2);
  tft.setTextColor(YELLOW);
  tft.setCursor(50, 110);
  tft.print("Teach Me Something");
  tft.setTextSize(3);
  tft.setCursor(5, 160);
  tft.setTextColor(GREEN);
  tft.print("Loading...");
  
  // Loading bar
  for (int i = 0; i < 320; i++) {
    tft.fillRect(0, 210, i, 10, RED);
    delay(2);
  }
  
  delay(500);
  drawHome();
  currentpage = 0;
}

/*______Button Setup_______*/
void setupButtons() {
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
}

/*______Read Buttons with Debouncing_______*/
bool readStartButton() {
  if (digitalRead(BTN_START) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      return true;
    }
  }
  return false;
}

bool readSelectButton() {
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      return true;
    }
  }
  return false;
}

/*______Draw Home Screen_______*/
void drawHome() {
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 319, 240, 8, WHITE);
  
  // Start button area
  tft.fillRoundRect(60, 180, 200, 40, 8, RED);
  tft.drawRoundRect(60, 180, 200, 40, 8, WHITE);
  
  tft.setTextSize(3);
  tft.setCursor(70, 100);
  tft.setTextColor(WHITE);
  tft.print("FLAPPY GAME");
  
  tft.setTextSize(2);
  tft.setCursor(50, 30);
  tft.setTextColor(LIME);
  tft.print("Teach Me Something");
  
  tft.setTextSize(3);
  tft.setCursor(115, 190);
  tft.setTextColor(WHITE);
  tft.print("START");
  
  // Instructions
  tft.setTextSize(1);
  tft.setCursor(50, 230);
  tft.setTextColor(YELLOW);
  tft.print("Press START to Play");
}

/*______Start Game_______*/
void startGame() {
  flX = 50;
  flY = 60;  // Changed from 25 to a safer starting position
  fallRate = 0;
  pillarPos = 200;
  gapPosition = 60;
  crashed = false;
  running = false;
  score = 0;
  currentWing = 0;
  currentpcolour = GREEN;
  highScore = EEPROM.read(addr);
  
  tft.fillScreen(BLUE);
  
  tft.setTextColor(YELLOW);

  
  // Draw Ground
  int ty = 230;
  for (int tx = 0; tx <= 300; tx += 20) {
    tft.fillTriangle(tx, ty, tx + 10, ty, tx, ty + 10, GREEN);
    tft.fillTriangle(tx + 10, ty + 10, tx + 10, ty, tx, ty + 10, YELLOW);
    tft.fillTriangle(tx + 10, ty, tx + 20, ty, tx + 10, ty + 10, YELLOW);
    tft.fillTriangle(tx + 20, ty + 10, tx + 20, ty, tx + 10, ty + 10, GREEN);
  }
  
  nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
  lastFlyTime = 0; // Reset fly cooldown
}

/*______Draw Loop_______*/
void drawLoop() {
  // Clear moving items
  clearPillar(pillarPos, gapPosition);
  clearFlappy(flX, flY);
  
  // Move items
  if (running) {
    // Apply gravity with maximum limit
    fallRate += GRAVITY;
    if (fallRate > MAX_FALL_RATE) {
      fallRate = MAX_FALL_RATE;
    }
    flY += fallRate;
    
    // Boundary check to prevent going above screen
    if (flY < 0) {
      flY = 0;
      fallRate = 0;
    }
    
    pillarPos -= 5;
    if (pillarPos == 0) {
      score = score + 5;
    } else if (pillarPos < -50) {
      pillarPos = 160 + (random(0, 6) * 10);;
        gapPosition = 20 + (random(0, 10) * 10);  // 20 + (0-9)*10 = 20,30,40,...,110
    }
  }
  
  // Draw moving items
  drawPillar(pillarPos, gapPosition);
  drawFlappy(flX, flY);
  
  // Animate wings
  switch (currentWing) {
    case 0: case 1: drawWing1(flX, flY); break;
    case 2: case 3: drawWing2(flX, flY); break;
    case 4: case 5: drawWing3(flX, flY); break;
  }
  
  // Change pillar color for high score
  if (score == highScore && score > 0) {
    currentpcolour = YELLOW;
  } else {
    currentpcolour = GREEN;
  }
  
  currentWing++;
  if (currentWing == 6) currentWing = 0;
  
  // Display score during game
  tft.fillRect(240, 5, 60, 30, BLUE);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(245, 10);
  tft.print("S:");
  tft.setCursor(266, 10);
  tft.print(score);
}

/*______Check Collisions_______*/
void checkCollision() {
  // Collision with top of screen
  if (flY <= 0) {
    crashed = true;
  }
  
  // Collision with ground (reduced from 206 to 200 for better collision)
  if (flY > 200) {
    crashed = true;
  }
  
  // Collision with pillar
  if (flX + 34 > pillarPos && flX < pillarPos + 50) {
    if (flY < gapPosition || flY + 24 > gapPosition + 65 + (random(0, 8) * 5)) {
      crashed = true;
    }
  }
  
  if (crashed && running) {
    running = false;
    
    tft.setTextColor(RED);
    tft.setTextSize(5);
    tft.setCursor(20, 75);
    tft.print("Game Over!");
    
    tft.setTextSize(4);
    tft.setCursor(50, 125);
    tft.print("Score:");
    
    tft.setCursor(220, 125);
    tft.setTextSize(5);
    tft.setTextColor(WHITE);
    tft.print(score);
    
    if (score > highScore) {
      highScore = score;
      EEPROM.write(addr, highScore);
      EEPROM.commit();
      tft.setCursor(50, 175);
      tft.setTextSize(3);
      tft.setTextColor(YELLOW);
      tft.print("NEW HIGH!");
    }
    
    delay(gameOverDelay);
  }
}

/*______Draw Pillars_______*/
void drawPillar(int x, int gap) {
  tft.fillRect(x + 2, 2, 46, gap - 4, currentpcolour);
  tft.fillRect(x + 2, gap + 92, 46, 136 - gap, currentpcolour);
  
  tft.drawRect(x, 0, 50, gap, BLACK);
  tft.drawRect(x + 1, 1, 48, gap - 2, BLACK);
  tft.drawRect(x, gap + 90, 50, 140 - gap, BLACK);
  tft.drawRect(x + 1, gap + 91, 48, 138 - gap, BLACK);
}

void clearPillar(int x, int gap) {
  tft.fillRect(x + 45, 0, 5, gap, BLUE);
  tft.fillRect(x + 45, gap + 90, 5, 140 - gap, BLUE);
}

/*______Draw Bird_______*/
void drawFlappy(int x, int y) {
  // Upper and lower body outlines
  tft.fillRect(x + 2, y + 8, 2, 10, BLACK);
  tft.fillRect(x + 4, y + 6, 2, 2, BLACK);
  tft.fillRect(x + 6, y + 4, 2, 2, BLACK);
  tft.fillRect(x + 8, y + 2, 4, 2, BLACK);
  tft.fillRect(x + 12, y, 12, 2, BLACK);
  tft.fillRect(x + 24, y + 2, 2, 2, BLACK);
  tft.fillRect(x + 26, y + 4, 2, 2, BLACK);
  tft.fillRect(x + 28, y + 6, 2, 6, BLACK);
  tft.fillRect(x + 10, y + 22, 10, 2, BLACK);
  tft.fillRect(x + 4, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 6, y + 20, 4, 2, BLACK);
  
  // Body fill
  tft.fillRect(x + 12, y + 2, 6, 2, YELLOW);
  tft.fillRect(x + 8, y + 4, 8, 2, YELLOW);
  tft.fillRect(x + 6, y + 6, 10, 2, YELLOW);
  tft.fillRect(x + 4, y + 8, 12, 2, YELLOW);
  tft.fillRect(x + 4, y + 10, 14, 2, YELLOW);
  tft.fillRect(x + 4, y + 12, 16, 2, YELLOW);
  tft.fillRect(x + 4, y + 14, 14, 2, YELLOW);
  tft.fillRect(x + 4, y + 16, 12, 2, YELLOW);
  tft.fillRect(x + 6, y + 18, 12, 2, YELLOW);
  tft.fillRect(x + 10, y + 20, 10, 2, YELLOW);
  
  // Eye
  tft.fillRect(x + 18, y + 2, 2, 2, BLACK);
  tft.fillRect(x + 16, y + 4, 2, 6, BLACK);
  tft.fillRect(x + 18, y + 10, 2, 2, BLACK);
  tft.fillRect(x + 18, y + 4, 2, 6, WHITE);
  tft.fillRect(x + 20, y + 2, 4, 10, WHITE);
  tft.fillRect(x + 24, y + 4, 2, 8, WHITE);
  tft.fillRect(x + 26, y + 6, 2, 6, WHITE);
  tft.fillRect(x + 24, y + 6, 2, 4, BLACK);
  
  // Beak
  tft.fillRect(x + 20, y + 12, 12, 2, BLACK);
  tft.fillRect(x + 18, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 14, 12, 2, RED);
  tft.fillRect(x + 32, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 16, y + 16, 2, 2, BLACK);
  tft.fillRect(x + 18, y + 16, 2, 2, RED);
  tft.fillRect(x + 20, y + 16, 12, 2, BLACK);
  tft.fillRect(x + 18, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 18, 10, 2, RED);
  tft.fillRect(x + 30, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 20, y + 20, 10, 2, BLACK);
}

void clearFlappy(int x, int y) {
  tft.fillRect(x, y, 34, 24, BLUE);
}

/*______Wing Animations_______*/
void drawWing1(int x, int y) {
  tft.fillRect(x, y + 14, 2, 6, BLACK);
  tft.fillRect(x + 2, y + 20, 8, 2, BLACK);
  tft.fillRect(x + 2, y + 12, 10, 2, BLACK);
  tft.fillRect(x + 12, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 10, y + 16, 2, 2, BLACK);
  tft.fillRect(x + 2, y + 14, 8, 6, WHITE);
  tft.fillRect(x + 8, y + 18, 2, 2, BLACK);
  tft.fillRect(x + 10, y + 14, 2, 2, WHITE);
}

void drawWing2(int x, int y) {
  tft.fillRect(x + 2, y + 10, 10, 2, BLACK);
  tft.fillRect(x + 2, y + 16, 10, 2, BLACK);
  tft.fillRect(x, y + 12, 2, 4, BLACK);
  tft.fillRect(x + 12, y + 12, 2, 4, BLACK);
  tft.fillRect(x + 2, y + 12, 10, 4, WHITE);
}

void drawWing3(int x, int y) {
  tft.fillRect(x + 2, y + 6, 8, 2, BLACK);
  tft.fillRect(x, y + 8, 2, 6, BLACK);
  tft.fillRect(x + 10, y + 8, 2, 2, BLACK);
  tft.fillRect(x + 12, y + 10, 2, 4, BLACK);
  tft.fillRect(x + 10, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 2, y + 14, 2, 2, BLACK);
  tft.fillRect(x + 4, y + 16, 6, 2, BLACK);
  tft.fillRect(x + 2, y + 8, 8, 6, WHITE);
  tft.fillRect(x + 4, y + 14, 6, 2, WHITE);
  tft.fillRect(x + 10, y + 10, 2, 4, WHITE);
}

/*______Reset Score_______*/
void resetScore() {
  if (!running && crashed) {
    EEPROM.write(addr, 0);
    EEPROM.commit();
    highScore = 0;
    startGame();
  }
}

/*______Main Loop_______*/
void loop() {
  // Check SELECT button for reset
  if (readSelectButton()) {
    if (currentpage == 1) {
      if (!running && crashed) {
        resetScore();
      }
    }
  }
  
  // Check START button for game control
  if (readStartButton()) {
    if (currentpage == 0) {
      // Start game from home screen
      currentpage = 1;
      startGame();
    } else if (currentpage == 1) {
      if (crashed) {
        // Restart game
        startGame();
      } else if (!running) {
        // Start flying
        running = true;
        fallRate = FLY_FORCE;  // Use reduced fly force
        lastFlyTime = millis();
      } else if (running && (millis() - lastFlyTime > flyCooldown)) {
        // Make bird fly up with cooldown to prevent multiple rapid inputs
        fallRate = FLY_FORCE;
        lastFlyTime = millis();
      }
    }
  }
  
  // Game logic for page 1 (Flappy Bird)
  if (currentpage == 1) {
    if (millis() > nextDrawLoopRunTime && !crashed) {
      drawLoop();
      checkCollision();
      nextDrawLoopRunTime += DRAW_LOOP_INTERVAL;
    }
    delay(10);
  }
  
  delay(5);
} 
