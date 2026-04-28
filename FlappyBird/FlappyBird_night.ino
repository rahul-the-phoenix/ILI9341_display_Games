#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

// Button pins
#define BTN_UP     13
#define BTN_DOWN   12
#define BTN_LEFT   27
#define BTN_RIGHT  26
#define BTN_A      14
#define BTN_B      33
#define BTN_START  32
#define BTN_SELECT 25

// Game Colors
#define BG_COLOR     TFT_BLACK
#define OBJECT_COLOR TFT_WHITE
#define DETAIL_COLOR TFT_BLACK

// Bird Colors
#define BIRD_BODY_COLOR   TFT_YELLOW
#define BIRD_DETAIL_COLOR TFT_RED

// Background dots/stars
#define MAX_DOTS 30
struct Dot {
  float x;
  float y;
  float speed;
  int brightness;
};
Dot bgDots[MAX_DOTS];

// Bird Bitmap (24x18)
const unsigned char bird_body_bmp[] PROGMEM = {
  0x00, 0x7e, 0x00,  
  0x01, 0xff, 0x80,  
  0x03, 0xff, 0xc0,  
  0x07, 0xff, 0xe0,  
  0x0f, 0xff, 0xf0,  
  0x1f, 0xff, 0xf8,  
  0x3f, 0xff, 0xfc,  
  0x7f, 0xff, 0xfe,  
  0xff, 0xff, 0xff,  
  0xff, 0xff, 0xff,  
  0x7f, 0xff, 0xfe,  
  0x7f, 0xff, 0xfe,  
  0x3f, 0xff, 0xfc,  
  0x1f, 0xff, 0xf8,  
  0x0f, 0xff, 0xf0,  
  0x07, 0xff, 0xe0,  
  0x01, 0xff, 0x80,  
  0x00, 0x7e, 0x00   
};

const unsigned char bird_details_bmp[] PROGMEM = {
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x70, 0x00,  
  0x00, 0xf8, 0x00,  
  0x01, 0xf8, 0x00,  
  0x01, 0xf0, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x7e,  
  0x00, 0x01, 0xff,  
  0x00, 0x00, 0x7e,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00,  
  0x00, 0x00, 0x00   
};

// Game Logic
struct Pipe {
  float x;
  int gapY;
  int gapH;
  bool counted;
};

const int MAX_PIPES = 4;
Pipe pipes[MAX_PIPES];
const int PIPE_W = 28;
const int CAP_H = 8;
const float SCROLL_SPEED = 1.8;

const int MIN_PIPE_DISTANCE = 120;
const int MAX_PIPE_DISTANCE = 250;

float birdY = 120, prevBirdY = 120;
float velocity = 0, gravity = 0.18, flapPower = -2.5;
int score = 0, highScore = 0;
bool playing = false;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;
int screenWidth, screenHeight;

bool isAnyButtonPressed() {
  return (!digitalRead(BTN_UP) || !digitalRead(BTN_DOWN) || 
          !digitalRead(BTN_LEFT) || !digitalRead(BTN_RIGHT) ||
          !digitalRead(BTN_A) || !digitalRead(BTN_B) ||
          !digitalRead(BTN_START) || !digitalRead(BTN_SELECT));
}

void showStartScreen();
void startGame();
void updateScreen();
void gameOver();
void createPipe(int index, float startX);
int getRandomDistance();
void initDots();
void updateDots();

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
  screenWidth = tft.width();
  screenHeight = tft.height();
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(OBJECT_COLOR);
  
  randomSeed(analogRead(0));
  initDots();
  showStartScreen();
}

// Background dots initialization
void initDots() {
  for (int i = 0; i < MAX_DOTS; i++) {
    bgDots[i].x = random(0, screenWidth);
    bgDots[i].y = random(0, screenHeight);
    bgDots[i].speed = random(3, 12) / 10.0;  // 0.3 to 1.2 speed
    bgDots[i].brightness = random(40, 180);  // Different brightness levels
  }
}

// Update background dots
void updateDots() {
  for (int i = 0; i < MAX_DOTS; i++) {
    // Clear old dot position
    tft.drawPixel((int)bgDots[i].x, (int)bgDots[i].y, BG_COLOR);
    
    // Move dot left
    bgDots[i].x -= bgDots[i].speed;
    
    // Wrap around when off screen
    if (bgDots[i].x < 0) {
      bgDots[i].x = screenWidth;
      bgDots[i].y = random(0, screenHeight);
      bgDots[i].speed = random(3, 12) / 10.0;
      bgDots[i].brightness = random(40, 180);
    }
    
    // Draw new dot with grayscale color
    uint16_t dotColor = tft.color565(bgDots[i].brightness, bgDots[i].brightness, bgDots[i].brightness);
    tft.drawPixel((int)bgDots[i].x, (int)bgDots[i].y, dotColor);
  }
}

int getRandomDistance() {
  return random(MIN_PIPE_DISTANCE, MAX_PIPE_DISTANCE + 1);
}

