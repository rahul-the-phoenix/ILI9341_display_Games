#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// --- Pin Configuration ---
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_SELECT   25

// --- Display Dimensions for ILI9341 (Full Screen) ---
#define TFT_WIDTH    320
#define TFT_HEIGHT   240

// --- Game Area - পুরো স্ক্রীন ব্যবহার করে ---
#define TOP_BOUNDARY    25    // Red line থেকে শুরু (লাল লাইন পার হবে না)
#define BOTTOM_BOUNDARY 232
#define LEFT_BOUNDARY   8
#define RIGHT_BOUNDARY  312

// --- Colors ---
#define COLOR_BG     TFT_BLACK
#define COLOR_BOT    TFT_RED
#define COLOR_YOU    TFT_CYAN
#define COLOR_BALL   TFT_YELLOW  // বল এখন হলুদ হবে, বেশি দেখা যাবে
#define COLOR_GRID   0x4208
#define COLOR_SNOW   0x7BEF

// --- Snow Effect ---
struct Snow { 
  float x, y, speed; 
};
Snow snowflakes[40];

// --- Game Variables ---
float ballX, ballY, ballDX, ballDY;
float botY, youY;
unsigned long currentScore = 0;
unsigned long highScore = 0;
unsigned long lastScoreUpdate = 0;
unsigned long lastSnowUpdate = 0;

// --- Game Constants ---
const int PADDLE_WIDTH = 5;
const int PADDLE_HEIGHT = 45;
const int BALL_SIZE = 6;  // বলের সাইজ বড় করা হলো (আগে ছিল 4)
float ballSpeed = 1.6;
float botSpeed = 1.4;
bool gameActive = false;
bool gamePaused = false;

void setup() {
  Serial.begin(115200);
  
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1); // Landscape: 320x240
  tft.fillScreen(TFT_BLACK);
  
  // Initialize starting positions
  botY = TOP_BOUNDARY + (BOTTOM_BOUNDARY - TOP_BOUNDARY - PADDLE_HEIGHT) / 2;
  youY = botY;
  
  // Initialize snowflakes
  for(int i = 0; i < 40; i++) {
    snowflakes[i].x = random(0, TFT_WIDTH);
    snowflakes[i].y = random(0, TFT_HEIGHT);
    snowflakes[i].speed = random(1, 6) / 10.0;
  }
  
  showStartScreen();
}

void drawSnow() {
  if(millis() - lastSnowUpdate < 40) return;
  lastSnowUpdate = millis();
  
  for(int i = 0; i < 40; i++) {
    tft.drawPixel((int)snowflakes[i].x, (int)snowflakes[i].y, COLOR_BG);
    
    snowflakes[i].y += snowflakes[i].speed;
    if(snowflakes[i].y >= TFT_HEIGHT) {
      snowflakes[i].y = 0;
      snowflakes[i].x = random(0, TFT_WIDTH);
    }
    
    tft.drawPixel((int)snowflakes[i].x, (int)snowflakes[i].y, COLOR_SNOW);
  }
}

void updateScoreUI() {
  // Clear top area
  tft.fillRect(0, 0, TFT_WIDTH, 22, COLOR_BG);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_ORANGE);
  tft.setCursor(10, 2);
  tft.print("HI:");
  tft.print(highScore);
  
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(TFT_WIDTH - 120, 2);
  tft.print("SCORE:");
  tft.print(currentScore);
  
  // Red boundary line - বল এই লাইন পার হবে না
  tft.drawFastHLine(0, 23, TFT_WIDTH, TFT_RED);
  tft.drawFastHLine(0, 24, TFT_WIDTH, TFT_RED);
}

void drawGameElements() {
  // Outer boundary (পুরো গেম এরিয়া)
  tft.drawRect(LEFT_BOUNDARY-2, TOP_BOUNDARY-2, 
               RIGHT_BOUNDARY - LEFT_BOUNDARY + 4, 
               BOTTOM_BOUNDARY - TOP_BOUNDARY + 4, TFT_WHITE);
  
  // Center dashed line
  for(int y = TOP_BOUNDARY; y < BOTTOM_BOUNDARY; y += 15) {
    tft.fillRect(TFT_WIDTH/2 - 2, y, 4, 8, COLOR_GRID);
  }
  
  // Draw paddles
  tft.fillRect(LEFT_BOUNDARY, (int)botY, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_BOT);
  tft.fillRect(RIGHT_BOUNDARY - PADDLE_WIDTH, (int)youY, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_YOU);
  
  // Draw ball (বড় সাইজ)
  tft.fillCircle((int)ballX, (int)ballY, BALL_SIZE, COLOR_BALL);
  // বলের ভিতরে একটু ইফেক্ট
  tft.fillCircle((int)ballX, (int)ballY, BALL_SIZE-2, TFT_WHITE);
  
  // Net effect
  tft.fillRect(TFT_WIDTH/2 - 1, TOP_BOUNDARY, 2, BOTTOM_BOUNDARY - TOP_BOUNDARY, 0x39E7);
}

