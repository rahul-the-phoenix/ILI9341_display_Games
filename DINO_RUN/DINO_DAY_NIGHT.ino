/*
 * =====================================================================
 *  DINO RUN  —  ILI9341 (TFT_eSPI) Version - BIGGER CACTUS
 *  Features: 3x jump height, faster speed, bigger cactus!
 *  Added: Play/Pause with START button, Fixed High Score
 *  FIXED: High score properly saves/updates
 *  FIXED: Pause shows red text overlay on game
 * =====================================================================
 */
#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

// ============= BUTTON PINS =============
#define BTN_UP     13
#define BTN_DOWN   12
#define BTN_LEFT   27
#define BTN_RIGHT  26
#define BTN_A      14
#define BTN_B      33
#define BTN_START  32
#define BTN_SELECT 25

TFT_eSPI tft = TFT_eSPI();

// ============= SCREEN DIMENSIONS =============
#define SCR_W  320
#define SCR_H  240
#define GROUND_Y   210

// ============= BITMAPS (Original sizes kept) =============
const unsigned char dino_run1[] PROGMEM = {
  0x00,0x00,0x07,0xf0,0x0f,0xf8,0x0f,0xf8,0x0f,0xf8,0x0f,0x80,0x0f,0xf8,0x0e,0x00,
  0x0f,0xe0,0x0f,0xe0,0x9f,0xe0,0xdf,0xe0,0xff,0xe0,0xff,0xe0,0xff,0xc0,0xff,0x80,
  0x7f,0x00,0x3e,0x00,0x22,0x00,0x22,0x00,0x62,0x00
};

const unsigned char dino_run2[] PROGMEM = {
  0x00,0x00,0x07,0xf0,0x0f,0xf8,0x0f,0xf8,0x0f,0xf8,0x0f,0x80,0x0f,0xf8,0x0e,0x00,
  0x0f,0xe0,0x0f,0xe0,0x9f,0xe0,0xdf,0xe0,0xff,0xe0,0xff,0xe0,0xff,0xc0,0xff,0x80,
  0x7f,0x00,0x3e,0x00,0x3c,0x00,0x06,0x00,0x06,0x00
};

const unsigned char cactus_small[] PROGMEM = {
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0xdf,0x00, 0xdf,0x00, 0xdf,0x00, 0xdf,0x00, 0xff,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00
};

const unsigned char cactus_large[] PROGMEM = {
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0xdd,0x00, 0xdd,0x00, 0xdd,0x00, 0xdd,0x00, 0xdd,0x00,
  0xff,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00,
  0x1c,0x00, 0x1c,0x00, 0x1c,0x00, 0x1c,0x00
};

const unsigned char cactus_double[] PROGMEM = {
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00,
  0xdf,0xfb,0x00, 0xdf,0xfb,0x00, 0xdf,0xfb,0x00, 0xff,0xff,0x00, 0x1c,0x38,0x00,
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00,
  0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00, 0x1c,0x38,0x00
};

const unsigned char cloud_big[] PROGMEM = {
  0x00,0xfe,0x00, 0x07,0xff,0x80, 0x1f,0xff,0xe0, 0x3f,0xff,0xf0,
  0x7f,0xff,0xf8, 0x7f,0xff,0xf8, 0x7f,0xff,0xf8, 0x3f,0xff,0xf0,
  0x1f,0xff,0xe0, 0x00,0x00,0x00
};

// ============= SCALE FACTORS =============
#define DINO_SCALE 2
#define CACTUS_SCALE 3

// Original bitmap sizes
#define DINO_W   16
#define DINO_H   21
#define CS_W     12
#define CS_H     18
#define CL_W     12
#define CL_H     24
#define CD_W     24
#define CD_H     19
#define CLOUD_W  24
#define CLOUD_H  10

// Scaled sizes
#define DINO_SW  (DINO_W * DINO_SCALE)
#define DINO_SH  (DINO_H * DINO_SCALE)

