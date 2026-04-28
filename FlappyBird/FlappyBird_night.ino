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

// Bird Bitmap (24x18)
// Better Bird Shape (24x18)
// Bird Body (24x18 pixels) - Main silhouette
const unsigned char bird_body_bmp[] PROGMEM = {
  0x00, 0x7e, 0x00,  // Row 1:  -------●●●●●●-------
  0x01, 0xff, 0x80,  // Row 2:  ------●●●●●●●●------
  0x03, 0xff, 0xc0,  // Row 3:  -----●●●●●●●●●●-----
  0x07, 0xff, 0xe0,  // Row 4:  ----●●●●●●●●●●●●----
  0x0f, 0xff, 0xf0,  // Row 5:  ---●●●●●●●●●●●●●●---
  0x1f, 0xff, 0xf8,  // Row 6:  --●●●●●●●●●●●●●●●●--
  0x3f, 0xff, 0xfc,  // Row 7:  -●●●●●●●●●●●●●●●●●●-
  0x7f, 0xff, 0xfe,  // Row 8:  ●●●●●●●●●●●●●●●●●●●●
  0xff, 0xff, 0xff,  // Row 9:  ●●●●●●●●●●●●●●●●●●●●●●●●
  0xff, 0xff, 0xff,  // Row 10: ●●●●●●●●●●●●●●●●●●●●●●●●
  0x7f, 0xff, 0xfe,  // Row 11: ●●●●●●●●●●●●●●●●●●●●
  0x7f, 0xff, 0xfe,  // Row 12: ●●●●●●●●●●●●●●●●●●●●
  0x3f, 0xff, 0xfc,  // Row 13: -●●●●●●●●●●●●●●●●●●-
  0x1f, 0xff, 0xf8,  // Row 14: --●●●●●●●●●●●●●●●●--
  0x0f, 0xff, 0xf0,  // Row 15: ---●●●●●●●●●●●●●●---
  0x07, 0xff, 0xe0,  // Row 16: ----●●●●●●●●●●●●----
  0x01, 0xff, 0x80,  // Row 17: ------●●●●●●●●------
  0x00, 0x7e, 0x00   // Row 18: -------●●●●●●-------
};

// Bird Details (24x18 pixels) - Eye and wing details
const unsigned char bird_details_bmp[] PROGMEM = {
  0x00, 0x00, 0x00,  // Row 1:  ------------------------
  0x00, 0x00, 0x00,  // Row 2:  ------------------------
  0x00, 0x00, 0x00,  // Row 3:  ------------------------
  0x00, 0x70, 0x00,  // Row 4:  ---------●●●-----------
  0x00, 0xf8, 0x00,  // Row 5:  --------●●●●●----------
  0x01, 0xf8, 0x00,  // Row 6:  -------●●●●●●----------
  0x01, 0xf0, 0x00,  // Row 7:  -------●●●●●-----------
  0x00, 0x00, 0x00,  // Row 8:  ------------------------
  0x00, 0x00, 0x7e,  // Row 9:  -----------------●●●●●●●
  0x00, 0x01, 0xff,  // Row 10: ---------------●●●●●●●●●
  0x00, 0x00, 0x7e,  // Row 11: -----------------●●●●●●●
  0x00, 0x00, 0x00,  // Row 12: ------------------------
  0x00, 0x00, 0x00,  // Row 13: ------------------------
  0x00, 0x00, 0x00,  // Row 14: ------------------------
  0x00, 0x00, 0x00,  // Row 15: ------------------------
  0x00, 0x00, 0x00,  // Row 16: ------------------------
  0x00, 0x00, 0x00,  // Row 17: ------------------------
  0x00, 0x00, 0x00   // Row 18: ------------------------
};

// Game Logic
struct Pipe {
  float x;
  int gapY;
  int gapH;
  bool counted;
};

const int MAX_PIPES = 4;  // 4টি পাইপ একসাথে (র্যান্ডম দূরত্বের জন্য বেশি রাখা)
Pipe pipes[MAX_PIPES];
const int PIPE_W = 28;
const int CAP_H = 8;
const float SCROLL_SPEED = 1.8;

// র্যান্ডম দূরত্বের জন্য কনফিগারেশন
const int MIN_PIPE_DISTANCE = 120;  // সর্বনিম্ন 120 পিক্সেল
const int MAX_PIPE_DISTANCE = 250;  // সর্বোচ্চ 250 পিক্সেল

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
  showStartScreen();
}

