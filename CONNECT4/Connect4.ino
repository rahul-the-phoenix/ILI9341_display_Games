#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Button Pins
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_SELECT   25

#define ROWS 7        // 7 rows
#define COLS 10       // 10 columns
#define CELL_SIZE 28  // Perfect size for 10x7 grid
#define OFFSET_X 20   // Perfect horizontal centering
#define OFFSET_Y 40   // Perfect vertical centering

// Colors (16-bit RGB565 format)
#define COL_BOARD   0x0000  // Pure Black background
#define COL_EMPTY   0xFFFF  // White empty cell
#define COL_PLAYER  0x07E0  // Green
#define COL_BOT     0xF800  // Red
#define COL_GLOSS   0xFFFF  // White gloss
#define COL_GOLD    0xFD20  // Gold color
#define COL_WIN     0x001F  // Blue win line
#define COL_BG      0x0000  // Black screen background
#define COL_BORDER  0x7BEF  // Light gray border
#define COL_GRID    0xAD55  // Light brown/gold grid line

int board[ROWS][COLS];
int selector = 4;  // Start at middle column (0-9 -> 4 is center-left)
bool playerTurn = true, gameOver = false;
int playerWins = 0, botWins = 0;
int winX[4], winY[4];

void drawGlossyCircle(int x, int y, int r, uint16_t color) {
  if (color == COL_EMPTY) {
    tft.fillCircle(x, y, r, COL_BOARD);
    tft.drawCircle(x, y, r, COL_GRID);
  } else {
    tft.fillCircle(x, y, r, color);
    // Draw glossy highlight at top-left
    tft.fillCircle(x - r/3, y - r/3, r/4, COL_GLOSS);
  }
}

void updateSelector() {
  // Clear selector area
  tft.fillRect(0, 0, 320, 38, COL_BG);
  
  if(!gameOver) {
    // Draw selector triangle
    int tx = OFFSET_X + selector * CELL_SIZE + CELL_SIZE/2;
    int ty = 30;
    
    // Draw glow effect for selector
    for(int i = 1; i <= 3; i++) {
      tft.fillTriangle(tx - (6 + i/2), ty - i,
                       tx + (6 + i/2), ty - i,
                       tx, ty + 10,
                       playerTurn ? 0x03E0 : 0xB800);
    }
    
    tft.fillTriangle(tx - 6, ty,
                     tx + 6, ty,
                     tx, ty + 12,
                     playerTurn ? COL_PLAYER : COL_BOT);
    
    // Draw turn indicator with box
    tft.fillRoundRect(10, 8, 100, 22, 4, COL_BORDER);
    tft.setTextSize(1);
    tft.setTextColor(playerTurn ? COL_PLAYER : COL_BOT, COL_BORDER);
    tft.setCursor(15, 13);
    tft.print(playerTurn ? "YOUR TURN" : "BOT TURN");
    
    // Show scores on top right
    tft.fillRoundRect(210, 8, 100, 22, 4, COL_BORDER);
    tft.setTextColor(COL_GLOSS, COL_BORDER);
    tft.setCursor(215, 13);
    tft.print("YOU:");
    tft.print(playerWins);
    tft.print(" BOT:");
    tft.print(botWins);
  }
}

void drawBoard() {
  // Calculate board dimensions
  int boardWidth = COLS * CELL_SIZE + 12;
  int boardHeight = ROWS * CELL_SIZE + 12;
  
  // Draw outer board border (3D effect)
  tft.fillRoundRect(OFFSET_X - 6, OFFSET_Y - 6, 
                    boardWidth, boardHeight, 
                    10, COL_BORDER);
  tft.fillRoundRect(OFFSET_X - 4, OFFSET_Y - 4, 
                    boardWidth - 4, boardHeight - 4, 
                    8, COL_BOARD);
  
  // Draw gold accent border
  tft.drawRoundRect(OFFSET_X - 5, OFFSET_Y - 5, 
                    boardWidth - 2, boardHeight - 2, 
                    9, COL_GOLD);
  
  // Draw grid lines
  for(int c = 0; c <= COLS; c++) {
    tft.drawLine(OFFSET_X + c * CELL_SIZE, OFFSET_Y, 
                 OFFSET_X + c * CELL_SIZE, OFFSET_Y + ROWS * CELL_SIZE, 
                 COL_GRID);
  }
  
  for(int r = 0; r <= ROWS; r++) {
    tft.drawLine(OFFSET_X, OFFSET_Y + r * CELL_SIZE, 
                 OFFSET_X + COLS * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, 
                 COL_GRID);
  }
  
  // Draw all cells (balls)
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      uint16_t color;
      if (board[r][c] == 1) color = COL_PLAYER;
      else if (board[r][c] == 2) color = COL_BOT;
      else color = COL_EMPTY;
      
      int ballRadius = CELL_SIZE/2 - 4;  // Perfect fit: 10 pixels
      
      drawGlossyCircle(OFFSET_X + c * CELL_SIZE + CELL_SIZE/2, 
                       OFFSET_Y + r * CELL_SIZE + CELL_SIZE/2, 
                       ballRadius, color);
    }
  }
  updateSelector();
}