#define CACTUS_SMALL_W  (CS_W * CACTUS_SCALE)
#define CACTUS_SMALL_H  (CS_H * CACTUS_SCALE)
#define CACTUS_LARGE_W  (CL_W * CACTUS_SCALE)
#define CACTUS_LARGE_H  (CL_H * CACTUS_SCALE)
#define CACTUS_DOUBLE_W (CD_W * CACTUS_SCALE)
#define CACTUS_DOUBLE_H (CD_H * CACTUS_SCALE)

#define DINO_X   40

// ============= COLORS =============
#define COL_WHITE  0xFFFF
#define COL_BLACK  0x0000
#define COL_RED    0xF800
#define COL_GREEN  0x07E0
#define COL_YELLOW 0xFFE0
#define COL_GREY   0x8410
#define COL_DKGREY 0x4208
#define COL_STAR   0xC618
#define COL_NIGHT_BG 0x000F

// ============= GAME VARIABLES =============
int   dinoY, prevDinoY;
float jumpVel;
const float GRAVITY = 1.35;
float cactusX, prevCactusX;
float gameSpeed;
float score;
int   highScore = 0;
bool  isJumping, playing;
bool  isPaused = false;
int   cactusType;

bool     isNightMode;
int      lastModeScore;
uint16_t bgColor, fgColor;

float cloud1X, cloud1Y, cloud2X, cloud2Y;
float prevCloud1X, prevCloud2X;

#define NUM_DOTS 25
float dotX[NUM_DOTS];
int   prevDotX[NUM_DOTS];
int   dotY[NUM_DOTS];

float bumpX[3];
int   prevBumpX[3];

int   dinoFrame = 0;
unsigned long lastFrameTime = 0;

unsigned long lastJumpTime = 0;
const unsigned long JUMP_DEBOUNCE = 80;

// Save game state for pause
float savedGameSpeed;
float savedCactusX;
float savedScore;
float savedCloud1X, savedCloud2X;
float savedDotX[NUM_DOTS];
float savedBumpX[3];
int   savedCactusType;

// EEPROM addresses
#define EEPROM_ADDR_HIGH 0
#define EEPROM_MAGIC_BYTE 2  // New: Magic byte to check if EEPROM is initialized

// ============= HELPER FUNCTIONS =============
void drawBitmapScaled(int x, int y, const uint8_t* bmp,
                      int bmpW, int bmpH, uint16_t color, uint16_t bg,
                      int scale, bool transparent = true)
{
  int bytesPerRow = (bmpW + 7) / 8;
  for (int row = 0; row < bmpH; row++) {
    for (int col = 0; col < bmpW; col++) {
      int byteIndex = row * bytesPerRow + col / 8;
      uint8_t b = pgm_read_byte(&bmp[byteIndex]);
      bool set = (b >> (7 - (col % 8))) & 1;
      if (set) {
        tft.fillRect(x + col * scale, y + row * scale, scale, scale, color);
      } else if (!transparent) {
        tft.fillRect(x + col * scale, y + row * scale, scale, scale, bg);
      }
    }
  }
}

void drawDino(int x, int y, const uint8_t* bmp, uint16_t color, uint16_t bg) {
  drawBitmapScaled(x, y, bmp, DINO_W, DINO_H, color, bg, DINO_SCALE);
}

void drawCactus(int x, int y, const uint8_t* bmp, int w, int h, uint16_t color, uint16_t bg) {
  drawBitmapScaled(x, y, bmp, w, h, color, bg, CACTUS_SCALE);
}

void drawCloud(int x, int y, uint16_t color, uint16_t bg) {
  drawBitmapScaled(x, y, cloud_big, CLOUD_W, CLOUD_H, color, bg, DINO_SCALE);
}

int cactusDrawY(int origH, int scale) { 
  return GROUND_Y - (origH * scale); 
}

