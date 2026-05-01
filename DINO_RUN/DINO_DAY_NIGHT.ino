/*
 * =====================================================================
 *  DINO RUN  —  ILI9341 (TFT_eSPI) Version - 3X JUMP HEIGHT
 *  Fixed: Dino jumps 3x higher (123px instead of 41px)
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

// ============= BITMAPS =============
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

// ============= SCALE FACTOR =============
#define SC 2

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
#define DINO_SW  (DINO_W*SC)
#define DINO_SH  (DINO_H*SC)

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
const float GRAVITY = 1.35;        // Same gravity
float cactusX, prevCactusX;
float gameSpeed;
float score;
int   highScore = 0;
bool  isJumping, playing;
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

#define EEPROM_ADDR 20

// ============= HELPER FUNCTIONS =============
void drawBitmapScaled(int x, int y, const uint8_t* bmp,
                      int bmpW, int bmpH, uint16_t color, uint16_t bg,
                      bool transparent = true)
{
  int bytesPerRow = (bmpW + 7) / 8;
  for (int row = 0; row < bmpH; row++) {
    for (int col = 0; col < bmpW; col++) {
      int byteIndex = row * bytesPerRow + col / 8;
      uint8_t b = pgm_read_byte(&bmp[byteIndex]);
      bool set = (b >> (7 - (col % 8))) & 1;
      if (set) {
        tft.fillRect(x + col * SC, y + row * SC, SC, SC, color);
      } else if (!transparent) {
        tft.fillRect(x + col * SC, y + row * SC, SC, SC, bg);
      }
    }
  }
}

int cactusDrawY(int origH) { return GROUND_Y - origH * SC; }

void drawBigBump(int x, uint16_t color) {
  tft.fillRect(x,     GROUND_Y - 4,  16, 2, color);
  tft.fillRect(x - 2, GROUND_Y - 2,  20, 2, color);
}

void eraseDino() {
  tft.fillRect(DINO_X, prevDinoY, DINO_SW, DINO_SH, bgColor);
}

void eraseCactus() {
  tft.fillRect((int)prevCactusX, GROUND_Y - CD_H * SC, CD_W * SC + 4, CD_H * SC + 2, bgColor);
}

void eraseClouds() {
  tft.fillRect((int)prevCloud1X, (int)cloud1Y, CLOUD_W * SC, CLOUD_H * SC, bgColor);
  tft.fillRect((int)prevCloud2X, (int)cloud2Y, CLOUD_W * SC, CLOUD_H * SC, bgColor);
}

void eraseBumps() {
  for (int i = 0; i < 3; i++)
    tft.fillRect(prevBumpX[i] - 4, GROUND_Y - 5, 26, 6, bgColor);
}

void eraseDots() {
  for (int i = 0; i < NUM_DOTS; i++)
    tft.fillRect(prevDotX[i], dotY[i], 2, 2, bgColor);
}

void showDinoMenu() {
  bgColor = COL_WHITE; fgColor = COL_BLACK;
  tft.fillScreen(bgColor);

  tft.setTextColor(COL_BLACK);
  tft.setTextSize(4);
  tft.setCursor(60, 60);
  tft.print("DINO RUN");

  drawBitmapScaled(140, 110, dino_run1, DINO_W, DINO_H, COL_BLACK, bgColor);

  tft.setTextSize(2);
  tft.setTextColor(0x07FF);
  tft.setCursor(50, 155);
  tft.print("SELECT : Jump/Start");
  tft.setTextSize(1);
  tft.setTextColor(COL_GREY);
  tft.setCursor(95, 180);
  tft.print("B : Back to Menu");

  tft.setTextSize(2);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(100, 200);
  tft.print("HI: "); tft.print(highScore);
}

void startDinoGame() {
  playing      = true;
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

void dinoGameOver() {
  playing = false;
  if ((int)score > highScore) {
    highScore = (int)score;
    EEPROM.write(EEPROM_ADDR, highScore & 0xFF);
    EEPROM.write(EEPROM_ADDR + 1, (highScore >> 8) & 0xFF);
    EEPROM.commit();
  }

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
  tft.print("Score: "); tft.print((int)score);
  tft.setCursor(90, 148);
  tft.print("Best:  "); tft.print(highScore);

  tft.setTextSize(1);
  tft.setTextColor(0x07FF);
  tft.setCursor(80, 174);
  tft.print("SELECT: Play Again   B: Menu");
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

  drawBitmapScaled((int)cloud1X, (int)cloud1Y, cloud_big, CLOUD_W, CLOUD_H, fgColor, bgColor);
  drawBitmapScaled((int)cloud2X, (int)cloud2Y, cloud_big, CLOUD_W, CLOUD_H, fgColor, bgColor);

  const uint8_t* dinoFrame_bmp = (dinoFrame == 0) ? dino_run1 : dino_run2;
  drawBitmapScaled(DINO_X, dinoY, dinoFrame_bmp, DINO_W, DINO_H, fgColor, bgColor);

  if (cactusX < SCR_W + 10 && cactusX > -CD_W * SC) {
    int cx = (int)cactusX;
    if (cactusType == 0)
      drawBitmapScaled(cx, cactusDrawY(CS_H), cactus_small, CS_W, CS_H, fgColor, bgColor);
    else if (cactusType == 1)
      drawBitmapScaled(cx, cactusDrawY(CL_H), cactus_large, CL_W, CL_H, fgColor, bgColor);
    else
      drawBitmapScaled(cx, cactusDrawY(CD_H), cactus_double, CD_W, CD_H, fgColor, bgColor);
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
  int lo = EEPROM.read(EEPROM_ADDR);
  int hi = EEPROM.read(EEPROM_ADDR + 1);
  if (lo != 255 || hi != 255) highScore = lo | (hi << 8);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COL_WHITE);

  randomSeed(analogRead(34));

  playing = false;
  showDinoMenu();
}

void loop() {
  static unsigned long lastBPress = 0;
  if (digitalRead(BTN_B) == LOW && millis() - lastBPress > 300) {
    lastBPress = millis();
    if (!playing) {
      showDinoMenu();
      return;
    }
    playing = false;
    showDinoMenu();
    return;
  }

  if (!playing) {
    static unsigned long lastSelPress = 0;
    if (digitalRead(BTN_SELECT) == LOW && millis() - lastSelPress > 300) {
      lastSelPress = millis();
      startDinoGame();
    }
    return;
  }

  // ============= JUMP WITH 3X HIGHER HEIGHT =============
  static unsigned long lastSel = 0;
  if ((digitalRead(BTN_SELECT) == LOW || digitalRead(BTN_A) == LOW) && 
      !isJumping && millis() - lastSel > JUMP_DEBOUNCE) {
    lastSel   = millis();
    isJumping = true;
    jumpVel   = -18.2;     // 3X HIGHER JUMP! (was -10.5, now -18.2)
                          // Results in 123px jump height instead of 41px
  }

  prevDinoY = dinoY;
  if (isJumping) {
    jumpVel += GRAVITY;
    dinoY   += (int)jumpVel;
    
    // Optional: Add ceiling to prevent jumping off screen
    if (dinoY < 20) {  // Prevent going above screen top
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

  if (cactusX < -CD_W * SC - 10) {
    int positions[] = {SCR_W, SCR_W + 30, SCR_W + 60, SCR_W + 100, SCR_W + 150, SCR_W + 200};
    cactusX    = positions[random(0, 6)];
    cactusType = random(0, 3);
  }

  prevCloud1X = cloud1X; prevCloud2X = cloud2X;
  cloud1X -= 1.5; cloud2X -= 1.0;
  if (cloud1X < -CLOUD_W * SC) { cloud1X = SCR_W; cloud1Y = random(25, 70); }
  if (cloud2X < -CLOUD_W * SC) { cloud2X = SCR_W; cloud2Y = random(25, 70); }

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
  if      (cactusType == 0) { cW = CS_W * SC; cH = CS_H * SC; }
  else if (cactusType == 1) { cW = CL_W * SC; cH = CL_H * SC; }
  else                      { cW = CD_W * SC; cH = CD_H * SC; }

  int cactusTop = GROUND_Y - cH;
  int dinoLeft  = DINO_X + 4;
  int dinoRight = DINO_X + DINO_SW - 4;
  int dinoBottom= dinoY  + DINO_SH - 2;

  int cactLeft  = (int)cactusX + 2;
  int cactRight = (int)cactusX + cW - 2;

  if (cactRight > dinoLeft && cactLeft < dinoRight) {
    if (dinoBottom > cactusTop) {
      dinoGameOver();
      while (digitalRead(BTN_SELECT) == HIGH && digitalRead(BTN_B) == HIGH) delay(10);
      delay(200);
      if (digitalRead(BTN_B) == LOW) { showDinoMenu(); return; }
      startDinoGame();
      return;
    }
  }

  drawDinoFrame();
  delay(16);
}
