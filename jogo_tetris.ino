/*Arduino TETRIS on 8x8 Matrix WS2812b
by mircemk, April 2025
Adapted by André Dias, May 2026
*/

#include <FastLED.h>

// LED Matrix configuration
#define LED_PIN     9
#define NUM_LEDS    128
#define BRIGHTNESS  30
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define MATRIX_WIDTH 8
#define MATRIX_HEIGHT 16


// Button pins
#define LEFT_BUTTON_PIN 5
#define RIGHT_BUTTON_PIN 3
#define ROTATE_BUTTON_PIN 4

// Game parameters
#define INITIAL_GAME_SPEED 500  // Milliseconds
#define SPEED_INCREASE 10       // ms to decrease after each piece
#define MIN_GAME_SPEED 150      // Fastest game speed in milliseconds


#define MODE_NORMAL 0
byte gameMode = MODE_NORMAL;

bool gameOverScreenShown = false;

// Colors
CRGB leds[NUM_LEDS];
#define BLACK     CRGB(0, 0, 0)
#define RED       CRGB(255, 0, 0)
#define GREEN     CRGB(0, 255, 0)
#define BLUE      CRGB(0, 0, 255)
#define YELLOW    CRGB(255, 255, 0)
#define CYAN      CRGB(0, 255, 255)
#define MAGENTA   CRGB(255, 0, 255)
#define ORANGE    CRGB(255, 165, 0)

// Tetromino shapes
// Each tetromino is defined as 4 cells, each cell having x and y coordinates
typedef struct {
  byte shapes[4][4][2];  // [rotation][cell][x,y]
  CRGB color;
} Tetromino;

// Tetromino types (I, O, T, S, Z, J, L)
Tetromino tetrominos[7] = {
  // I-piece
  {
    {{{0,0}, {1,0}, {2,0}, {3,0}}, 
     {{0,0}, {0,1}, {0,2}, {0,3}},
     {{0,0}, {1,0}, {2,0}, {3,0}},
     {{0,0}, {0,1}, {0,2}, {0,3}}},
    CYAN
  },
  // O-piece
  {
    {{{0,0}, {1,0}, {0,1}, {1,1}},
     {{0,0}, {1,0}, {0,1}, {1,1}},
     {{0,0}, {1,0}, {0,1}, {1,1}},
     {{0,0}, {1,0}, {0,1}, {1,1}}},
    YELLOW
  },
  // T-piece
  {
    {{{0,0}, {1,0}, {2,0}, {1,1}},
     {{1,0}, {0,1}, {1,1}, {1,2}},
     {{1,0}, {0,1}, {1,1}, {2,1}},
     {{0,0}, {0,1}, {0,2}, {1,1}}},
    MAGENTA
  },
  // S-piece
  {
    {{{1,0}, {2,0}, {0,1}, {1,1}},
     {{0,0}, {0,1}, {1,1}, {1,2}},
     {{1,0}, {2,0}, {0,1}, {1,1}},
     {{0,0}, {0,1}, {1,1}, {1,2}}},
    GREEN
  },
  // Z-piece
  {
    {{{0,0}, {1,0}, {1,1}, {2,1}},
     {{1,0}, {0,1}, {1,1}, {0,2}},
     {{0,0}, {1,0}, {1,1}, {2,1}},
     {{1,0}, {0,1}, {1,1}, {0,2}}},
    RED
  },
  // J-piece
  {
    {{{0,0}, {0,1}, {1,1}, {2,1}},
     {{1,0}, {2,0}, {1,1}, {1,2}},
     {{0,0}, {1,0}, {2,0}, {2,1}},
     {{0,0}, {0,1}, {0,2}, {1,0}}},
    BLUE
  },
  // L-piece
  {
    {{{2,0}, {0,1}, {1,1}, {2,1}},
     {{0,0}, {1,0}, {1,1}, {1,2}},
     {{0,0}, {1,0}, {2,0}, {0,1}},
     {{0,0}, {0,1}, {0,2}, {1,2}}},
    ORANGE
  }
};