void drawWinLine() {
  // Draw sparkling win line animation
  for (int i = 0; i < 3; i++) {
    int x1 = OFFSET_X + winX[i] * CELL_SIZE + CELL_SIZE/2;
    int y1 = OFFSET_Y + winY[i] * CELL_SIZE + CELL_SIZE/2;
    int x2 = OFFSET_X + winX[i+1] * CELL_SIZE + CELL_SIZE/2;
    int y2 = OFFSET_Y + winY[i+1] * CELL_SIZE + CELL_SIZE/2;
    
    // Draw multiple layers for glowing effect
    for (int thickness = -2; thickness <= 2; thickness++) {
      for (int ox = -thickness; ox <= thickness; ox++) {
        for (int oy = -thickness; oy <= thickness; oy++) {
          if(abs(ox) + abs(oy) <= thickness * 2) {
            tft.drawLine(x1 + ox, y1 + oy, x2 + ox, y2 + oy, COL_WIN);
          }
        }
      }
    }
    delay(100);
  }
  
  // Flash effect
  for(int flash = 0; flash < 3; flash++) {
    for (int i = 0; i < 3; i++) {
      int x1 = OFFSET_X + winX[i] * CELL_SIZE + CELL_SIZE/2;
      int y1 = OFFSET_Y + winY[i] * CELL_SIZE + CELL_SIZE/2;
      int x2 = OFFSET_X + winX[i+1] * CELL_SIZE + CELL_SIZE/2;
      int y2 = OFFSET_Y + winY[i+1] * CELL_SIZE + CELL_SIZE/2;
      tft.drawLine(x1, y1, x2, y2, COL_GLOSS);
    }
    delay(80);
    for (int i = 0; i < 3; i++) {
      int x1 = OFFSET_X + winX[i] * CELL_SIZE + CELL_SIZE/2;
      int y1 = OFFSET_Y + winY[i] * CELL_SIZE + CELL_SIZE/2;
      int x2 = OFFSET_X + winX[i+1] * CELL_SIZE + CELL_SIZE/2;
      int y2 = OFFSET_Y + winY[i+1] * CELL_SIZE + CELL_SIZE/2;
      tft.drawLine(x1, y1, x2, y2, COL_WIN);
    }
    delay(80);
  }
  delay(300);
}

void animateDisc(int col, int targetRow, uint16_t color) {
  int x = OFFSET_X + col * CELL_SIZE + CELL_SIZE/2;
  int ballRadius = CELL_SIZE/2 - 4;
  
  for (int row = 0; row <= targetRow; row++) {
    int y = OFFSET_Y + row * CELL_SIZE + CELL_SIZE/2;
    
    // Clear previous position with empty cell
    if (row > 0) {
      drawGlossyCircle(x, OFFSET_Y + (row - 1) * CELL_SIZE + CELL_SIZE/2, 
                      ballRadius, COL_EMPTY);
    }
    
    // Draw current position with shadow effect
    tft.fillCircle(x + 2, y + 2, ballRadius, 0x0000);
    drawGlossyCircle(x, y, ballRadius, color);
    delay(45);
  }
}

bool checkWin(int p) {
  // Horizontal check
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS - 3; c++) {
      if (board[r][c] == p && board[r][c+1] == p && 
          board[r][c+2] == p && board[r][c+3] == p) {
        for (int i = 0; i < 4; i++) { 
          winY[i] = r; 
          winX[i] = c + i; 
        }
        return true;
      }
    }
  }
  
  // Vertical check
  for (int r = 0; r < ROWS - 3; r++) {
    for (int c = 0; c < COLS; c++) {
      if (board[r][c] == p && board[r+1][c] == p && 
          board[r+2][c] == p && board[r+3][c] == p) {
        for (int i = 0; i < 4; i++) { 
          winY[i] = r + i; 
          winX[i] = c; 
        }
        return true;
      }
    }
  }
  
  // Diagonal (bottom-left to top-right)
  for (int r = 3; r < ROWS; r++) {
    for (int c = 0; c < COLS - 3; c++) {
      if (board[r][c] == p && board[r-1][c+1] == p && 
          board[r-2][c+2] == p && board[r-3][c+3] == p) {
        for (int i = 0; i < 4; i++) { 
          winY[i] = r - i; 
          winX[i] = c + i; 
        }
        return true;
      }
    }
  }
  
  // Diagonal (top-left to bottom-right)
  for (int r = 0; r < ROWS - 3; r++) {
    for (int c = 0; c < COLS - 3; c++) {
      if (board[r][c] == p && board[r+1][c+1] == p && 
          board[r+2][c+2] == p && board[r+3][c+3] == p) {
        for (int i = 0; i < 4; i++) { 
          winY[i] = r + i; 
          winX[i] = c + i; 
        }
        return true;
      }
    }
  }
  return false;
}

bool isDraw() {
  for (int c = 0; c < COLS; c++) {
    if (board[0][c] == 0) return false;
  }
  return true;
}

