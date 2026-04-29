#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// ============= BUTTON PINS =============
#define BTN_UP       13
#define BTN_DOWN     12
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_A        14
#define BTN_B        33
#define BTN_START    32
#define BTN_SELECT   25

// ============= GAME MODE ENUM =============
enum GameMode {
  GAME_MENU,
  GAME_SNAKE,
  GAME_FLAPPY,
  GAME_CONNECT4,
  GAME_TICTACTOE,
  GAME_RPS,
  GAME_NINJA,
  GAME_PONG
};

GameMode currentGame = GAME_MENU;
int selectedGame     = 0;   // 0..6

int screenWidth  = 320;
int screenHeight = 240;

// ============= MENU DESIGN CONSTANTS =============
#define MENU_BG        0x0000
#define MENU_HEADER_H  42
#define MENU_ITEM_H    28
#define MENU_ITEMS     7
#define MENU_LIST_Y    (MENU_HEADER_H + 2)
#define MENU_FOOTER_Y  (MENU_LIST_Y + MENU_ITEMS * MENU_ITEM_H + 2)

const uint16_t GAME_ACCENT[7] = {
  0x07E0,  // Snake      – Green
  0xFFE0,  // Flappy     – Yellow
  0xF81F,  // Connect4   – Magenta
  0x837F,  // TicTacToe  – Lavender
  0xFC00,  // RPS        – Orange
  0x07FF,  // Ninja Up   – Cyan
  0xF800   // Pong       – Red
};

const uint16_t GAME_DIM[7] = {
  0x0120,  // dim green
  0x2100,  // dim yellow
  0x2004,  // dim magenta
  0x1010,  // dim lavender
  0x2000,  // dim orange
  0x0208,  // dim cyan
  0x2000   // dim red
};

const char* GAME_NAMES[7] = {
  "SNAKE",
  "FLAPPY BIRD",
  "CONNECT 4",
  "TIC TAC TOE",
  "ROCK PAPER SCISSORS",
  "NINJA UP",
  "DINO PONG"
};

const char* GAME_TAGS[7] = {
  "EAT GROW SURVIVE",
  "TAP  FLY  DODGE",
  "CONNECT & BLOCK",
  "THINK & PLACE",
  "CHOOSE & DUEL",
  "SWITCH DODGE RUSH",
  "RALLY SURVIVE WIN"
};

const char* GAME_MODE_BADGE[7] = {
  "1P", "1P", "BOT", "BOT", "BOT", "1P", "BOT"
};

int menuHighScores[7] = {0,0,0,0,0,0,0};

// ============= FORWARD DECLARATIONS =============
void showMenu();
void updateMenuSelection(int oldSel, int newSel);
void returnToMenu();
void resetSnakeGame();
void showStartScreenFlappy();
void resetGameC4();
void resetTTTGame();
void showStartScreenNinja();
void showStartScreenPong();

// ================================================================
// =================== SNAKE VARIABLES ============================
// ================================================================
#define SNAKE_GRID    10
#define MAX_SNAKE_LEN 200
#define COL_BG        0x0000
#define COL_SNAKE_HEAD 0xF800
#define COL_SNAKE_BODY 0xAFE5
#define COL_FOOD      0x07FF
#define COL_BONUS     0xF81F
#define COL_SCORE     0xFFFF
#define COL_GOLD      0xFD20

struct Point { int x, y; };
Point snake[MAX_SNAKE_LEN];
int snakeLen, dirX, dirY, score;
int highScoreSnake = 0;
bool gameOverSnake = false, isPausedSnake = false;
int foodCounter = 0;
unsigned long lastMoveTime, lastPulseTime;
int moveDelay = 150, pulseState = 0;
Point food, bonusFood;
bool bonusActive = false;
unsigned long bonusStartTime;

// ================================================================
// =================== FLAPPY VARIABLES ===========================
// ================================================================
#define MAX_DOTS 30
struct Dot { float x, y, speed; int brightness; };
Dot bgDots[MAX_DOTS];

struct Pipe { float x; int gapY, gapH; bool counted; };
Pipe pipes[4];

const int   PIPE_W          = 28;
const int   CAP_H           = 8;
const float SCROLL_SPEED    = 3.5;
const int   MIN_PIPE_DIST   = 120;
const int   MAX_PIPE_DIST   = 250;

float birdY = 120, prevBirdY = 120, velocity = 0;
const float gravity = 0.18, flapPower = -2.5;
int scoreFlappy = 0, highScoreFlappy = 0;
bool playing = false, isPausedFlappy = false;
bool isCountdown = false;
unsigned long countdownStartTime = 0;
int countdownValue = 3;

#define BG_COLOR          TFT_BLACK
#define PIPE_COLOR        TFT_GREEN
#define PIPE_CAP_COLOR    TFT_DARKGREEN
#define BIRD_BODY_COLOR   TFT_YELLOW
#define BIRD_DETAIL_COLOR TFT_RED

