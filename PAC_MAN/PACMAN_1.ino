// ================================================================
//  CLASSIC PAC-MAN  |  ILI9341 320x240  |  TFT_eSPI  |  ESP32
// ================================================================
//  Buttons:  UP=13  DOWN=12  LEFT=27  RIGHT=26
//            A=14   B=33    START=32  SELECT=25
//
//  Make sure your TFT_eSPI User_Setup.h has:
//    #define ILI9341_DRIVER
//    and correct MOSI/MISO/SCK/CS/DC/RST pins for your wiring
// ================================================================

#include <TFT_eSPI.h>
#include <SPI.h>
#include <EEPROM.h>

// ── Button Pins ──────────────────────────────────────────────────
#define BTN_UP     13
#define BTN_DOWN   12
#define BTN_LEFT   27
#define BTN_RIGHT  26
#define BTN_A      14
#define BTN_B      33
#define BTN_START  32
#define BTN_SELECT 25

#define EEPROM_SIZE 4

TFT_eSPI tft = TFT_eSPI();

// ── Display constants ────────────────────────────────────────────
#define SCREEN_W   320
#define SCREEN_H   240
#define UI_H        24   // top bar height
#define GAME_H     (SCREEN_H - UI_H)  // 216px for game

// ── Maze dimensions ──────────────────────────────────────────────
// Classic Pac-Man maze: 28 cols x 31 rows  (we use a simplified 21x11)
// Tile size fitted: 320/21 ~= 15px wide, 216/11 ~= 19px tall → use 14x14 with centering
#define COLS  21
#define ROWS  11
#define TILE  14   // px per tile

// Horizontal offset to center maze: (320 - 21*14) / 2 = (320-294)/2 = 13
// Vertical offset: UI_H + (216 - 11*14)/2 = 24 + (216-154)/2 = 24+31 = 55... 
// Actually let's just start at UI_H with small padding
#define OX    13   // x offset
#define OY    (UI_H + 7)  // y offset  (31px top padding in game area)

// ── 16-bit colors ────────────────────────────────────────────────
#define C_BLACK    0x0000
#define C_WHITE    0xFFFF
#define C_YELLOW   0xFFE0
#define C_RED      0xF800
#define C_GREEN    0x07E0
#define C_BLUE     0x001F
#define C_CYAN     0x07FF
#define C_MAGENTA  0xF81F
#define C_ORANGE   0xFD20
#define C_PINK     0xFC18
#define C_DKBLUE   0x000B   // wall color
#define C_LBLUE    0x2D7F   // wall border
#define C_DOT      0xFFFB   // pellet color (near white)
#define C_PWRPEL   0xFFE0   // power pellet (yellow)

// ── Maze cell values ─────────────────────────────────────────────
#define M_EMPTY  0   // walkable, no dot
#define M_DOT    1   // has dot
#define M_WALL   2   // wall block
#define M_PPEL   3   // power pellet
#define M_DOOR   4   // ghost house door (walkable for ghosts only)

// ── Classic fixed maze (21 cols x 11 rows) ───────────────────────
// 2=wall, 1=dot, 3=power pellet, 0=empty, 4=ghost door
const byte MAZE_TEMPLATE[ROWS][COLS] = {
  { 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 },
  { 2,3,1,1,1,1,1,1,1,2,0,2,1,1,1,1,1,1,1,3,2 },
  { 2,1,2,2,1,2,2,2,1,2,0,2,1,2,2,2,1,2,2,1,2 },
  { 2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2 },
  { 2,1,2,2,1,2,4,2,2,2,2,2,2,2,1,2,1,2,2,1,2 },
  { 2,1,1,1,1,2,0,0,0,0,0,0,0,2,1,1,1,1,1,1,2 },
  { 2,1,2,2,1,2,2,2,2,2,2,2,2,2,1,2,1,2,2,1,2 },
  { 2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2 },
  { 2,1,2,2,1,2,2,2,1,2,0,2,1,2,2,2,1,2,2,1,2 },
  { 2,3,1,1,1,1,1,1,1,2,0,2,1,1,1,1,1,1,1,3,2 },
  { 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2 },
};

byte maze[ROWS][COLS];   // live game copy

