#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

TFT_eSPI tft = TFT_eSPI();

/*______Button Definitions_______*/
#define BTN_START    32  // Fly / Start (active LOW)
#define BTN_SELECT   25  // Pause / Resume (active LOW)

/*______Game Constants_______*/
#define DRAW_LOOP_INTERVAL 50
#define MAX_FALL_RATE 15
#define FLY_FORCE -6
#define GRAVITY 1
#define GROUND_Y 220

#define addr 0

/*______Game Variables_______*/
int currentpage;
int currentWing;
int flX, flY, fallRate;
int pillarPos, gapPosition;
int gapHeight;
int score;
int highScore = 0;
bool running = false;
bool crashed = false;
bool paused  = false;          // ← NEW

unsigned long lastDebounceTime = 0;
unsigned long lastButtonPress  = 0;
unsigned long lastPausePress   = 0;  // ← NEW separate debounce for pause
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

int currentpcolour = WHITE;

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
void drawPauseOverlay();
void removePauseOverlay();
void setupButtons();
bool readStartButton();
bool readPauseButton();   // ← NEW
int  getRandomGapHeight();

/*______Setup_______*/
void setup() {
  Serial.begin(9600);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  setupButtons();

  EEPROM.begin(512);
  highScore = EEPROM.read(addr);

  tft.setTextSize(3); tft.setTextColor(WHITE);
  tft.setCursor(50, 10);  tft.print("This Project");
  tft.setCursor(90, 50);  tft.print("Done By");
  tft.setTextSize(2); tft.setTextColor(YELLOW);
  tft.setCursor(50, 110); tft.print("Teach Me Something");
  tft.setTextSize(3); tft.setCursor(5, 160); tft.setTextColor(GREEN);
  tft.print("Loading...");

  for (int i = 0; i < 320; i++) { tft.fillRect(0, 210, i, 10, RED); delay(2); }
  delay(500);
  drawHome();
  currentpage = 0;
}

