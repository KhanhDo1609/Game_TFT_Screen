
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// ================= PINS =================
#define TFT_CS 10
#define TFT_DC 9
#define TFT_RST 8

#define JOY_X A0
#define JOY_Y A1
#define JOY_SW 2

#define ENC_CLK 3
#define ENC_DT 4
#define ENC_SW 5

#define BACK_BTN 6
#define BUZZER 7

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// ================= STATES =================
enum Screen {
  MENU, SNAKE, SHOOTER, BREAKOUT, PLACEHOLDER
};

Screen screen = MENU;

byte selected = 0;
bool gameOver = false;
bool gameWon = false;

unsigned int score = 0;
unsigned long lastTick = 0;
unsigned long lastMenuMove = 0;
unsigned long lastPress = 0;

bool oldJoySW = HIGH;
bool oldEncSW = HIGH;
bool oldBackSW = HIGH;
int oldEncCLK = HIGH;

const char* const menuItems[] = {
  "Snake",
  "Space Shooter",
  "Brick Breaker",
  "Flappy Bird",
  "Racing",
  "YouTube Music"
};

// ================= HELPERS =================
void beep(int frequency = 1200, int duration = 35) {
  tone(BUZZER, frequency, duration);
}

void drawHeader(const __FlashStringHelper* title) {
  tft.fillRect(0, 0, 320, 30, ILI9341_BLUE);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);

  tft.setCursor(6, 7);
  tft.print(title);

  tft.setCursor(220, 7);
  tft.print(score);
}

void startScreen() {
  score = 0;
  gameOver = false;
  gameWon = false;
  lastTick = millis();
  tft.fillScreen(ILI9341_BLACK);
}

void endGame(bool victory = false) {
  gameOver = true;
  gameWon = victory;

  beep(victory ? 1800 : 250, 250);

  tft.fillRoundRect(
    35, 75, 250, 100, 8,
    ILI9341_DARKGREY
  );

  tft.setTextSize(3);
  tft.setTextColor(
    victory ? ILI9341_GREEN : ILI9341_RED
  );

  tft.setCursor(60, 89);

  if (victory) {
    tft.print(F("YOU WIN!"));
  } else {
    tft.print(F("GAME OVER"));
  }

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(58, 136);
  tft.print(F("Press to retry"));
}

// ================= MAIN MENU =================
void drawMenu() {
  tft.fillScreen(ILI9341_BLACK);

  tft.fillRect(0, 0, 320, 43, ILI9341_BLUE);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(44, 13);
  tft.print(F("ARDUINO ARCADE"));

  for (byte i = 0; i < 6; i++) {
    int y = 51 + i * 28;

    if (i == selected) {
      tft.fillRoundRect(
        12, y - 3, 296, 27, 5,
        ILI9341_GREEN
      );
      tft.setTextColor(ILI9341_BLACK);
    } else {
      tft.setTextColor(ILI9341_WHITE);
    }

    tft.setCursor(25, y);
    tft.print(i + 1);
    tft.print(F(". "));
    tft.print(menuItems[i]);
  }

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setCursor(20, 226);
  tft.print(F("Joystick / Encoder to select"));
}

void moveMenu(int direction) {
  int next = selected + direction;

  if (next < 0) next = 5;
  if (next > 5) next = 0;

  selected = next;
  beep(900, 20);
  drawMenu();
}

// ================= SNAKE =================
const byte CELL = 10;
const byte COLS = 32;
const byte ROWS = 20;
const byte MAX_SNAKE = 100;

byte snakeX[MAX_SNAKE];
byte snakeY[MAX_SNAKE];

byte snakeLength = 3;
byte foodX, foodY;

int dirX = 1, dirY = 0;
int nextX = 1, nextY = 0;

void snakeCell(int x, int y, uint16_t color) {
  tft.fillRect(
    x * CELL,
    35 + y * CELL,
    9, 9,
    color
  );
}

void spawnFood() {
  // Search for an empty cell, starting randomly.
  int start = random((long)COLS * ROWS);

  for (int offset = 0; offset < COLS * ROWS; offset++) {
    int index = (start + offset) % (COLS * ROWS);
    byte x = index % COLS;
    byte y = index / COLS;

    bool occupied = false;

    for (byte i = 0; i < snakeLength; i++) {
      if (snakeX[i] == x && snakeY[i] == y) {
        occupied = true;
        break;
      }
    }

    if (!occupied) {
      foodX = x;
      foodY = y;
      snakeCell(foodX, foodY, ILI9341_RED);
      return;
    }
  }

  endGame(true);
}