// ── Pac-Man state ────────────────────────────────────────────────
struct Entity {
  int x, y;       // tile coords
  int dir;         // 0=R 1=D 2=L 3=U
  int nextDir;
};

Entity pac;

// ── Ghost state ──────────────────────────────────────────────────
#define NUM_GHOSTS 4
struct Ghost {
  int x, y;
  int dir;
  uint16_t color;
  bool frightened;   // blue / edible
  int homeX, homeY; // scatter target
};

Ghost ghosts[NUM_GHOSTS];

// ── Game state ───────────────────────────────────────────────────
int  score      = 0;
int  highScore  = 0;
int  lives      = 3;
int  level      = 1;
bool gameOver   = false;
bool won        = false;
int  dotsLeft   = 0;
bool mouthOpen  = true;

// ── Power pellet (frightened) state ─────────────────────────────
bool  frightenedMode    = false;
unsigned long frightenStart = 0;
#define FRIGHTEN_MS  8000   // 8 seconds

// ── Extra life (heart) ───────────────────────────────────────────
bool heartActive    = false;
int  heartX         = -1, heartY = -1;
unsigned long heartSpawnTime  = 0;
unsigned long nextHeartSpawn  = 15000;
#define HEART_DURATION  7000  // 7 seconds visible

// ── Timing ───────────────────────────────────────────────────────
unsigned long lastPacMove   = 0;
unsigned long lastGhostMove = 0;
unsigned long lastMouth     = 0;
unsigned long lastBlink     = 0;

// ── Direction helpers ────────────────────────────────────────────
const int DX[4] = { 1, 0, -1, 0 };
const int DY[4] = { 0, 1,  0,-1 };

// ── BFS for ghost AI ─────────────────────────────────────────────
struct Point { int x, y; };

Point bfsNext(int sx, int sy, int tx, int ty, bool isGhost) {
  if (sx == tx && sy == ty) return {sx, sy};

  Point parent[COLS][ROWS];
  bool  visited[COLS][ROWS];
  memset(visited, 0, sizeof(visited));

  Point q[COLS * ROWS];
  int head = 0, tail = 0;
  q[tail++] = {sx, sy};
  visited[sx][sy] = true;
  bool found = false;

  while (head < tail) {
    Point cur = q[head++];
    if (cur.x == tx && cur.y == ty) { found = true; break; }
    for (int d = 0; d < 4; d++) {
      int nx = cur.x + DX[d];
      int ny = cur.y + DY[d];
      if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) continue;
      if (visited[nx][ny]) continue;
      byte cell = maze[ny][nx];
      // Ghosts can pass door, Pac-Man cannot
      if (cell == M_WALL) continue;
      if (!isGhost && cell == M_DOOR) continue;
      visited[nx][ny] = true;
      parent[nx][ny] = cur;
      q[tail++] = {nx, ny};
    }
  }

  if (!found) return {sx, sy};

  Point step = {tx, ty};
  while (parent[step.x][step.y].x != sx || parent[step.x][step.y].y != sy)
    step = parent[step.x][step.y];
  return step;
}

// ── Pixel helpers ────────────────────────────────────────────────
// Convert tile → screen pixel (top-left of tile)
inline int tileToScreenX(int tx) { return OX + tx * TILE; }
inline int tileToScreenY(int ty) { return OY + ty * TILE; }

// Center of tile
inline int tileCX(int tx) { return OX + tx * TILE + TILE / 2; }
inline int tileCY(int ty) { return OY + ty * TILE + TILE / 2; }

// ================================================================
//  DRAW FUNCTIONS
// ================================================================

// ── Draw single wall tile with styled border ──────────────────────
void drawWall(int tx, int ty) {
  int px = tileToScreenX(tx);
  int py = tileToScreenY(ty);
  tft.fillRect(px, py, TILE - 1, TILE - 1, C_DKBLUE);
  tft.drawRect(px, py, TILE - 1, TILE - 1, C_LBLUE);
}

// ── Draw dot ─────────────────────────────────────────────────────
void drawDot(int tx, int ty) {
  tft.fillCircle(tileCX(tx), tileCY(ty), 2, C_DOT);
}

// ── Draw power pellet ─────────────────────────────────────────────
void drawPowerPellet(int tx, int ty) {
  tft.fillCircle(tileCX(tx), tileCY(ty), 5, C_PWRPEL);
}

