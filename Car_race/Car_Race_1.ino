#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h> 

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

// ILI9341 Color Palette
#define ROAD_GRAY     TFT_DARKGREY
#define GRASS_BLACK   TFT_BLACK
#define LINE_YELLOW   TFT_YELLOW
#define CURB_RED      TFT_RED
#define WINDOW_BLUE   TFT_CYAN
#define TAXI_YELLOW   TFT_ORANGE
#define RACER_RED     TFT_RED
#define TEXT_COLOR    TFT_WHITE

const int laneX[] = {80, 160, 240};
int playerLane = 1;
float currentPlayerX = 160; 
int targetX = 160;          
float slideSpeed = 6.0; ে

int score = 0;
bool gameOver = false;
bool isPaused = false; 
int lineOffset = 0;
int highScore = 0;
float enemyY = -80;
int enemyLane = 1;
int enemyType = 0; 
float gameSpeed = 4.5;      

int vehiclePoints[] = {3, 4, 8, 10, 12};
int vehicleHeights[] = {32, 32, 32, 40, 58}; 

// Button debounce
unsigned long lastButtonPress = 0;
const int debounceDelay = 200;

// Long press tracking
unsigned long leftPressStart = 0;
unsigned long rightPressStart = 0;
bool leftProcessing = false;
bool rightProcessing = false;
const int longPressTime = 170; 

// Button state tracking
bool lastLeftState = HIGH, lastRightState = HIGH;
bool lastSelectState = HIGH, lastStartState = HIGH;


void showStartScreen();
void resetGame();
void drawRoadBase();
void drawPlayer(float xPos);
void drawEnemy(int lane, int y, int type);
void triggerCrashEffect(int xPos);
void showGameOver();
bool readButtonWithDebounce(int pin, bool &lastState);
void handleLaneChange();

void setup() {
  Serial.begin(115200);
  
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  
  EEPROM.begin(512); 
  highScore = EEPROM.read(0); 
  if(highScore == 255) highScore = 0; 
  
  randomSeed(analogRead(34));
  
  resetGame();
  showStartScreen();
}

void showStartScreen() {
  tft.fillScreen(GRASS_BLACK);
  tft.setTextColor(LINE_YELLOW);
  tft.setTextSize(3);
  tft.setCursor(70, 60);
  tft.print("RACING Car");
  
  tft.setTextSize(1);
  tft.setTextColor(TEXT_COLOR);
  tft.setCursor(80, 110);
  tft.print("PRESS SELECT TO START");
  tft.setCursor(50, 130);
  tft.print("LEFT/RIGHT: MOVE LANES");
  tft.setCursor(70, 150);
  tft.print("SELECT: PAUSE/RESUME");
  tft.setCursor(70, 170);
  tft.print("START: PAUSE/RESUME");
  
  while(digitalRead(BTN_SELECT) == HIGH && digitalRead(BTN_START) == HIGH) {
    delay(10);
  }
  delay(300);
  resetGame();
}

void resetGame() {
  tft.fillScreen(GRASS_BLACK);
  drawRoadBase();
  playerLane = 1;
  currentPlayerX = laneX[playerLane];
  targetX = laneX[playerLane];
  enemyY = -80;
  enemyType = random(0, 5);
  enemyLane = random(0, 3);
  score = 0;
  gameSpeed = 3.8; 
  gameOver = false;
  isPaused = false;
  lineOffset = 0;
  
  // Reset long press flags
  leftProcessing = false;
  rightProcessing = false;
  leftPressStart = 0;
  rightPressStart = 0;
}

void drawRoadBase() {
  // Main road
  tft.fillRect(30, 0, 260, 240, ROAD_GRAY);
  
  // Side curb (Red-White border)
  for(int i = 0; i < 240; i += 20) {
    tft.fillRect(30, i, 5, 10, TFT_WHITE);
    tft.fillRect(30, i+10, 5, 10, CURB_RED);
    tft.fillRect(285, i, 5, 10, TFT_WHITE);
    tft.fillRect(285, i+10, 5, 10, CURB_RED);
  }
  
  // Lane markings
 // tft.fillRect(157, 0, 2, 240, TFT_WHITE);
}