void startSnake() {
  screen = SNAKE;
  startScreen();

  snakeLength = 3;

  for (byte i = 0; i < snakeLength; i++) {
    snakeX[i] = 8 - i;
    snakeY[i] = 10;
  }

  dirX = nextX = 1;
  dirY = nextY = 0;

  drawHeader(F("SNAKE"));

  for (byte i = 0; i < snakeLength; i++) {
    snakeCell(snakeX[i], snakeY[i], ILI9341_GREEN);
  }

  spawnFood();
}

void updateSnake(int joyX, int joyY) {
  // Direction selection
  if (abs(joyX - 512) > abs(joyY - 512)) {
    if (joyX < 300 && dirX != 1) {
      nextX = -1;
      nextY = 0;
    } else if (joyX > 700 && dirX != -1) {
      nextX = 1;
      nextY = 0;
    }
  } else {
    if (joyY < 300 && dirY != 1) {
      nextX = 0;
      nextY = -1;
    } else if (joyY > 700 && dirY != -1) {
      nextX = 0;
      nextY = 1;
    }
  }

  // Higher score = shorter movement interval
  unsigned long interval =
    max(75L, 200L - (long)score);

  if (millis() - lastTick < interval) return;
  lastTick = millis();

  dirX = nextX;
  dirY = nextY;

  int nx = snakeX[0] + dirX;
  int ny = snakeY[0] + dirY;

  if (nx < 0 || nx >= COLS ||
      ny < 0 || ny >= ROWS) {
    endGame();
    return;
  }

  bool eating = (nx == foodX && ny == foodY);
  bool growing = eating && snakeLength < MAX_SNAKE;

  int checkLength = snakeLength - (growing ? 0 : 1);

  for (int i = 0; i < checkLength; i++) {
    if (snakeX[i] == nx && snakeY[i] == ny) {
      endGame();
      return;
    }
  }

  int tailX = snakeX[snakeLength - 1];
  int tailY = snakeY[snakeLength - 1];

  // Old head becomes body
  snakeCell(snakeX[0], snakeY[0], ILI9341_GREEN);

  if (growing) {
    snakeLength++;
  } else {
    snakeCell(tailX, tailY, ILI9341_BLACK);
  }

  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  snakeX[0] = nx;
  snakeY[0] = ny;

  snakeCell(nx, ny, ILI9341_YELLOW);

  if (eating) {
    score += 10;
    beep(1800, 50);
    drawHeader(F("SNAKE"));

    if (snakeLength >= MAX_SNAKE) {
      endGame(true);
    } else {
      spawnFood();
    }
  }
}

// ================= SPACE SHOOTER =================
int shipX = 160;
int enemyX = 160;
int enemyY = 40;

int bulletX = 0;
int bulletY = -1;

unsigned long lastShot = 0;

void drawShip(int x, uint16_t color) {
  tft.fillTriangle(
    x, 202,
    x - 12, 228,
    x + 12, 228,
    color
  );
}

void drawEnemy(int x, int y, uint16_t color) {
  tft.fillRect(x - 10, y, 20, 14, color);
}

void drawBullet(int x, int y, uint16_t color) {
  tft.fillRect(x - 2, y, 4, 9, color);
}

void startShooter() {
  screen = SHOOTER;
  startScreen();

  shipX = 160;
  enemyX = random(20, 300);
  enemyY = 40;

  bulletY = -1;
  lastShot = 0;

  drawHeader(F("SHOOTER"));
  drawShip(shipX, ILI9341_CYAN);
  drawEnemy(enemyX, enemyY, ILI9341_RED);
}

