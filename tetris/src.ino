/*
 * =============================================
 *  TETRIS - ILI9341 / ESP32
 * =============================================
 *  BTN_UP     (13) = Rotate (CW)
 *  BTN_LEFT   (27) = Move Left
 *  BTN_RIGHT  (26) = Move Right
 *  BTN_START  (32) = Pause / Resume
 *  BTN_SELECT (25) = Title/Gameover: Start Game
 *                    Playing: Hard Drop
 *
 *  AUTO-LOCK: piece নিচে ঠেকলে 500ms পরে
 *  নিজে থেকেই lock হয়ে যাবে।
 *  Move/Rotate করলে lock timer reset হবে।
 *
 *  User_Setup.h (TFT_eSPI):
 *    #define ILI9341_DRIVER
 *    #define TFT_CS   5
 *    #define TFT_DC   21
 *    #define TFT_RST  22
 *    #define TFT_MOSI 23
 *    #define TFT_SCLK 18
 *    #define TFT_MISO 19
 * =============================================
 */

#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

// ── Pins ──────────────────────────────────────
#define BTN_UP     13
#define BTN_LEFT   27
#define BTN_RIGHT  26
#define BTN_START  32
#define BTN_SELECT 25

// ── Display ───────────────────────────────────
TFT_eSPI tft = TFT_eSPI();
#define SCREEN_W  320
#define SCREEN_H  240

// ── Board ─────────────────────────────────────
#define BOARD_COLS  10
#define BOARD_ROWS  20
#define CELL        11
#define BOARD_X     10
#define BOARD_Y     10
#define BOARD_W     (BOARD_COLS * CELL)
#define BOARD_H     (BOARD_ROWS * CELL)
#define PANEL_X     (BOARD_X + BOARD_W + 8)
#define PANEL_Y     10

// ── Colors ────────────────────────────────────
#define COL_BG     TFT_BLACK
#define COL_BORDER 0x4208
#define COL_GRID   0x2104
#define COL_LABEL  0x7BEF

const uint16_t PIECE_COLORS[9] = {
  TFT_BLACK,   // 0 empty
  0x07FF,      // 1 I  cyan
  0xFFE0,      // 2 O  yellow
  0xF81F,      // 3 T  magenta
  0x07E0,      // 4 S  green
  0xF800,      // 5 Z  red
  0x001F,      // 6 J  blue
  0xFC00,      // 7 L  orange
  0x2104,      // 8 ghost
};

// ── EEPROM ────────────────────────────────────
#define EEPROM_SIZE   8
#define HI_SCORE_ADDR 0
#define HI_LINES_ADDR 4