void drawPlayer(float xPos) {
  static float lastX = 160;
  int x = (int)xPos - 15;
  int lx = (int)lastX - 15;
  int y = 198;


  if (lx != x) {
    tft.fillRect(lx - 5, y - 5, 40, 50, ROAD_GRAY);
    
 
    if (lx <= 35) {
      for(int i = 0; i < 240; i += 20) {
        tft.fillRect(30, i, 5, 10, TFT_WHITE);
        tft.fillRect(30, i+10, 5, 10, CURB_RED);
      }
    }
    if (lx + 35 >= 285) {
      for(int i = 0; i < 240; i += 20) {
        tft.fillRect(285, i, 5, 10, TFT_WHITE);
        tft.fillRect(285, i+10, 5, 10, CURB_RED);
      }
    }
  }

  // Shadow
  tft.fillRoundRect(x+4, y+4, 30, 40, 5, TFT_DARKGREY);
  // Main body
  tft.fillRoundRect(x, y, 30, 40, 5, RACER_RED);
  // Windshield
  tft.fillRect(x+6, y+8, 18, 8, WINDOW_BLUE);
  // Window detail
  tft.fillRect(x+3, y+18, 24, 2, WINDOW_BLUE);
  // Headlights
  tft.fillRect(x+2, y+2, 5, 4, TFT_YELLOW);
  tft.fillRect(x+23, y+2, 5, 4, TFT_YELLOW);
  // Taillights
  tft.fillRect(x+2, y+34, 5, 4, TFT_RED);
  tft.fillRect(x+23, y+34, 5, 4, TFT_RED);
  // Wheels
  tft.fillRect(x-2, y+8, 4, 12, TFT_BLACK);
  tft.fillRect(x+28, y+8, 4, 12, TFT_BLACK);
  tft.fillRect(x-2, y+24, 4, 12, TFT_BLACK);
  tft.fillRect(x+28, y+24, 4, 12, TFT_BLACK);
  
  lastX = xPos;
}

void drawEnemy(int lane, int y, int type) {
  int x = laneX[lane] - 18;
  
  switch(type) {
    case 0:
      tft.fillRoundRect(x+4, y, 28, 40, 5, TFT_BLUE);
      tft.fillRect(x+8, y+8, 20, 10, TFT_BLACK);
      tft.fillRect(x+8, y+28, 20, 4, TFT_BLACK);
      tft.fillRect(x+6, y, 6, 4, TFT_WHITE);
      tft.fillRect(x+24, y, 6, 4, TFT_WHITE);
      tft.fillRect(x+6, y+38, 6, 4, TFT_RED);
      tft.fillRect(x+24, y+38, 6, 4, TFT_RED);
      break;
      
    case 1:
      tft.fillRoundRect(x+4, y, 28, 40, 5, TFT_GREEN);
      tft.fillRect(x+16, y, 6, 40, TFT_BLACK);
      tft.fillRect(x+7, y+10, 22, 8, TFT_BLACK);
      tft.fillRect(x+7, y+28, 22, 3, TFT_BLACK);
      tft.fillRect(x+6, y, 5, 4, TFT_WHITE);
      tft.fillRect(x+25, y, 5, 4, TFT_WHITE);
      break;
      
    case 2:
      tft.fillRect(x+10, y, 16, 12, TFT_BLACK);
      tft.fillRect(x+10, y+36, 16, 12, TFT_BLACK);
      tft.fillRoundRect(x+11, y+8, 14, 32, 4, TFT_RED);
      tft.fillRect(x+13, y+12, 10, 24, TFT_DARKGREY);
      tft.fillCircle(x+17, y+18, 8, TFT_BLACK);
      tft.fillRect(x+5, y+20, 26, 8, TFT_OLIVE);
      tft.fillCircle(x+17, y+36, 6, TFT_BLACK);
      break;
      
    case 3:
      tft.fillRoundRect(x+4, y, 28, 44, 5, TFT_WHITE);
      tft.fillRect(x+8, y+2, 6, 3, TFT_RED);
      tft.fillRect(x+18, y+2, 6, 3, TFT_BLUE);
      tft.fillRect(x+7, y+8, 22, 8, TFT_BLACK);
      tft.fillRect(x+11, y+24, 14, 3, TFT_RED);
      tft.fillRect(x+16, y+19, 3, 13, TFT_RED);
      tft.fillRect(x+6, y+18, 2, 10, TFT_CYAN);
      tft.fillRect(x+28, y+18, 2, 10, TFT_CYAN);
      break;
      
    case 4:
      tft.fillRoundRect(x, y, 36, 60, 3, TFT_LIGHTGREY);
      tft.fillRect(x, y, 36, 40, TFT_ORANGE);
      tft.fillRect(x+5, y+44, 26, 12, TFT_BLACK);
      tft.fillRect(x+3, y+58, 6, 3, TFT_YELLOW);
      tft.fillRect(x+27, y+58, 6, 3, TFT_YELLOW);
      tft.fillRect(x-3, y+44, 4, 12, TFT_BLACK);
      tft.fillRect(x+35, y+44, 4, 12, TFT_BLACK);
      break;
  }
}