const unsigned char bird_body_bmp[] PROGMEM = {
  0x00,0x7e,0x00,0x01,0xff,0x80,0x03,0xff,0xc0,0x07,0xff,0xe0,0x0f,0xff,0xf0,
  0x1f,0xff,0xf8,0x3f,0xff,0xfc,0x7f,0xff,0xfe,0xff,0xff,0xff,0xff,0xff,0xff,
  0x7f,0xff,0xfe,0x7f,0xff,0xfe,0x3f,0xff,0xfc,0x1f,0xff,0xf8,0x0f,0xff,0xf0,
  0x07,0xff,0xe0,0x01,0xff,0x80,0x00,0x7e,0x00
};
const unsigned char bird_details_bmp[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x70,0x00,0x00,0xf8,0x00,
  0x01,0xf8,0x00,0x01,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0x7e,0x00,0x01,0xff,
  0x00,0x00,0x7e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// ================================================================
// =================== CONNECT 4 VARIABLES ========================
// ================================================================
#define C4_ROWS    7
#define C4_COLS    10
#define CELL_SIZE  28
#define OFFSET_X   20
#define OFFSET_Y   40
#define COL_BOARD  0x0000
#define COL_EMPTY  0xFFFF
#define COL_PLAYER 0x07E0
#define COL_BOT    0xF800
#define COL_GLOSS  0xFFFF
#define COL_WIN    0x001F
#define COL_BG_C4  0x0000
#define COL_BORDER 0x7BEF
#define COL_GRID   0xAD55

int c4board[C4_ROWS][C4_COLS];
int selector = 4;
bool playerTurn = true, gameOverC4 = false;
int playerWins = 0, botWins = 0;
int winX[4], winY[4];

// ================================================================
// =================== TIC TAC TOE VARIABLES ======================
// ================================================================
#define TTT_SIZE      3
#define TTT_CELL_SIZE 80
#define TTT_OFFSET_X  40
#define TTT_OFFSET_Y  30
#define TTT_BG        0x0000
#define TTT_GRID      0xFFFF
#define TTT_PLAYER    0x07E0
#define TTT_BOT       0xF800
#define TTT_HIGHLIGHT 0xFD20
#define TTT_TEXT      0xFFFF

char tttBoard[TTT_SIZE][TTT_SIZE];
bool playerTurnTTT = true, gameOverTTT = false;
int playerScoreTTT = 0, botScoreTTT = 0, drawScoreTTT = 0;
int selectedRow = 1, selectedCol = 1;

// ================================================================
// =================== RPS VARIABLES ==============================
// ================================================================
#define MINT_BG       0x9F5A
#define COLOR_ROCK    0x52AA
#define COLOR_PAPER   0xFFFF
#define COLOR_SC_H    0xF800
#define COLOR_TEXT    0x0000
#define COLOR_WIN_RPS 0x07E0
#define COLOR_LOSS    0xF800
#define COLOR_GOLD_RPS 0xFD20

int pScore = 0, bScore = 0;
int currentSel = 1;

// ================================================================
// =================== NINJA UP VARIABLES =========================
// ================================================================
#define NINJA_COLOR  TFT_CYAN
#define WALL_COLOR   TFT_WHITE
#define SPIKE_COLOR  TFT_RED
#define BLUE_SPIKE   TFT_BLUE
#define NDOT_COLOR   0x7BEF
#define TRAIL_COLOR  0x0410

#define NINJA_WALL_LEFT   0
#define NINJA_WALL_RIGHT  200
#define NINJA_WALL_THICK  6
#define NINJA_PW          16
#define NINJA_PH          24

const int TRAIL_SIZE = 5;
int trailX[TRAIL_SIZE], trailY[TRAIL_SIZE];

struct Spike {
  float y;
  int side;
  bool isBlue;
};

struct NinjaDot {
  float x, y, speed;
};

const int MAX_NINJA_DOTS = 20;
NinjaDot ninjaDots[MAX_NINJA_DOTS];
Spike spikes[2];

int ninjaPlayerSide = 0;
float ninjaPlayerY = 100;
unsigned long ninjaScore = 0;
unsigned long ninjaHighScore = 0;
bool ninjaPlaying = false;
bool ninjaPaused = false;
float ninjaGameSpeed = 2.5;
bool ninjaLeftPressed = false, ninjaRightPressed = false;
unsigned long lastNinjaLeft = 0, lastNinjaRight = 0;
const int NINJA_DEBOUNCE = 50;

bool isRushMode = false;
unsigned long rushStartTime = 0;
const long rushDuration = 4000;

// ================================================================
// =================== PONG VARIABLES =============================
// ================================================================
#define PONG_TOP     25
#define PONG_BOTTOM  232
#define PONG_LEFT    8
#define PONG_RIGHT   312
#define PADDLE_W     5
#define PADDLE_H     45
#define BALL_SZ      6
#define COLOR_BOT_P  TFT_RED
#define COLOR_YOU_P  TFT_CYAN
#define COLOR_BALL_P TFT_GREEN
#define COLOR_GRID_P 0x4208
#define COLOR_SNOW_P 0x7BEF

struct Snow { float x, y, speed; };
Snow snowflakes[40];

float ballX, ballY, ballDX, ballDY;
float botPaddleY, youPaddleY;
unsigned long pongScore = 0;
unsigned long pongHighScore = 0;
unsigned long lastPongScoreUpdate = 0;
unsigned long lastSnowUpdate = 0;
float pongBallSpeed = 2.0;
bool pongActive = false;
bool pongPaused = false;

// ================================================================
// =================== VERTICAL MENU SYSTEM =======================
// ================================================================
void drawMenuChrome() {
  tft.fillScreen(MENU_BG);

  // Top accent line
  tft.drawFastHLine(0, 0, screenWidth, 0x0841);
  tft.fillRect(0, 1, screenWidth, MENU_HEADER_H - 1, 0x0008);

  // Left bracket
  tft.drawFastVLine(8, 6, 30, 0x2124);
  tft.drawFastHLine(8, 6, 5, 0x2124);
  tft.drawFastHLine(8, 35, 5, 0x2124);

  // Title
  tft.setTextSize(3);
  tft.setTextColor(0x07FF);
  tft.setCursor(24, 10);
  tft.print("ARCADE");

  tft.setTextSize(1);
  tft.setTextColor(0x2945);
  tft.setCursor(24, 32);
  tft.print("GAME CONSOLE  v3.0  7 GAMES");

  // Right bracket
  tft.drawFastVLine(screenWidth - 8, 6, 30, 0x2124);
  tft.drawFastHLine(screenWidth - 13, 6, 5, 0x2124);
  tft.drawFastHLine(screenWidth - 13, 35, 5, 0x2124);

  // Dot cluster
  for (int i = 0; i < 4; i++) {
    tft.fillCircle(screenWidth - 20 - i * 8, 20, 2,
      (i==0)?0xF800:(i==1)?0xFFE0:(i==2)?0x07E0:0x07FF);
  }

  // Header separator
  tft.drawFastHLine(0, MENU_HEADER_H, screenWidth, 0x1082);

  // Footer
  int fy = MENU_FOOTER_Y;
  if (fy < screenHeight - 14) {
    tft.drawFastHLine(0, fy, screenWidth, 0x1082);
    tft.fillRect(0, fy+1, screenWidth, screenHeight - fy - 1, 0x0008);
    tft.setTextSize(1);
    tft.setTextColor(0x3186); tft.setCursor(12, fy+8); tft.print("[UP/DN]");
    tft.setTextColor(0x2945); tft.setCursor(58, fy+8); tft.print("Nav");
    tft.setTextColor(0x07E0); tft.setCursor(100, fy+8); tft.print("[A]");
    tft.setTextColor(0x2945); tft.setCursor(117, fy+8); tft.print("Launch");
    tft.setTextColor(0xF800); tft.setCursor(176, fy+8); tft.print("[B]");
    tft.setTextColor(0x2945); tft.setCursor(193, fy+8); tft.print("Back");
    tft.setTextColor(0x1082); tft.setCursor(screenWidth-78, fy+8); tft.print("ESP32 TFT");
  }
  tft.drawFastHLine(0, screenHeight-1, screenWidth, 0x0841);
}

void drawMenuItem(int idx, bool active) {
  int y = MENU_LIST_Y + idx * MENU_ITEM_H;
  uint16_t accent = GAME_ACCENT[idx];
  uint16_t dim    = GAME_DIM[idx];

  if (active) {
    tft.fillRect(0,  y, 4,             MENU_ITEM_H, accent);
    tft.fillRect(4,  y, screenWidth-4, MENU_ITEM_H, dim);
  } else {
    tft.fillRect(0, y, screenWidth, MENU_ITEM_H, MENU_BG);
  }
  tft.drawFastHLine(0, y + MENU_ITEM_H - 1, screenWidth, 0x0841);

  // Pixel icon
  int iconX = 12, iconY = y + (MENU_ITEM_H - 14) / 2;
  uint16_t iconColor = active ? accent : (uint16_t)0x1082;
  switch (idx) {
    case 0: // Snake
      tft.drawFastHLine(iconX, iconY+3, 6, iconColor);
      tft.drawFastVLine(iconX+6, iconY+3, 5, iconColor);
      tft.drawFastHLine(iconX+2, iconY+8, 5, iconColor);
      tft.fillRect(iconX+1, iconY+1, 4, 3, iconColor);
      break;
    case 1: // Flappy
      tft.fillTriangle(iconX, iconY+7, iconX+14, iconY+3, iconX+14, iconY+11, iconColor);
      tft.fillCircle(iconX+12, iconY+3, 2, iconColor);
      break;
    case 2: // Connect 4
      for(int r=0;r<2;r++) for(int c=0;c<4;c++)
        tft.fillCircle(iconX+2+c*4, iconY+3+r*5, 1, iconColor);
      break;
    case 3: // TTT
      tft.drawFastVLine(iconX+5,  iconY, 13, iconColor);
      tft.drawFastVLine(iconX+10, iconY, 13, iconColor);
      tft.drawFastHLine(iconX, iconY+4,  13, iconColor);
      tft.drawFastHLine(iconX, iconY+9,  13, iconColor);
      break;
    case 4: // RPS fist
      tft.fillRoundRect(iconX+2, iconY+4, 10, 8, 2, iconColor);
      for(int f=0;f<4;f++) tft.fillRoundRect(iconX+2+f*3, iconY, 2, 5, 1, iconColor);
      break;
    case 5: // Ninja – two vertical lines
      tft.fillRect(iconX+2,  iconY+1, 3, 12, iconColor);
      tft.fillRect(iconX+9,  iconY+1, 3, 12, iconColor);
      tft.fillTriangle(iconX+5, iconY+7, iconX, iconY+7, iconX+3, iconY+4, iconColor);
      break;
    case 6: // Pong – paddle + ball
      tft.fillRect(iconX, iconY+2, 3, 10, iconColor);
      tft.fillRect(iconX+11, iconY+2, 3, 10, iconColor);
      tft.fillCircle(iconX+7, iconY+7, 2, iconColor);
      break;
  }

  // Name
  tft.setTextSize(1);
  tft.setTextColor(active ? accent : (uint16_t)0x4228);
  tft.setCursor(36, y + 5);
  tft.print(GAME_NAMES[idx]);

  // Subtitle
  tft.setTextColor(active ? (uint16_t)0x2945 : (uint16_t)0x2104);
  tft.setCursor(36, y + 16);
  tft.print(GAME_TAGS[idx]);

  // Right: badge + score
  int badgeX = screenWidth - 52;
  tft.fillRoundRect(badgeX, y+3, 22, 10, 2, active ? accent : (uint16_t)0x1082);
  tft.setTextSize(1);
  tft.setTextColor(active ? (uint16_t)MENU_BG : (uint16_t)0x3186);
  tft.setCursor(badgeX+2, y+5);
  tft.print(GAME_MODE_BADGE[idx]);

  tft.setTextColor(active ? (uint16_t)0x3186 : (uint16_t)0x2104);
  tft.setCursor(badgeX, y+17);
  char sbuf[8]; sprintf(sbuf, "%04d", menuHighScores[idx]);
  tft.print(sbuf);

  if (active) {
    tft.fillTriangle(screenWidth-5, y+MENU_ITEM_H/2-4,
                     screenWidth-5, y+MENU_ITEM_H/2+4,
                     screenWidth,   y+MENU_ITEM_H/2, accent);
  }
}

void updateMenuSelection(int oldSel, int newSel) {
  drawMenuItem(oldSel, false);
  drawMenuItem(newSel, true);
}

void showMenu() {
  drawMenuChrome();
  for (int i = 0; i < MENU_ITEMS; i++) drawMenuItem(i, i == selectedGame);
}

void returnToMenu() {
  currentGame = GAME_MENU;
  showMenu();
}

// ================================================================
// =================== SNAKE GAME =================================
// ================================================================
void updateSnakeScoreDisplay() {
  tft.fillRect(0, 0, tft.width(), 25, COL_BG);
  tft.setTextColor(COL_SCORE); tft.setTextSize(2);
  tft.setCursor(10, 5); tft.print("Score: "); tft.print(score);
  tft.setCursor(tft.width()-150, 5); tft.print("HIGH: "); tft.print(highScoreSnake);
}

void showSnakePauseScreen(bool pause) {
  if (pause) {
    tft.fillRect(60,80,200,60,0x001F); tft.drawRect(60,80,200,60,COL_SCORE);
    tft.setCursor(120,105); tft.setTextColor(COL_SCORE); tft.setTextSize(3); tft.print("PAUSED");
    tft.setTextSize(2);
  } else {
    tft.fillRect(60,80,200,60,COL_BG);
    drawSnakeFood(false);
    if (bonusActive) drawBonusFood(false);
  }
}

void spawnFood() {
  drawSnakeFood(true);
  int maxGX=(tft.width()-20)/SNAKE_GRID, maxGY=(tft.height()-50)/SNAKE_GRID;
  bool ok;
  do {
    ok=true;
    food.x=random(2,maxGX-2)*SNAKE_GRID+10;
    food.y=random(4,maxGY-2)*SNAKE_GRID+40;
    for(int i=0;i<snakeLen;i++)
      if(abs(snake[i].x*SNAKE_GRID+10-food.x)<SNAKE_GRID&&abs(snake[i].y*SNAKE_GRID+40-food.y)<SNAKE_GRID){ok=false;break;}
  } while(!ok);
  drawSnakeFood(false);
}

void spawnBonusFood() {
  if(bonusActive) return;
  bonusActive=true; bonusStartTime=millis();
  int maxGX=(tft.width()-20)/SNAKE_GRID, maxGY=(tft.height()-50)/SNAKE_GRID;
  bool ok;
  do {
    ok=true;
    bonusFood.x=random(2,maxGX-2)*SNAKE_GRID+10;
    bonusFood.y=random(4,maxGY-2)*SNAKE_GRID+40;
    for(int i=0;i<snakeLen;i++)
      if(abs(snake[i].x*SNAKE_GRID+10-bonusFood.x)<SNAKE_GRID&&abs(snake[i].y*SNAKE_GRID+40-bonusFood.y)<SNAKE_GRID){ok=false;break;}
    if(abs(food.x-bonusFood.x)<SNAKE_GRID&&abs(food.y-bonusFood.y)<SNAKE_GRID) ok=false;
  } while(!ok);
  drawBonusFood(false);
}

void drawSnakePart(int index, bool erase) {
  int x=snake[index].x*SNAKE_GRID+10, y=snake[index].y*SNAKE_GRID+40;
  if(erase){tft.fillRect(x,y,SNAKE_GRID,SNAKE_GRID,COL_BG);return;}
  uint16_t color=(index==0)?COL_SNAKE_HEAD:COL_SNAKE_BODY;
  tft.fillRoundRect(x,y,SNAKE_GRID-1,SNAKE_GRID-1,3,color);
  if(index==0){
    tft.fillCircle(x+3,y+3,2,COL_BG); tft.fillCircle(x+SNAKE_GRID-4,y+3,2,COL_BG);
    tft.fillCircle(x+3,y+3,1,COL_SCORE); tft.fillCircle(x+SNAKE_GRID-4,y+3,1,COL_SCORE);
  }
}

void drawSnakeFood(bool erase) {
  if(erase){tft.fillCircle(food.x,food.y,8,COL_BG);return;}
  tft.fillCircle(food.x,food.y,7,COL_FOOD);
  tft.fillCircle(food.x,food.y,4,COL_SCORE);
  tft.fillCircle(food.x,food.y,2,0x001F);
}

void drawBonusFood(bool erase) {
  if(!bonusActive&&!erase) return;
  if(erase){tft.fillCircle(bonusFood.x,bonusFood.y,15,COL_BG);return;}
  int rad=8+(pulseState%4);
  tft.fillCircle(bonusFood.x,bonusFood.y,12,COL_BG);
  tft.fillCircle(bonusFood.x,bonusFood.y,rad,COL_BONUS);
  tft.fillCircle(bonusFood.x,bonusFood.y,5,COL_GOLD);
  for(int i=0;i<8;i++){
    float a=i*45*3.14159/180;
    tft.fillCircle(bonusFood.x+cos(a)*12,bonusFood.y+sin(a)*12,2,COL_GOLD);
  }
}

void showSnakeGameOver() {
  if(score>highScoreSnake){highScoreSnake=score;menuHighScores[0]=highScoreSnake;}
  tft.fillScreen(COL_BG);
  tft.drawRect(5,5,tft.width()-10,tft.height()-10,COL_GOLD);
  tft.drawRect(8,8,tft.width()-16,tft.height()-16,COL_GOLD);
  tft.setTextSize(3); tft.setTextColor(COL_SNAKE_HEAD);
  tft.setCursor(tft.width()/2-80,40); tft.print("GAME OVER");
  tft.drawFastHLine(30,85,tft.width()-60,COL_SCORE);
  tft.fillRoundRect(20,100,tft.width()-40,90,10,0x1082);
  tft.setTextSize(2); tft.setTextColor(COL_SCORE);
  tft.setCursor(40,120); tft.print("YOUR SCORE : "); tft.setTextColor(COL_GOLD); tft.print(score);
  tft.setTextColor(COL_SCORE); tft.setCursor(40,150); tft.print("HIGH SCORE : ");
  tft.setTextColor(COL_FOOD); tft.print(highScoreSnake);
  tft.setTextSize(2); tft.setTextColor(COL_SCORE); tft.setCursor(50,200); tft.print("Press SELECT to Menu");
  while(digitalRead(BTN_SELECT)==HIGH&&digitalRead(BTN_A)==HIGH) delay(50);
  delay(300); returnToMenu();
}

void resetSnakeGame() {
  tft.fillScreen(COL_BG);
  snakeLen=5; score=0; foodCounter=0; moveDelay=150;
  bonusActive=false; isPausedSnake=false; gameOverSnake=false; pulseState=0;
  int maxGX=(tft.width()-20)/SNAKE_GRID, maxGY=(tft.height()-50)/SNAKE_GRID;
  int sx=maxGX/2, sy=maxGY/2;
  for(int i=0;i<snakeLen;i++){snake[i]={sx-i,sy};drawSnakePart(i,false);}
  dirX=1; dirY=0;
  spawnFood(); updateSnakeScoreDisplay();
  lastMoveTime=millis(); lastPulseTime=millis();
}

void runSnakeGame() {
  static unsigned long lastSelectPress=0,lastBPress=0;
  if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){lastBPress=millis();returnToMenu();return;}
  if(digitalRead(BTN_SELECT)==LOW&&millis()-lastSelectPress>300){
    lastSelectPress=millis();
    if(gameOverSnake) resetSnakeGame();
    else{isPausedSnake=!isPausedSnake;showSnakePauseScreen(isPausedSnake);}
  }
  if(!gameOverSnake&&!isPausedSnake){
    if(digitalRead(BTN_UP)==LOW&&dirY==0){dirX=0;dirY=-1;}
    else if(digitalRead(BTN_DOWN)==LOW&&dirY==0){dirX=0;dirY=1;}
    else if(digitalRead(BTN_LEFT)==LOW&&dirX==0){dirX=-1;dirY=0;}
    else if(digitalRead(BTN_RIGHT)==LOW&&dirX==0){dirX=1;dirY=0;}

    if(millis()-lastPulseTime>150){
      lastPulseTime=millis(); pulseState++;
      if(bonusActive){
        if(millis()-bonusStartTime>4000){drawBonusFood(true);bonusActive=false;}
        else drawBonusFood(false);
      }
      drawSnakeFood(false); updateSnakeScoreDisplay();
    }
    if(millis()-lastMoveTime>moveDelay){
      lastMoveTime=millis();
      int nx=snake[0].x+dirX, ny=snake[0].y+dirY;
      int maxX=(tft.width()-20)/SNAKE_GRID, maxY=(tft.height()-50)/SNAKE_GRID;
      if(nx<0||nx>=maxX||ny<0||ny>=maxY){gameOverSnake=true;showSnakeGameOver();return;}
      Point nh={nx,ny};
      for(int i=0;i<snakeLen;i++)
        if(nh.x==snake[i].x&&nh.y==snake[i].y){gameOverSnake=true;showSnakeGameOver();return;}
      bool ate=false;
      int hpx=nh.x*SNAKE_GRID+10+SNAKE_GRID/2, hpy=nh.y*SNAKE_GRID+40+SNAKE_GRID/2;
      if(abs(hpx-food.x)<SNAKE_GRID&&abs(hpy-food.y)<SNAKE_GRID){
        score+=10;snakeLen++;foodCounter++;ate=true;
        if(moveDelay>50)moveDelay-=2;
        spawnFood();
        if(foodCounter>=5&&!bonusActive){spawnBonusFood();foodCounter=0;}
      } else if(bonusActive&&abs(hpx-bonusFood.x)<SNAKE_GRID+2&&abs(hpy-bonusFood.y)<SNAKE_GRID+2){
        unsigned long t=(millis()-bonusStartTime)/1000;
        int dyn=80-(t*10);if(dyn<10)dyn=10;
        score+=dyn;snakeLen++;bonusActive=false;ate=true;drawBonusFood(true);
      }
      if(!ate) drawSnakePart(snakeLen-1,true);
      for(int i=snakeLen-1;i>0;i--) snake[i]=snake[i-1];
      snake[0]=nh;
      for(int i=(ate?0:1);i<snakeLen;i++) drawSnakePart(i,false);
      if(!ate) drawSnakePart(0,false);
      updateSnakeScoreDisplay();
    }
  }
}

