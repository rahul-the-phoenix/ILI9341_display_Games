#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

// Button Pins
#define BTN_LEFT     27
#define BTN_RIGHT    26
#define BTN_SELECT   25

// --- Colors for ILI9341 (16-bit RGB565) ---
#define MINT_BG      0x9F5A  // Light Mint
#define COLOR_ROCK   0x52AA 
#define COLOR_PAPER  0xFFFF 
#define COLOR_SC_H   0xF800 
#define COLOR_TEXT   0x0000 
#define COLOR_WIN    0x07E0  // Bright Green
#define COLOR_LOSS   0xF800  // Red
#define COLOR_GOLD   0xFD20  // Gold

int pScore = 0, bScore = 0;
int currentSel = 1;

// Screen dimensions for ILI9341 (320x240)
int screenWidth = 320;
int screenHeight = 240;

// --- Graphics Functions ---
void drawMintBackground() {
  tft.fillScreen(MINT_BG);
}

// Realistic Rock Drawing Function
void drawRealRock(int x, int y, int s) {
  // Rock colors
  uint16_t rockDark = 0x3DEF;   // Dark gray
  uint16_t rockLight = 0x7BEF;  // Light gray
  uint16_t rockMid = 0x5ACB;    // Medium gray
  
  // Base shape - rounded irregular
  tft.fillRoundRect(x+2, y+4, s-4, s-8, 10, rockMid);
  
  // 3D shadow effect - bottom side darker
  tft.fillRoundRect(x+2, y+s-12, s-4, 8, 6, rockDark);
  // Right side darker
  tft.fillRect(x+s-10, y+8, 6, s-16, rockDark);
  
  // Left and top side lighter (light reflection)
  tft.fillRoundRect(x+2, y+4, 8, s-8, 4, rockLight);
  tft.fillRect(x+6, y+4, s-12, 6, rockLight);
  
  // Natural cracks
  for(int i=0; i<4; i++) {
    int startX = x+random(8, s-8);
    int startY = y+random(10, s-10);
    tft.drawLine(startX, startY, startX+random(-5, 5), startY+random(8, 15), rockDark);
    tft.drawLine(startX, startY, startX+random(5, 10), startY+random(-3, 3), rockDark);
  }
  
  // Pit marks (holes)
  for(int i=0; i<8; i++) {
    tft.fillCircle(x+random(8, s-8), y+random(10, s-10), random(1, 3), rockDark);
  }
  
  // Micro texture (tiny dots)
  for(int i=0; i<40; i++) {
    tft.drawPixel(x+random(5, s-5), y+random(8, s-8), 0x2104);
  }
  
  // Sparkle effects (light reflections)
  tft.drawPixel(x+random(6, s-6), y+random(8, s-8), 0xFFFF);
  tft.drawPixel(x+random(6, s-6), y+random(8, s-8), 0xFFFF);
  
  // Outline for definition
  tft.drawRoundRect(x+2, y+4, s-4, s-8, 10, 0x2945);
  
  // Additional gravel texture
  for(int i=0; i<15; i++) {
    tft.drawPixel(x+random(5, s-5), y+random(10, s-10), 0x2104);
  }
}

void drawProPaper(int x, int y, int s) {
  tft.fillRect(x, y, s-4, s, COLOR_PAPER);
  tft.drawRect(x, y, s-4, s, 0x0000);
  for(int i=6; i<s; i+=6) tft.drawFastHLine(x+2, y+i, s-8, 0xAD7F);
  tft.fillRect(x+2, y+2, 3, s-4, 0x94B2);
}

void drawProScissor(int x, int y, int s) {
  tft.drawCircle(x+8, y+s-10, 7, COLOR_SC_H);
  tft.drawCircle(x+s-12, y+s-10, 7, COLOR_SC_H);
  tft.fillTriangle(x+10, y+s-15, x+s-5, y+5, x+s-10, y+2, 0x0000);
  tft.fillTriangle(x+s-14, y+s-15, x+5, y+5, x+10, y+2, 0x0000);
  tft.drawLine(x+12, y+s-18, x+s-12, y+8, 0xFFFF);
}