void triggerCrashEffect(int xPos) {
  int yPos = 210;
  
  for(int i = 0; i < 360; i += 45) {
    float angle = i * 0.017453;
    int x1 = xPos + cos(angle) * 35;
    int y1 = yPos + sin(angle) * 35;
    tft.fillTriangle(xPos, yPos, x1, y1, xPos + 8, yPos + 8, TFT_ORANGE);
  }
  delay(100);

  for(int i = 22; i < 382; i += 45) {
    float angle = i * 0.017453;
    int x1 = xPos + cos(angle) * 25;
    int y1 = yPos + sin(angle) * 25;
    tft.fillTriangle(xPos, yPos, x1, y1, xPos - 5, yPos + 5, TFT_YELLOW);
  }
  delay(100);

  tft.fillCircle(xPos, yPos, 12, TFT_WHITE);
  delay(50);
  
  for(int i = 0; i < 30; i++) {
    tft.drawPixel(xPos + random(-30, 30), yPos + random(-30, 30), TFT_WHITE);
  }
  delay(50);
  
  for(int i = 0; i < 5; i++) {
    tft.invertDisplay(true);
    delay(30);
    tft.invertDisplay(false);
    delay(30);
  }
}

void showGameOver() {
  if (score > highScore) {
    highScore = score;
    EEPROM.write(0, highScore);
    EEPROM.commit();
  }
  
  delay(500);
  tft.fillScreen(TFT_BLACK);
  
  tft.drawRect(10, 10, 300, 220, TFT_YELLOW);
  tft.drawRect(12, 12, 296, 216, TFT_ORANGE);
  
  tft.setTextColor(TFT_RED);
  tft.setTextSize(3);
  tft.setCursor(65, 50);
  tft.print("GAME OVER!");
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(90, 100);
  tft.print("SCORE: ");
  tft.setTextColor(TFT_WHITE);
  tft.print(score);
  
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(90, 130);
  tft.print("HIGH:  ");
  tft.setTextColor(TFT_WHITE);
  tft.print(highScore);
  
  tft.setTextSize(1);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(70, 180);
  tft.print("PRESS SELECT TO PLAY");
}

bool readButtonWithDebounce(int pin, bool &lastState) {
  bool currentState = digitalRead(pin);
  if (currentState == LOW && lastState == HIGH && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    lastState = currentState;
    return true;
  }
  lastState = currentState;
  return false;
}