// ================================================================
// =================== FLAPPY BIRD ================================
// ================================================================
void initDots(){
  for(int i=0;i<MAX_DOTS;i++)
    bgDots[i]={(float)random(0,screenWidth),(float)random(0,screenHeight),random(3,12)/10.0f,random(40,180)};
}
void updateDots(){
  for(int i=0;i<MAX_DOTS;i++){
    tft.drawPixel((int)bgDots[i].x,(int)bgDots[i].y,BG_COLOR);
    bgDots[i].x-=bgDots[i].speed;
    if(bgDots[i].x<0) bgDots[i]={(float)screenWidth,(float)random(0,screenHeight),random(3,12)/10.0f,random(40,180)};
    tft.drawPixel((int)bgDots[i].x,(int)bgDots[i].y,tft.color565(bgDots[i].brightness,bgDots[i].brightness,bgDots[i].brightness));
  }
}
void startCountdown(){isCountdown=true;countdownValue=3;countdownStartTime=millis();}
void updateCountdown(){
  if(!isCountdown) return;
  int cx=screenWidth/2,cy=screenHeight/2;
  unsigned long e=millis()-countdownStartTime;
  int nv=4-(e/1000);
  if(nv!=countdownValue&&nv>=0){
    countdownValue=nv;
    tft.fillRect(cx-40,cy-30,40,30,BG_COLOR);
    tft.setTextColor(TFT_YELLOW);tft.setTextSize(4);
    if(countdownValue>0){tft.setCursor(cx-12,cy-20);tft.print(countdownValue);}
  }
  if(e>=3000){tft.fillRect(cx-40,cy-30,80,60,BG_COLOR);isCountdown=false;isPausedFlappy=false;}
}
int getRandomPipeDist(){return random(MIN_PIPE_DIST,MAX_PIPE_DIST+1);}
void createPipe(int idx,float sx){pipes[idx]={sx,random(50,screenHeight-130),random(70,110),false};}
void startFlappyGame(){
  tft.fillScreen(BG_COLOR);initDots();scoreFlappy=0;isPausedFlappy=false;isCountdown=false;
  birdY=screenHeight/2;velocity=0;
  float cx=screenWidth+50;
  for(int i=0;i<4;i++){createPipe(i,cx);cx+=getRandomPipeDist();}
  playing=true;
}
void updateFlappyScreen(){
  updateDots();
  tft.fillRect(60,(int)prevBirdY,28,22,BG_COLOR);
  for(int i=0;i<4;i++){
    int ope=pipes[i].x+PIPE_W+5;
    if(ope>0&&ope<screenWidth) tft.fillRect(ope,0,10,screenHeight,BG_COLOR);
    if(pipes[i].x<screenWidth&&pipes[i].x+PIPE_W>0){
      tft.fillRect(pipes[i].x,0,PIPE_W,pipes[i].gapY,PIPE_COLOR);
      tft.fillRect(pipes[i].x-3,pipes[i].gapY-CAP_H,PIPE_W+6,CAP_H,PIPE_CAP_COLOR);
      int by=pipes[i].gapY+pipes[i].gapH;
      tft.fillRect(pipes[i].x,by,PIPE_W,screenHeight-by,PIPE_COLOR);
      tft.fillRect(pipes[i].x-3,by,PIPE_W+6,CAP_H,PIPE_CAP_COLOR);
    }
  }
  tft.drawBitmap(60,(int)birdY,bird_body_bmp,24,18,BIRD_BODY_COLOR);
  tft.drawBitmap(60,(int)birdY,bird_details_bmp,24,18,BIRD_DETAIL_COLOR);
  tft.setTextColor(TFT_RED);tft.setTextSize(2);
  tft.setCursor(screenWidth-75,8);tft.print(scoreFlappy);
}
void showStartScreenFlappy(){
  tft.fillScreen(BG_COLOR);
  for(int i=0;i<MAX_DOTS;i++){
    uint16_t dc=tft.color565(bgDots[i].brightness,bgDots[i].brightness,bgDots[i].brightness);
    tft.drawPixel((int)bgDots[i].x,(int)bgDots[i].y,dc);
  }
  tft.fillRoundRect(40,25,240,70,15,PIPE_COLOR);
  tft.setTextColor(BG_COLOR);tft.setTextSize(3);
  tft.setCursor(85,48);tft.print("FLAPPY");tft.setCursor(100,78);tft.print("BIRD");
  tft.fillRoundRect(30,110,260,115,10,PIPE_CAP_COLOR);
  tft.setTextColor(PIPE_COLOR);tft.setTextSize(1);
  tft.setCursor(55,125);tft.print("SELECT:Flap  START:Pause  B:Menu");
  tft.setCursor(55,140);tft.print("Avoid pipes to score!");
  tft.setTextColor(PIPE_COLOR);tft.setCursor(40,160);tft.print("High Score: ");tft.print(highScoreFlappy);
}
void flappyGameOver(){
  playing=false;isPausedFlappy=false;isCountdown=false;
  if(scoreFlappy>highScoreFlappy){highScoreFlappy=scoreFlappy;menuHighScores[1]=highScoreFlappy;}
  int cx=screenWidth/2;
  for(int i=0;i<3;i++){
    tft.fillRoundRect(cx-100,45,200,130,15,PIPE_COLOR);delay(100);
    tft.fillRoundRect(cx-100,45,200,130,15,PIPE_CAP_COLOR);delay(100);
  }
  tft.fillRoundRect(cx-100,45,200,130,15,PIPE_COLOR);
  tft.setTextColor(BG_COLOR);tft.setTextSize(2);tft.setCursor(cx-88,68);tft.print("GAME OVER");
  tft.fillRect(cx-70,95,140,50,BG_COLOR);
  tft.setTextColor(PIPE_COLOR);tft.setTextSize(2);tft.setCursor(cx-60,105);tft.print("SCORE: ");tft.print(scoreFlappy);
  tft.setTextSize(1);tft.setCursor(cx-45,130);tft.print("BEST: ");tft.print(highScoreFlappy);
  tft.setTextColor(BG_COLOR);tft.setCursor(cx-75,162);tft.print("PRESS SELECT BUTTON");
  delay(500);
  while(digitalRead(BTN_SELECT)==HIGH) delay(50);
  delay(200); showStartScreenFlappy();
}
void runFlappyGame(){
  static unsigned long lastStartPress=0,lastSelectPress=0,lastBPress=0;
  if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){lastBPress=millis();playing=false;returnToMenu();return;}
  if(!playing){
    updateDots();
    static int blinkCtr=0;
    if(millis()-lastStartPress>500){
      lastStartPress=millis();blinkCtr++;
      tft.fillRect(40,175,220,20,BG_COLOR);
      tft.setTextSize(1);tft.setTextColor(PIPE_COLOR);tft.setCursor(40,180);
      tft.print(blinkCtr%2==0?"> Press SELECT to Start <":"  Press SELECT to Start  ");
    }
    delay(30);
    if(digitalRead(BTN_SELECT)==LOW){delay(100);startFlappyGame();}
    return;
  }
  if(!isPausedFlappy&&!isCountdown&&digitalRead(BTN_START)==LOW&&millis()-lastSelectPress>200){
    lastSelectPress=millis();isPausedFlappy=true;
    tft.setTextColor(TFT_WHITE);tft.setTextSize(3);
    tft.setCursor(screenWidth/2-55,screenHeight/2-25);tft.print("PAUSED!");
    delay(200);return;
  }
  if(isPausedFlappy&&!isCountdown&&digitalRead(BTN_SELECT)==LOW&&millis()-lastSelectPress>200){
    lastSelectPress=millis();
    tft.fillRect(screenWidth/2-80,screenHeight/2-40,160,50,BG_COLOR);
    startCountdown();delay(200);return;
  }
  if(isCountdown){updateCountdown();delay(50);return;}
  if(isPausedFlappy){delay(50);return;}
  if(digitalRead(BTN_SELECT)==LOW&&millis()-lastSelectPress>50){lastSelectPress=millis();velocity=flapPower;}
  velocity+=gravity;prevBirdY=birdY;birdY+=velocity;
  for(int i=0;i<4;i++){
    pipes[i].x-=SCROLL_SPEED;
    if(!pipes[i].counted&&pipes[i].x+PIPE_W<60){pipes[i].counted=true;scoreFlappy+=5;}
    if(pipes[i].x<-PIPE_W-20){
      float fx=-999;
      for(int j=0;j<4;j++) if(pipes[j].x>fx) fx=pipes[j].x;
      createPipe(i,fx+getRandomPipeDist());
    }
    if(pipes[i].x<60+24&&pipes[i].x+PIPE_W>60)
      if(birdY<pipes[i].gapY||birdY+18>pipes[i].gapY+pipes[i].gapH){flappyGameOver();return;}
  }
  if(birdY<0||birdY>screenHeight-18){flappyGameOver();return;}
  updateFlappyScreen();delay(12);
}