// ── Tetrominoes ───────────────────────────────
const int8_t PIECES[8][4][4][2] = {
  {{{0,0},{0,0},{0,0},{0,0}}}, // 0 unused
  // 1-I
  {{{1,0},{1,1},{1,2},{1,3}},{{0,2},{1,2},{2,2},{3,2}},
   {{2,0},{2,1},{2,2},{2,3}},{{0,1},{1,1},{2,1},{3,1}}},
  // 2-O
  {{{0,1},{0,2},{1,1},{1,2}},{{0,1},{0,2},{1,1},{1,2}},
   {{0,1},{0,2},{1,1},{1,2}},{{0,1},{0,2},{1,1},{1,2}}},
  // 3-T
  {{{0,1},{1,0},{1,1},{1,2}},{{0,1},{1,1},{1,2},{2,1}},
   {{1,0},{1,1},{1,2},{2,1}},{{0,1},{1,0},{1,1},{2,1}}},
  // 4-S
  {{{0,1},{0,2},{1,0},{1,1}},{{0,1},{1,1},{1,2},{2,2}},
   {{1,1},{1,2},{2,0},{2,1}},{{0,0},{1,0},{1,1},{2,1}}},
  // 5-Z
  {{{0,0},{0,1},{1,1},{1,2}},{{0,2},{1,1},{1,2},{2,1}},
   {{1,0},{1,1},{2,1},{2,2}},{{0,1},{1,0},{1,1},{2,0}}},
  // 6-J
  {{{0,0},{1,0},{1,1},{1,2}},{{0,1},{0,2},{1,1},{2,1}},
   {{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,0},{2,1}}},
  // 7-L
  {{{0,2},{1,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},
   {{1,0},{1,1},{1,2},{2,0}},{{0,0},{0,1},{1,1},{2,1}}},
};

// ── Game state ────────────────────────────────
enum GameState { ST_TITLE, ST_PLAYING, ST_PAUSED, ST_GAMEOVER };
GameState gameState;

uint8_t  board[BOARD_ROWS][BOARD_COLS];

int8_t   pieceType, pieceRot, pieceRow, pieceCol;
int8_t   nextType;
int8_t   ghostRow;

uint32_t score, hiScore, hiLines;
uint16_t lines;
uint8_t  level;

unsigned long dropTimer;
unsigned long dropInterval;

// Landing lock
#define LOCK_DELAY    500   // ms after landing before auto-lock
bool          landingMode;
unsigned long landingTimer;

// DAS
#define DAS_DELAY     180
#define DAS_INTERVAL  80
unsigned long dasTimer;
bool dasLeft, dasRight, dasActive;

// Button prev states
bool prevUp, prevLeft, prevRight, prevStart, prevSelect;

// ── EEPROM ────────────────────────────────────
void loadHiScores() {
  EEPROM.begin(EEPROM_SIZE);
  hiScore  = ((uint32_t)EEPROM.read(HI_SCORE_ADDR)   <<24)|
             ((uint32_t)EEPROM.read(HI_SCORE_ADDR+1) <<16)|
             ((uint32_t)EEPROM.read(HI_SCORE_ADDR+2) << 8)|
              (uint32_t)EEPROM.read(HI_SCORE_ADDR+3);
  hiLines  = ((uint32_t)EEPROM.read(HI_LINES_ADDR)   <<24)|
             ((uint32_t)EEPROM.read(HI_LINES_ADDR+1) <<16)|
             ((uint32_t)EEPROM.read(HI_LINES_ADDR+2) << 8)|
              (uint32_t)EEPROM.read(HI_LINES_ADDR+3);
  if (hiScore > 9999999) hiScore = 0;
  if (hiLines > 9999)    hiLines = 0;
}
void saveHiScores() {
  EEPROM.write(HI_SCORE_ADDR,   (hiScore>>24)&0xFF);
  EEPROM.write(HI_SCORE_ADDR+1, (hiScore>>16)&0xFF);
  EEPROM.write(HI_SCORE_ADDR+2, (hiScore>> 8)&0xFF);
  EEPROM.write(HI_SCORE_ADDR+3,  hiScore     &0xFF);
  EEPROM.write(HI_LINES_ADDR,   (hiLines>>24)&0xFF);
  EEPROM.write(HI_LINES_ADDR+1, (hiLines>>16)&0xFF);
  EEPROM.write(HI_LINES_ADDR+2, (hiLines>> 8)&0xFF);
  EEPROM.write(HI_LINES_ADDR+3,  hiLines     &0xFF);
  EEPROM.commit();
}

// ── Speed ─────────────────────────────────────
unsigned long speedForLevel(uint8_t lv) {
  const unsigned long s[] = {
    800,720,630,550,470,380,300,215,143,100,
     83, 83, 83, 67, 67, 67, 50, 50, 50, 33
  };
  return s[lv < 20 ? lv : 19];
}

// ── Piece helpers ─────────────────────────────
int8_t rndPiece() { return (int8_t)random(1,8); }

void getCells(int8_t type,int8_t rot,int8_t row,int8_t col,
              int8_t r[4],int8_t c[4]) {
  for (int i=0;i<4;i++) {
    r[i]=row+PIECES[type][rot][i][0];
    c[i]=col+PIECES[type][rot][i][1];
  }
}

bool isValid(int8_t type,int8_t rot,int8_t row,int8_t col) {
  int8_t r[4],c[4]; getCells(type,rot,row,col,r,c);
  for (int i=0;i<4;i++) {
    if (c[i]<0||c[i]>=BOARD_COLS) return false;
    if (r[i]>=BOARD_ROWS)          return false;
    if (r[i]>=0&&board[r[i]][c[i]]) return false;
  }
  return true;
}

int8_t calcGhost() {
  int8_t g=pieceRow;
  while (isValid(pieceType,pieceRot,g+1,pieceCol)) g++;
  return g;
}

// ── Draw ──────────────────────────────────────
void drawCell(int sx,int sy,uint16_t color,bool ghost=false) {
  if (ghost) {
    tft.drawRect(sx,sy,CELL,CELL,color);
    tft.drawRect(sx+1,sy+1,CELL-2,CELL-2,color);
  } else {
    tft.fillRect(sx,sy,CELL,CELL,color);
    tft.drawFastHLine(sx,sy,CELL,0xFFFF);
    tft.drawFastVLine(sx,sy,CELL,0xFFFF);
    uint16_t dk=((color>>11&0x1F)>>1)<<11|((color>>5&0x3F)>>1)<<5|((color&0x1F)>>1);
    tft.drawFastHLine(sx,sy+CELL-1,CELL,dk);
    tft.drawFastVLine(sx+CELL-1,sy,CELL,dk);
  }
}

void drawBoardCell(int row,int col) {
  int sx=BOARD_X+col*CELL, sy=BOARD_Y+row*CELL;
  if (!board[row][col]) {
    tft.fillRect(sx,sy,CELL,CELL,COL_BG);
    tft.drawPixel(sx,sy,COL_GRID);
  } else drawCell(sx,sy,PIECE_COLORS[board[row][col]]);
}

void drawBoard() {
  for (int r=0;r<BOARD_ROWS;r++)
    for (int c=0;c<BOARD_COLS;c++) drawBoardCell(r,c);
}

void drawBorder() {
  tft.drawRect(BOARD_X-1,BOARD_Y-1,BOARD_W+2,BOARD_H+2,COL_BORDER);
  tft.drawRect(BOARD_X-2,BOARD_Y-2,BOARD_W+4,BOARD_H+4,COL_BORDER);
}

void restoreBoardUnder(int8_t type,int8_t rot,int8_t row,int8_t col) {
  int8_t r[4],c[4]; getCells(type,rot,row,col,r,c);
  for (int i=0;i<4;i++)
    if (r[i]>=0&&r[i]<BOARD_ROWS&&c[i]>=0&&c[i]<BOARD_COLS&&board[r[i]][c[i]])
      drawBoardCell(r[i],c[i]);
}

void erasePiece(int8_t type,int8_t rot,int8_t row,int8_t col) {
  int8_t r[4],c[4]; getCells(type,rot,row,col,r,c);
  for (int i=0;i<4;i++) {
    if (r[i]<0||r[i]>=BOARD_ROWS||c[i]<0||c[i]>=BOARD_COLS) continue;
    if (board[r[i]][c[i]]) continue;
    int sx=BOARD_X+c[i]*CELL, sy=BOARD_Y+r[i]*CELL;
    tft.fillRect(sx,sy,CELL,CELL,COL_BG);
    tft.drawPixel(sx,sy,COL_GRID);
  }
}

void drawPiece(int8_t type,int8_t rot,int8_t row,int8_t col,bool ghost=false) {
  int8_t r[4],c[4]; getCells(type,rot,row,col,r,c);
  for (int i=0;i<4;i++) {
    if (r[i]<0||r[i]>=BOARD_ROWS||c[i]<0||c[i]>=BOARD_COLS) continue;
    drawCell(BOARD_X+c[i]*CELL,BOARD_Y+r[i]*CELL,
             ghost?PIECE_COLORS[8]:PIECE_COLORS[type],ghost);
  }
}

// ── Panel ─────────────────────────────────────
void panelLabel(const char* s,int y) {
  tft.setTextColor(COL_LABEL);tft.setTextSize(1);
  tft.setCursor(PANEL_X,y);tft.print(s);
}
void panelValue(uint32_t v,int y,uint16_t col=TFT_WHITE) {
  tft.fillRect(PANEL_X,y,105,10,COL_BG);
  tft.setTextColor(col);tft.setTextSize(1);
  tft.setCursor(PANEL_X,y);tft.print(v);
}

void drawNextPiece() {
  int nx=PANEL_X,ny=PANEL_Y+20;
  tft.fillRect(nx,ny,52,52,COL_BG);
  tft.drawRect(nx-1,ny-1,54,54,COL_BORDER);
  for (int i=0;i<4;i++) {
    int8_t pr=PIECES[nextType][0][i][0];
    int8_t pc=PIECES[nextType][0][i][1];
    drawCell(nx+pc*12+2,ny+pr*12+2,PIECE_COLORS[nextType]);
  }
}

void drawPanel() {
  panelLabel("NEXT",  PANEL_Y+10);  drawNextPiece();
  panelLabel("SCORE", PANEL_Y+82);  panelValue(score,   PANEL_Y+92,  TFT_YELLOW);
  panelLabel("BEST",  PANEL_Y+108); panelValue(hiScore, PANEL_Y+118, 0x07FF);
  panelLabel("LINES", PANEL_Y+134); panelValue(lines,   PANEL_Y+144, TFT_GREEN);
  panelLabel("LEVEL", PANEL_Y+160); panelValue(level,   PANEL_Y+170, TFT_MAGENTA);
  tft.setTextColor(0x4208);tft.setTextSize(1);
  tft.setCursor(PANEL_X,PANEL_Y+192);tft.print("UP :Rotate");
  tft.setCursor(PANEL_X,PANEL_Y+203);tft.print("SEL:HardDrop");
  tft.setCursor(PANEL_X,PANEL_Y+214);tft.print("STA:Pause");
}

// ── Scoring ───────────────────────────────────
void addScore(int cleared) {
  const uint16_t pts[]={0,100,300,500,800};
  score+=(uint32_t)pts[cleared]*(level+1);
  lines+=cleared;
  level=lines/10; if (level>19) level=19;
  dropInterval=speedForLevel(level);
}

void checkLines() {
  uint8_t cnt=0, fullR[4];
  for (int r=BOARD_ROWS-1;r>=0;r--) {
    bool full=true;
    for (int c=0;c<BOARD_COLS;c++) if (!board[r][c]){full=false;break;}
    if (full) fullR[cnt++]=r;
  }
  if (!cnt) return;
  // Flash
  for (int f=0;f<4;f++) {
    uint16_t fc=(f%2==0)?TFT_WHITE:COL_BG;
    for (int i=0;i<cnt;i++) tft.fillRect(BOARD_X,BOARD_Y+fullR[i]*CELL,BOARD_W,CELL,fc);
    delay(55);
  }
  // Sort ascending
  for (int a=0;a<cnt-1;a++)
    for (int b=a+1;b<cnt;b++)
      if (fullR[a]>fullR[b]){uint8_t t=fullR[a];fullR[a]=fullR[b];fullR[b]=t;}
  // Remove
  for (int i=0;i<cnt;i++) {
    int row=fullR[i]-i;
    for (int r=row;r>0;r--)
      for (int c=0;c<BOARD_COLS;c++) board[r][c]=board[r-1][c];
    for (int c=0;c<BOARD_COLS;c++) board[0][c]=0;
  }
  addScore(cnt);
  drawBoard();
  panelValue(score,  PANEL_Y+92,  TFT_YELLOW);
  panelValue(lines,  PANEL_Y+144, TFT_GREEN);
  panelValue(level,  PANEL_Y+170, TFT_MAGENTA);
}

// ── Lock ──────────────────────────────────────
bool lockPiece() {
  int8_t r[4],c[4];
  getCells(pieceType,pieceRot,pieceRow,pieceCol,r,c);
  for (int i=0;i<4;i++) {
    if (r[i]<0) return false;        // above board = game over
    board[r[i]][c[i]]=pieceType;
  }
  for (int i=0;i<4;i++)
    if (r[i]>=0&&r[i]<BOARD_ROWS) drawBoardCell(r[i],c[i]);
  checkLines();
  return true;
}

// ── Spawn ─────────────────────────────────────
bool spawnPiece() {
  pieceType=nextType; nextType=rndPiece();
  pieceRot=0; pieceRow=-1;
  pieceCol=BOARD_COLS/2-2;
  if (!isValid(pieceType,pieceRot,pieceRow,pieceCol)) return false;
  ghostRow=calcGhost();
  drawPiece(pieceType,pieceRot,ghostRow,pieceCol,true);
  drawPiece(pieceType,pieceRot,pieceRow,pieceCol);
  drawNextPiece();
  landingMode=false; landingTimer=0;
  return true;
}

// ── Game over ─────────────────────────────────
void triggerGameOver() {
  if (score>hiScore) hiScore=score;
  if (lines>hiLines) hiLines=lines;
  saveHiScores();
  gameState=ST_GAMEOVER;

  tft.fillRect(22,82,215,80,0x0821);
  tft.drawRect(22,82,215,80,TFT_RED);
  tft.setTextColor(TFT_RED);tft.setTextSize(2);
  tft.setCursor(50,90);tft.print("GAME OVER");
  tft.setTextSize(1);tft.setTextColor(TFT_WHITE);
  tft.setCursor(35,114);tft.print("Score : ");tft.print(score);
  tft.setCursor(35,128);tft.print("Lines : ");tft.print(lines);
  if (score>=hiScore||lines>=hiLines) {
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(52,142);tft.print("** NEW RECORD! **");
  } else {
    tft.setTextColor(0x7BEF);
    tft.setCursor(35,142);tft.print("Best  : ");tft.print(hiScore);
  }
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(38,154);tft.print("SELECT to play again");
}

// ── Hard drop ─────────────────────────────────
void doHardDrop() {
  erasePiece(pieceType,pieceRot,ghostRow,pieceCol);
  erasePiece(pieceType,pieceRot,pieceRow,pieceCol);
  restoreBoardUnder(pieceType,pieceRot,pieceRow,pieceCol);
  int dropped=0;
  while (isValid(pieceType,pieceRot,pieceRow+1,pieceCol)) {pieceRow++;dropped++;}
  score+=dropped*2;
  panelValue(score,PANEL_Y+92,TFT_YELLOW);
  if (!lockPiece())  { triggerGameOver(); return; }
  if (!spawnPiece()) { triggerGameOver(); return; }
  dropTimer=millis();
}

// ── Rotate ────────────────────────────────────
void doRotate() {
  int8_t newRot=(pieceRot+1)%4;
  int8_t kicks[]={0,-1,1,-2,2};
  for (int k=0;k<5;k++) {
    int8_t nc=pieceCol+kicks[k];
    if (isValid(pieceType,newRot,pieceRow,nc)) {
      erasePiece(pieceType,pieceRot,ghostRow,pieceCol);
      erasePiece(pieceType,pieceRot,pieceRow,pieceCol);
      restoreBoardUnder(pieceType,pieceRot,pieceRow,pieceCol);
      pieceRot=newRot; pieceCol=nc;
      ghostRow=calcGhost();
      drawPiece(pieceType,pieceRot,ghostRow,pieceCol,true);
      drawPiece(pieceType,pieceRot,pieceRow,pieceCol);
      if (landingMode) landingTimer=millis(); // reset lock delay
      return;
    }
  }
}

// ── Move left/right ───────────────────────────
void doMove(int dc) {
  if (!isValid(pieceType,pieceRot,pieceRow,pieceCol+dc)) return;
  erasePiece(pieceType,pieceRot,ghostRow,pieceCol);
  erasePiece(pieceType,pieceRot,pieceRow,pieceCol);
  restoreBoardUnder(pieceType,pieceRot,pieceRow,pieceCol);
  pieceCol+=dc;
  ghostRow=calcGhost();
  drawPiece(pieceType,pieceRot,ghostRow,pieceCol,true);
  drawPiece(pieceType,pieceRot,pieceRow,pieceCol);
  if (landingMode) landingTimer=millis(); // reset lock delay
}

// ── Auto drop one row ─────────────────────────
// Returns false if locked and spawn failed (game over)
bool doAutoDrop() {
  if (isValid(pieceType,pieceRot,pieceRow+1,pieceCol)) {
    erasePiece(pieceType,pieceRot,ghostRow,pieceCol);
    erasePiece(pieceType,pieceRot,pieceRow,pieceCol);
    restoreBoardUnder(pieceType,pieceRot,pieceRow,pieceCol);
    pieceRow++;
    ghostRow=calcGhost();
    drawPiece(pieceType,pieceRot,ghostRow,pieceCol,true);
    drawPiece(pieceType,pieceRot,pieceRow,pieceCol);
    return true;
  }
  // Can't drop — lock
  erasePiece(pieceType,pieceRot,ghostRow,pieceCol);
  if (!lockPiece())  { triggerGameOver(); return false; }
  if (!spawnPiece()) { triggerGameOver(); return false; }
  dropTimer=millis();
  return true;
}

// ── Title screen ──────────────────────────────
void drawTitle() {
  tft.fillScreen(COL_BG);
  for (int y=0;y<SCREEN_H;y+=3)
    tft.drawFastHLine(0,y,SCREEN_W,0x0821);

  tft.setTextSize(4);tft.setTextColor(0x07FF);
  tft.setCursor(42,28);tft.print("TETRIS");

  tft.setTextSize(1);tft.setTextColor(TFT_YELLOW);
  tft.setCursor(72,76);tft.print("ESP32 + ILI9341 Edition");

  uint16_t deco[]={0x07FF,0xFFE0,0xF81F,0x07E0,0xF800,0x001F,0xFC00};
  for (int i=0;i<7;i++) tft.fillRect(70+i*26,88,24,10,deco[i]);

  tft.setTextColor(COL_LABEL);
  tft.setCursor(60,112);tft.print("BEST SCORE : ");
  tft.setTextColor(TFT_YELLOW);tft.print(hiScore);
  tft.setTextColor(COL_LABEL);
  tft.setCursor(60,126);tft.print("BEST LINES : ");
  tft.setTextColor(TFT_GREEN);tft.print(hiLines);

  tft.setTextColor(TFT_WHITE);
  tft.setCursor(55,148);tft.print("LEFT / RIGHT  :  Move");
  tft.setCursor(55,162);tft.print("UP            :  Rotate");
  tft.setCursor(55,176);tft.print("SELECT        :  Hard Drop");
  tft.setCursor(55,190);tft.print("START         :  Pause");

  // prompt (blinking handled in loop)
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(62,SCREEN_H-14);
  tft.print("Press SELECT to Start!");
}

// ── Pause overlay ─────────────────────────────
void drawPaused() {
  tft.fillRect(25,98,175,46,0x0821);
  tft.drawRect(25,98,175,46,TFT_YELLOW);
  tft.setTextColor(TFT_YELLOW);tft.setTextSize(2);
  tft.setCursor(43,106);tft.print("** PAUSED **");
  tft.setTextSize(1);tft.setTextColor(TFT_WHITE);
  tft.setCursor(43,126);tft.print("START to continue");
}
void erasePaused() {
  for (int r=9;r<=13&&r<BOARD_ROWS;r++)
    for (int c=0;c<BOARD_COLS;c++) drawBoardCell(r,c);
  // Also redraw ghost + piece
  drawPiece(pieceType,pieceRot,ghostRow,pieceCol,true);
  drawPiece(pieceType,pieceRot,pieceRow,pieceCol);
}

// ── Init game ─────────────────────────────────
void initGame() {
  memset(board,0,sizeof(board));
  score=0;lines=0;level=0;
  dropInterval=speedForLevel(0);
  dropTimer=millis();
  landingMode=false;
  dasLeft=dasRight=dasActive=false;
  nextType=rndPiece();
  tft.fillScreen(COL_BG);
  drawBorder();drawBoard();drawPanel();
  spawnPiece();
  gameState=ST_PLAYING;
}

// ── Setup ─────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(BTN_UP,    INPUT_PULLUP);
  pinMode(BTN_LEFT,  INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_SELECT,INPUT_PULLUP);

  randomSeed(analogRead(0)^analogRead(1)^millis());

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(COL_BG);

  loadHiScores();
  prevUp=prevLeft=prevRight=prevStart=prevSelect=HIGH;

  gameState=ST_TITLE;
  drawTitle();
}