/*______Button Setup_______*/
void setupButtons() {
  pinMode(BTN_START,  INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
}

bool readStartButton() {
  if (digitalRead(BTN_SELECT) == LOW) {
    if (millis() - lastDebounceTime > debounceDelay) {
      lastDebounceTime = millis();
      return true;
    }
  }
  return false;
}

// Pause button — BTN_START (pin 32) is free when game is running
bool readPauseButton() {
  if (digitalRead(BTN_START) == LOW) {
    if (millis() - lastPausePress > 300) {   // 300 ms to avoid double-fire
      lastPausePress = millis();
      return true;
    }
  }
  return false;
}

/*______Random Gap_______*/
int getRandomGapHeight() {
  int gaps[] = {65, 70, 75, 80, 85, 90};
  return gaps[random(0, 6)];
}

/*______Home Screen_______*/
void drawHome() {
  tft.fillScreen(BLACK);
  tft.drawRoundRect(0, 0, 319, 240, 8, WHITE);

  tft.fillRoundRect(60, 180, 200, 40, 8, RED);
  tft.drawRoundRect(60, 180, 200, 40, 8, WHITE);

  tft.setTextSize(3); tft.setCursor(70, 100); tft.setTextColor(WHITE);
  tft.print("FLAPPY GAME");
  tft.setTextSize(2); tft.setCursor(50, 30);  tft.setTextColor(LIME);
  tft.print("Teach Me Something");
  tft.setTextSize(3); tft.setCursor(115, 190); tft.setTextColor(WHITE);
  tft.print("START");
  tft.setTextSize(1); tft.setCursor(50, 230);  tft.setTextColor(YELLOW);
  tft.print("Press START to Play");
}

/*______Start Game_______*/
void startGame() {
  flX      = 50;
  flY      = 100;
  fallRate = 0;
  pillarPos   = 250;
  gapPosition = 40 + random(0, 80);
  gapHeight   = getRandomGapHeight();
  crashed  = false;
  running  = false;
  paused   = false;          // ← reset pause
  score    = 0;
  currentWing    = 0;
  currentpcolour = WHITE;
  highScore = EEPROM.read(addr);
  randomSeed(millis());

  tft.fillScreen(BLACK);

  int ty = GROUND_Y;
  for (int tx = 0; tx <= 320; tx += 20) {
    tft.fillTriangle(tx,      ty,      tx+10, ty,    tx,    ty+10, WHITE);
    tft.fillTriangle(tx+10,   ty+10,   tx+10, ty,    tx,    ty+10, BLACK);
    tft.fillTriangle(tx+10,   ty,      tx+20, ty,    tx+10, ty+10, BLACK);
    tft.fillTriangle(tx+20,   ty+10,   tx+20, ty,    tx+10, ty+10, WHITE);
  }
  tft.fillRect(0, GROUND_Y + 8, 320, 12, GREEN);

  nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
  lastFlyTime = 0;
}

/*______Pause Overlay_______*/
void drawPauseOverlay() {
  // semi-transparent dark box in the centre
  tft.fillRoundRect(60, 80, 200, 80, 10, 0x18C3);   // dark blue-grey
  tft.drawRoundRect(60, 80, 200, 80, 10, WHITE);

  tft.setTextColor(WHITE);
  tft.setTextSize(4);
  tft.setCursor(85, 95);
  tft.print("PAUSED");

  tft.setTextSize(1);
  tft.setTextColor(YELLOW);
  tft.setCursor(72, 145);
  tft.print("Press PAUSE btn to resume");
}

void removePauseOverlay() {
  // Redraw the sky region the overlay covered
  tft.fillRoundRect(60, 80, 200, 80, 10, BLACK);

  // Redraw pillar and bird that may have been under the overlay
  drawPillar(pillarPos, gapPosition, gapHeight);
  drawFlappy(flX, flY);
}

/*______Draw Loop_______*/
void drawLoop() {
  clearPillar(pillarPos, gapPosition, gapHeight);
  clearFlappy(flX, flY);

  if (running) {
    fallRate += GRAVITY;
    if (fallRate > MAX_FALL_RATE) fallRate = MAX_FALL_RATE;
    flY += fallRate;
    if (flY < 0) { flY = 0; fallRate = 0; }

    pillarPos -= 5;
    if (pillarPos <= 0 && pillarPos > -5) {
      score += 5;
    } else if (pillarPos < -50) {
      pillarPos   = 250 + random(0, 7) * 10;
      gapPosition = 20 + random(0, 90);
      gapHeight   = getRandomGapHeight();
    }
  }

  drawPillar(pillarPos, gapPosition, gapHeight);
  drawFlappy(flX, flY);

  switch (currentWing) {
    case 0: case 1: drawWing1(flX, flY); break;
    case 2: case 3: drawWing2(flX, flY); break;
    case 4: case 5: drawWing3(flX, flY); break;
  }



  currentWing++;
  if (currentWing == 6) currentWing = 0;

  // Score
  tft.fillRect(240, 5, 70, 30, WHITE);
  tft.setTextColor(BLACK); tft.setTextSize(2);
  tft.setCursor(245, 10); tft.print("S:");
  tft.setCursor(265, 10); tft.print(score);
}

/*______Check Collisions_______*/
void checkCollision() {
  if (flY <= 0)              crashed = true;
  if (flY + 24 >= GROUND_Y)  crashed = true;

  if (flX + 34 > pillarPos && flX < pillarPos + 50) {
    if (flY < gapPosition || flY + 24 > gapPosition + gapHeight) {
      crashed = true;
    }
  }

  if (crashed && running) {
    running = false;
    paused  = false;

    tft.setTextColor(RED);   tft.setTextSize(5); tft.setCursor(20, 75);  tft.print("Game Over!");
    tft.setTextColor(WHITE); tft.setTextSize(4); tft.setCursor(50, 125); tft.print("Score:");
    tft.setCursor(220, 125); tft.setTextSize(5); tft.print(score);

    if (score > highScore) {
      highScore = score;
      EEPROM.write(addr, highScore);
      EEPROM.commit();
      tft.setCursor(50, 175); tft.setTextSize(3); tft.setTextColor(YELLOW);
      tft.print("NEW HIGH!");
    }
    delay(gameOverDelay);
  }
}

/*______Draw Pillars_______*/
void drawPillar(int x, int gap, int gapH) {
  if (gap > 0) {
    tft.fillRect(x+2, 2, 46, gap-4, currentpcolour);
    tft.drawRect(x,   0, 50, gap,   BLACK);
    tft.drawRect(x+1, 1, 48, gap-2, BLACK);
  }
  int bStart = gap + gapH;
  int bH     = GROUND_Y - bStart;
  if (bH > 0) {
    tft.fillRect(x+2, bStart+2, 46, bH-4, currentpcolour);
    tft.drawRect(x,   bStart,   50, bH,   BLACK);
    tft.drawRect(x+1, bStart+1, 48, bH-2, BLACK);
  }
}

void clearPillar(int x, int gap, int gapH) {
  if (gap > 0)
    tft.fillRect(x+45, 0, 5, gap, BLACK);
  int bStart = gap + gapH;
  int bH     = GROUND_Y - bStart;
  if (bH > 0)
    tft.fillRect(x+45, bStart, 5, bH, BLACK);
}

/*______Draw Bird_______*/
void drawFlappy(int x, int y) {
  tft.fillRect(x+2,  y+8,  2, 10, BLACK);
  tft.fillRect(x+4,  y+6,  2, 2,  BLACK);
  tft.fillRect(x+6,  y+4,  2, 2,  BLACK);
  tft.fillRect(x+8,  y+2,  4, 2,  BLACK);
  tft.fillRect(x+12, y,   12, 2,  BLACK);
  tft.fillRect(x+24, y+2,  2, 2,  BLACK);
  tft.fillRect(x+26, y+4,  2, 2,  BLACK);
  tft.fillRect(x+28, y+6,  2, 6,  BLACK);
  tft.fillRect(x+10, y+22,10, 2,  BLACK);
  tft.fillRect(x+4,  y+18, 2, 2,  BLACK);
  tft.fillRect(x+6,  y+20, 4, 2,  BLACK);

  tft.fillRect(x+12, y+2,  6, 2,  YELLOW);
  tft.fillRect(x+8,  y+4,  8, 2,  YELLOW);
  tft.fillRect(x+6,  y+6, 10, 2,  YELLOW);
  tft.fillRect(x+4,  y+8, 12, 2,  YELLOW);
  tft.fillRect(x+4,  y+10,14, 2,  YELLOW);
  tft.fillRect(x+4,  y+12,16, 2,  YELLOW);
  tft.fillRect(x+4,  y+14,14, 2,  YELLOW);
  tft.fillRect(x+4,  y+16,12, 2,  YELLOW);
  tft.fillRect(x+6,  y+18,12, 2,  YELLOW);
  tft.fillRect(x+10, y+20,10, 2,  YELLOW);

  tft.fillRect(x+18, y+2,  2, 2,  BLACK);
  tft.fillRect(x+16, y+4,  2, 6,  BLACK);
  tft.fillRect(x+18, y+10, 2, 2,  BLACK);
  tft.fillRect(x+18, y+4,  2, 6,  WHITE);
  tft.fillRect(x+20, y+2,  4, 10, WHITE);
  tft.fillRect(x+24, y+4,  2, 8,  WHITE);
  tft.fillRect(x+26, y+6,  2, 6,  WHITE);
  tft.fillRect(x+24, y+6,  2, 4,  BLACK);

  tft.fillRect(x+20, y+12,12, 2,  BLACK);
  tft.fillRect(x+18, y+14, 2, 2,  BLACK);
  tft.fillRect(x+20, y+14,12, 2,  RED);
  tft.fillRect(x+32, y+14, 2, 2,  BLACK);
  tft.fillRect(x+16, y+16, 2, 2,  BLACK);
  tft.fillRect(x+18, y+16, 2, 2,  RED);
  tft.fillRect(x+20, y+16,12, 2,  BLACK);
  tft.fillRect(x+18, y+18, 2, 2,  BLACK);
  tft.fillRect(x+20, y+18,10, 2,  RED);
  tft.fillRect(x+30, y+18, 2, 2,  BLACK);
  tft.fillRect(x+20, y+20,10, 2,  BLACK);
}

void clearFlappy(int x, int y) {
  tft.fillRect(x, y, 34, 24, BLACK);
}

/*______Wing Animations_______*/
void drawWing1(int x, int y) {
  tft.fillRect(x,    y+14, 2, 6,  BLACK);
  tft.fillRect(x+2,  y+20, 8, 2,  BLACK);
  tft.fillRect(x+2,  y+12,10, 2,  BLACK);
  tft.fillRect(x+12, y+14, 2, 2,  BLACK);
  tft.fillRect(x+10, y+16, 2, 2,  BLACK);
  tft.fillRect(x+2,  y+14, 8, 6,  WHITE);
  tft.fillRect(x+8,  y+18, 2, 2,  BLACK);
  tft.fillRect(x+10, y+14, 2, 2,  WHITE);
}
void drawWing2(int x, int y) {
  tft.fillRect(x+2,  y+10,10, 2,  BLACK);
  tft.fillRect(x+2,  y+16,10, 2,  BLACK);
  tft.fillRect(x,    y+12, 2, 4,  BLACK);
  tft.fillRect(x+12, y+12, 2, 4,  BLACK);
  tft.fillRect(x+2,  y+12,10, 4,  WHITE);
}
void drawWing3(int x, int y) {
  tft.fillRect(x+2,  y+6,  8, 2,  BLACK);
  tft.fillRect(x,    y+8,  2, 6,  BLACK);
  tft.fillRect(x+10, y+8,  2, 2,  BLACK);
  tft.fillRect(x+12, y+10, 2, 4,  BLACK);
  tft.fillRect(x+10, y+14, 2, 2,  BLACK);
  tft.fillRect(x+2,  y+14, 2, 2,  BLACK);
  tft.fillRect(x+4,  y+16, 6, 2,  BLACK);
  tft.fillRect(x+2,  y+8,  8, 6,  WHITE);
  tft.fillRect(x+4,  y+14, 6, 2,  WHITE);
  tft.fillRect(x+10, y+10, 2, 4,  WHITE);
}

/*______Main Loop_______*/
void loop() {

  // ── PAUSE button (BTN_START pin 32) ──
  // Only works when game is actively running (not crashed, not on home screen)
  if (currentpage == 1 && running && !crashed) {
    if (readPauseButton()) {
      paused = !paused;
      if (paused) {
        drawPauseOverlay();
        // Shift the draw timer forward so there's no instant frame on resume
        nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
      } else {
        removePauseOverlay();
        nextDrawLoopRunTime = millis() + DRAW_LOOP_INTERVAL;
      }
    }
  }

  // ── FLY / START button (BTN_SELECT pin 25) ──
  if (readStartButton()) {
    if (currentpage == 0) {
      currentpage = 1;
      startGame();
    } else if (currentpage == 1) {
      if (crashed) {
        startGame();
      } else if (!running) {
        running  = true;
        fallRate = FLY_FORCE;
        lastFlyTime = millis();
      } else if (running && !paused && millis() - lastFlyTime > flyCooldown) {
        // Flap only when not paused
        fallRate = FLY_FORCE;
        lastFlyTime = millis();
      }
    }
  }

  // ── GAME LOOP ──
  if (currentpage == 1 && !paused) {
    if (millis() > nextDrawLoopRunTime && !crashed) {
      drawLoop();
      checkCollision();
      nextDrawLoopRunTime += DRAW_LOOP_INTERVAL;
    }
    delay(10);
  }

  delay(5);
}