void updateShooter(int joyX, bool shooting) {
  if (millis() - lastTick < 40) return;
  lastTick = millis();

  // Erase only moving objects
  drawShip(shipX, ILI9341_BLACK);
  drawEnemy(enemyX, enemyY, ILI9341_BLACK);

  if (bulletY >= 30) {
    drawBullet(bulletX, bulletY, ILI9341_BLACK);
  }

  // Move spaceship
  if (joyX < 350) shipX -= 5;
  if (joyX > 670) shipX += 5;

  shipX = constrain(shipX, 15, 305);

  // Shoot
  if (shooting && bulletY < 0 &&
      millis() - lastShot > 170) {
    bulletX = shipX;
    bulletY = 190;
    lastShot = millis();
    beep(1700, 15);
  }

  if (bulletY >= 0) {
    bulletY -= 10;
    if (bulletY < 30) bulletY = -1;
  }

  // Enemy speed increases with score
  int enemySpeed = min(10, 2 + (int)score / 30);
  enemyY += enemySpeed;

  // Bullet collision
  if (bulletY >= 0 &&
      bulletX >= enemyX - 12 &&
      bulletX <= enemyX + 12 &&
      bulletY <= enemyY + 14 &&
      bulletY + 9 >= enemyY) {

    score += 10;
    beep(2100, 45);

    enemyX = random(20, 300);
    enemyY = 40;
    bulletY = -1;

    drawHeader(F("SHOOTER"));
  }

  // Enemy reaches player area
  if (enemyY + 14 >= 202) {
    endGame();
    return;
  }

  drawShip(shipX, ILI9341_CYAN);
  drawEnemy(enemyX, enemyY, ILI9341_RED);

  if (bulletY >= 30) {
    drawBullet(bulletX, bulletY, ILI9341_YELLOW);
  }
}

// ================= BRICK BREAKER =================
const byte BRICK_ROWS = 4;
const byte BRICK_COLS = 8;

bool bricks[BRICK_ROWS][BRICK_COLS];

int paddleX = 160;
int ballX = 160, ballY = 150;

int ballVX = 2, ballVY = -2;
int ballStep = 0;

void drawPaddle(int x, uint16_t color) {
  tft.fillRect(x - 27, 225, 54, 8, color);
}

void drawBall(int x, int y, uint16_t color) {
  tft.fillCircle(x, y, 4, color);
}

void drawBrick(byte r, byte c, uint16_t color) {
  tft.fillRect(
    c * 39 + 4,
    r * 15 + 40,
    35, 11,
    color
  );
}

void startBreakout() {
  screen = BREAKOUT;
  startScreen();

  paddleX = 160;
  ballX = 160;
  ballY = 150;

  ballVX = 2;
  ballVY = -2;
  ballStep = 0;

  drawHeader(F("BRICKS"));

  for (byte r = 0; r < BRICK_ROWS; r++) {
    for (byte c = 0; c < BRICK_COLS; c++) {
      bricks[r][c] = true;
      drawBrick(
        r, c,
        r % 2 ? ILI9341_ORANGE : ILI9341_GREEN
      );
    }
  }

  drawPaddle(paddleX, ILI9341_CYAN);
  drawBall(ballX, ballY, ILI9341_WHITE);
}

void updateBreakout(int joyX) {
  // Increase update frequency as score increases
  unsigned long interval =
    max(14L, 35L - (long)score / 25);

  if (millis() - lastTick < interval) return;
  lastTick = millis();

  // Erase only ball and paddle
  drawBall(ballX, ballY, ILI9341_BLACK);
  drawPaddle(paddleX, ILI9341_BLACK);

  if (joyX < 350) paddleX -= 6;
  if (joyX > 670) paddleX += 6;

  paddleX = constrain(paddleX, 28, 292);

  // Move ball
  ballX += ballVX;
  ballY += ballVY;

  // Wall collisions
  if (ballX <= 5) {
    ballX = 5;
    ballVX = abs(ballVX);
  }

  if (ballX >= 315) {
    ballX = 315;
    ballVX = -abs(ballVX);
  }

  if (ballY <= 35) {
    ballY = 35;
    ballVY = abs(ballVY);
  }

  // Paddle collision
  if (ballVY > 0 &&
      ballY + 4 >= 225 &&
      ballY - 4 <= 233 &&
      abs(ballX - paddleX) <= 31) {

    ballY = 220;
    ballVY = -abs(ballVY);

    // Change horizontal direction based on impact
    int offset = ballX - paddleX;

    if (offset < -10) ballVX = -abs(ballVX);
    if (offset > 10) ballVX = abs(ballVX);

    beep(1100, 25);
  }

  if (ballY > 240) {
    endGame();
    return;
  }

  // Brick collision
  bool hitBrick = false;

  for (byte r = 0; r < BRICK_ROWS && !hitBrick; r++) {
    for (byte c = 0; c < BRICK_COLS; c++) {
      if (!bricks[r][c]) continue;

      int bx = c * 39 + 4;
      int by = r * 15 + 40;

      if (ballX + 4 >= bx &&
          ballX - 4 <= bx + 35 &&
          ballY + 4 >= by &&
          ballY - 4 <= by + 11) {

        bricks[r][c] = false;
        drawBrick(r, c, ILI9341_BLACK);

        ballVY = -ballVY;
        score += 10;
        hitBrick = true;

        beep(1800, 40);
        drawHeader(F("BRICKS"));
        break;
      }
    }
  }

  // All bricks destroyed
  if (score >= BRICK_ROWS * BRICK_COLS * 10) {
    endGame(true);
    return;
  }

  // Draw new positions
  drawPaddle(paddleX, ILI9341_CYAN);
  drawBall(ballX, ballY, ILI9341_WHITE);
}