void updateScore() {
  tft.fillRoundRect(0, 0, screenWidth, 40, 8, 0x0000);
  tft.drawRoundRect(0, 0, screenWidth, 40, 8, COLOR_GOLD);
  
  tft.setTextColor(0xFFFF);
  tft.setTextSize(2);
  tft.setCursor(30, 12); 
  tft.print("PLAYER: ");
  tft.print(pScore);
  
  tft.setCursor(200, 12); 
  tft.print("BOT: ");
  tft.print(bScore);
  
  tft.drawFastVLine(160, 5, 30, COLOR_GOLD);
}

void drawSelectionBoxes() {
  tft.fillRect(0, 100, screenWidth, 80, MINT_BG);
  
  for (int i = 1; i <= 3; i++) {
    int x = 40 + (i - 1) * 95;
    uint16_t border = (currentSel == i) ? 0xF800 : 0x0000; 
    
    tft.drawRect(x-3, 147, 70, 70, border); 
    tft.drawRect(x-4, 146, 72, 72, border);
    tft.drawRect(x-5, 145, 74, 74, border);
    
    if(currentSel == i) {
      for(int g=1; g<=3; g++) {
        tft.drawRect(x-3-g, 147-g, 70+(g*2), 70+(g*2), 0xF820);
      }
    }
    
    if(i==1) drawRealRock(x+18, 153, 50);
    else if(i==2) drawProPaper(x+18, 153, 50);
    else if(i==3) drawProScissor(x+18, 153, 50);
  }
  
  tft.setTextSize(1);
  tft.setTextColor(0xFFFF);
  tft.setCursor(20, 225);
  tft.print("LEFT/RIGHT: Select   SELECT: Play");
}

void drawVSAnimation() {
  delay(800);
}

void showResultAnimation(String result, uint16_t color) {
  int boxX = 60, boxY = 90, boxW = 200, boxH = 60;
  
  for(int i=0; i<10; i++) {
    tft.drawRoundRect(boxX-i, boxY-i, boxW+(i*2), boxH+(i*2), 10, color);
    delay(20);
  }
  
  tft.fillRoundRect(boxX, boxY, boxW, boxH, 10, 0x0000);
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 10, color);
  
  tft.setTextSize(3);
  tft.setTextColor(color);
  
  int textX = boxX + (boxW/2) - (result.length() * 9);
  tft.setCursor(textX, boxY + 20);
  tft.print(result);
  
  delay(1500);
}