void drawBigBump(int x, uint16_t color) {
  tft.fillRect(x,     GROUND_Y - 4,  16, 2, color);
  tft.fillRect(x - 2, GROUND_Y - 2,  20, 2, color);
}

void showPauseOverlay() {
  // Draw transparent red overlay with PAUSE text
  tft.fillRect(0, 80, SCR_W, 50, COL_RED);
  tft.setTextColor(COL_WHITE);
  tft.setTextSize(3);
  tft.setCursor(110, 92);
  tft.print("PAUSED");
  tft.setTextSize(1);
  tft.setCursor(100, 115);
  tft.print("Press START to Resume");
}

void erasePauseOverlay() {
  // Restore the game area that was covered by pause overlay
  tft.fillRect(0, 80, SCR_W, 50, bgColor);
  // Redraw the score area that might have been affected
  tft.setTextSize(2);
  tft.setTextColor(fgColor);
  tft.setCursor(10, 5);
  tft.print("HI:"); tft.print(highScore);
  tft.setCursor(240, 5);
  tft.print((int)score);
}

void eraseDino() {
  tft.fillRect(DINO_X, prevDinoY, DINO_SW, DINO_SH, bgColor);
}

void eraseCactus() {
  tft.fillRect((int)prevCactusX, GROUND_Y - CACTUS_LARGE_H, 
               CACTUS_DOUBLE_W + 10, CACTUS_LARGE_H + 5, bgColor);
}

void eraseClouds() {
  tft.fillRect((int)prevCloud1X, (int)cloud1Y, CLOUD_W * DINO_SCALE, 
               CLOUD_H * DINO_SCALE, bgColor);
  tft.fillRect((int)prevCloud2X, (int)cloud2Y, CLOUD_W * DINO_SCALE, 
               CLOUD_H * DINO_SCALE, bgColor);
}

void eraseBumps() {
  for (int i = 0; i < 3; i++)
    tft.fillRect(prevBumpX[i] - 4, GROUND_Y - 5, 26, 6, bgColor);
}

void eraseDots() {
  for (int i = 0; i < NUM_DOTS; i++)
    tft.fillRect(prevDotX[i], dotY[i], 2, 2, bgColor);
}

void saveGameState() {
  savedGameSpeed = gameSpeed;
  savedCactusX = cactusX;
  savedScore = score;
  savedCloud1X = cloud1X;
  savedCloud2X = cloud2X;
  savedCactusType = cactusType;
  
  for (int i = 0; i < NUM_DOTS; i++) {
    savedDotX[i] = dotX[i];
  }
  
  for (int i = 0; i < 3; i++) {
    savedBumpX[i] = bumpX[i];
  }
}

void restoreGameState() {
  gameSpeed = savedGameSpeed;
  cactusX = savedCactusX;
  score = savedScore;
  cloud1X = savedCloud1X;
  cloud2X = savedCloud2X;
  cactusType = savedCactusType;
  
  for (int i = 0; i < NUM_DOTS; i++) {
    dotX[i] = savedDotX[i];
    prevDotX[i] = (int)savedDotX[i];
  }
  
  for (int i = 0; i < 3; i++) {
    bumpX[i] = savedBumpX[i];
    prevBumpX[i] = (int)savedBumpX[i];
  }
}

void showDinoMenu() {
  bgColor = COL_WHITE; fgColor = COL_BLACK;
  tft.fillScreen(bgColor);

  tft.setTextColor(COL_BLACK);
  tft.setTextSize(4);
  tft.setCursor(60, 60);
  tft.print("DINO RUN");

  drawDino(140, 110, dino_run1, COL_BLACK, bgColor);

  tft.setTextSize(2);
  tft.setTextColor(0x07FF);
  tft.setCursor(60, 155);
  tft.print("SELECT: Jump/Start");

  tft.setTextSize(2);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(100, 200);
  tft.print("HI: "); tft.print(highScore);
}