// ================= PLACEHOLDERS =================
void startPlaceholder() {
  screen = PLACEHOLDER;
  tft.fillScreen(ILI9341_BLACK);

  tft.setTextSize(2);
  tft.setTextColor(ILI9341_GREEN);

  tft.setCursor(30, 70);
  tft.print(menuItems[selected]);

  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(55, 120);
  tft.print(F("COMING SOON"));

  tft.setTextSize(1);
  tft.setCursor(55, 200);
  tft.print(F("Press BACK to return"));
}

// ================= START SELECTED =================
void startSelected() {
  beep(1500, 60);

  switch (selected) {
    case 0:
      startSnake();
      break;
    case 1:
      startShooter();
      break;
    case 2:
      startBreakout();
      break;
    default:
      startPlaceholder();
      break;
  }
}

void restartGame() {
  switch (screen) {
    case SNAKE:
      startSnake();
      break;
    case SHOOTER:
      startShooter();
      break;
    case BREAKOUT:
      startBreakout();
      break;
    default:
      break;
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);

  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(ENC_CLK, INPUT_PULLUP);
  pinMode(ENC_DT, INPUT_PULLUP);
  pinMode(ENC_SW, INPUT_PULLUP);
  pinMode(BACK_BTN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  oldEncCLK = digitalRead(ENC_CLK);

  randomSeed(analogRead(A2));

  tft.begin();
  tft.setRotation(1);

  screen = MENU;
  drawMenu();
}

// ================= LOOP =================
void loop() {
  unsigned long now = millis();

  int joyX = analogRead(JOY_X);
  int joyY = analogRead(JOY_Y);

  bool joySW = digitalRead(JOY_SW);
  bool encSW = digitalRead(ENC_SW);
  bool backSW = digitalRead(BACK_BTN);

  int encCLK = digitalRead(ENC_CLK);

  bool joyPressed =
    joySW == LOW && oldJoySW == HIGH;

  bool encPressed =
    encSW == LOW && oldEncSW == HIGH;

  bool backPressed =
    backSW == LOW && oldBackSW == HIGH;

  oldJoySW = joySW;
  oldEncSW = encSW;
  oldBackSW = backSW;

  // Back button works in every game
  if (backPressed && screen != MENU) {
    screen = MENU;
    beep(1000, 40);
    drawMenu();
    lastPress = now;
    oldEncCLK = encCLK;
    return;
  }

  if (screen == MENU) {
    // Joystick menu movement
    if (now - lastMenuMove > 220) {
      if (joyY < 300) {
        moveMenu(-1);
        lastMenuMove = now;
      } else if (joyY > 700) {
        moveMenu(1);
        lastMenuMove = now;
      }
    }

    // Rotary encoder menu movement
    if (oldEncCLK == HIGH &&
        encCLK == LOW &&
        now - lastMenuMove > 70) {

      if (digitalRead(ENC_DT) == HIGH) {
        moveMenu(1);
      } else {
        moveMenu(-1);
      }

      lastMenuMove = now;
    }

    // Select
    if ((joyPressed || encPressed) &&
        now - lastPress > 180) {
      startSelected();
      lastPress = now;
    }
  } else {
    if (gameOver) {
      if ((joyPressed || encPressed) &&
          now - lastPress > 180) {
        restartGame();
        lastPress = now;
      }
    } else {
      switch (screen) {
        case SNAKE:
          updateSnake(joyX, joyY);
          break;

        case SHOOTER:
          updateShooter(joyX, joySW == LOW);
          break;

        case BREAKOUT:
          updateBreakout(joyX);
          break;

        default:
          break;
      }
    }
  }

  oldEncCLK = encCLK;
  delay(3);
}