// ── Erase tile (fill black) ───────────────────────────────────────
void clearTile(int tx, int ty) {
  tft.fillRect(tileToScreenX(tx), tileToScreenY(ty), TILE, TILE, C_BLACK);
}

// ── Draw pixel heart ─────────────────────────────────────────────
void drawHeart(int px, int py, uint16_t color) {
  tft.fillRect(px+1, py,   3, 1, color);
  tft.fillRect(px+5, py,   3, 1, color);
  tft.fillRect(px,   py+1, 9, 3, color);
  tft.fillRect(px+1, py+4, 7, 1, color);
  tft.fillRect(px+2, py+5, 5, 1, color);
  tft.fillRect(px+3, py+6, 3, 1, color);
  tft.drawPixel(px+4, py+7, color);
}

// ── Draw heart on maze tile ───────────────────────────────────────
void drawHeartTile(int tx, int ty) {
  drawHeart(tileToScreenX(tx) + 2, tileToScreenY(ty) + 2, C_RED);
}

// ── Draw entire maze from scratch ─────────────────────────────────
void drawMaze() {
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      switch (maze[y][x]) {
        case M_WALL:  drawWall(x, y);        break;
        case M_DOT:   drawDot(x, y);         break;
        case M_PPEL:  drawPowerPellet(x, y); break;
        default:      clearTile(x, y);       break;
      }
    }
  }
  // Draw heart if active
  if (heartActive) drawHeartTile(heartX, heartY);
}

