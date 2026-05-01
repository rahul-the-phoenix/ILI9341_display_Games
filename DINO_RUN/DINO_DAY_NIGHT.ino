#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

// --- Button Configuration ---
#define BTN_UP     13
#define BTN_DOWN   12
#define BTN_LEFT   27
#define BTN_RIGHT  26
#define BTN_A      14
#define BTN_B      33
#define BTN_START  32
#define BTN_SELECT 25

TFT_eSPI tft = TFT_eSPI();

// --- Bitmaps (same as original, but adjusted for ILI9341) ---
const unsigned char dino_run1[] PROGMEM = { 
  0x00,0x00,0x07,0xf0,0x0f,0xf8,0x0f,0xf8,0x0f,0xf8,0x0f,0x80,0x0f,0xf8,0x0e,0x00,
  0x0f,0xe0,0x0f,0xe0,0x9f,0xe0,0xdf,0xe0,0xff,0xe0,0xff,0xe0,0xff,0xc0,0xff,0x80,
  0x7f,0x00,0x3e,0x00,0x22,0x00,0x22,0x00,0x62,0x00 
};

const unsigned char cactus_small[] PROGMEM = {
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0xdf,0x00, 0xdf,0x00, 0xdf,0x00,
  0xdf,0x00, 0xff,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00
};

const unsigned char cactus_large[] PROGMEM = {
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0xdd,0x00, 0xdd,0x00, 0xdd,0x00,
  0xdd,0x00, 0xdd,0x00, 0xff,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00
};

const unsigned char cactus_double[] PROGMEM = {
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 
  0xdf,0xfb,0x00, 0xdf,0xfb,0x00, 0xdf,0xfb,0x00, 0xff,0xff,0x00, 0x1c,0x38,0x00, 
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00,
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00
};

const unsigned char cloud_big[] PROGMEM = {
  0x00, 0xfe, 0x00, 0x07, 0xff, 0x80, 0x1f, 0xff, 0xe0, 0x3f, 0xff, 0xf0, 
  0x7f, 0xff, 0xf8, 0x7f, 0xff, 0xf8, 0x7f, 0xff, 0xf8, 0x3f, 0xff, 0xf0,
  0x1f, 0xff, 0xe0, 0x00, 0x00, 0x00
};

// --- Game Variables ---
int dinoY = 200, prevDinoY = 200;  // Adjusted for ILI9341 (320x240)
float jumpVel = 0;
const float gravity = 1.05; 
float cactusX = 320;  // Screen width for ILI9341
int prevCactusX = 320;
float gameSpeed = 4.0;
float score = 0;
int highScore = 0;
bool isJumping = false;
bool playing = false;
int cactusType = 0;

bool isNightMode = false;
int lastModeScore = 0;
uint16_t bgColor = TFT_WHITE;
uint16_t fgColor = TFT_BLACK;

float cloud1X = 50, cloud1Y = 40;
float cloud2X = 180, cloud2Y = 60;
int prevCloud1X, prevCloud2X;

// --- Moving Road Dots (adjusted for ILI9341)---
#define NUM_DOTS 30      
float dotsX[NUM_DOTS];       
int prevDotsX[NUM_DOTS];
int dotsY[NUM_DOTS];
float bumpsX[4];
int prevBumpsX[4];

void setup() {
  Serial.begin(115200);
  
  // Button pins
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1);  // Landscape orientation
  tft.fillScreen(TFT_WHITE);
  
  randomSeed(analogRead(0));
  
  // Initialize road dots
  for (int i = 0; i < NUM_DOTS; i++) {
    dotsX[i] = random(0, 320);
    dotsY[i] = random(215, 230);  // Ground level for ILI9341
  }
  
  // Initialize bumps
  for (int i = 0; i < 4; i++) {
    bumpsX[i] = 320 + (i * 200);
  }
  
  EEPROM.begin(512);
  EEPROM.get(0, highScore);
  if (highScore > 10000) highScore = 0;  // Sanity check
  
  showMenu();
}