// ================================================================
// =================== CONNECT 4 ==================================
// ================================================================
void drawGlossyCircle(int x,int y,int r,uint16_t color){
  if(color==COL_EMPTY){tft.fillCircle(x,y,r,COL_BOARD);tft.drawCircle(x,y,r,COL_GRID);}
  else{tft.fillCircle(x,y,r,color);tft.fillCircle(x-r/3,y-r/3,r/4,COL_GLOSS);}
}
void updateSelectorC4(){
  tft.fillRect(0,0,320,38,COL_BG_C4);
  if(!gameOverC4){
    int tx=OFFSET_X+selector*CELL_SIZE+CELL_SIZE/2,ty=30;
    for(int i=1;i<=3;i++) tft.fillTriangle(tx-(6+i/2),ty-i,tx+(6+i/2),ty-i,tx,ty+10,playerTurn?0x03E0:0xB800);
    tft.fillTriangle(tx-6,ty,tx+6,ty,tx,ty+12,playerTurn?COL_PLAYER:COL_BOT);
    tft.fillRoundRect(10,8,100,22,4,COL_BORDER);
    tft.setTextSize(1);tft.setTextColor(playerTurn?COL_PLAYER:COL_BOT,COL_BORDER);
    tft.setCursor(15,13);tft.print(playerTurn?"YOUR TURN":"BOT TURN");
    tft.fillRoundRect(210,8,100,22,4,COL_BORDER);
    tft.setTextColor(COL_GLOSS,COL_BORDER);tft.setCursor(215,13);
    tft.print("YOU:"); tft.print(playerWins); tft.print(" BOT:"); tft.print(botWins);
  }
}
void drawBoardC4(){
  int bw=C4_COLS*CELL_SIZE+12,bh=C4_ROWS*CELL_SIZE+12;
  tft.fillRoundRect(OFFSET_X-6,OFFSET_Y-6,bw,bh,10,COL_BORDER);
  tft.fillRoundRect(OFFSET_X-4,OFFSET_Y-4,bw-4,bh-4,8,COL_BOARD);
  tft.drawRoundRect(OFFSET_X-5,OFFSET_Y-5,bw-2,bh-2,9,COL_GOLD);
  for(int c=0;c<=C4_COLS;c++) tft.drawLine(OFFSET_X+c*CELL_SIZE,OFFSET_Y,OFFSET_X+c*CELL_SIZE,OFFSET_Y+C4_ROWS*CELL_SIZE,COL_GRID);
  for(int r=0;r<=C4_ROWS;r++) tft.drawLine(OFFSET_X,OFFSET_Y+r*CELL_SIZE,OFFSET_X+C4_COLS*CELL_SIZE,OFFSET_Y+r*CELL_SIZE,COL_GRID);
  for(int r=0;r<C4_ROWS;r++) for(int c=0;c<C4_COLS;c++){
    uint16_t col=(c4board[r][c]==1)?COL_PLAYER:(c4board[r][c]==2)?COL_BOT:COL_EMPTY;
    drawGlossyCircle(OFFSET_X+c*CELL_SIZE+CELL_SIZE/2,OFFSET_Y+r*CELL_SIZE+CELL_SIZE/2,CELL_SIZE/2-4,col);
  }
  updateSelectorC4();
}
void animateDiscC4(int col,int tr,uint16_t color){
  int x=OFFSET_X+col*CELL_SIZE+CELL_SIZE/2,br=CELL_SIZE/2-4;
  for(int r=0;r<=tr;r++){
    int y=OFFSET_Y+r*CELL_SIZE+CELL_SIZE/2;
    if(r>0) drawGlossyCircle(x,OFFSET_Y+(r-1)*CELL_SIZE+CELL_SIZE/2,br,COL_EMPTY);
    tft.fillCircle(x+2,y+2,br,0x0000);drawGlossyCircle(x,y,br,color);delay(45);
  }
}
bool checkWinC4(int p){
  for(int r=0;r<C4_ROWS;r++) for(int c=0;c<C4_COLS-3;c++)
    if(c4board[r][c]==p&&c4board[r][c+1]==p&&c4board[r][c+2]==p&&c4board[r][c+3]==p)
      {for(int i=0;i<4;i++){winY[i]=r;winX[i]=c+i;}return true;}
  for(int r=0;r<C4_ROWS-3;r++) for(int c=0;c<C4_COLS;c++)
    if(c4board[r][c]==p&&c4board[r+1][c]==p&&c4board[r+2][c]==p&&c4board[r+3][c]==p)
      {for(int i=0;i<4;i++){winY[i]=r+i;winX[i]=c;}return true;}
  for(int r=3;r<C4_ROWS;r++) for(int c=0;c<C4_COLS-3;c++)
    if(c4board[r][c]==p&&c4board[r-1][c+1]==p&&c4board[r-2][c+2]==p&&c4board[r-3][c+3]==p)
      {for(int i=0;i<4;i++){winY[i]=r-i;winX[i]=c+i;}return true;}
  for(int r=0;r<C4_ROWS-3;r++) for(int c=0;c<C4_COLS-3;c++)
    if(c4board[r][c]==p&&c4board[r+1][c+1]==p&&c4board[r+2][c+2]==p&&c4board[r+3][c+3]==p)
      {for(int i=0;i<4;i++){winY[i]=r+i;winX[i]=c+i;}return true;}
  return false;
}
bool isDrawC4(){for(int c=0;c<C4_COLS;c++)if(c4board[0][c]==0)return false;return true;}
void drawWinLineC4(){
  for(int i=0;i<3;i++){
    int x1=OFFSET_X+winX[i]*CELL_SIZE+CELL_SIZE/2,y1=OFFSET_Y+winY[i]*CELL_SIZE+CELL_SIZE/2;
    int x2=OFFSET_X+winX[i+1]*CELL_SIZE+CELL_SIZE/2,y2=OFFSET_Y+winY[i+1]*CELL_SIZE+CELL_SIZE/2;
    for(int t=-2;t<=2;t++) for(int o=-2;o<=2;o++) if(abs(o)+abs(t)<=4) tft.drawLine(x1+o,y1+t,x2+o,y2+t,COL_WIN);
    delay(100);
  }
}
void showGameOverC4(String msg,uint16_t tc){
  delay(600);
  tft.fillRect(40,50,240,140,0x0000);tft.drawRoundRect(40,50,240,140,10,COL_GOLD);tft.drawRoundRect(42,52,236,136,8,tc);
  tft.setTextSize(2);tft.setTextColor(tc,0x0000);
  if(msg=="YOU WIN!")tft.setCursor(108,75);else if(msg=="BOT WINS!")tft.setCursor(98,75);else tft.setCursor(128,75);
  tft.print(msg);
  tft.drawFastHLine(70,105,180,COL_GOLD);
  tft.setTextSize(2);tft.setTextColor(COL_GLOSS,0x0000);
  tft.setCursor(85,120);tft.print("YOU: ");tft.print(playerWins);
  tft.setCursor(85,145);tft.print("BOT: ");tft.print(botWins);
  tft.setTextSize(1);tft.setTextColor(COL_GOLD,0x0000);tft.setCursor(72,175);tft.print("PRESS SELECT TO PLAY AGAIN");
}
bool dropDiscC4(int col,int p){
  for(int r=C4_ROWS-1;r>=0;r--)
    if(c4board[r][col]==0){animateDiscC4(col,r,(p==1)?COL_PLAYER:COL_BOT);c4board[r][col]=p;return true;}
  return false;
}
void botMoveC4(){
  delay(300);int mc=-1;
  for(int c=0;c<C4_COLS&&mc==-1;c++) if(c4board[0][c]==0){
    int r;for(r=C4_ROWS-1;r>=0;r--) if(c4board[r][c]==0)break;
    c4board[r][c]=2;if(checkWinC4(2))mc=c;c4board[r][c]=0;
  }
  if(mc==-1) for(int c=0;c<C4_COLS&&mc==-1;c++) if(c4board[0][c]==0){
    int r;for(r=C4_ROWS-1;r>=0;r--) if(c4board[r][c]==0)break;
    c4board[r][c]=1;if(checkWinC4(1))mc=c;c4board[r][c]=0;
  }
  if(mc==-1){do{mc=random(C4_COLS);}while(c4board[0][mc]!=0);}
  dropDiscC4(mc,2);
  if(checkWinC4(2)){botWins++;menuHighScores[2]=playerWins;drawWinLineC4();showGameOverC4("BOT WINS!",COL_BOT);gameOverC4=true;}
  else if(isDrawC4()){showGameOverC4("DRAW!",COL_GOLD);gameOverC4=true;}
  else{playerTurn=true;updateSelectorC4();}
}
void resetGameC4(){
  tft.fillScreen(COL_BG_C4);
  for(int r=0;r<C4_ROWS;r++) for(int c=0;c<C4_COLS;c++) c4board[r][c]=0;
  selector=4;gameOverC4=false;playerTurn=(random(2)==0);
  drawBoardC4();if(!playerTurn){delay(500);botMoveC4();}
}
void runConnect4Game(){
  static unsigned long lastL=0,lastR=0,lastSel=0,lastB=0;
  if(digitalRead(BTN_B)==LOW&&millis()-lastB>300){lastB=millis();gameOverC4=true;returnToMenu();return;}
  if(!gameOverC4&&playerTurn){
    if(digitalRead(BTN_LEFT)==LOW&&selector>0&&millis()-lastL>120){lastL=millis();selector--;updateSelectorC4();}
    if(digitalRead(BTN_RIGHT)==LOW&&selector<C4_COLS-1&&millis()-lastR>120){lastR=millis();selector++;updateSelectorC4();}
    if(digitalRead(BTN_SELECT)==LOW&&millis()-lastSel>200){
      lastSel=millis();
      if(dropDiscC4(selector,1)){
        if(checkWinC4(1)){playerWins++;menuHighScores[2]=playerWins;drawWinLineC4();showGameOverC4("YOU WIN!",COL_PLAYER);gameOverC4=true;}
        else if(isDrawC4()){showGameOverC4("DRAW!",COL_GOLD);gameOverC4=true;}
        else{playerTurn=false;updateSelectorC4();delay(300);botMoveC4();}
      }
    }
  } else if(gameOverC4&&digitalRead(BTN_SELECT)==LOW&&millis()-lastSel>300){lastSel=millis();resetGameC4();}
  delay(10);
}