void showGameOver(String msg, uint16_t txtCol) {
  delay(600);
  
  // Create semi-transparent overlay effect
  tft.fillRect(40, 50, 240, 140, 0x0000);
  tft.drawRoundRect(40, 50, 240, 140, 10, COL_GOLD);
  tft.drawRoundRect(42, 52, 236, 136, 8, txtCol);
  
  // Show winner message
  tft.setTextSize(2);
  tft.setTextColor(txtCol, 0x0000);
  
  if (msg == "YOU WIN!") {
    tft.setCursor(108, 75);
  } else if (msg == "BOT WINS!") {
    tft.setCursor(98, 75);
  } else if (msg == "DRAW!") {
    tft.setCursor(128, 75);
  }
  tft.print(msg);
  
  // Draw separator
  tft.drawFastHLine(70, 105, 180, COL_GOLD);
  
  // Show scores
  tft.setTextSize(2);
  tft.setTextColor(COL_GLOSS, 0x0000);
  tft.setCursor(85, 120);
  tft.print("YOU: ");
  tft.print(playerWins);
  tft.setCursor(85, 145);
  tft.print("BOT: ");
  tft.print(botWins);
  
  // Play again prompt
  tft.setTextSize(1);
  tft.setTextColor(COL_GOLD, 0x0000);
  tft.setCursor(72, 175);
  tft.print("PRESS SELECT TO PLAY AGAIN");
}

bool dropDisc(int col, int p) {
  for (int r = ROWS - 1; r >= 0; r--) {
    if (board[r][col] == 0) {
      animateDisc(col, r, (p == 1) ? COL_PLAYER : COL_BOT);
      board[r][col] = p;
      return true;
    }
  }
  return false;
}

void botMove() {
  delay(300);
  int mc = -1;
  
  // Check for bot winning move
  for (int c = 0; c < COLS; c++) {
    if (board[0][c] == 0) {
      int r;
      for (r = ROWS - 1; r >= 0; r--) 
        if (board[r][c] == 0) break;
      
      board[r][c] = 2;
      if (checkWin(2)) mc = c;
      board[r][c] = 0;
      if (mc != -1) break;
    }
  }
  
  // Block player winning move
  if (mc == -1) {
    for (int c = 0; c < COLS; c++) {
      if (board[0][c] == 0) {
        int r;
        for (r = ROWS - 1; r >= 0; r--) 
          if (board[r][c] == 0) break;
        
        board[r][c] = 1;
        if (checkWin(1)) mc = c;
        board[r][c] = 0;
        if (mc != -1) break;
      }
    }
  }
  
  // Random move if no strategic move found
  if (mc == -1) {
    do { 
      mc = random(COLS); 
    } while (board[0][mc] != 0);
  }
  
  dropDisc(mc, 2);
  
  if (checkWin(2)) { 
    botWins++; 
    drawWinLine(); 
    showGameOver("BOT WINS!", COL_BOT); 
    gameOver = true; 
  } 
  else if (isDraw()) {
    showGameOver("DRAW!", COL_GOLD);
    gameOver = true;
  }
  else { 
    playerTurn = true; 
    updateSelector(); 
  }
}

void resetGame() {
  tft.fillScreen(COL_BG);
  
  // Initialize board
  for (int r = 0; r < ROWS; r++) 
    for (int c = 0; c < COLS; c++) 
      board[r][c] = 0;
  
  selector = 4;
  gameOver = false;
  
  // Random who starts first
  playerTurn = (random(2) == 0);
  
  drawBoard();
  
  if(!playerTurn) {
    delay(500);
    botMove();
  }
}

void setup() {
  // Initialize buttons
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  // Initialize display
  tft.init();
  tft.setRotation(1); // Landscape orientation
  tft.fillScreen(COL_BG);
  tft.setTextColor(COL_GLOSS, COL_BG);
  tft.setTextSize(1);
  
  randomSeed(analogRead(0));
  resetGame();
}

void loop() {
  if (!gameOver) {
    if (playerTurn) {
      // Handle left button with debounce
      if (digitalRead(BTN_LEFT) == LOW && selector > 0) { 
        selector--; 
        updateSelector(); 
        delay(120);
      }
      
      // Handle right button
      if (digitalRead(BTN_RIGHT) == LOW && selector < COLS - 1) { 
        selector++; 
        updateSelector(); 
        delay(120);
      }
      
      // Handle select button
      if (digitalRead(BTN_SELECT) == LOW) {
        if (dropDisc(selector, 1)) {
          if (checkWin(1)) { 
            playerWins++; 
            drawWinLine(); 
            showGameOver("YOU WIN!", COL_PLAYER); 
            gameOver = true; 
          } 
          else if (isDraw()) {
            showGameOver("DRAW!", COL_GOLD);
            gameOver = true;
          }
          else { 
            playerTurn = false; 
            updateSelector();
            delay(300);
            botMove();
          }
        }
        delay(200);
      }
    }
  } 
  else if (digitalRead(BTN_SELECT) == LOW) { 
    delay(300); 
    resetGame(); 
  }
}