void loop() {
  if (!playing) {
    if (digitalRead(BTN_START) == LOW || digitalRead(BTN_A) == LOW) {
      delay(50);  // Debounce
      startGame();
    }
    return;
  }

  // Jump on any button press
  if ((digitalRead(BTN_UP) == LOW || digitalRead(BTN_A) == LOW || 
       digitalRead(BTN_B) == LOW) && !isJumping) {
    isJumping = true;
    jumpVel = -10.6; 
  }

  prevDinoY = dinoY;
  dinoY += jumpVel;
  if (isJumping) jumpVel += gravity;
  if (dinoY >= 200) { dinoY = 200; isJumping = false; }

  prevCactusX = (int)cactusX;
  cactusX -= gameSpeed;
  gameSpeed += 0.0006; 
  score += 0.40;

  // Update road dots
  for (int i = 0; i < NUM_DOTS; i++) {
    prevDotsX[i] = (int)dotsX[i];
    dotsX[i] -= gameSpeed;
    if (dotsX[i] < -5) {
      dotsX[i] = 320 + random(0, 100);
      dotsY[i] = random(215, 230);
    }
  }

  // Update bumps
  for (int i = 0; i < 4; i++) {
    prevBumpsX[i] = (int)bumpsX[i];
    bumpsX[i] -= gameSpeed;
    if (bumpsX[i] < -50) bumpsX[i] = 320 + random(150, 500); 
  }

  // Toggle night mode
  if ((int)score > 0 && (int)score % 300 == 0 && (int)score != lastModeScore) {
    isNightMode = !isNightMode;
    lastModeScore = (int)score;
    bgColor = isNightMode ? TFT_BLACK : TFT_WHITE;
    fgColor = isNightMode ? TFT_WHITE : TFT_BLACK;
    tft.fillScreen(bgColor); 
  }

  // Update clouds
  prevCloud1X = (int)cloud1X; 
  prevCloud2X = (int)cloud2X;
  cloud1X -= 0.8; 
  cloud2X -= 0.5;
  
  // Cactus spawning logic
  if (cactusX < -50) { 
    int positions[] = {260, 290, 300, 320, 350, 380, 400};
    int basePos = positions[random(0, 7)]; 
    int s = random(-35, 50);
    
    int extraGap;
    if (random(0, 2) == 0) {
      extraGap = basePos; 
    } else {
      extraGap = 320 + s; 
    }
    cactusX = extraGap;
    cactusType = random(0, 3); 
  }
  
  if (cloud1X < -50) { cloud1X = 320; cloud1Y = random(30, 80); }
  if (cloud2X < -50) { cloud2X = 320; cloud2Y = random(30, 80); }

  // Collision detection (adjusted for ILI9341)
  int cW = (cactusType == 2) ? 24 : 12; 
  int cH = (cactusType == 1) ? 24 : 18; 
  if (cactusX < 50 && (cactusX + cW) > 30) {  // Dino position at x=30
    if (dinoY + 20 > (232 - cH)) {  // Ground level adjusted
      playing = false;
      if ((int)score > highScore) {
        highScore = (int)score;
        EEPROM.put(0, highScore);
        EEPROM.commit();
      }
      gameOver();
    }
  }

  drawFrame();
  delay(15); 
}

void startGame() {
  playing = true; 
  score = 0; 
  lastModeScore = 0;
  isNightMode = false; 
  bgColor = TFT_WHITE; 
  fgColor = TFT_BLACK;
  gameSpeed = 4.0; 
  cactusX = 320;
  cloud1X = 50; 
  cloud2X = 180;
  
  for (int i = 0; i < 4; i++) {
    bumpsX[i] = 320 + (i * 180);
  }
  
  for (int i = 0; i < NUM_DOTS; i++) {
    dotsX[i] = random(0, 320);
  }
  
  dinoY = 200;
  isJumping = false;
  jumpVel = 0;
  
  tft.fillScreen(bgColor);
}