void handleLaneChange() {
  // Left button handling with long press
  bool leftPressed = (digitalRead(BTN_LEFT) == LOW);
  
  if (leftPressed && !leftProcessing && millis() - lastButtonPress > debounceDelay) {
    leftPressStart = millis();
    leftProcessing = true;
  }
  
  if (leftProcessing && leftPressed) {
    unsigned long pressDuration = millis() - leftPressStart;
    if (pressDuration >= longPressTime) {
     
      playerLane = 0;
      targetX = laneX[playerLane];
      leftProcessing = false;
      lastButtonPress = millis();
    }
  }
  
  if (leftProcessing && !leftPressed) {
    if (millis() - leftPressStart < longPressTime) {
   
      if (playerLane > 0) {
        playerLane--;
        targetX = laneX[playerLane];
      }
    }
    leftProcessing = false;
    leftPressStart = 0;
  }
  
  // Right button handling with long press
  bool rightPressed = (digitalRead(BTN_RIGHT) == LOW);
  
  if (rightPressed && !rightProcessing && millis() - lastButtonPress > debounceDelay) {
    rightPressStart = millis();
    rightProcessing = true;
  }
  
  if (rightProcessing && rightPressed) {
    unsigned long pressDuration = millis() - rightPressStart;
    if (pressDuration >= longPressTime) {
     
      playerLane = 2;
      targetX = laneX[playerLane];
      rightProcessing = false;
      lastButtonPress = millis();
    }
  }
  
  if (rightProcessing && !rightPressed) {
    if (millis() - rightPressStart < longPressTime) {
     
      if (playerLane < 2) {
        playerLane++;
        targetX = laneX[playerLane];
      }
    }
    rightProcessing = false;
    rightPressStart = 0;
  }
}

void loop() {

  if (readButtonWithDebounce(BTN_SELECT, lastSelectState) || 
      readButtonWithDebounce(BTN_START, lastStartState)) {
    if (gameOver) {
      resetGame();
    } else {
      isPaused = !isPaused;
      if (isPaused) {
        tft.fillRoundRect(80, 90, 160, 50, 10, TFT_BLUE);
        tft.drawRoundRect(80, 90, 160, 50, 10, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        tft.setCursor(125, 105);
        tft.print("PAUSED");
        tft.setTextSize(1);
        tft.setCursor(115, 125);
        tft.print("PRESS START");
      } else {
        tft.fillRect(80, 90, 160, 50, ROAD_GRAY);
        drawRoadBase(); 
      }
    }
  }

  if (!gameOver && !isPaused) {
   
    handleLaneChange();

    if (abs(currentPlayerX - targetX) > 2) {
      if (currentPlayerX < targetX) currentPlayerX += slideSpeed;
      else currentPlayerX -= slideSpeed;
    } else {
      currentPlayerX = targetX;
    }

    
    tft.fillRect(laneX[enemyLane] - 25, (int)enemyY - 5, 50, (int)gameSpeed + 8, ROAD_GRAY); 
    
    enemyY += gameSpeed;

  
    if (enemyY > 250) {
      tft.fillRect(laneX[enemyLane] - 25, 200, 50, 40, ROAD_GRAY); 
      score += vehiclePoints[enemyType];
      enemyY = -80;
      enemyLane = random(0, 3);
      enemyType = random(0, 5);
      gameSpeed += 0.05;
      if (gameSpeed > 8.0) gameSpeed = 8.0;
    }

   
    if (abs(currentPlayerX - laneX[enemyLane]) < 25 && 
        (enemyY + vehicleHeights[enemyType]) > 195 && 
        enemyY < 225) {
      triggerCrashEffect((int)currentPlayerX);
      gameOver = true;
      showGameOver();
      return;
    }

  
    lineOffset = (lineOffset + (int)gameSpeed) % 48;
    for(int i = -48; i < 240; i += 48) {
      tft.fillRect(115, i + lineOffset, 3, 24, LINE_YELLOW);
      tft.fillRect(202, i + lineOffset, 3, 24, LINE_YELLOW);
      tft.fillRect(115, i + lineOffset - 6, 3, 6, ROAD_GRAY);
      tft.fillRect(202, i + lineOffset - 6, 3, 6, ROAD_GRAY);
    }

    drawPlayer(currentPlayerX);
    drawEnemy(enemyLane, (int)enemyY, enemyType);

    tft.fillRect(250, 5, 65, 15, GRASS_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(1);
    tft.setCursor(255, 8);
    tft.print("S:");
    tft.setTextColor(LINE_YELLOW);
    tft.print(score);
    
    delay(16);
  }
}