void startDinoGame() {
  playing      = true;
  isPaused     = false;
  score        = 0;
  lastModeScore= 0;
  isNightMode  = false;
  bgColor      = COL_WHITE;
  fgColor      = COL_BLACK;
  
  gameSpeed    = 6.5;
  cactusX      = SCR_W;
  dinoY        = GROUND_Y - DINO_SH;
  prevDinoY    = dinoY;
  jumpVel      = 0;
  isJumping    = false;
  dinoFrame    = 0;
  cactusType   = random(0, 3);

  cloud1X = 100; cloud1Y = 40;
  cloud2X = 260; cloud2Y = 70;
  prevCloud1X = cloud1X; prevCloud2X = cloud2X;

  for (int i = 0; i < 3; i++) {
    bumpX[i] = SCR_W + i * 200.0;
    prevBumpX[i] = (int)bumpX[i];
  }
  for (int i = 0; i < NUM_DOTS; i++) {
    dotX[i]    = random(0, SCR_W);
    dotY[i]    = random(GROUND_Y + 2, GROUND_Y + 12);
    prevDotX[i]= (int)dotX[i];
  }

  tft.fillScreen(bgColor);
  tft.drawFastHLine(0, GROUND_Y, SCR_W, fgColor);
}

void saveHighScore() {
  // Use magic byte to ensure EEPROM is properly initialized
  EEPROM.write(EEPROM_MAGIC_BYTE, 0xAA);  // Magic byte to mark valid data
  EEPROM.write(EEPROM_ADDR_HIGH, highScore & 0xFF);
  EEPROM.write(EEPROM_ADDR_HIGH + 1, (highScore >> 8) & 0xFF);
  EEPROM.commit();
}

void loadHighScore() {
  // Check magic byte first
  uint8_t magic = EEPROM.read(EEPROM_MAGIC_BYTE);
  
  if (magic == 0xAA) {
    // Valid data exists
    int lo = EEPROM.read(EEPROM_ADDR_HIGH);
    int hi = EEPROM.read(EEPROM_ADDR_HIGH + 1);
    highScore = lo | (hi << 8);
    
    // Sanity check - if highScore is unrealistic, reset to 0
    if (highScore > 9999 || highScore < 0) {
      highScore = 0;
      saveHighScore();  // Save the reset value
    }
  } else {
    // First run or corrupted EEPROM - initialize with 0
    highScore = 0;
    saveHighScore();  // Write initial values
  }
}

void dinoGameOver() {
  playing = false;
  isPaused = false;
  
  int currentScore = (int)score;
  
  // Update high score if current score is higher
  if (currentScore > highScore) {
    highScore = currentScore;
    saveHighScore();  // Save immediately
  }

  // Game over animation
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(COL_RED); delay(60);
    tft.fillScreen(bgColor); delay(60);
  }

  tft.fillRoundRect(60, 70, 200, 110, 12, COL_WHITE);
  tft.drawRoundRect(60, 70, 200, 110, 12, COL_BLACK);
  tft.drawRoundRect(62, 72, 196, 106, 10, COL_RED);

  tft.setTextColor(COL_RED);
  tft.setTextSize(3);
  tft.setCursor(75, 88);
  tft.print("GAME OVER");

  tft.drawFastHLine(80, 118, 160, COL_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(90, 126);
  tft.print("Score: "); tft.print(currentScore);
  tft.setCursor(90, 148);
  tft.print("Best:  "); tft.print(highScore);

  tft.setTextSize(2);
  tft.setTextColor(0x07FF);
  tft.setCursor(60, 174);
  tft.print("SELECT: Play Again");
}