void showMenu() {
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(90, 80); 
  tft.print("DINO RUN");
  tft.setTextSize(1);
  tft.setCursor(100, 130); 
  tft.print("Press START or A");
  tft.setCursor(85, 150); 
  tft.print("to Start Game");
  
  // Show high score
  tft.setTextSize(2);
  tft.setCursor(100, 190);
  tft.print("HI: ");
  tft.print(highScore);
}

void gameOver() {
  tft.setTextColor(TFT_RED, bgColor);
  tft.setTextSize(2);
  tft.setCursor(80, 80); 
  tft.print("GAME OVER");
  tft.setTextColor(fgColor);
  tft.setTextSize(2);
  tft.setCursor(90, 120); 
  tft.print("Score: "); 
  tft.print((int)score);
  tft.setCursor(70, 150); 
  tft.print("HI Score: "); 
  tft.print(highScore);
  tft.setTextSize(1);
  tft.setCursor(80, 190);
  tft.print("Press START to Play");
  
  delay(500);
}

// Draw ground bumps
void drawBigBump(int x, uint16_t color) {
  if (x < 0 || x > 320) return;
  tft.drawFastHLine(x + 4, 232, 4, color);
  tft.drawFastHLine(x + 2, 233, 8, color); 
  tft.drawFastHLine(x + 1, 234, 10, color); 
  tft.drawFastHLine(x, 235, 12, color);
}

void drawFrame() {
  // Erase old positions
  tft.fillRect(30, prevDinoY, 20, 25, bgColor); 
  tft.fillRect(prevCactusX, 170, 30, 70, bgColor); 
  tft.fillRect(prevCloud1X, (int)cloud1Y, 30, 15, bgColor); 
  tft.fillRect(prevCloud2X, (int)cloud2Y, 30, 15, bgColor); 
  
  // Erase old bumps
  for(int i = 0; i < 4; i++) {
    tft.fillRect(prevBumpsX[i], 232, 15, 8, bgColor);
  }
  
  // Erase old dots
  for(int i = 0; i < NUM_DOTS; i++) {
    tft.drawPixel(prevDotsX[i], dotsY[i], bgColor);
  }

  // Draw ground line
  tft.drawFastHLine(0, 235, 320, fgColor);

  // Draw road dots
  for (int i = 0; i < NUM_DOTS; i++) {
    int dx = (int)dotsX[i];
    if (dx > 0 && dx < 319) {
      tft.drawPixel(dx, dotsY[i], fgColor);
    }
  }

  // Draw clouds
  tft.drawBitmap((int)cloud1X, (int)cloud1Y, cloud_big, 24, 10, fgColor);
  tft.drawBitmap((int)cloud2X, (int)cloud2Y, cloud_big, 24, 10, fgColor);
  
  // Draw dino
  tft.drawBitmap(30, dinoY, dino_run1, 16, 20, fgColor);

  // Draw cactus
  if (cactusType == 0)      
    tft.drawBitmap((int)cactusX, 235 - 18, cactus_small, 12, 18, fgColor);
  else if (cactusType == 1) 
    tft.drawBitmap((int)cactusX, 235 - 24, cactus_large, 12, 24, fgColor);
  else                      
    tft.drawBitmap((int)cactusX, 235 - 20, cactus_double, 24, 20, fgColor);

  // Draw ground bumps
  for (int i = 0; i < 4; i++) {
    int bx = (int)bumpsX[i];
    if (bx > 0 && bx < 310) drawBigBump(bx, fgColor);
  }

  // Draw score and high score
  tft.fillRect(10, 10, 60, 15, bgColor);  
  tft.fillRect(250, 10, 60, 15, bgColor); 
  tft.setTextColor(fgColor, bgColor);
  tft.setTextSize(2);
  tft.setCursor(10, 10); 
  tft.print("HI "); 
  tft.print(highScore);
  tft.setCursor(250, 10); 
  tft.print((int)score);
  tft.setTextSize(1);
}