// র্যান্ডম দূরত্ব তৈরি করার ফাংশন (120-250 পিক্সেল)
int getRandomDistance() {
  return random(MIN_PIPE_DISTANCE, MAX_PIPE_DISTANCE + 1);
}

void loop() {
  if (!playing) {
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

  // সব পাইপ একই স্পিডে স্ক্রল হবে
  for (int i = 0; i < MAX_PIPES; i++) {
    pipes[i].x -= SCROLL_SPEED;
    
    // স্কোর চেক - পাখি পাইপ পার হলে
    if (!pipes[i].counted && pipes[i].x + PIPE_W < 60) {
      pipes[i].counted = true;
      score += 5;
    }
    
    // পাইপ স্ক্রিন থেকে বেরিয়ে গেলে র্যান্ডম দূরত্বে নতুন পাইপ তৈরি
    if (pipes[i].x < -PIPE_W - 20) {
      float farthestX = -999;
      for (int j = 0; j < MAX_PIPES; j++) {
        if (pipes[j].x > farthestX) {
          farthestX = pipes[j].x;
        }
      }
      // র্যান্ডম দূরত্ব যোগ করা হচ্ছে (120-250 পিক্সেল)
      int randomDistance = getRandomDistance();
      createPipe(i, farthestX + randomDistance);
    }
    
    // কলিশন চেক
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
  
  // ডিবাগের জন্য সিরিয়াল প্রিন্ট (চাইলে কমেন্ট করে দিতে পারেন)
  // Serial.print("New pipe at X: ");
  // Serial.print(startX);
  // Serial.print(" Distance from previous: ");
  // Serial.println(startX - getPreviousPipeX(index));
}

void startGame() {
  tft.fillScreen(BG_COLOR);
  score = 0; 
  birdY = screenHeight / 2;
  velocity = 0;
  
  // শুরুতে 4টি পাইপ র্যান্ডম দূরত্বে তৈরি করা
  float currentX = screenWidth + 50;
  for (int i = 0; i < MAX_PIPES; i++) {
    createPipe(i, currentX);
    currentX += getRandomDistance();  // র্যান্ডম দূরত্ব যোগ করা
  }
  
  playing = true;
  lastButtonPress = millis();
}

void updateScreen() {
  // 1. পাখির ট্রেইল ক্লিয়ার
  tft.fillRect(60, (int)prevBirdY, 28, 22, BG_COLOR);
  
  // 2. সব পাইপ আপডেট করা - স্মুথ স্ক্রলিং
  for (int i = 0; i < MAX_PIPES; i++) {
    // পুরনো পাইপ মুছে ফেলা
    int oldPipeEnd = pipes[i].x + PIPE_W + 5;
    if (oldPipeEnd > 0 && oldPipeEnd < screenWidth) {
      tft.fillRect(oldPipeEnd, 0, 10, screenHeight, BG_COLOR);
    }
    
    // নতুন পাইপ আঁকা (শুধু স্ক্রিনের ভিতরে থাকলে)
    if (pipes[i].x < screenWidth && pipes[i].x + PIPE_W > 0) {
      // উপরের পাইপ
      tft.fillRect(pipes[i].x, 0, PIPE_W, pipes[i].gapY, OBJECT_COLOR);
      tft.fillRect(pipes[i].x - 3, pipes[i].gapY - CAP_H, PIPE_W + 6, CAP_H, OBJECT_COLOR);
      
      // নিচের পাইপ
      int bottomY = pipes[i].gapY + pipes[i].gapH;
      tft.fillRect(pipes[i].x, bottomY, PIPE_W, screenHeight - bottomY, OBJECT_COLOR);
      tft.fillRect(pipes[i].x - 3, bottomY, PIPE_W + 6, CAP_H, OBJECT_COLOR);
    }
  }
  
  // 3. পাখি ড্র করা
  tft.drawBitmap(60, (int)birdY, bird_body_bmp, 24, 18, BIRD_BODY_COLOR);
  tft.drawBitmap(60, (int)birdY, bird_details_bmp, 24, 18, BIRD_DETAIL_COLOR);
  
  // 4. স্কোর দেখানো
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
  tft.print("SCORE: ");
  tft.print(score);
  
  tft.setCursor(centerX - 55, 115);
  tft.print("BEST: ");
  tft.print(highScore);
  
  tft.setTextSize(1);
  tft.setCursor(centerX - 60, 140);
  tft.print("Press any key");
  
  delay(1500);
}