void drawDinoFrame() {
  eraseDino();
  eraseCactus();
  eraseClouds();
  eraseBumps();
  eraseDots();
  tft.fillRect(0, 0, SCR_W, 22, bgColor);

  tft.drawFastHLine(0, GROUND_Y, SCR_W, fgColor);

  for (int i = 0; i < NUM_DOTS; i++) {
    int dx = (int)dotX[i];
    if (dx > 0 && dx < SCR_W - 2)
      tft.fillRect(dx, dotY[i], 2, 2, fgColor);
  }

  for (int i = 0; i < 3; i++) {
    int bx = (int)bumpX[i];
    if (bx > 0 && bx < SCR_W - 20)
      drawBigBump(bx, fgColor);
  }

  drawCloud((int)cloud1X, (int)cloud1Y, fgColor, bgColor);
  drawCloud((int)cloud2X, (int)cloud2Y, fgColor, bgColor);

  const uint8_t* dinoFrame_bmp = (dinoFrame == 0) ? dino_run1 : dino_run2;
  drawDino(DINO_X, dinoY, dinoFrame_bmp, fgColor, bgColor);

  if (cactusX < SCR_W + 50 && cactusX > -CACTUS_DOUBLE_W) {
    int cx = (int)cactusX;
    if (cactusType == 0) {
      drawCactus(cx, cactusDrawY(CS_H, CACTUS_SCALE), cactus_small, CS_W, CS_H, fgColor, bgColor);
    }
    else if (cactusType == 1) {
      drawCactus(cx, cactusDrawY(CL_H, CACTUS_SCALE), cactus_large, CL_W, CL_H, fgColor, bgColor);
    }
    else {
      drawCactus(cx, cactusDrawY(CD_H, CACTUS_SCALE), cactus_double, CD_W, CD_H, fgColor, bgColor);
    }
  }

  tft.setTextSize(2);
  tft.setTextColor(fgColor);
  tft.setCursor(10, 5);
  tft.print("HI:"); tft.print(highScore);
  tft.setCursor(240, 5);
  tft.print((int)score);

  if (isNightMode) {
    tft.fillCircle(295, 35, 14, COL_YELLOW);
    tft.fillCircle(302, 29, 11, COL_NIGHT_BG);
    tft.drawPixel(50,  20, COL_STAR);
    tft.drawPixel(120, 15, COL_STAR);
    tft.drawPixel(200, 28, COL_STAR);
    tft.drawPixel(80,  40, COL_STAR);
    tft.drawPixel(260, 18, COL_STAR);
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A,     INPUT_PULLUP);
  pinMode(BTN_B,     INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT,INPUT_PULLUP);

  EEPROM.begin(512);
  
  // Load high score with magic byte check
  loadHighScore();

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COL_WHITE);

  randomSeed(analogRead(34));

  playing = false;
  isPaused = false;
  showDinoMenu();
}