void checkUltimateWinner() {
  if (pScore >= 5 || bScore >= 5) {
    delay(500);
    
    for(int i=0; i<screenWidth; i+=10) {
        tft.drawFastVLine(i, 0, screenHeight, 0x0000);
        delay(8);
    }
    
    tft.drawRect(5, 5, screenWidth-10, screenHeight-10, COLOR_GOLD);
    tft.drawRect(8, 8, screenWidth-16, screenHeight-16, COLOR_GOLD);
    tft.drawRect(11, 11, screenWidth-22, screenHeight-22, COLOR_GOLD);
    
    tft.setTextSize(3);
    
    if (pScore >= 5) {
      tft.fillScreen(0x0000);
      tft.setTextColor(COLOR_WIN);
      tft.setCursor(65, 50);
      tft.println("VICTORY!");
      
      tft.setTextSize(2);
      tft.setTextColor(0xFFFF);
      tft.setCursor(45, 90);
      tft.println("YOU ARE SUPREME!");
      
      tft.fillRect(135, 130, 50, 30, COLOR_GOLD);
      tft.fillRect(155, 160, 10, 20, COLOR_GOLD);
      tft.fillCircle(160, 135, 25, COLOR_GOLD);
      tft.fillCircle(160, 135, 20, 0x0000);
      tft.fillRect(130, 165, 60, 5, COLOR_GOLD);
    } 
    else {
      tft.fillScreen(0x0000);
      tft.setTextColor(COLOR_LOSS);
      tft.setCursor(55, 50);
      tft.println("GAME OVER");
      
      tft.setTextSize(2);
      tft.setTextColor(0xFFFF);
      tft.setCursor(55, 90);
      tft.println("BOT IS SUPREME!");
      
      tft.drawLine(130, 130, 190, 170, COLOR_LOSS);
      tft.drawLine(190, 130, 130, 170, COLOR_LOSS);
      tft.drawLine(140, 130, 180, 170, COLOR_LOSS);
      tft.drawLine(180, 130, 140, 170, COLOR_LOSS);
    }
    
    unsigned long blinkStart = millis();
    while (digitalRead(BTN_SELECT) == HIGH) {
      if((millis() - blinkStart) > 500) {
        tft.setTextSize(1);
        tft.setTextColor(0xAD7F);
        tft.setCursor(85, 210);
        tft.print("PRESS SELECT");
        delay(100);
        tft.fillRect(85, 210, 150, 15, 0x0000);
        blinkStart = millis();
      }
      delay(10);
    }
    
    tft.fillScreen(0xFFFF);
    delay(100);
    
    pScore = 0; 
    bScore = 0;
    drawMintBackground();
    updateScore();
    drawSelectionBoxes();
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
  
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(MINT_BG);
  
  randomSeed(analogRead(34));
  
  drawMintBackground();
  updateScore();
  drawSelectionBoxes();
}

void loop() {
  static unsigned long lastButtonTime = 0;
  
  if (digitalRead(BTN_LEFT) == LOW && millis() - lastButtonTime > 200) {
    lastButtonTime = millis();
    currentSel--;
    if(currentSel < 1) currentSel = 3;
    drawSelectionBoxes(); 
  }
  
  if (digitalRead(BTN_RIGHT) == LOW && millis() - lastButtonTime > 200) {
    lastButtonTime = millis();
    currentSel++;
    if(currentSel > 3) currentSel = 1;
    drawSelectionBoxes();
  }
  
  if (digitalRead(BTN_SELECT) == LOW && millis() - lastButtonTime > 300) {
    lastButtonTime = millis();
    
    tft.fillRect(0, 45, screenWidth, 55, MINT_BG);
    drawVSAnimation();
    tft.fillRect(0, 45, screenWidth, 80, MINT_BG);
    
    tft.setTextSize(2);
    tft.setTextColor(COLOR_TEXT);
    tft.setCursor(40, 55);
    tft.print("YOU");
    
    if(currentSel == 1) drawRealRock(35, 75, 60);
    else if(currentSel == 2) drawProPaper(35, 75, 60);
    else drawProScissor(35, 75, 60);
    
    int botPick = random(1, 4);
    
    tft.setTextSize(2);
    tft.setCursor(200, 55);
    tft.print("BOT");
    
    for(int i = 0; i < 5; i++) {
        tft.setCursor(170, 90);
        tft.setTextColor(0xF800);
        tft.print("THINKING");
        delay(200);
        tft.fillRect(170, 90, 100, 15, MINT_BG);
        delay(100);
    }
    
    tft.fillRect(140, 70, 130, 50, MINT_BG);
    
    if(botPick == 1) drawRealRock(185, 75, 60);
    else if(botPick == 2) drawProPaper(185, 75, 60);
    else drawProScissor(185, 75, 60);
    
    delay(1000);
    
    String result;
    uint16_t resultColor;
    
    if (currentSel == botPick) {
        result = "DRAW!";
        resultColor = 0xFFFF;
    } else if ((currentSel == 1 && botPick == 3) || 
               (currentSel == 2 && botPick == 1) || 
               (currentSel == 3 && botPick == 2)) {
        pScore++;
        result = "YOU WIN!";
        resultColor = COLOR_WIN;
    } else {
        bScore++;
        result = "YOU LOST!";
        resultColor = COLOR_LOSS;
    }
    
    updateScore();
    showResultAnimation(result, resultColor);
    
    if (pScore >= 5 || bScore >= 5) {
      checkUltimateWinner();
    } else {
      drawMintBackground();
      updateScore();
      drawSelectionBoxes();
    }
  }        
  delay(10);
}
