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
#define GROUND_Y 220         // Ground Y position (was 230, adjusted)

// EEPROM address for highscore
#define addr 0

/*______Game Variables_______*/
int currentpage;
int currentWing;
int flX, flY, fallRate;
int pillarPos, gapPosition;
int gapHeight;  // Variable for gap height
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
unsigned long lastFlyTime = 0;
const unsigned long flyCooldown = 150;

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
void drawPillar(int x, int gap, int gapH);
void clearPillar(int x, int gap, int gapH);
void drawFlappy(int x, int y);
void clearFlappy(int x, int y);
void drawWing1(int x, int y);
void drawWing2(int x, int y);
void drawWing3(int x, int y);
void resetScore();
void setupButtons();
bool readStartButton();
bool readSelectButton();
int getRandomGapHeight();

/*______Setup Function_______*/
void setup() {
  Serial.begin(9600);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BLACK);
  
  setupButtons();
  
  EEPROM.begin(512);
  highScore = EEPROM.read(addr);
  
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

/*______Read Buttons_______*/
bool readStartButton() {
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      return true;
    }
  }
  return false;
}

bool readSelectButton() {
  if (digitalRead(BTN_START) == LOW) {
    if (millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      return true;
    }
  }
  return false;
}

/*______Get Random Gap Height (65-90 in steps of 5)_______*/
int getRandomGapHeight() {
  int gaps[] = {65, 70, 75, 80, 85, 90};
  int index = random(0, 6);
  return gaps[index];
}

/*______Draw Home Screen_______*/
void drawHome() {
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 319, 240, 8, WHITE);
  
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
  
  tft.setTextSize(1);
  tft.setCursor(50, 230);
  tft.setTextColor(YELLOW);
  tft.print("Press START to Play");
}

/*______Start Game_______*/
void startGame() {
  flX = 50;
  flY = 100;  // Middle position for better gameplay
  fallRate = 0;
  pillarPos = 250;
  gapPosition = 40 + random(0, 80);  // Random top pillar height
  gapHeight = getRandomGapHeight();
  crashed = false;
  running = false;
  score = 0;
  currentWing = 0;
  currentpcolour = GREEN;
  highScore = EEPROM.read(addr);
  randomSeed(millis());
  
  tft.fillScreen(BLUE);
  
  // Draw Ground with grass effect
  int ty = GROUND_Y;
  for (int tx = 0; tx <= 320; tx += 20) {
    tft.fillTriangle(tx, ty, tx + 10, ty, tx, ty + 10, GREEN);
    tft.fillTriangle(tx + 10, ty + 10, tx + 10, ty, tx, ty + 10, YELLOW);
    tft.fillTriangle(tx + 10, ty, tx + 20, ty, tx + 10, ty + 10, YELLOW);
    tft.fillTriangle(tx + 20, ty + 10, tx + 20, ty, tx + 10, ty + 10, GREEN);
  }
  
  // Draw ground base
  tft.fillRect(0, GROUND_Y + 8, 320, 12, GREEN);
  
  nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
  lastFlyTime = 0;
}

/*______Draw Loop_______*/
void drawLoop() {
  clearPillar(pillarPos, gapPosition, gapHeight);
  clearFlappy(flX, flY);
  
  if (running) {
    fallRate += GRAVITY;
    if (fallRate > MAX_FALL_RATE) {
      fallRate = MAX_FALL_RATE;
    }
    flY += fallRate;
    
    if (flY < 0) {
      flY = 0;
      fallRate = 0;
    }
    
    pillarPos -= 5;
    if (pillarPos <= 0 && pillarPos > -5) {
      score = score + 5;
    } else if (pillarPos < -50) {
      pillarPos = 250+random(0,7)* 10;  // Start from right edge
      gapPosition = 20 + random(0, 90);  // Top pillar height 20-110
      gapHeight = getRandomGapHeight();  // Random gap between pillars
    }
  }
  
  drawPillar(pillarPos, gapPosition, gapHeight);
  drawFlappy(flX, flY);
  
  // Animate wings
  switch (currentWing) {
    case 0: case 1: drawWing1(flX, flY); break;
    case 2: case 3: drawWing2(flX, flY); break;
    case 4: case 5: drawWing3(flX, flY); break;
  }
  
  if (score == highScore && score > 0) {
    currentpcolour = YELLOW;
  } else {
    currentpcolour = GREEN;
  }
  
  currentWing++;
  if (currentWing == 6) currentWing = 0;
  
  tft.fillRect(240, 5, 70, 30, BLUE);
  tft.setTextColor(WHITE);
  tft.setTextSize(2);
  tft.setCursor(245, 10);
  tft.print("S:");
  tft.setCursor(265, 10);
  tft.print(score);
}

/*______Check Collisions_______*/
void checkCollision() {
  if (flY <= 0) {
    crashed = true;
  }
  
  // Ground collision - bird hits ground
  if (flY + 24 >= GROUND_Y) {
    crashed = true;
  }
  
  // Pillar collision
  if (flX + 34 > pillarPos && flX < pillarPos + 50) {
    // Check if bird is above top pillar OR below bottom pillar
    if (flY < gapPosition || flY + 24 > gapPosition + gapHeight) {
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

/*______Draw Pillars with Proper Ground Connection_______*/
void drawPillar(int x, int gap, int gapH) {
  // Top pillar - from top to gap position
  if (gap > 0) {
    tft.fillRect(x + 2, 2, 46, gap - 4, currentpcolour);
    tft.drawRect(x, 0, 50, gap, BLACK);
    tft.drawRect(x + 1, 1, 48, gap - 2, BLACK);
  }
  
  // Bottom pillar - from (gap + gapH) to GROUND_Y
  int bottomPillarStart = gap + gapH;
  int bottomPillarHeight = GROUND_Y - bottomPillarStart;
  
  if (bottomPillarHeight > 0) {
    tft.fillRect(x + 2, bottomPillarStart + 2, 46, bottomPillarHeight - 4, currentpcolour);
    tft.drawRect(x, bottomPillarStart, 50, bottomPillarHeight, BLACK);
    tft.drawRect(x + 1, bottomPillarStart + 1, 48, bottomPillarHeight - 2, BLACK);
  }
}

void clearPillar(int x, int gap, int gapH) {
  // Clear top pillar area
  if (gap > 0) {
    tft.fillRect(x + 45, 0, 5, gap, BLUE);
  }
  
  // Clear bottom pillar area
  int bottomPillarStart = gap + gapH;
  int bottomPillarHeight = GROUND_Y - bottomPillarStart;
  if (bottomPillarHeight > 0) {
    tft.fillRect(x + 45, bottomPillarStart, 5, bottomPillarHeight, BLUE);
  }
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
  if (readSelectButton()) {
    if (currentpage == 1) {
      if (!running && crashed) {
        resetScore();
      }
    }
  }
  
  if (readStartButton()) {
    if (currentpage == 0) {
      currentpage = 1;
      startGame();
    } else if (currentpage == 1) {
      if (crashed) {
        startGame();
      } else if (!running) {
        running = true;
        fallRate = FLY_FORCE;
        lastFlyTime = millis();
      } else if (running && (millis() - lastFlyTime > flyCooldown)) {
        fallRate = FLY_FORCE;
        lastFlyTime = millis();
      }
    }
  }
  
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