void loop() {
  static unsigned long lastStartPress = 0;
  static unsigned long lastSelectPress = 0;
  static unsigned long pauseOverlayTime = 0;
  static bool pauseOverlayShown = false;
  
  // Handle START button (Pause/Resume)
  if (playing && digitalRead(BTN_START) == LOW && millis() - lastStartPress > 300) {
    lastStartPress = millis();
    
    if (!isPaused) {
      // Pause the game
      isPaused = true;
      saveGameState();
      showPauseOverlay();  // Show overlay on top of game
      pauseOverlayShown = true;
    } else {
      // Resume the game
      isPaused = false;
      erasePauseOverlay();  // Remove the overlay
      pauseOverlayShown = false;
      restoreGameState();
      // Force redraw of the game area
      drawDinoFrame();
    }
    delay(100);
    return;
  }

  // If paused, just wait (don't update game logic)
  if (isPaused) {
    delay(50);
    return;
  }

  // Handle menu and game start
  if (!playing) {
    if (digitalRead(BTN_SELECT) == LOW && millis() - lastSelectPress > 300) {
      lastSelectPress = millis();
      startDinoGame();
    }
    return;
  }

  // Jump with 3X higher height
  static unsigned long lastSel = 0;
  if ((digitalRead(BTN_SELECT) == LOW || digitalRead(BTN_A) == LOW) && 
      !isJumping && millis() - lastSel > JUMP_DEBOUNCE && !isPaused) {
    lastSel   = millis();
    isJumping = true;
    jumpVel   = -18.2;
  }

  prevDinoY = dinoY;
  if (isJumping) {
    jumpVel += GRAVITY;
    dinoY   += (int)jumpVel;
    
    if (dinoY < 20) {
      dinoY = 20;
      jumpVel = 0;
      isJumping = false;
    }
    
    if (dinoY >= GROUND_Y - DINO_SH) {
      dinoY     = GROUND_Y - DINO_SH;
      isJumping = false;
      jumpVel   = 0;
    }
  }

  prevCactusX = cactusX;
  cactusX    -= gameSpeed;
  
  gameSpeed  += 0.0025;
  if (gameSpeed > 22.0) gameSpeed = 22.0;
  score      += 0.45;

  if (cactusX < -CACTUS_DOUBLE_W - 20) {
    int positions[] = {SCR_W, SCR_W + 50, SCR_W + 100, SCR_W + 150, SCR_W + 200, SCR_W + 250};
    cactusX    = positions[random(0, 6)];
    cactusType = random(0, 3);
  }

  prevCloud1X = cloud1X; prevCloud2X = cloud2X;
  cloud1X -= 1.5; cloud2X -= 1.0;
  if (cloud1X < -CLOUD_W * DINO_SCALE) { cloud1X = SCR_W; cloud1Y = random(25, 70); }
  if (cloud2X < -CLOUD_W * DINO_SCALE) { cloud2X = SCR_W; cloud2Y = random(25, 70); }

  for (int i = 0; i < NUM_DOTS; i++) {
    prevDotX[i] = (int)dotX[i];
    dotX[i]    -= gameSpeed;
    if (dotX[i] < -5) {
      dotX[i] = SCR_W + random(0, 80);
      dotY[i] = random(GROUND_Y + 2, GROUND_Y + 12);
    }
  }
  for (int i = 0; i < 3; i++) {
    prevBumpX[i] = (int)bumpX[i];
    bumpX[i]    -= gameSpeed;
    if (bumpX[i] < -30) bumpX[i] = SCR_W + random(100, 400);
  }

  if ((int)score > 0 && (int)score % 200 == 0 && (int)score != lastModeScore) {
    isNightMode   = !isNightMode;
    lastModeScore = (int)score;
    bgColor = isNightMode ? COL_NIGHT_BG : COL_WHITE;
    fgColor = isNightMode ? COL_WHITE     : COL_BLACK;
    tft.fillScreen(bgColor);
    tft.drawFastHLine(0, GROUND_Y, SCR_W, fgColor);
  }

  if (millis() - lastFrameTime > 100) {
    lastFrameTime = millis();
    dinoFrame     = 1 - dinoFrame;
  }

  // Collision detection
  int cW, cH;
  if      (cactusType == 0) { cW = CACTUS_SMALL_W; cH = CACTUS_SMALL_H; }
  else if (cactusType == 1) { cW = CACTUS_LARGE_W; cH = CACTUS_LARGE_H; }
  else                      { cW = CACTUS_DOUBLE_W; cH = CACTUS_DOUBLE_H; }

  int cactusTop = GROUND_Y - cH;
  int dinoLeft  = DINO_X + 4;
  int dinoRight = DINO_X + DINO_SW - 4;
  int dinoBottom= dinoY  + DINO_SH - 2;

  int cactLeft  = (int)cactusX + 4;
  int cactRight = (int)cactusX + cW - 4;

  if (cactRight > dinoLeft && cactLeft < dinoRight) {
    if (dinoBottom > cactusTop) {
      dinoGameOver();
      while (digitalRead(BTN_SELECT) == HIGH) delay(10);
      delay(200);
      startDinoGame();
      return;
    }
  }

  drawDinoFrame();
  delay(16);
}