// ================================================================
// =================== TIC TAC TOE ================================
// ================================================================
void drawTTTBoard(){
  for(int i=1;i<TTT_SIZE;i++){
    tft.drawLine(TTT_OFFSET_X+i*TTT_CELL_SIZE,TTT_OFFSET_Y,TTT_OFFSET_X+i*TTT_CELL_SIZE,TTT_OFFSET_Y+TTT_SIZE*TTT_CELL_SIZE,TTT_GRID);
    tft.drawLine(TTT_OFFSET_X,TTT_OFFSET_Y+i*TTT_CELL_SIZE,TTT_OFFSET_X+TTT_SIZE*TTT_CELL_SIZE,TTT_OFFSET_Y+i*TTT_CELL_SIZE,TTT_GRID);
  }
  for(int r=0;r<TTT_SIZE;r++) for(int c=0;c<TTT_SIZE;c++){
    int x=TTT_OFFSET_X+c*TTT_CELL_SIZE,y=TTT_OFFSET_Y+r*TTT_CELL_SIZE;
    if(tttBoard[r][c]=='X'){
      tft.drawLine(x+15,y+15,x+TTT_CELL_SIZE-15,y+TTT_CELL_SIZE-15,TTT_BOT);
      tft.drawLine(x+TTT_CELL_SIZE-15,y+15,x+15,y+TTT_CELL_SIZE-15,TTT_BOT);
    } else if(tttBoard[r][c]=='O'){
      tft.drawCircle(x+TTT_CELL_SIZE/2,y+TTT_CELL_SIZE/2,TTT_CELL_SIZE/2-15,TTT_PLAYER);
      tft.fillCircle(x+TTT_CELL_SIZE/2,y+TTT_CELL_SIZE/2,TTT_CELL_SIZE/2-18,TTT_BG);
      tft.drawCircle(x+TTT_CELL_SIZE/2,y+TTT_CELL_SIZE/2,TTT_CELL_SIZE/2-16,TTT_PLAYER);
    }
  }
}
void drawTTTSelector(){
  int x=TTT_OFFSET_X+selectedCol*TTT_CELL_SIZE,y=TTT_OFFSET_Y+selectedRow*TTT_CELL_SIZE;
  for(int i=0;i<3;i++) tft.drawRect(x-i,y-i,TTT_CELL_SIZE+i*2,TTT_CELL_SIZE+i*2,TTT_HIGHLIGHT);
}
void updateTTTScore(){
  tft.fillRect(0,0,320,28,TTT_BG);tft.drawRect(0,0,320,28,TTT_GRID);
  tft.setTextSize(1);tft.setTextColor(TTT_TEXT,TTT_BG);
  tft.setCursor(10,8);tft.print("YOU: ");tft.print(playerScoreTTT);
  tft.setCursor(120,8);tft.print("DRAW: ");tft.print(drawScoreTTT);
  tft.setCursor(230,8);tft.print("BOT: ");tft.print(botScoreTTT);
  if(!gameOverTTT&&playerTurnTTT){tft.setCursor(10,20);tft.print("YOUR TURN (O)");}
  else if(!gameOverTTT&&!playerTurnTTT){tft.setCursor(10,20);tft.print("BOT TURN (X)...");}
}
bool checkWinTTT(char p,int&r1,int&c1,int&r2,int&c2,int&r3,int&c3){
  for(int i=0;i<TTT_SIZE;i++) if(tttBoard[i][0]==p&&tttBoard[i][1]==p&&tttBoard[i][2]==p){r1=i;c1=0;r2=i;c2=1;r3=i;c3=2;return true;}
  for(int i=0;i<TTT_SIZE;i++) if(tttBoard[0][i]==p&&tttBoard[1][i]==p&&tttBoard[2][i]==p){r1=0;c1=i;r2=1;c2=i;r3=2;c3=i;return true;}
  if(tttBoard[0][0]==p&&tttBoard[1][1]==p&&tttBoard[2][2]==p){r1=0;c1=0;r2=1;c2=1;r3=2;c3=2;return true;}
  if(tttBoard[0][2]==p&&tttBoard[1][1]==p&&tttBoard[2][0]==p){r1=0;c1=2;r2=1;c2=1;r3=2;c3=0;return true;}
  return false;
}
void drawWinningLineTTT(int r1,int c1,int r2,int c2,int r3,int c3){
  int x1=TTT_OFFSET_X+c1*TTT_CELL_SIZE+TTT_CELL_SIZE/2,y1=TTT_OFFSET_Y+r1*TTT_CELL_SIZE+TTT_CELL_SIZE/2;
  int x3=TTT_OFFSET_X+c3*TTT_CELL_SIZE+TTT_CELL_SIZE/2,y3=TTT_OFFSET_Y+r3*TTT_CELL_SIZE+TTT_CELL_SIZE/2;
  for(int s=0;s<=10;s++){
    int cx=x1+(x3-x1)*s/10,cy=y1+(y3-y1)*s/10;
    for(int t=-3;t<=3;t++) for(int o=-2;o<=2;o++) if(abs(t)+abs(o)<=3) tft.drawLine(x1+o,y1+t,cx+o,cy+t,TFT_WHITE);
    delay(50);
  }
  for(int fl=0;fl<5;fl++){
    for(int t=-3;t<=3;t++) for(int o=-2;o<=2;o++) if(abs(t)+abs(o)<=3) tft.drawLine(x1+o,y1+t,x3+o,y3+t,TTT_BG);
    delay(80);
    for(int t=-3;t<=3;t++) for(int o=-2;o<=2;o++) if(abs(t)+abs(o)<=3) tft.drawLine(x1+o,y1+t,x3+o,y3+t,TFT_WHITE);
    delay(80);
  }
  delay(500);
}
bool isDrawTTT(){for(int i=0;i<TTT_SIZE;i++) for(int j=0;j<TTT_SIZE;j++) if(tttBoard[i][j]==' ')return false;return true;}
void showGameOverTTT(String msg){
  delay(500);
  tft.fillRect(40,90,240,80,0x0000);tft.drawRect(40,90,240,80,TTT_GRID);tft.drawRect(43,93,234,74,TTT_GRID);
  tft.setTextSize(2);
  if(msg=="YOU WIN!"){tft.setTextColor(TTT_PLAYER);tft.setCursor(110,115);}
  else if(msg=="BOT WINS!"){tft.setTextColor(TTT_BOT);tft.setCursor(100,115);}
  else{tft.setTextColor(TTT_GRID);tft.setCursor(120,115);}
  tft.print(msg);
  tft.setTextSize(1);tft.setTextColor(TTT_TEXT);tft.setCursor(80,145);tft.print("PRESS SELECT TO CONTINUE");
  while(digitalRead(BTN_SELECT)==HIGH) delay(50);
  delay(300);
}
void botMoveTTT(){
  if(gameOverTTT||playerTurnTTT) return;
  delay(400);
  int br=-1,bc=-1,t1,t2,t3,t4,t5,t6;
  for(int i=0;i<TTT_SIZE&&br==-1;i++) for(int j=0;j<TTT_SIZE&&br==-1;j++) if(tttBoard[i][j]==' '){
    tttBoard[i][j]='X';if(checkWinTTT('X',t1,t2,t3,t4,t5,t6)){br=i;bc=j;}tttBoard[i][j]=' ';
  }
  if(br==-1) for(int i=0;i<TTT_SIZE&&br==-1;i++) for(int j=0;j<TTT_SIZE&&br==-1;j++) if(tttBoard[i][j]==' '){
    tttBoard[i][j]='O';if(checkWinTTT('O',t1,t2,t3,t4,t5,t6)){br=i;bc=j;}tttBoard[i][j]=' ';
  }
  if(br==-1&&tttBoard[1][1]==' '){br=1;bc=1;}
  if(br==-1){int cn[4][2]={{0,0},{0,2},{2,0},{2,2}};for(int i=0;i<4&&br==-1;i++)if(tttBoard[cn[i][0]][cn[i][1]]==' '){br=cn[i][0];bc=cn[i][1];}}
  if(br==-1){do{br=random(TTT_SIZE);bc=random(TTT_SIZE);}while(tttBoard[br][bc]!=' ');}
  tttBoard[br][bc]='X';drawTTTBoard();
  int wr1,wc1,wr2,wc2,wr3,wc3;
  if(checkWinTTT('X',wr1,wc1,wr2,wc2,wr3,wc3)){
    drawWinningLineTTT(wr1,wc1,wr2,wc2,wr3,wc3);
    botScoreTTT++;menuHighScores[3]=playerScoreTTT;gameOverTTT=true;updateTTTScore();showGameOverTTT("BOT WINS!");resetTTTGame();
  } else if(isDrawTTT()){
    drawScoreTTT++;gameOverTTT=true;updateTTTScore();showGameOverTTT("DRAW!");resetTTTGame();
  } else{playerTurnTTT=true;updateTTTScore();drawTTTSelector();}
}
void resetTTTGame(){
  for(int i=0;i<TTT_SIZE;i++) for(int j=0;j<TTT_SIZE;j++) tttBoard[i][j]=' ';
  playerTurnTTT=(random(2)==0);gameOverTTT=false;selectedRow=1;selectedCol=1;
  tft.fillScreen(TTT_BG);drawTTTBoard();updateTTTScore();drawTTTSelector();
  if(!playerTurnTTT){delay(500);botMoveTTT();}
}
void runTicTacToe(){
  static unsigned long lastMv=0,lastB=0;
  if(digitalRead(BTN_B)==LOW&&millis()-lastB>300){lastB=millis();returnToMenu();return;}
  if(!gameOverTTT&&playerTurnTTT&&millis()-lastMv>200){
    bool mv=false;
    if(digitalRead(BTN_UP)==LOW&&selectedRow>0){selectedRow--;mv=true;}
    else if(digitalRead(BTN_DOWN)==LOW&&selectedRow<TTT_SIZE-1){selectedRow++;mv=true;}
    else if(digitalRead(BTN_LEFT)==LOW&&selectedCol>0){selectedCol--;mv=true;}
    else if(digitalRead(BTN_RIGHT)==LOW&&selectedCol<TTT_SIZE-1){selectedCol++;mv=true;}
    else if(digitalRead(BTN_SELECT)==LOW&&tttBoard[selectedRow][selectedCol]==' '){
      tttBoard[selectedRow][selectedCol]='O';drawTTTBoard();
      int r1,c1,r2,c2,r3,c3;
      if(checkWinTTT('O',r1,c1,r2,c2,r3,c3)){
        drawWinningLineTTT(r1,c1,r2,c2,r3,c3);
        playerScoreTTT++;menuHighScores[3]=playerScoreTTT;gameOverTTT=true;updateTTTScore();showGameOverTTT("YOU WIN!");resetTTTGame();
      } else if(isDrawTTT()){drawScoreTTT++;gameOverTTT=true;updateTTTScore();showGameOverTTT("DRAW!");resetTTTGame();}
      else{playerTurnTTT=false;updateTTTScore();botMoveTTT();}
      mv=true;
    }
    if(mv){lastMv=millis();tft.fillScreen(TTT_BG);drawTTTBoard();updateTTTScore();drawTTTSelector();}
  }
  delay(10);
}