void resetBall() {
  ballX = TFT_WIDTH / 2;
  ballY = (TOP_BOUNDARY + BOTTOM_BOUNDARY) / 2;
  ballDX = (random(0, 2) == 0) ? ballSpeed : -ballSpeed;
  ballDY = ((random(-12, 13)) / 10.0);
  
  if(abs(ballDY) < 0.4) ballDY = (ballDY > 0) ? 0.4 : -0.4;
  
  currentScore = 0;
  botY = TOP_BOUNDARY + (BOTTOM_BOUNDARY - TOP_BOUNDARY - PADDLE_HEIGHT) / 2;
  youY = botY;
}

void gameOver() {
  gameActive = false;
  if(currentScore > highScore) highScore = currentScore;
  
  tft.fillScreen(TFT_BLACK);
  
  int panelW = 260;
  int panelH = 130;
  int panelX = (TFT_WIDTH - panelW) / 2;
  int panelY = (TFT_HEIGHT - panelH) / 2;
  
  tft.fillRoundRect(panelX, panelY, panelW, panelH, 15, TFT_WHITE);
  tft.fillRoundRect(panelX + 4, panelY + 4, panelW - 8, panelH - 8, 12, COLOR_BG);
  
  tft.setTextSize(3);
  tft.setTextColor(COLOR_BOT);
  tft.setCursor(panelX + 45, panelY + 35);
  tft.print("GAME OVER");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(panelX + 50, panelY + 70);
  tft.print("Score: ");
  tft.print(currentScore);
  
  tft.setCursor(panelX + 50, panelY + 95);
  tft.print("Best: ");
  tft.print(highScore);
  
  tft.setTextColor(TFT_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(panelX + 50, panelY + 118);
  tft.print("PRESS SELECT TO PLAY");
}

void showStartScreen() {
  tft.fillScreen(TFT_BLACK);
  
  tft.setTextSize(5);
  tft.setTextColor(COLOR_YOU);
  tft.setCursor(TFT_WIDTH/2 - 90, TFT_HEIGHT/2 - 40);
  tft.print("DINO");
  
  tft.setTextSize(5);
  tft.setCursor(TFT_WIDTH/2 - 85, TFT_HEIGHT/2 + 10);
  tft.print("PONG");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(TFT_WIDTH/2 - 80, TFT_HEIGHT/2 + 60);
  tft.print("Press SELECT");
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(TFT_WIDTH/2 - 65, TFT_HEIGHT - 30);
  tft.print("Survive to Score!");
  
  for(int i = 0; i < 120; i++) {
    drawSnow();
    delay(15);
  }
}

void loop() {
  if(digitalRead(BTN_SELECT) == LOW) {
    delay(60);
    if(digitalRead(BTN_SELECT) == LOW) {
      if(!gameActive) {
        gameActive = true;
        gamePaused = false;
        resetBall();
        tft.fillScreen(COLOR_BG);
        updateScoreUI();
        drawGameElements();
      } else {
        gamePaused = !gamePaused;
        if(gamePaused) {
          tft.setTextSize(3);
          tft.setTextColor(TFT_YELLOW);
          tft.setCursor(TFT_WIDTH/2 - 50, TFT_HEIGHT/2 - 15);
          tft.print("PAUSED");
        } else {
          tft.fillRect(TFT_WIDTH/2 - 60, TFT_HEIGHT/2 - 25, 120, 50, COLOR_BG);
        }
      }
      while(digitalRead(BTN_SELECT) == LOW) delay(10);
    }
  }
  
  if(!gameActive || gamePaused) return;
  
  // Update score
  if(millis() - lastScoreUpdate > 100) {
    currentScore++;
    lastScoreUpdate = millis();
    updateScoreUI();
  }
  
  // Save old positions
  int oldBallX = (int)ballX;
  int oldBallY = (int)ballY;
  int oldBotY = (int)botY;
  int oldYouY = (int)youY;
  
  // Player input
  if(digitalRead(BTN_UP) == LOW && youY > TOP_BOUNDARY) {
    youY -= 4.5;
  }
  if(digitalRead(BTN_DOWN) == LOW && youY < BOTTOM_BOUNDARY - PADDLE_HEIGHT) {
    youY += 4.5;
  }
  
  // Bot AI
  float ballCenter = ballY;
  float paddleCenter = botY + PADDLE_HEIGHT/2;
  float error = ballCenter - paddleCenter;
  
  if(abs(error) > 3) {
    if(error > 0) botY += botSpeed;
    else botY -= botSpeed;
  }
  
  // Keep boundaries (TOP_BOUNDARY এখন 25, তাই লাল লাইন পার হবে না)
  if(botY < TOP_BOUNDARY) botY = TOP_BOUNDARY;
  if(botY > BOTTOM_BOUNDARY - PADDLE_HEIGHT) botY = BOTTOM_BOUNDARY - PADDLE_HEIGHT;
  if(youY < TOP_BOUNDARY) youY = TOP_BOUNDARY;
  if(youY > BOTTOM_BOUNDARY - PADDLE_HEIGHT) youY = BOTTOM_BOUNDARY - PADDLE_HEIGHT;
  
  // Update ball
  ballX += ballDX;
  ballY += ballDY;
  
  // Top/bottom collision (লাল লাইন পার হবে না)
  if(ballY - BALL_SIZE <= TOP_BOUNDARY) {
    ballY = TOP_BOUNDARY + BALL_SIZE;
    ballDY = abs(ballDY);
  }
  if(ballY + BALL_SIZE >= BOTTOM_BOUNDARY) {
    ballY = BOTTOM_BOUNDARY - BALL_SIZE;
    ballDY = -abs(ballDY);
  }
  
  // Left paddle collision
  if(ballX - BALL_SIZE <= LEFT_BOUNDARY + PADDLE_WIDTH && 
     ballX + BALL_SIZE >= LEFT_BOUNDARY &&
     ballY + BALL_SIZE >= botY && 
     ballY - BALL_SIZE <= botY + PADDLE_HEIGHT) {
    
    float hitPos = (ballY - botY) / PADDLE_HEIGHT;
    ballDY = (hitPos - 0.5) * 4.5;
    ballDX = abs(ballDX) + 0.05;
    
    if(abs(ballDY) > 3.8) ballDY = (ballDY > 0) ? 3.8 : -3.8;
    if(ballDX > 3.0) ballDX = 3.0;
    
    ballX = LEFT_BOUNDARY + PADDLE_WIDTH + BALL_SIZE;
  }
  
  // Right paddle collision
  if(ballX + BALL_SIZE >= RIGHT_BOUNDARY - PADDLE_WIDTH &&
     ballX - BALL_SIZE <= RIGHT_BOUNDARY &&
     ballY + BALL_SIZE >= youY &&
     ballY - BALL_SIZE <= youY + PADDLE_HEIGHT) {
    
    float hitPos = (ballY - youY) / PADDLE_HEIGHT;
    ballDY = (hitPos - 0.5) * 4.5;
    ballDX = -abs(ballDX) - 0.05;
    
    if(abs(ballDY) > 3.8) ballDY = (ballDY > 0) ? 3.8 : -3.8;
    if(abs(ballDX) > 3.0) ballDX = (ballDX > 0) ? 3.0 : -3.0;
    
    ballX = RIGHT_BOUNDARY - PADDLE_WIDTH - BALL_SIZE;
  }
  
  // Death condition
  if(ballX + BALL_SIZE <= LEFT_BOUNDARY || ballX - BALL_SIZE >= RIGHT_BOUNDARY) {
    gameOver();
    return;
  }
  
  // Erase old positions
  tft.fillRect(LEFT_BOUNDARY, oldBotY, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_BG);
  tft.fillRect(RIGHT_BOUNDARY - PADDLE_WIDTH, oldYouY, PADDLE_WIDTH, PADDLE_HEIGHT, COLOR_BG);
  tft.fillCircle(oldBallX, oldBallY, BALL_SIZE, COLOR_BG);
  
  // Update snow
  drawSnow();
  
  // Draw everything
  drawGameElements();
  
  delay(12);
}