const byte digits[10][8] = {
  // 0
  {B00000000,
   B00111000,
   B01000100,
   B01000100,
   B01000100,
   B01000100,
   B00111000,
   B00000000},
  // 1
  {B00000000,
   B00010000,
   B00110000,
   B00010000,
   B00010000,
   B00010000,
   B00111000,
   B00000000},
  // 2
  {B00000000,
   B00111000,
   B01000100,
   B00001000,
   B00010000,
   B00100000,
   B01111100,
   B00000000},
  // 3
  {B00000000,
   B00111000,
   B01000100,
   B00001000,
   B00001100,
   B01000100,
   B00111000,
   B00000000},
  // 4
  {B00000000,
   B00001000,
   B00011000,
   B00101000,
   B01001000,
   B01111100,
   B00001000,
   B00000000},
  // 5
  {B00000000,
   B01111100,
   B01000000,
   B01111000,
   B00000100,
   B01000100,
   B00111000,
   B00000000},
  // 6
  {B00000000,
   B00111000,
   B01000000,
   B01111000,
   B01000100,
   B01000100,
   B00111000,
   B00000000},
  // 7
  {B00000000,
   B01111100,
   B00000100,
   B00001000,
   B00010000,
   B00100000,
   B00100000,
   B00000000},
  // 8
  {B00000000,
   B00111000,
   B01000100,
   B00111000,
   B01000100,
   B01000100,
   B00111000,
   B00000000},
  // 9
  {B00000000,
   B00111000,
   B01000100,
   B01000100,
   B00111100,
   B00000100,
   B00111000,
   B00000000}
};

const byte SMILEY[8] = {
  B00111100,
  B01000010,
  B10100101,
  B10000001,
  B10100101,
  B10011001,
  B01000010,
  B00111100
};


const Tetromino* currentTetrominoSet;

void displayEndAnimation() {

  // Cor base da animação
  CRGB waveColor = CRGB::Blue;

  // Continua até um botão ser pressionado
  while (true) {

    // Sai da animação se algum botão for pressionado
    if (digitalRead(LEFT_BUTTON_PIN) == LOW ||
        digitalRead(RIGHT_BUTTON_PIN) == LOW ||
        digitalRead(ROTATE_BUTTON_PIN) == LOW) {
      break;
    }

    // Tempo da animação
    uint16_t t = millis() / 8;

    // Atualiza todos os LEDs
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      for (int x = 0; x < MATRIX_WIDTH; x++) {

        // Gera uma onda sinusoidal
        uint8_t brightness = sin8((x * 20) + (y * 15) + t);

        // Aplica brilho à cor
        leds[getPixelIndex(x, y)] = waveColor;
        leds[getPixelIndex(x, y)].nscale8(brightness);
      }
    }

    FastLED.show();
    delay(20);
  }

  clearDisplay();
}

void displayScrollingScore(long score) {
  // Convert score to string
  char scoreStr[7];
  sprintf(scoreStr, "%ld", score);
  int scoreLen = strlen(scoreStr);
  
  // Display each digit scrolling from right to left
  for (int pos = 8; pos >= -scoreLen * 6; pos--) {
    clearDisplay();
    
    // Display each digit in its current position
    for (int i = 0; i < scoreLen; i++) {
      int digitPos = pos + (i * 6);  // 6 pixels spacing between digits
      if (digitPos < 8 && digitPos > -6) {  // Only display if digit is visible
        int digit = scoreStr[i] - '0';
        displayLetter(digits[digit], digitPos, CRGB(255, 255, 0));  // Orange color
      }
    }
    
    FastLED.show();
    delay(100);  // Scroll speed
  }
  
  // Pause at the end
  delay(500);
}


// Add this function to select game mode

void displayLetter(const byte* letter, int xOffset, CRGB color) {
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 8; x++) {
      if (xOffset + x >= 0 && xOffset + x < 8) {  // Only draw if within display bounds
        if (letter[y] & (1 << (7-x))) {
          leds[getPixelIndex(xOffset + x, y)] = color;
        }
      }
    }
  }
}



// Game state
bool gameBoard[MATRIX_WIDTH][MATRIX_HEIGHT] = {0};  // True if a cell is occupied
CRGB boardColors[MATRIX_WIDTH][MATRIX_HEIGHT];      // Color of each cell

// Current tetromino state
byte currentPiece = 0;      // Index of current tetromino
byte currentRotation = 0;   // Current rotation (0-3)
int currentX = 3;           // X position of top-left corner
int currentY = 0;           // Y position of top-left corner
unsigned long lastFallTime = 0;
unsigned long gameSpeed = INITIAL_GAME_SPEED;
boolean gameOver = false;
unsigned int score = 0;

// Button state variables
bool leftPressed = false;
bool rightPressed = false;
bool rotatePressed = false;
unsigned long lastButtonCheckTime = 0;
#define DEBOUNCE_TIME 200  // Debounce time in milliseconds