// ================================================================
// =================== ROCK PAPER SCISSORS ========================
// ================================================================
void drawMintBackground(){tft.fillScreen(MINT_BG);}
void drawRealRock(int x,int y,int s){
  uint16_t rd=0x3DEF,rl=0x7BEF,rm=0x5ACB;
  tft.fillRoundRect(x+2,y+4,s-4,s-8,10,rm);
  tft.fillRoundRect(x+2,y+s-12,s-4,8,6,rd);
  tft.fillRect(x+s-10,y+8,6,s-16,rd);
  tft.fillRoundRect(x+2,y+4,8,s-8,4,rl);
  tft.fillRect(x+6,y+4,s-12,6,rl);
  for(int i=0;i<4;i++){
    int sx=x+random(8,s-8),sy=y+random(10,s-10);
    tft.drawLine(sx,sy,sx+random(-5,5),sy+random(8,15),rd);
    tft.drawLine(sx,sy,sx+random(5,10),sy+random(-3,3),rd);
  }
  for(int i=0;i<8;i++) tft.fillCircle(x+random(8,s-8),y+random(10,s-10),random(1,3),rd);
  tft.drawRoundRect(x+2,y+4,s-4,s-8,10,0x2945);
}
void drawProPaper(int x,int y,int s){
  tft.fillRect(x,y,s-4,s,COLOR_PAPER);tft.drawRect(x,y,s-4,s,0x0000);
  for(int i=6;i<s;i+=6)tft.drawFastHLine(x+2,y+i,s-8,0xAD7F);
  tft.fillRect(x+2,y+2,3,s-4,0x94B2);
}
void drawProScissor(int x,int y,int s){
  tft.drawCircle(x+8,y+s-10,7,COLOR_SC_H);tft.drawCircle(x+s-12,y+s-10,7,COLOR_SC_H);
  tft.fillTriangle(x+10,y+s-15,x+s-5,y+5,x+s-10,y+2,0x0000);
  tft.fillTriangle(x+s-14,y+s-15,x+5,y+5,x+10,y+2,0x0000);
  tft.drawLine(x+12,y+s-18,x+s-12,y+8,0xFFFF);
}
void updateScoreRPS(){
  tft.fillRoundRect(0,0,screenWidth,40,8,0x0000);tft.drawRoundRect(0,0,screenWidth,40,8,COLOR_GOLD_RPS);
  tft.setTextColor(0xFFFF);tft.setTextSize(2);
  tft.setCursor(30,12);tft.print("PLAYER: ");tft.print(pScore);
  tft.setCursor(200,12);tft.print("BOT: ");tft.print(bScore);
  tft.drawFastVLine(160,5,30,COLOR_GOLD_RPS);
}
void drawSelectionBoxes(){
  tft.fillRect(0,100,screenWidth,80,MINT_BG);
  for(int i=1;i<=3;i++){
    int x=40+(i-1)*95;
    uint16_t border=(currentSel==i)?0xF800:0x0000;
    tft.drawRect(x-3,147,70,70,border);tft.drawRect(x-4,146,72,72,border);tft.drawRect(x-5,145,74,74,border);
    if(currentSel==i) for(int g=1;g<=3;g++) tft.drawRect(x-3-g,147-g,70+g*2,70+g*2,0xF820);
    if(i==1) drawRealRock(x+18,153,50);
    else if(i==2) drawProPaper(x+18,153,50);
    else drawProScissor(x+18,153,50);
  }
  tft.setTextSize(1);tft.setTextColor(0xFFFF);
  tft.setCursor(20,225);tft.print("LEFT/RIGHT: Select   SELECT: Play | B: Menu");
}
void showResultAnimationRPS(String result,uint16_t color){
  int bx=60,by=90,bw=200,bh=60;
  for(int i=0;i<10;i++){tft.drawRoundRect(bx-i,by-i,bw+i*2,bh+i*2,10,color);delay(20);}
  tft.fillRoundRect(bx,by,bw,bh,10,0x0000);tft.drawRoundRect(bx,by,bw,bh,10,color);
  tft.setTextSize(3);tft.setTextColor(color);
  tft.setCursor(bx+(bw/2)-(result.length()*9),by+20);tft.print(result);
  delay(1500);
}
void checkUltimateWinner(){
  if(pScore<5&&bScore<5) return;
  delay(500);
  for(int i=0;i<screenWidth;i+=10){tft.drawFastVLine(i,0,screenHeight,0x0000);delay(8);}
  tft.setTextSize(3);
  if(pScore>=5){
    tft.fillScreen(0x0000);tft.setTextColor(COLOR_WIN_RPS);tft.setCursor(65,50);tft.println("VICTORY!");
    tft.setTextSize(2);tft.setTextColor(0xFFFF);tft.setCursor(45,90);tft.println("YOU ARE SUPREME!");
    menuHighScores[4]=pScore;
  } else {
    tft.fillScreen(0x0000);tft.setTextColor(COLOR_LOSS);tft.setCursor(55,50);tft.println("GAME OVER");
    tft.setTextSize(2);tft.setTextColor(0xFFFF);tft.setCursor(55,90);tft.println("BOT IS SUPREME!");
  }
  unsigned long bs=millis();
  while(digitalRead(BTN_SELECT)==HIGH){
    if(millis()-bs>500){
      tft.setTextSize(1);tft.setTextColor(0xAD7F);tft.setCursor(85,210);tft.print("PRESS SELECT");
      delay(100);tft.fillRect(85,210,150,15,0x0000);bs=millis();
    }
    delay(10);
  }
  pScore=0;bScore=0;drawMintBackground();updateScoreRPS();drawSelectionBoxes();delay(500);
}
void runRPSGame(){
  static unsigned long lastBtn=0,lastB=0;
  if(digitalRead(BTN_B)==LOW&&millis()-lastB>300){lastB=millis();returnToMenu();return;}
  if(digitalRead(BTN_LEFT)==LOW&&millis()-lastBtn>200){lastBtn=millis();currentSel--;if(currentSel<1)currentSel=3;drawSelectionBoxes();}
  if(digitalRead(BTN_RIGHT)==LOW&&millis()-lastBtn>200){lastBtn=millis();currentSel++;if(currentSel>3)currentSel=1;drawSelectionBoxes();}
  if(digitalRead(BTN_SELECT)==LOW&&millis()-lastBtn>300){
    lastBtn=millis();
    tft.fillRect(0,45,screenWidth,55,MINT_BG);delay(800);tft.fillRect(0,45,screenWidth,80,MINT_BG);
    tft.setTextSize(2);tft.setTextColor(COLOR_TEXT);tft.setCursor(40,55);tft.print("YOU");
    if(currentSel==1)drawRealRock(35,75,60);else if(currentSel==2)drawProPaper(35,75,60);else drawProScissor(35,75,60);
    int bp=random(1,4);
    tft.setCursor(200,55);tft.print("BOT");
    for(int i=0;i<5;i++){tft.setCursor(170,90);tft.setTextColor(0xF800);tft.print("THINKING");delay(200);tft.fillRect(170,90,100,15,MINT_BG);delay(100);}
    tft.fillRect(140,70,130,50,MINT_BG);
    if(bp==1)drawRealRock(185,75,60);else if(bp==2)drawProPaper(185,75,60);else drawProScissor(185,75,60);
    delay(1000);
    String result;uint16_t rc;
    if(currentSel==bp){result="DRAW!";rc=0xFFFF;}
    else if((currentSel==1&&bp==3)||(currentSel==2&&bp==1)||(currentSel==3&&bp==2)){pScore++;result="YOU WIN!";rc=COLOR_WIN_RPS;}
    else{bScore++;result="YOU LOST!";rc=COLOR_LOSS;}
    updateScoreRPS();showResultAnimationRPS(result,rc);
    if(pScore>=5||bScore>=5){checkUltimateWinner();}
    else{drawMintBackground();updateScoreRPS();drawSelectionBoxes();}
  }
  delay(10);
}

// ================================================================
// =================== NINJA UP GAME ==============================
// ================================================================
void initNinjaDots(){
  for(int i=0;i<MAX_NINJA_DOTS;i++){
    ninjaDots[i].x=random(NINJA_WALL_LEFT+NINJA_WALL_THICK+5,NINJA_WALL_RIGHT-5);
    ninjaDots[i].y=random(0,240);
    ninjaDots[i].speed=(random(5,15)/10.0);
  }
}

void showStartScreenNinja(){
  tft.fillScreen(BG_COLOR);
  tft.setTextColor(NINJA_COLOR);tft.setTextSize(3);
  tft.setCursor(60,30);tft.print("NINJA UP!");
  tft.setTextColor(WALL_COLOR);tft.setTextSize(1);
  tft.setCursor(30,80);tft.print("SELECT : Start Game");
  tft.setCursor(30,98);tft.print("LEFT   : Move to Left Wall");
  tft.setCursor(30,116);tft.print("RIGHT  : Move to Right Wall");
  tft.setCursor(30,134);tft.print("START  : Pause / Resume");
  tft.setCursor(30,152);tft.print("B      : Back to Menu");
  tft.setCursor(30,170);tft.print("Avoid RED spikes!");
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(30,188);tft.print("Catch BLUE spikes = RUSH MODE!");
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(60,210);tft.print("Press SELECT to begin");
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(30,228);tft.print("Best: ");tft.print(ninjaHighScore);
}