// ── Top UI bar ────────────────────────────────────────────────────
void drawUI() {
  tft.fillRect(0, 0, SCREEN_W, UI_H, C_BLACK);
  tft.drawLine(0, UI_H - 1, SCREEN_W, UI_H - 1, C_CYAN);

  // Score
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(1);
  tft.setCursor(4, 4);
  tft.print("SCORE");
  tft.setTextColor(C_YELLOW, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(4, 12);
  // right-align score in 6 digits
  char buf[10];
  sprintf(buf, "%06d", score);
  tft.print(buf);

  // High score
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(1);
  tft.setCursor(110, 4);
  tft.print("BEST");
  tft.setTextColor(C_CYAN, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(110, 12);
  sprintf(buf, "%06d", highScore);
  tft.print(buf);

  // Level
  tft.setTextColor(C_WHITE, C_BLACK);
  tft.setTextSize(1);
  tft.setCursor(225, 4);
  tft.print("LVL");
  tft.setTextColor(C_ORANGE, C_BLACK);
  tft.setTextSize(2);
  tft.setCursor(225, 12);
  tft.print(level);

  // Lives (hearts)
  for (int i = 0; i < lives; i++) {
    drawHeart(265 + i * 18, 8, C_RED);
  }
}

// ── Pac-Man drawing ───────────────────────────────────────────────
void drawPacman(uint16_t color) {
  int cx = tileCX(pac.x);
  int cy = tileCY(pac.y);
  tft.fillCircle(cx, cy, TILE / 2 - 1, color);
  if (mouthOpen) {
    // Mouth triangle based on direction
    int mx1, my1, mx2, my2, r = TILE / 2 + 1;
    switch (pac.dir) {
      case 0: // right
        tft.fillTriangle(cx, cy, cx+r, cy-4, cx+r, cy+4, C_BLACK); break;
      case 1: // down
        tft.fillTriangle(cx, cy, cx-4, cy+r, cx+4, cy+r, C_BLACK); break;
      case 2: // left
        tft.fillTriangle(cx, cy, cx-r, cy-4, cx-r, cy+4, C_BLACK); break;
      case 3: // up
        tft.fillTriangle(cx, cy, cx-4, cy-r, cx+4, cy-r, C_BLACK); break;
    }
    // Eye
    int ex = cx + (pac.dir==0?2 : pac.dir==2?-2 : 0);
    int ey = cy + (pac.dir==3?-3 : pac.dir==1?0 : -3);
    tft.fillCircle(ex, ey, 1, C_BLACK);
  }
}

void erasePacman() {
  tft.fillRect(tileToScreenX(pac.x), tileToScreenY(pac.y), TILE, TILE, C_BLACK);
  // Restore tile content under pacman
  byte cell = maze[pac.y][pac.x];
  if (cell == M_DOT)  drawDot(pac.x, pac.y);
  if (cell == M_PPEL) drawPowerPellet(pac.x, pac.y);
}

// ── Ghost drawing ─────────────────────────────────────────────────
void drawGhost(int i) {
  int cx = tileCX(ghosts[i].x);
  int cy = tileCY(ghosts[i].y);
  int r  = TILE / 2 - 1;
  int gx = cx - r;
  int gy = cy - r;
  int gw = r * 2;

  // Frightened → flash white/blue in last 2 seconds
  uint16_t bodyColor;
  if (ghosts[i].frightened) {
    unsigned long elapsed = millis() - frightenStart;
    if (elapsed > FRIGHTEN_MS - 2000 && (millis() / 250) % 2 == 0)
      bodyColor = C_WHITE;
    else
      bodyColor = C_BLUE;
  } else {
    bodyColor = ghosts[i].color;
  }

  // Body (rounded top)
  tft.fillCircle(cx, gy + r, r, bodyColor);
  tft.fillRect(gx, gy + r, gw, r, bodyColor);

  // Wavy bottom (3 bumps)
  int bw = gw / 3;
  for (int b = 0; b < 3; b++) {
    tft.fillCircle(gx + bw * b + bw / 2, gy + gw - 1, bw / 2, C_BLACK);
  }

  // Eyes (only if not frightened)
  if (!ghosts[i].frightened) {
    tft.fillCircle(cx - 2, gy + r - 1, 2, C_WHITE);
    tft.fillCircle(cx + 2, gy + r - 1, 2, C_WHITE);
    tft.fillCircle(cx - 2 + DX[ghosts[i].dir], gy + r - 1 + DY[ghosts[i].dir], 1, C_BLUE);
    tft.fillCircle(cx + 2 + DX[ghosts[i].dir], gy + r - 1 + DY[ghosts[i].dir], 1, C_BLUE);
  } else {
    // X eyes when frightened
    tft.drawLine(cx-3, gy+r-3, cx-1, gy+r-1, C_WHITE);
    tft.drawLine(cx-1, gy+r-3, cx-3, gy+r-1, C_WHITE);
    tft.drawLine(cx+1, gy+r-3, cx+3, gy+r-1, C_WHITE);
    tft.drawLine(cx+3, gy+r-3, cx+1, gy+r-1, C_WHITE);
  }
}

void eraseGhost(int i) {
  tft.fillRect(tileToScreenX(ghosts[i].x), tileToScreenY(ghosts[i].y), TILE, TILE, C_BLACK);
  byte cell = maze[ghosts[i].y][ghosts[i].x];
  if (cell == M_DOT)  drawDot(ghosts[i].x, ghosts[i].y);
  if (cell == M_PPEL) drawPowerPellet(ghosts[i].x, ghosts[i].y);
}

// ================================================================
//  GAME LOGIC
// ================================================================

void initMaze() {
  dotsLeft = 0;
  for (int y = 0; y < ROWS; y++)
    for (int x = 0; x < COLS; x++) {
      maze[y][x] = MAZE_TEMPLATE[y][x];
      if (maze[y][x] == M_DOT || maze[y][x] == M_PPEL) dotsLeft++;
    }
}

void spawnEntities() {
  // Pac-Man starts at col 10, row 7
  pac.x = 10; pac.y = 7;
  pac.dir = 0; pac.nextDir = 0;

  // Ghosts start in/around ghost house (row 4-5, cols 9-11)
  // Blinky (red)   – starts outside house
  ghosts[0] = { 10, 4, 0, C_RED,     false, 20, 0  };
  // Pinky (pink)   – inside house
  ghosts[1] = { 9,  5, 2, C_PINK,    false, 0,  0  };
  // Inky (cyan)    – inside house
  ghosts[2] = { 10, 5, 0, C_CYAN,    false, 20, 10 };
  // Clyde (orange) – inside house
  ghosts[3] = { 11, 5, 0, C_ORANGE,  false, 0,  10 };

  frightenedMode = false;
  heartActive    = false;
  heartX = -1; heartY = -1;
  nextHeartSpawn = millis() + 15000;
}

// Can Pac-Man/ghost move to tile (nx,ny)?
bool canMove(int nx, int ny, bool isGhost) {
  if (nx < 0 || nx >= COLS || ny < 0 || ny >= ROWS) return false;
  byte c = maze[ny][nx];
  if (c == M_WALL) return false;
  if (!isGhost && c == M_DOOR) return false;
  return true;
}

void saveHigh() {
  if (score > highScore) {
    highScore = score;
    EEPROM.put(0, highScore);
    EEPROM.commit();
  }
}

// ================================================================
//  SCREENS
// ================================================================

void splashScreen() {
  tft.fillScreen(C_BLACK);

  // Title
  tft.setTextColor(C_YELLOW);
  tft.setTextSize(4);
  tft.setCursor(55, 50);
  tft.print("PAC-MAN");

  // Animated dots decoration
  for (int i = 0; i < 7; i++)
    tft.fillCircle(30 + i * 40, 110, 6, C_WHITE);

  // Pac-Man icon
  tft.fillCircle(160, 150, 20, C_YELLOW);
  tft.fillTriangle(160, 150, 182, 140, 182, 160, C_BLACK);

  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(90, 185);
  tft.print("Press  SELECT  to  Start");

  tft.setTextColor(0x8410);
  tft.setCursor(105, 210);
  tft.print("BFS Ghost AI  |  ESP32");

  while (digitalRead(BTN_SELECT) == HIGH) delay(50);
  delay(300);
}

void gameOverScreen() {
  saveHigh();
  delay(400);
  tft.fillScreen(C_BLACK);

  // Red box
  tft.fillRoundRect(60, 55, 200, 130, 10, 0x3000);
  tft.drawRoundRect(60, 55, 200, 130, 10, C_RED);

  tft.setTextColor(C_RED);
  tft.setTextSize(3);
  tft.setCursor(73, 70);
  tft.print("GAME OVER");

  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(80, 115);
  tft.print("Score: ");
  tft.setTextColor(C_YELLOW);
  tft.print(score);

  tft.setTextColor(C_WHITE);
  tft.setCursor(80, 140);
  tft.print("Best:  ");
  tft.setTextColor(C_CYAN);
  tft.print(highScore);

  tft.setTextSize(1);
  tft.setTextColor(0xAD75);
  tft.setCursor(85, 172);
  tft.print("SELECT = Restart");

  while (digitalRead(BTN_SELECT) == HIGH) delay(50);
  delay(300);
}

void youWinScreen() {
  saveHigh();
  delay(400);
  tft.fillScreen(C_BLACK);

  tft.fillRoundRect(60, 55, 200, 130, 10, 0x0320);
  tft.drawRoundRect(60, 55, 200, 130, 10, C_GREEN);

  tft.setTextColor(C_GREEN);
  tft.setTextSize(3);
  tft.setCursor(82, 70);
  tft.print("YOU WIN!");

  tft.setTextColor(C_WHITE);
  tft.setTextSize(2);
  tft.setCursor(80, 115);
  tft.print("Score: ");
  tft.setTextColor(C_YELLOW);
  tft.print(score);

  tft.setTextSize(1);
  tft.setTextColor(0xAD75);
  tft.setCursor(85, 172);
  tft.print("SELECT = Next Level");

  while (digitalRead(BTN_SELECT) == HIGH) delay(50);
  delay(300);
}

void levelUpScreen(int lvl) {
  tft.fillScreen(C_BLACK);
  tft.fillRoundRect(80, 90, 160, 60, 8, 0x1082);
  tft.drawRoundRect(80, 90, 160, 60, 8, C_YELLOW);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(2);
  tft.setCursor(95, 105);
  tft.print("LEVEL ");
  tft.print(lvl);
  tft.setTextSize(1);
  tft.setTextColor(C_YELLOW);
  tft.setCursor(108, 130);
  tft.print("GET  READY!");
  delay(2000);
}

// ================================================================
//  INIT / RESTART
// ================================================================

void startLevel() {
  tft.fillScreen(C_BLACK);
  initMaze();
  spawnEntities();
  drawUI();
  drawMaze();
  // Draw ghosts and pac
  for (int i = 0; i < NUM_GHOSTS; i++) drawGhost(i);
  drawPacman(C_YELLOW);

  lastPacMove   = millis();
  lastGhostMove = millis();
  lastMouth     = millis();
  gameOver = false;
  won      = false;
}

void fullReset() {
  score    = 0;
  level    = 1;
  lives    = 3;
  gameOver = false;
  won      = false;
  startLevel();
}

// ================================================================
//  SETUP
// ================================================================

void setup() {
  pinMode(BTN_UP,     INPUT_PULLUP);
  pinMode(BTN_DOWN,   INPUT_PULLUP);
  pinMode(BTN_LEFT,   INPUT_PULLUP);
  pinMode(BTN_RIGHT,  INPUT_PULLUP);
  pinMode(BTN_A,      INPUT_PULLUP);
  pinMode(BTN_B,      INPUT_PULLUP);
  pinMode(BTN_START,  INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, highScore);
  if (highScore < 0 || highScore > 9999999) highScore = 0;

  tft.init();
  tft.setRotation(1);   // Landscape 320x240
  tft.fillScreen(C_BLACK);

  splashScreen();
  fullReset();
}

// ================================================================
//  LOOP
// ================================================================

void loop() {

  // ── Mouth animation ────────────────────────────────────────────
  if (millis() - lastMouth > 160) {
    lastMouth = millis();
    mouthOpen = !mouthOpen;
    drawPacman(C_YELLOW);
  }

  // ── Power pellet timer ────────────────────────────────────────
  if (frightenedMode && millis() - frightenStart > FRIGHTEN_MS) {
    frightenedMode = false;
    for (int i = 0; i < NUM_GHOSTS; i++) ghosts[i].frightened = false;
  }

  // ── Heart spawn / expiry ──────────────────────────────────────
  if (!heartActive && millis() > nextHeartSpawn) {
    // Find a random walkable non-wall tile
    int attempts = 0;
    while (attempts++ < 30) {
      int rx = random(1, COLS - 1), ry = random(1, ROWS - 1);
      if (maze[ry][rx] == M_EMPTY || maze[ry][rx] == M_DOT) {
        heartX = rx; heartY = ry;
        heartActive    = true;
        heartSpawnTime = millis();
        drawHeartTile(heartX, heartY);
        break;
      }
    }
    nextHeartSpawn = millis() + random(20000, 30000);
  }
  if (heartActive && millis() - heartSpawnTime > HEART_DURATION) {
    // Erase heart, restore tile
    clearTile(heartX, heartY);
    if (maze[heartY][heartX] == M_DOT)  drawDot(heartX, heartY);
    heartActive = false; heartX = -1; heartY = -1;
  }

  // ── Pac-Man movement ──────────────────────────────────────────
  int pacSpeed = max(120, 220 - level * 15);

  if (millis() - lastPacMove > (unsigned long)pacSpeed) {
    lastPacMove = millis();

    // Read buttons → desired next direction
    if      (digitalRead(BTN_RIGHT) == LOW) pac.nextDir = 0;
    else if (digitalRead(BTN_DOWN)  == LOW) pac.nextDir = 1;
    else if (digitalRead(BTN_LEFT)  == LOW) pac.nextDir = 2;
    else if (digitalRead(BTN_UP)    == LOW) pac.nextDir = 3;

    // Try to turn to nextDir first
    int nx = pac.x + DX[pac.nextDir];
    int ny = pac.y + DY[pac.nextDir];
    if (canMove(nx, ny, false)) {
      pac.dir = pac.nextDir;
    }

    // Move in current direction
    nx = pac.x + DX[pac.dir];
    ny = pac.y + DY[pac.dir];

    if (canMove(nx, ny, false)) {
      erasePacman();
      pac.x = nx; pac.y = ny;

      // ── Eat dot ──
      if (maze[pac.y][pac.x] == M_DOT) {
        maze[pac.y][pac.x] = M_EMPTY;
        score += 10; dotsLeft--;
        drawUI();
      }
      // ── Eat power pellet ──
      else if (maze[pac.y][pac.x] == M_PPEL) {
        maze[pac.y][pac.x] = M_EMPTY;
        score += 50; dotsLeft--;
        frightenedMode = true;
        frightenStart  = millis();
        for (int i = 0; i < NUM_GHOSTS; i++) ghosts[i].frightened = true;
        drawUI();
      }

      // ── Eat heart (extra life) ──
      if (heartActive && pac.x == heartX && pac.y == heartY) {
        if (lives < 5) { lives++; drawUI(); }
        heartActive = false; heartX = -1; heartY = -1;
      }

      drawPacman(C_YELLOW);

      // ── Check win ──
      if (dotsLeft <= 0) {
        won = true;
        youWinScreen();
        level++;
        levelUpScreen(level);
        startLevel();
        return;
      }
    }

    // ── Ghost collision after Pac moves ──
    for (int i = 0; i < NUM_GHOSTS; i++) {
      if (pac.x == ghosts[i].x && pac.y == ghosts[i].y) {
        if (ghosts[i].frightened) {
          // Eat ghost
          score += 200;
          ghosts[i].frightened = false;
          // Respawn ghost at home
          ghosts[i].x = 10; ghosts[i].y = 5;
          drawUI();
        } else {
          // Lose a life
          lives--;
          drawUI();
          if (lives <= 0) {
            gameOver = true;
            gameOverScreen();
            fullReset();
            return;
          }
          // Restart positions (keep maze state)
          spawnEntities();
          tft.fillScreen(C_BLACK);
          drawUI();
          drawMaze();
          for (int j = 0; j < NUM_GHOSTS; j++) drawGhost(j);
          drawPacman(C_YELLOW);
          delay(500);
          return;
        }
      }
    }
  }

  // ── Ghost movement ────────────────────────────────────────────
  // Ghost speed scales with level; frightened ghosts move slower
  int ghostSpeed = max(180, 350 - level * 20);
  if (frightenedMode) ghostSpeed = ghostSpeed * 2;

  if (millis() - lastGhostMove > (unsigned long)ghostSpeed) {
    lastGhostMove = millis();

    for (int i = 0; i < NUM_GHOSTS; i++) {
      eraseGhost(i);

      int tx, ty;
      if (ghosts[i].frightened) {
        // Frightened: move randomly away (pick random valid direction)
        int dirs[4] = {0, 1, 2, 3};
        // Shuffle
        for (int s = 3; s > 0; s--) {
          int j = random(0, s + 1);
          int tmp = dirs[s]; dirs[s] = dirs[j]; dirs[j] = tmp;
        }
        tx = ghosts[i].x; ty = ghosts[i].y;
        for (int d = 0; d < 4; d++) {
          int nx = ghosts[i].x + DX[dirs[d]];
          int ny = ghosts[i].y + DY[dirs[d]];
          if (canMove(nx, ny, true)) { tx = nx; ty = ny; break; }
        }
      } else {
        // BFS chase Pac-Man (Blinky/Inky) or scatter (Pinky/Clyde)
        int targetX = pac.x;
        int targetY = pac.y;
        if (i == 1) { // Pinky targets 2 tiles ahead of Pac
          targetX = constrain(pac.x + 2 * DX[pac.dir], 0, COLS - 1);
          targetY = constrain(pac.y + 2 * DY[pac.dir], 0, ROWS - 1);
        } else if (i == 3 && // Clyde scatters if close
                   abs(ghosts[i].x - pac.x) + abs(ghosts[i].y - pac.y) < 4) {
          targetX = ghosts[i].homeX;
          targetY = ghosts[i].homeY;
        }
        Point nxt = bfsNext(ghosts[i].x, ghosts[i].y, targetX, targetY, true);
        tx = nxt.x; ty = nxt.y;
      }

      ghosts[i].x = tx; ghosts[i].y = ty;
      drawGhost(i);

      // Collision check after ghost moves
      if (pac.x == ghosts[i].x && pac.y == ghosts[i].y) {
        if (ghosts[i].frightened) {
          score += 200;
          ghosts[i].frightened = false;
          ghosts[i].x = 10; ghosts[i].y = 5;
          drawUI();
        } else {
          lives--;
          drawUI();
          if (lives <= 0) {
            gameOver = true;
            gameOverScreen();
            fullReset();
            return;
          }
          spawnEntities();
          tft.fillScreen(C_BLACK);
          drawUI();
          drawMaze();
          for (int j = 0; j < NUM_GHOSTS; j++) drawGhost(j);
          drawPacman(C_YELLOW);
          delay(500);
          return;
        }
      }
    }
  }
}