void setup() {

 randomSeed(analogRead(A2) * analogRead(A3));  // Using multiple readings for better randomness
 
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  clearDisplay();
  
  // Initialize button pins
  pinMode(LEFT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(ROTATE_BUTTON_PIN, INPUT_PULLUP);
  
  Serial.begin(9600);
  Serial.println("Tetris initialized!");

  currentTetrominoSet = tetrominos;
  displaySplashScreen();

  waitForStartButton();

  spawnNewPiece();
}

void loop() {
   if (gameOver) {
    if (!gameOverScreenShown) {
      displayGameOver();
      gameOverScreenShown = true;
    } else if (checkAnyButtonPressed()) {
      // Wait for button release to prevent immediate restart
      delay(200);
      resetGame();
      gameOverScreenShown = false;
    }
    return;
  }
  
  checkButtons();
  
  // Move the piece down at regular intervals
  if (millis() - lastFallTime > gameSpeed) {
    if (!movePieceDown()) {
      // Piece has landed
      placePiece();
      clearLines();
      if (!spawnNewPiece()) {
        gameOver = true;
      }
      
      // Increase game speed 
      if (gameSpeed > MIN_GAME_SPEED) {
        gameSpeed -= SPEED_INCREASE;
      }
    }
    lastFallTime = millis();
  }
  
  updateDisplay();
}

void waitForStartButton() {

  bool state = false;

  while (true) {

    fill_solid(leds, NUM_LEDS,
               state ? CRGB::Blue : CRGB::Black);

    FastLED.show();

    state = !state;

    for (int i = 0; i < 10; i++) {

      if (checkAnyButtonPressed()) {
        delay(200);
        clearDisplay();
        return;
      }

      delay(50);
    }
  }
}

void checkButtons() {
  // Check buttons with debounce
  if (millis() - lastButtonCheckTime > DEBOUNCE_TIME) {
    // Check left button
    if (digitalRead(LEFT_BUTTON_PIN) == LOW && !leftPressed) {
      leftPressed = true;
      movePieceLeft();
      lastButtonCheckTime = millis();
    } else if (digitalRead(LEFT_BUTTON_PIN) == HIGH) {
      leftPressed = false;
    }
    
    // Check right button
    if (digitalRead(RIGHT_BUTTON_PIN) == LOW && !rightPressed) {
      rightPressed = true;
      movePieceRight();
      lastButtonCheckTime = millis();
    } else if (digitalRead(RIGHT_BUTTON_PIN) == HIGH) {
      rightPressed = false;
    }
    
    // Check rotate button
    if (digitalRead(ROTATE_BUTTON_PIN) == LOW && !rotatePressed) {
      rotatePressed = true;
      rotatePiece();
      lastButtonCheckTime = millis();
    } else if (digitalRead(ROTATE_BUTTON_PIN) == HIGH) {
      rotatePressed = false;
    }
  }
}

bool checkAnyButtonPressed() {
  return (digitalRead(LEFT_BUTTON_PIN) == LOW || 
          digitalRead(RIGHT_BUTTON_PIN) == LOW || 
          digitalRead(ROTATE_BUTTON_PIN) == LOW);
}

// Helper functions for the LED matrix Type

int getPixelIndex(int x, int y) {
  int fx = (MATRIX_WIDTH - 1) - x;
  int fy = (MATRIX_HEIGHT - 1) - y;
  return fy * MATRIX_WIDTH + fx;
}

void clearDisplay() {
  fill_solid(leds, NUM_LEDS, BLACK);
  FastLED.show();
}

void updateDisplay() {
  fill_solid(leds, NUM_LEDS, BLACK);
  
  // Draw the fixed blocks
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      if (gameBoard[x][y]) {
        leds[getPixelIndex(x, y)] = boardColors[x][y];
      }
    }
  }
  
  // Draw the current piece
  for (int i = 0; i < 4; i++) {
    int x = currentX + currentTetrominoSet[currentPiece].shapes[currentRotation][i][0];
    int y = currentY + currentTetrominoSet[currentPiece].shapes[currentRotation][i][1];
    
    if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
      leds[getPixelIndex(x, y)] = currentTetrominoSet[currentPiece].color;
    }
  }
  
  FastLED.show();
}

// Game mechanics
bool isValidPosition(int pieceIndex, int rotation, int posX, int posY) {
  for (int i = 0; i < 4; i++) {
    int x = posX + currentTetrominoSet[pieceIndex].shapes[rotation][i][0];
    int y = posY + currentTetrominoSet[pieceIndex].shapes[rotation][i][1];
    
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) {
      return false;
    }
    
    if (y >= 0 && gameBoard[x][y]) {
      return false;
    }
  }
  return true;
}

bool movePieceLeft() {
  if (isValidPosition(currentPiece, currentRotation, currentX - 1, currentY)) {
    currentX--;
    return true;
  }
  return false;
}