// ── Loop ──────────────────────────────────────
void loop() {
  unsigned long now=millis();

  bool curUp    =digitalRead(BTN_UP);
  bool curLeft  =digitalRead(BTN_LEFT);
  bool curRight =digitalRead(BTN_RIGHT);
  bool curStart =digitalRead(BTN_START);
  bool curSelect=digitalRead(BTN_SELECT);

  // ===== TITLE =====
  if (gameState==ST_TITLE) {
    static bool blinkOn=true;
    static unsigned long blinkT=0;
    if (now-blinkT>550) {
      blinkT=now;blinkOn=!blinkOn;
      tft.setTextColor(blinkOn?TFT_CYAN:COL_BG);
      tft.setTextSize(1);
      tft.setCursor(62,SCREEN_H-14);
      tft.print("Press SELECT to Start!");
    }
    if (prevSelect==HIGH&&curSelect==LOW) initGame();
  }

  // ===== GAME OVER =====
  else if (gameState==ST_GAMEOVER) {
    if (prevSelect==HIGH&&curSelect==LOW) {
      gameState=ST_TITLE; drawTitle();
    }
  }

  // ===== PAUSED =====
  else if (gameState==ST_PAUSED) {
    if (prevStart==HIGH&&curStart==LOW) {
      erasePaused();
      gameState=ST_PLAYING;
      dropTimer=millis();
      if (landingMode) landingTimer=millis();
    }
  }

  // ===== PLAYING =====
  else if (gameState==ST_PLAYING) {

    // START → pause
    if (prevStart==HIGH&&curStart==LOW) {
      gameState=ST_PAUSED; drawPaused();
      goto done;
    }

    // UP → rotate
    if (prevUp==HIGH&&curUp==LOW) doRotate();

    // SELECT → hard drop
    if (prevSelect==HIGH&&curSelect==LOW) {
      doHardDrop();
      goto done;
    }

    // LEFT with DAS
    if (prevLeft==HIGH&&curLeft==LOW) {
      doMove(-1);
      dasLeft=true; dasRight=false; dasActive=false; dasTimer=now;
    }
    if (curLeft==LOW&&dasLeft) {
      if (!dasActive&&now-dasTimer>DAS_DELAY){dasActive=true;dasTimer=now;}
      if (dasActive&&now-dasTimer>DAS_INTERVAL){doMove(-1);dasTimer=now;}
    }
    if (curLeft==HIGH){dasLeft=false;dasActive=false;}

    // RIGHT with DAS
    if (prevRight==HIGH&&curRight==LOW) {
      doMove(1);
      dasRight=true;dasLeft=false;dasActive=false;dasTimer=now;
    }
    if (curRight==LOW&&dasRight) {
      if (!dasActive&&now-dasTimer>DAS_DELAY){dasActive=true;dasTimer=now;}
      if (dasActive&&now-dasTimer>DAS_INTERVAL){doMove(1);dasTimer=now;}
    }
    if (curRight==HIGH){dasRight=false;dasActive=false;}

    // ── Landing lock logic ──────────────────
    bool canFall=isValid(pieceType,pieceRot,pieceRow+1,pieceCol);

    if (canFall) {
      // Piece can still fall — disable landing mode
      landingMode=false;
      // Auto drop at current speed
      if (now-dropTimer>=dropInterval) {
        doAutoDrop();
        dropTimer=now;
      }
    } else {
      // Piece is resting on something
      if (!landingMode) {
        // Just landed — start lock delay
        landingMode=true;
        landingTimer=now;
      }
      // After LOCK_DELAY ms without move/rotate → auto lock
      if (now-landingTimer>=LOCK_DELAY) {
        landingMode=false;
        erasePiece(pieceType,pieceRot,ghostRow,pieceCol);
        if (!lockPiece())  { triggerGameOver(); goto done; }
        if (!spawnPiece()) { triggerGameOver(); goto done; }
        dropTimer=millis();
      }
    }
  }

done:
  prevUp    =curUp;
  prevLeft  =curLeft;
  prevRight =curRight;
  prevStart =curStart;
  prevSelect=curSelect;
  delay(8);
}