void startNinjaGame(){
  tft.fillScreen(BG_COLOR);
  ninjaPlayerY=100; ninjaPlayerSide=0;
  ninjaScore=0; ninjaGameSpeed=2.5;
  isRushMode=false; ninjaPaused=false;
  initNinjaDots();
  for(int i=0;i<TRAIL_SIZE;i++){trailX[i]=-30;trailY[i]=-30;}
  spikes[0]={-50.0f,(int)random(0,2),false};
  spikes[1]={-200.0f,(int)random(0,2),false};
  ninjaPlaying=true;
}

void updateNinjaScoreDisplay(){
  tft.fillRect(220,0,100,60,BG_COLOR);
  tft.setTextColor(TFT_GREEN,BG_COLOR);tft.setTextSize(1);tft.setCursor(250,10);tft.print("SCORE");
  tft.setTextColor(TFT_YELLOW,BG_COLOR);tft.setTextSize(2);tft.setCursor(244,22);
  char sb[15];sprintf(sb,"%04lu",ninjaScore/10);tft.print(sb);
  tft.setTextColor(TFT_CYAN,BG_COLOR);tft.setTextSize(1);tft.setCursor(250,50);tft.print("BEST:");
  tft.setTextColor(TFT_WHITE,BG_COLOR);tft.setTextSize(2);tft.setCursor(244,62);
  sprintf(sb,"%04lu",ninjaHighScore);tft.print(sb);
}

void ninjaGameOver(){
  // Screen shake
  for(int i=0;i<5;i++){tft.invertDisplay(true);delay(40);tft.invertDisplay(false);delay(40);}
  ninjaPlaying=false; ninjaPaused=false;
  unsigned long cs=ninjaScore/10;
  if(cs>ninjaHighScore){ninjaHighScore=cs;menuHighScores[5]=(int)ninjaHighScore;}
  tft.fillRoundRect(40,70,240,100,10,TFT_RED);
  tft.drawRoundRect(40,70,240,100,10,TFT_WHITE);
  tft.setTextColor(TFT_WHITE);tft.setTextSize(2);tft.setCursor(90,90);tft.print("GAME OVER!");
  tft.setTextSize(1);
  tft.setCursor(90,115);tft.print("Score: ");tft.print(cs);
  tft.setCursor(90,135);tft.print("Best:  ");tft.print(ninjaHighScore);
  tft.setTextSize(1);tft.setCursor(65,180);tft.setTextColor(TFT_YELLOW);
  tft.print("SELECT: Play Again   B: Menu");
}

void updateNinjaGame(){
  // Button read
  bool leftState=(digitalRead(BTN_LEFT)==LOW);
  bool rightState=(digitalRead(BTN_RIGHT)==LOW);
  if(leftState&&!ninjaLeftPressed&&(millis()-lastNinjaLeft>NINJA_DEBOUNCE)){
    ninjaPlayerSide=0;ninjaLeftPressed=true;lastNinjaLeft=millis();
  } else if(!leftState) ninjaLeftPressed=false;
  if(rightState&&!ninjaRightPressed&&(millis()-lastNinjaRight>NINJA_DEBOUNCE)){
    ninjaPlayerSide=1;ninjaRightPressed=true;lastNinjaRight=millis();
  } else if(!rightState) ninjaRightPressed=false;

  float effectiveSpeed=ninjaGameSpeed;
  int scoreMul=1;
  bool showPlayer=true;
  uint16_t playerColor=NINJA_COLOR;

  if(!isRushMode){
    ninjaGameSpeed+=0.0024;
    if(ninjaGameSpeed>6.0) ninjaGameSpeed=6.0;
  }
  if(isRushMode){
    unsigned long el=millis()-rushStartTime;
    if(el<rushDuration){
      if(el<=6000){effectiveSpeed=ninjaGameSpeed*4.0;scoreMul=5;playerColor=BLUE_SPIKE;}
      else{effectiveSpeed=ninjaGameSpeed;scoreMul=1;playerColor=NINJA_COLOR;if((el/120)%2==0)showPlayer=false;}
    } else isRushMode=false;
  }
  ninjaScore+=scoreMul;

  // Clear play area (only between walls)
  tft.fillRect(NINJA_WALL_LEFT+NINJA_WALL_THICK,0,(NINJA_WALL_RIGHT-NINJA_WALL_LEFT)-NINJA_WALL_THICK,240,BG_COLOR);

  // Dots
  for(int i=0;i<MAX_NINJA_DOTS;i++){
    ninjaDots[i].y+=ninjaDots[i].speed+(effectiveSpeed*0.3);
    if(ninjaDots[i].y>240){
      ninjaDots[i].y=0;
      ninjaDots[i].x=random(NINJA_WALL_LEFT+NINJA_WALL_THICK+5,NINJA_WALL_RIGHT-5);
    }
    tft.drawPixel((int)ninjaDots[i].x,(int)ninjaDots[i].y,NDOT_COLOR);
  }

  // Trail
  for(int i=TRAIL_SIZE-1;i>0;i--){trailX[i]=trailX[i-1];trailY[i]=trailY[i-1];}
  int ninjaX=(ninjaPlayerSide==0)?(NINJA_WALL_LEFT+NINJA_WALL_THICK+2):(NINJA_WALL_RIGHT-NINJA_PW-2);
  trailX[0]=ninjaX;trailY[0]=(int)ninjaPlayerY;
  for(int i=1;i<TRAIL_SIZE;i++) if(trailX[i]>0) tft.drawRect(trailX[i],trailY[i],NINJA_PW,NINJA_PH,TRAIL_COLOR);

  // Walls
  tft.fillRect(NINJA_WALL_LEFT,0,NINJA_WALL_THICK,240,WALL_COLOR);
  tft.fillRect(NINJA_WALL_RIGHT,0,NINJA_WALL_THICK,240,WALL_COLOR);
  tft.drawRect(NINJA_WALL_LEFT-1,0,NINJA_WALL_THICK+2,240,TFT_DARKGREY);
  tft.drawRect(NINJA_WALL_RIGHT-1,0,NINJA_WALL_THICK+2,240,TFT_DARKGREY);

  // Spikes
  for(int i=0;i<2;i++){
    spikes[i].y+=effectiveSpeed;
    int ss=28;
    uint16_t sc=spikes[i].isBlue?BLUE_SPIKE:SPIKE_COLOR;
    if(spikes[i].side==0){
      tft.fillTriangle(NINJA_WALL_LEFT+NINJA_WALL_THICK,(int)spikes[i].y,
                       NINJA_WALL_LEFT+NINJA_WALL_THICK,(int)spikes[i].y+ss,
                       NINJA_WALL_LEFT+NINJA_WALL_THICK+ss,(int)spikes[i].y+ss/2,sc);
    } else {
      tft.fillTriangle(NINJA_WALL_RIGHT,(int)spikes[i].y,
                       NINJA_WALL_RIGHT,(int)spikes[i].y+ss,
                       NINJA_WALL_RIGHT-ss,(int)spikes[i].y+ss/2,sc);
    }
    // Collision
    if(spikes[i].side==ninjaPlayerSide){
      if(spikes[i].y+ss>ninjaPlayerY&&spikes[i].y<ninjaPlayerY+NINJA_PH){
        if(spikes[i].isBlue){
          isRushMode=true;rushStartTime=millis();spikes[i].y=300;
        } else if(!isRushMode){
          delay(1000);ninjaGameOver();return;
        }
      }
    }
    if(spikes[i].y>240){
      int other=(i==0)?1:0;
      spikes[i].y=spikes[other].y-random(120,180);
      spikes[i].side=random(0,2);
      spikes[i].isBlue=(random(0,12)==1);
    }
  }

  // Draw player
  if(showPlayer){
    tft.fillRect(ninjaX,(int)ninjaPlayerY,NINJA_PW,NINJA_PH,playerColor);
    tft.fillCircle(ninjaX+NINJA_PW-4,(int)ninjaPlayerY+6,2,TFT_BLACK);
    tft.fillCircle(ninjaX+NINJA_PW-4,(int)ninjaPlayerY+NINJA_PH-6,2,TFT_BLACK);
  }

  updateNinjaScoreDisplay();

  // Rush mode bar
  if(isRushMode&&(millis()-rushStartTime)<rushDuration){
    tft.setTextSize(1);tft.setCursor(225,200);tft.setTextColor(TFT_YELLOW,BG_COLOR);tft.print("RUSH!");
    int bw=80,bx=220,by=210;
    int ep=((millis()-rushStartTime)*100)/rushDuration;
    tft.fillRect(bx,by,bw,6,TFT_DARKGREY);
    tft.fillRect(bx,by,bw-(bw*ep/100),6,TFT_RED);
    unsigned long rt=(rushDuration-(millis()-rushStartTime))/1000;
    tft.setTextColor(TFT_WHITE,BG_COLOR);tft.setCursor(225,220);tft.print(rt);tft.print("s");
  } else {
    tft.fillRect(220,190,100,50,BG_COLOR);
  }
  delay(10);
}

void runNinjaGame(){
  static unsigned long lastBPress=0;
  if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){
    lastBPress=millis();ninjaPlaying=false;returnToMenu();return;
  }
  if(!ninjaPlaying){
    // On start screen
    if(digitalRead(BTN_SELECT)==LOW){delay(200);startNinjaGame();}
    return;
  }
  // Pause
  if(digitalRead(BTN_START)==LOW){
    delay(200);ninjaPaused=!ninjaPaused;
    if(ninjaPaused){
      tft.fillRoundRect(50,90,160,50,10,TFT_BLUE);
      tft.drawRoundRect(50,90,160,50,10,TFT_WHITE);
      tft.setTextColor(TFT_WHITE);tft.setTextSize(2);
      tft.setCursor(90,105);tft.print("PAUSED");
      tft.setTextSize(1);tft.setCursor(65,125);tft.print("START to Resume");
    } else {
      tft.fillRect(50,90,160,50,BG_COLOR);
    }
    delay(200);
  }
  if(ninjaPaused) return;

  // After game over: SELECT restarts, B already handled
  if(!ninjaPlaying){
    if(digitalRead(BTN_SELECT)==LOW){delay(200);startNinjaGame();}
    return;
  }
  updateNinjaGame();
}

// ================================================================
// =================== PONG GAME ==================================
// ================================================================
void drawPongSnow(){
  if(millis()-lastSnowUpdate<40) return;
  lastSnowUpdate=millis();
  for(int i=0;i<40;i++){
    tft.drawPixel((int)snowflakes[i].x,(int)snowflakes[i].y,TFT_BLACK);
    snowflakes[i].y+=snowflakes[i].speed;
    if(snowflakes[i].y>=screenHeight){snowflakes[i].y=0;snowflakes[i].x=random(0,screenWidth);}
    tft.drawPixel((int)snowflakes[i].x,(int)snowflakes[i].y,COLOR_SNOW_P);
  }
}