void loop() {
  if (!playing) {
    // Update dots on start screen too
    updateDots();
    delay(30);
    
    if (isAnyButtonPressed() && (millis() - lastButtonPress > debounceDelay)) {
      lastButtonPress = millis();
      delay(100);
      startGame();
    }
    return;
  }

  if (isAnyButtonPressed() && (millis() - lastButtonPress > 50)) {
    lastButtonPress = millis();
    velocity = flapPower;
  }

  velocity += gravity;
  prevBirdY = birdY;
  birdY += velocity;

  for (int i = 0; i < MAX_PIPES; i++) {
    pipes[i].x -= SCROLL_SPEED;
    
    if (!pipes[i].counted && pipes[i].x + PIPE_W < 60) {
      pipes[i].counted = true;
      score += 5;
    }
    
    if (pipes[i].x < -PIPE_W - 20) {
      float farthestX = -999;
      for (int j = 0; j < MAX_PIPES; j++) {
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
        gameOver(); 
        return;
      }
    }
  }
  
  if (birdY < 0 || birdY > screenHeight - 18) { 
    gameOver(); 
    return; 
  }
  
  updateScreen();
  delay(12);
}

void createPipe(int index, float startX) {
  pipes[index].x = startX;
  pipes[index].gapY = random(50, screenHeight - 130);
  pipes[index].gapH = random(70, 110);
  pipes[index].counted = false;
}

void startGame() {
  tft.fillScreen(BG_COLOR);
  initDots();  // Reset dots
  score = 0; 
  birdY = screenHeight / 2;
  velocity = 0;
  
  float currentX = screenWidth + 50;
  for (int i = 0; i < MAX_PIPES; i++) {
    createPipe(i, currentX);
    currentX += getRandomDistance();
  }
  
  playing = true;
  lastButtonPress = millis();
}

void updateScreen() {
  // 1. Update background dots FIRST (for parallax effect)
  updateDots();
  
  // 2. Clear bird trail
  tft.fillRect(60, (int)prevBirdY, 28, 22, BG_COLOR);
  
  // 3. Update pipes
  for (int i = 0; i < MAX_PIPES; i++) {
    int oldPipeEnd = pipes[i].x + PIPE_W + 5;
    if (oldPipeEnd > 0 && oldPipeEnd < screenWidth) {
      tft.fillRect(oldPipeEnd, 0, 10, screenHeight, BG_COLOR);
    }
    
    if (pipes[i].x < screenWidth && pipes[i].x + PIPE_W > 0) {
      // Upper pipe
      tft.fillRect(pipes[i].x, 0, PIPE_W, pipes[i].gapY, OBJECT_COLOR);
      tft.fillRect(pipes[i].x - 3, pipes[i].gapY - CAP_H, PIPE_W + 6, CAP_H, OBJECT_COLOR);
      
      // Lower pipe
      int bottomY = pipes[i].gapY + pipes[i].gapH;
      tft.fillRect(pipes[i].x, bottomY, PIPE_W, screenHeight - bottomY, OBJECT_COLOR);
      tft.fillRect(pipes[i].x - 3, bottomY, PIPE_W + 6, CAP_H, OBJECT_COLOR);
    }
  }
  
  // 4. Draw bird
  tft.drawBitmap(60, (int)birdY, bird_body_bmp, 24, 18, BIRD_BODY_COLOR);
  tft.drawBitmap(60, (int)birdY, bird_details_bmp, 24, 18, BIRD_DETAIL_COLOR);
  
  // 5. Draw score
  tft.fillRect(screenWidth - 80, 5, 70, 20, BG_COLOR);
  tft.setTextColor(OBJECT_COLOR);
  tft.setTextSize(2);
  tft.setCursor(screenWidth - 75, 8);
  tft.print("SCORE:");
  tft.print(score);
  
  tft.setTextSize(1);
}

void showStartScreen() {
  tft.fillScreen(BG_COLOR);
  
  // Draw dots on start screen
  for (int i = 0; i < MAX_DOTS; i++) {
    uint16_t dotColor = tft.color565(bgDots[i].brightness, bgDots[i].brightness, bgDots[i].brightness);
    tft.drawPixel((int)bgDots[i].x, (int)bgDots[i].y, dotColor);
  }
  
  tft.setTextColor(OBJECT_COLOR);
  tft.setTextSize(3);
  tft.setCursor(35, 40);
  tft.print("FLAPPY");
  tft.setCursor(50, 75);
  tft.print("BIRD");
  
  tft.setTextSize(1);
  tft.setCursor(40, 120);
  tft.print("Press ANY Button");
  tft.setCursor(55, 135);
  tft.print("to Start");
  
  tft.setTextSize(1);
  tft.setCursor(15, 170);
  tft.print("Random Pipe Distance:");
  tft.setCursor(30, 185);
  tft.print("120 - 250 pixels");
  tft.setCursor(45, 210);
  tft.print("Good Luck!");
}

void gameOver() {
  playing = false;
  if (score > highScore) highScore = score;
  
  int centerX = screenWidth / 2;
  
  tft.fillRoundRect(centerX - 90, 50, 180, 100, 10, OBJECT_COLOR);
  tft.setTextColor(BG_COLOR);
  tft.setTextSize(2);
  tft.setCursor(centerX - 65, 68);
  tft.print("GAME OVER");
  
  tft.setTextSize(1);
  tft.setCursor(centerX - 55, 95);
  tft.print("S: ");
  tft.print(score);
  
  tft.setCursor(centerX - 55, 115);
  tft.print("BEST: ");
  tft.print(highScore);
  
  tft.setTextSize(1);
  tft.setCursor(centerX - 60, 140);
  tft.print("Press any key");
  
  delay(1500);
}
