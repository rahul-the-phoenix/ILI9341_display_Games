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
#define PIPE_COLOR   TFT_GREEN
#define PIPE_CAP_COLOR TFT_DARKGREEN
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
bool isPaused = false;
bool isCountdown = false;
unsigned long countdownStartTime = 0;
int countdownValue = 3;
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 200;
int screenWidth, screenHeight;

int animationFrame = 0;
unsigned long lastAnimationUpdate = 0;

// SELECT বাটন - ফ্ল্যাপ এবং রিজিউম করার জন্য
bool isSelectButtonPressed() {
  return (!digitalRead(BTN_SELECT));
}

// START বাটন - শুধুমাত্র পজ করার জন্য
bool isStartButtonPressed() {
  return (!digitalRead(BTN_START));
}

void showStartScreen();
void startGame();
void updateScreen();
void gameOver();
void createPipe(int index, float startX);
int getRandomDistance();
void initDots();
void updateDots();
void showPauseOverlay();
void hidePauseOverlay();
void startCountdown();
void updateCountdown();

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
  tft.setTextColor(PIPE_COLOR);
  
  randomSeed(analogRead(0));
  initDots();
  showStartScreen();
}

// Background dots initialization
void initDots() {
  for (int i = 0; i < MAX_DOTS; i++) {
    bgDots[i].x = random(0, screenWidth);
    bgDots[i].y = random(0, screenHeight);
    bgDots[i].speed = random(3, 12) / 10.0;
    bgDots[i].brightness = random(40, 180);
  }
}

// Update background dots
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

void showPauseOverlay() {
  int centerX = screenWidth / 2;
  
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(centerX - 55, screenHeight/2 - 25);
  tft.print("PAUSED!");
}

void hidePauseOverlay() {
  int centerX = screenWidth / 2;
  
  // Clear only the overlay area
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
    
    // Clear previous countdown number
   tft.fillRect(centerX - 40, centerY - 30, 40, 30, BG_COLOR);
    
    // Show current countdown number
    tft.setTextColor(TFT_YELLOW);
    tft.setTextSize(4);
    if (countdownValue > 0) {
      tft.setCursor(centerX - 12, centerY - 20);
      tft.print(countdownValue);
     }// else if (countdownValue == 0) {
    //   tft.setCursor(centerX - 28, centerY - 20);
    //   tft.print("GO!");
    // }
  }
  
  // Countdown finished
  if (elapsed >= 3000) {
    // Clear countdown display
    tft.fillRect(centerX - 40, centerY - 30, 80, 60, BG_COLOR);
    isCountdown = false;
    isPaused = false;
  }
}

int getRandomDistance() {
  return random(MIN_PIPE_DISTANCE, MAX_PIPE_DISTANCE + 1);
}

void loop() {
  if (!playing) {
    updateDots();
    
    if (millis() - lastAnimationUpdate > 500) {
      lastAnimationUpdate = millis();
      animationFrame = !animationFrame;
      
      tft.fillRect(40, 120, 180, 30, BG_COLOR);
      tft.setTextSize(1);
      tft.setTextColor(PIPE_COLOR);
      tft.setCursor(40, 120);
      if (animationFrame) {
        tft.print("> Press SELECT to Start <");
      } else {
        tft.print("  Press SELECT to Start  ");
      }
    }
    
    delay(30);
    
    // SELECT বাটন দিয়ে গেম শুরু
    if (isSelectButtonPressed() && (millis() - lastButtonPress > debounceDelay)) {
      lastButtonPress = millis();
      delay(100);
      startGame();
    }
    return;
  }

  // START বাটন দিয়ে পজ করা (শুধুমাত্র পজ)
  if (!isPaused && !isCountdown && isStartButtonPressed() && (millis() - lastButtonPress > debounceDelay)) {
    lastButtonPress = millis();
    isPaused = true;
    showPauseOverlay();
    delay(200);
    return;
  }
  
  // পজ মোডে SELECT বাটন দিয়ে কাউন্টডাউন শুরু
  if (isPaused && !isCountdown && isSelectButtonPressed() && (millis() - lastButtonPress > debounceDelay)) {
    lastButtonPress = millis();
    hidePauseOverlay();  // PAUSED লেখা মুছে ফেলুন
    startCountdown();    // কাউন্টডাউন শুরু করুন
    delay(200);
    return;
  }
  
  // কাউন্টডাউন আপডেট করুন
  if (isCountdown) {
    updateCountdown();
    delay(50);
    return;
  }
  
  // পজ মোডে থাকলে গেম আপডেট করবেন না
  if (isPaused) {
    delay(50);
    return;
  }

  // SELECT বাটন দিয়ে ফ্ল্যাপ করুন
  if (isSelectButtonPressed() && (millis() - lastButtonPress > 50)) {
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
  initDots();
  score = 0; 
  isPaused = false;
  isCountdown = false;
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
  updateDots();
  
  tft.fillRect(60, (int)prevBirdY, 28, 22, BG_COLOR);
  
  for (int i = 0; i < MAX_PIPES; i++) {
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
  tft.print(score);
  
  tft.setTextSize(1);
}

void showStartScreen() {
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
  
  tft.fillRoundRect(30, 110, 260, 105, 10, PIPE_CAP_COLOR);
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
  
  tft.setTextSize(1);
  tft.setTextColor(PIPE_COLOR);
  tft.setCursor(40, 215);
  tft.print("High Score: ");
  tft.print(highScore);
}

void gameOver() {
  playing = false;
  isPaused = false;
  isCountdown = false;
  if (score > highScore) highScore = score;
  
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
  tft.print(score);
  
  tft.setTextSize(1);
  tft.setCursor(centerX - 45, 130);
  tft.print("BEST: ");
  tft.print(highScore);
  
  tft.setTextSize(1);
  tft.setTextColor(BG_COLOR);
  tft.setCursor(centerX - 75, 162);
  tft.print("PRESS SELECT BUTTON");
  
  delay(500);
  while (!isSelectButtonPressed()) {
    delay(50);
  }
  delay(200);
  showStartScreen();
}