bool movePieceRight() {
  if (isValidPosition(currentPiece, currentRotation, currentX + 1, currentY)) {
    currentX++;
    return true;
  }
  return false;
}

bool movePieceDown() {
  if (isValidPosition(currentPiece, currentRotation, currentX, currentY + 1)) {
    currentY++;
    return true;
  }
  return false;
}

bool rotatePiece() {
  byte nextRotation = (currentRotation + 1) % 4;
  if (isValidPosition(currentPiece, nextRotation, currentX, currentY)) {
    currentRotation = nextRotation;
    return true;
  }
  // Try wall kick (adjust the position if rotation is blocked by a wall)
  // Try moving left
  if (isValidPosition(currentPiece, nextRotation, currentX - 1, currentY)) {
    currentX--;
    currentRotation = nextRotation;
    return true;
  }
  // Try moving right
  if (isValidPosition(currentPiece, nextRotation, currentX + 1, currentY)) {
    currentX++;
    currentRotation = nextRotation;
    return true;
  }
  return false;
}

void placePiece() {
  for (int i = 0; i < 4; i++) {
    int x = currentX + currentTetrominoSet[currentPiece].shapes[currentRotation][i][0];
    int y = currentY + currentTetrominoSet[currentPiece].shapes[currentRotation][i][1];
    
    if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) {
      gameBoard[x][y] = true;
      boardColors[x][y] = currentTetrominoSet[currentPiece].color;
    }
  }
}



bool spawnNewPiece() {
  static byte lastPiece = random(0, 7);
  byte newPiece;
  
  do {
    newPiece = random(0, 7);
  } while (newPiece == lastPiece && random(0, 100) < 70);
  
  lastPiece = newPiece;
  currentPiece = newPiece;
  
  // Use different rotation options based on game mode
  currentRotation = random(0, 4);
  
  currentX = (MATRIX_WIDTH / 2) - 1;
  currentY = 0;
  
  if (!isValidPosition(currentPiece, currentRotation, currentX, currentY)) {
    return false;
  }
  return true;
}

void clearLines() {
  int linesCleared = 0;
  
  for (int y = MATRIX_HEIGHT - 1; y >= 0; y--) {
    bool lineIsFull = true;
    
    // Check if the line is full
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      if (!gameBoard[x][y]) {
        lineIsFull = false;
        break;
      }
    }
    
    if (lineIsFull) {
      linesCleared++;
      
      // Flash the line
      for (int i = 0; i < 3; i++) {
        // Flash white
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          leds[getPixelIndex(x, y)] = CRGB::White;
        }
        FastLED.show();
        delay(50);
        
        // Flash black
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          leds[getPixelIndex(x, y)] = CRGB::Black;
        }
        FastLED.show();
        delay(50);
      }
      
      // Move all lines above this one down
      for (int moveY = y; moveY > 0; moveY--) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
          gameBoard[x][moveY] = gameBoard[x][moveY - 1];
          boardColors[x][moveY] = boardColors[x][moveY - 1];
        }
      }
      
      // Clear the top line
      for (int x = 0; x < MATRIX_WIDTH; x++) {
        gameBoard[x][0] = false;
      }
      
      // Since the lines have moved down, we need to check this row again
      y++;
    }
  }
  
  // Update score
  if (linesCleared > 0) {

    // More points for clearing multiple lines at once
    score += (linesCleared * linesCleared) * 100;
  }
}

void resetGame() {
  // Set the appropriate tetromino set based on game mode
  currentTetrominoSet = tetrominos;
  
  // Clear the display first
  clearDisplay();
  
  // Reset game board
  for (int x = 0; x < MATRIX_WIDTH; x++) {
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
      gameBoard[x][y] = false;
    }
  }
  
  // Reset game parameters
  gameSpeed = INITIAL_GAME_SPEED;
  gameOver = false;
  score = 0;
  gameOverScreenShown = false;
  
  // Show a quick start animation
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(20);
  }
  clearDisplay();
  delay(500);
  
  // Spawn a new piece
  spawnNewPiece();
}

void displaySplashScreen() {
  
  // Final flash effect
  for (int i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::White);
    FastLED.show();
    delay(100);
    clearDisplay();
    delay(100);
  }
}



void displayGameOver() {
  
  // Flash "Game Over" effect
  for (int i = 0; i < 3; i++) {
    fill_solid(leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
    delay(500);
    clearDisplay();
    FastLED.show();
    delay(500);
  }
  
  // Display final score
  delay(500);
  displayScrollingScore(score);
  
  // Show stable smiley and wait for restart
  displayEndAnimation();
  
  gameOverScreenShown = true;
}