void drawPongElements(){
  tft.drawRect(PONG_LEFT-2,PONG_TOP-2,PONG_RIGHT-PONG_LEFT+4,PONG_BOTTOM-PONG_TOP+4,TFT_WHITE);
  for(int y=PONG_TOP;y<PONG_BOTTOM;y+=15)
    tft.fillRect(screenWidth/2-2,y,4,8,COLOR_GRID_P);
  tft.fillRect(PONG_LEFT,(int)botPaddleY,PADDLE_W,PADDLE_H,COLOR_BOT_P);
  tft.fillRect(PONG_RIGHT-PADDLE_W,(int)youPaddleY,PADDLE_W,PADDLE_H,COLOR_YOU_P);
  tft.fillCircle((int)ballX,(int)ballY,BALL_SZ,COLOR_BALL_P);
  tft.fillRect(screenWidth/2-1,PONG_TOP,2,PONG_BOTTOM-PONG_TOP,0x39E7);
}

void updatePongScoreUI(){
  tft.fillRect(0,0,screenWidth,22,TFT_BLACK);
  tft.setTextSize(2);tft.setTextColor(TFT_ORANGE);
  tft.setCursor(10,2);tft.print("HI:");tft.print(pongHighScore);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(screenWidth-120,2);tft.print("SC:");tft.print(pongScore);
}

void resetPongBall(){
  ballX=screenWidth/2;ballY=(PONG_TOP+PONG_BOTTOM)/2;
  ballDX=(random(0,2)==0)?pongBallSpeed:-pongBallSpeed;
  ballDY=((random(-12,13))/10.0);
  if(abs(ballDY)<0.4) ballDY=(ballDY>0)?0.4:-0.4;
  pongScore=0;
  botPaddleY=PONG_TOP+(PONG_BOTTOM-PONG_TOP-PADDLE_H)/2;
  youPaddleY=botPaddleY;
}

void pongGameOver(){
  pongActive=false;
  if(pongScore>pongHighScore){pongHighScore=pongScore;menuHighScores[6]=(int)pongHighScore;}
  tft.fillScreen(TFT_BLACK);
  int px=(screenWidth-260)/2,py=(screenHeight-130)/2;
  tft.fillRoundRect(px,py,260,130,15,TFT_WHITE);
  tft.fillRoundRect(px+4,py+4,252,122,12,TFT_BLACK);
  tft.setTextSize(3);tft.setTextColor(COLOR_BOT_P);
  tft.setCursor(px+35,py+30);tft.print("GAME OVER");
  tft.setTextSize(2);tft.setTextColor(TFT_WHITE);
  tft.setCursor(px+40,py+65);tft.print("Score: ");tft.print(pongScore);
  tft.setCursor(px+40,py+90);tft.print("Best:  ");tft.print(pongHighScore);
  tft.setTextColor(TFT_YELLOW);tft.setTextSize(1);
  tft.setCursor(px+40,py+115);tft.print("SELECT: Play Again   B: Menu");
}

void showStartScreenPong(){
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(5);tft.setTextColor(COLOR_YOU_P);
  tft.setCursor(screenWidth/2-90,screenHeight/2-50);tft.print("DINO");
  tft.setCursor(screenWidth/2-85,screenHeight/2+5);tft.print("PONG");
  tft.setTextSize(2);tft.setTextColor(TFT_WHITE);
  tft.setCursor(screenWidth/2-95,screenHeight/2+55);tft.print("SELECT: Start");
  tft.setTextSize(1);tft.setTextColor(TFT_GREEN);
  tft.setCursor(screenWidth/2-80,screenHeight-35);tft.print("UP/DN: Move  |  B: Back");
  tft.setCursor(screenWidth/2-65,screenHeight-22);tft.print("Best: ");tft.print(pongHighScore);
  for(int i=0;i<40;i++){
    snowflakes[i].x=random(0,screenWidth);
    snowflakes[i].y=random(0,screenHeight);
    snowflakes[i].speed=random(1,6)/10.0;
  }
  // Animate snow briefly
  for(int f=0;f<80;f++){drawPongSnow();delay(12);}
}

void runPongGame(){
  static unsigned long lastBPress=0;
  if(digitalRead(BTN_B)==LOW&&millis()-lastBPress>300){
    lastBPress=millis();pongActive=false;returnToMenu();return;
  }
  if(!pongActive){
    drawPongSnow();
    if(digitalRead(BTN_SELECT)==LOW){
      delay(60);
      pongActive=true;pongPaused=false;
      resetPongBall();
      tft.fillScreen(TFT_BLACK);
      updatePongScoreUI();drawPongElements();
    }
    return;
  }
  if(digitalRead(BTN_SELECT)==LOW){
    delay(60);
    pongPaused=!pongPaused;
    if(pongPaused){
      tft.setTextSize(3);tft.setTextColor(TFT_YELLOW);
      tft.setCursor(screenWidth/2-50,screenHeight/2-15);tft.print("PAUSED");
    } else {
      tft.fillRect(screenWidth/2-60,screenHeight/2-25,120,50,TFT_BLACK);
    }
    while(digitalRead(BTN_SELECT)==LOW) delay(10);
  }
  if(pongPaused){delay(10);return;}

  // Score timer
  if(millis()-lastPongScoreUpdate>250){
    pongScore++;lastPongScoreUpdate=millis();updatePongScoreUI();
  }

  int oldBX=(int)ballX,oldBY=(int)ballY,oldBotY=(int)botPaddleY,oldYouY=(int)youPaddleY;

  // Player input
  if(digitalRead(BTN_UP)==LOW&&youPaddleY>PONG_TOP) youPaddleY-=4.5;
  if(digitalRead(BTN_DOWN)==LOW&&youPaddleY<PONG_BOTTOM-PADDLE_H) youPaddleY+=4.5;

  // Perfect bot AI
  float targetBot=ballY-(PADDLE_H/2);
  if(targetBot<PONG_TOP) targetBot=PONG_TOP;
  if(targetBot>PONG_BOTTOM-PADDLE_H) targetBot=PONG_BOTTOM-PADDLE_H;
  botPaddleY=targetBot;

  // Boundaries
  if(youPaddleY<PONG_TOP) youPaddleY=PONG_TOP;
  if(youPaddleY>PONG_BOTTOM-PADDLE_H) youPaddleY=PONG_BOTTOM-PADDLE_H;

  // Ball update
  ballX+=ballDX; ballY+=ballDY;

  // Top/bottom bounce
  if(ballY-BALL_SZ<=PONG_TOP){ballY=PONG_TOP+BALL_SZ;ballDY=abs(ballDY);}
  if(ballY+BALL_SZ>=PONG_BOTTOM){ballY=PONG_BOTTOM-BALL_SZ;ballDY=-abs(ballDY);}

  // Bot paddle (left) collision
  if(ballX-BALL_SZ<=PONG_LEFT+PADDLE_W&&ballX+BALL_SZ>=PONG_LEFT&&
     ballY+BALL_SZ>=botPaddleY&&ballY-BALL_SZ<=botPaddleY+PADDLE_H){
    float hp=(ballY-botPaddleY)/PADDLE_H;
    ballDY=(hp-0.5)*4.5;ballDX=abs(ballDX)+0.05;
    if(abs(ballDY)>3.8)ballDY=(ballDY>0)?3.8:-3.8;
    if(ballDX>3.0)ballDX=3.0;
    ballX=PONG_LEFT+PADDLE_W+BALL_SZ;
  }
  // Player paddle (right) collision
  if(ballX+BALL_SZ>=PONG_RIGHT-PADDLE_W&&ballX-BALL_SZ<=PONG_RIGHT&&
     ballY+BALL_SZ>=youPaddleY&&ballY-BALL_SZ<=youPaddleY+PADDLE_H){
    float hp=(ballY-youPaddleY)/PADDLE_H;
    ballDY=(hp-0.5)*4.5;ballDX=-abs(ballDX)-0.05;
    if(abs(ballDY)>3.8)ballDY=(ballDY>0)?3.8:-3.8;
    if(abs(ballDX)>3.0)ballDX=(ballDX>0)?3.0:-3.0;
    ballX=PONG_RIGHT-PADDLE_W-BALL_SZ;
  }

  // Ball out
  if(ballX+BALL_SZ<=PONG_LEFT||ballX-BALL_SZ>=PONG_RIGHT){pongGameOver();return;}

  // Erase old
  tft.fillRect(PONG_LEFT,oldBotY,PADDLE_W,PADDLE_H,TFT_BLACK);
  tft.fillRect(PONG_RIGHT-PADDLE_W,oldYouY,PADDLE_W,PADDLE_H,TFT_BLACK);
  tft.fillCircle(oldBX,oldBY,BALL_SZ,TFT_BLACK);

  drawPongSnow();
  drawPongElements();
  delay(12);
}

// ================================================================
// ===================== SETUP & LOOP =============================
// ================================================================
void setup(){
  Serial.begin(115200);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_DOWN,  INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A,     INPUT_PULLUP);
  pinMode(BTN_B,     INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT,INPUT_PULLUP);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  screenWidth =tft.width();
  screenHeight=tft.height();

  randomSeed(analogRead(34));
  initDots();
  showMenu();
}

void loop(){
  static unsigned long lastBtnTime=0;
  static bool upPressed=false,downPressed=false;

  switch(currentGame){

    case GAME_MENU:{
      if(digitalRead(BTN_UP)==LOW&&!upPressed&&millis()-lastBtnTime>200){
        lastBtnTime=millis();upPressed=true;
        int old=selectedGame;
        selectedGame=(selectedGame>0)?selectedGame-1:MENU_ITEMS-1;
        updateMenuSelection(old,selectedGame);
      } else if(digitalRead(BTN_UP)==HIGH) upPressed=false;

      if(digitalRead(BTN_DOWN)==LOW&&!downPressed&&millis()-lastBtnTime>200){
        lastBtnTime=millis();downPressed=true;
        int old=selectedGame;
        selectedGame=(selectedGame<MENU_ITEMS-1)?selectedGame+1:0;
        updateMenuSelection(old,selectedGame);
      } else if(digitalRead(BTN_DOWN)==HIGH) downPressed=false;

      if(digitalRead(BTN_A)==LOW&&millis()-lastBtnTime>200){
        lastBtnTime=millis();
        switch(selectedGame){
          case 0: currentGame=GAME_SNAKE;    resetSnakeGame();         break;
          case 1: currentGame=GAME_FLAPPY;   showStartScreenFlappy();  break;
          case 2: currentGame=GAME_CONNECT4; resetGameC4();            break;
          case 3: currentGame=GAME_TICTACTOE;resetTTTGame();           break;
          case 4: currentGame=GAME_RPS;
                  drawMintBackground();updateScoreRPS();drawSelectionBoxes(); break;
          case 5: currentGame=GAME_NINJA;    showStartScreenNinja();   break;
          case 6: currentGame=GAME_PONG;     showStartScreenPong();    break;
        }
      }
      break;
    }

    case GAME_SNAKE:    runSnakeGame();    break;
    case GAME_FLAPPY:   runFlappyGame();   break;
    case GAME_CONNECT4: runConnect4Game(); break;
    case GAME_TICTACTOE:runTicTacToe();    break;
    case GAME_RPS:      runRPSGame();      break;
    case GAME_NINJA:    runNinjaGame();    break;
    case GAME_PONG:     runPongGame();     break;
  }

  delay(10);
}
