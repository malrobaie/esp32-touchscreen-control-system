#include <Arduino_GFX_Library.h>
#include <Wire.h>

#define BLACK     0x0000
#define WHITE     0xFFFF
#define RED       0xF800
#define GREEN     0x07E0
#define BLUE      0x001F
#define YELLOW    0xFFE0
#define CYAN      0x07FF
#define LIGHTBLUE 0x867F
#define LIGHTGREY 0xC618
#define PURPLE    0x801F

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8
#define TFT_MOSI 11
#define TFT_SCK  12
#define TFT_MISO 13

#define TOUCH_SDA 18
#define TOUCH_SCL 17
#define TOUCH_RST 16
#define TOUCH_ADDR 0x38

#define SCREEN_W 480
#define SCREEN_H 320

#define GAME_TOP 70
#define GAME_W 480
#define GAME_H 250

Arduino_DataBus *bus = new Arduino_ESP32SPI(
  TFT_DC,
  TFT_CS,
  TFT_SCK,
  TFT_MOSI,
  TFT_MISO,
  HSPI,
  true
);

Arduino_GFX *gfx = new Arduino_ST7796(
  bus,
  TFT_RST,
  1,
  true,
  320,
  480
);

Arduino_Canvas *canvas = nullptr;

enum ScreenMode {
  STOPWATCH,
  GAME
};

ScreenMode mode = STOPWATCH;

// Stopwatch
bool running = false;
unsigned long startTime = 0;
unsigned long savedTime = 0;
unsigned long lastSecond = 999999;
unsigned long lastTouch = 0;

// Game constants
const int BIRD_X = 90;
const int BIRD_R = 10;
const int PIPE_W = 45;
const int GAP_H = 100;

// Smoother edge motion than 6 px/frame
const float PIPE_SPEED = 4.5;

// Game variables
float birdY = 160;
float birdVelocity = 0;
float pipeX = 480;
int gapY = 130;
int score = 0;
int prevScore = -1;
bool gameOver = false;
unsigned long lastFrame = 0;

bool getRawTouch(uint16_t &x, uint16_t &y) {
  Wire.beginTransmission(TOUCH_ADDR);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;

  Wire.requestFrom(TOUCH_ADDR, 5);
  if (Wire.available() < 5) return false;

  uint8_t status = Wire.read();
  uint8_t xh = Wire.read();
  uint8_t xl = Wire.read();
  uint8_t yh = Wire.read();
  uint8_t yl = Wire.read();

  if ((status & 0x0F) == 0) return false;

  x = ((xh & 0x0F) << 8) | xl;
  y = ((yh & 0x0F) << 8) | yl;

  return true;
}

// Convert portrait raw touch to landscape screen coordinates
bool getTouch(uint16_t &sx, uint16_t &sy) {
  uint16_t rx, ry;
  if (!getRawTouch(rx, ry)) return false;

  sx = ry;
  sy = 319 - rx;
  return true;
}

void drawTime(unsigned long elapsed) {
  unsigned long seconds = elapsed / 1000;
  unsigned long minutes = seconds / 60;
  seconds %= 60;

  char timeText[20];
  sprintf(timeText, "%02lu:%02lu", minutes, seconds);

  gfx->fillRect(100, 120, 300, 90, BLACK);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(7);
  gfx->setCursor(120, 130);
  gfx->println(timeText);
}

void drawStartStopButton() {
  gfx->fillRoundRect(60, 250, 140, 55, 12, running ? RED : GREEN);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(88, 267);
  gfx->println(running ? "STOP" : "START");
}

void drawStopwatchUI() {
  gfx->fillScreen(BLACK);

  gfx->setTextColor(CYAN);
  gfx->setTextSize(4);
  gfx->setCursor(90, 35);
  gfx->println("STOPWATCH");

  drawTime(running ? savedTime + millis() - startTime : savedTime);
  drawStartStopButton();

  gfx->fillRoundRect(220, 250, 110, 55, 12, YELLOW);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(236, 267);
  gfx->println("RESET");

  gfx->fillRoundRect(350, 250, 90, 55, 12, CYAN);
  gfx->setTextColor(BLACK);
  gfx->setTextSize(3);
  gfx->setCursor(362, 267);
  gfx->println("GAME");

  gfx->setTextColor(LIGHTGREY);
  gfx->setTextSize(2);
  gfx->setCursor(50, 330);
  gfx->println("Tap GAME to play Flappy");
}

void drawGameHeader() {
  gfx->fillRect(0, 0, 480, GAME_TOP, LIGHTBLUE);

  gfx->setTextColor(BLACK);
  gfx->setTextSize(2);

  gfx->setCursor(10, 15);
  gfx->print("Score: ");
  gfx->println(score);

  gfx->setCursor(360, 15);
  gfx->println("BACK");

  gfx->drawFastHLine(0, GAME_TOP - 1, 480, WHITE);

  prevScore = score;
}

void updateScoreHeader() {
  if (score != prevScore) {
    gfx->fillRect(0, 0, 210, GAME_TOP - 2, LIGHTBLUE);
    gfx->setTextColor(BLACK);
    gfx->setTextSize(2);
    gfx->setCursor(10, 15);
    gfx->print("Score: ");
    gfx->println(score);
    prevScore = score;
  }
}

void resetGame() {
  birdY = 160;
  birdVelocity = 0;
  pipeX = SCREEN_W;
  gapY = GAME_TOP + 35 + random(0, 120);
  score = 0;
  prevScore = -1;
  gameOver = false;
  lastFrame = millis();

  gfx->fillScreen(BLACK);
  drawGameHeader();

  canvas->fillScreen(BLACK);
  canvas->flush();
}

void drawGame() {
  updateScoreHeader();

  canvas->fillScreen(BLACK);

  int drawPipeX = (int)pipeX;

  int topPipeHeight = gapY - GAME_TOP;
  int bottomPipeY = gapY + GAP_H - GAME_TOP;
  int bottomPipeHeight = GAME_H - bottomPipeY;

  if (drawPipeX > -PIPE_W && drawPipeX < SCREEN_W) {
    canvas->fillRect(drawPipeX, 0, PIPE_W, topPipeHeight, PURPLE);
    canvas->fillRect(drawPipeX, bottomPipeY, PIPE_W, bottomPipeHeight, PURPLE);
  }

  canvas->fillCircle(BIRD_X, (int)birdY - GAME_TOP, BIRD_R, YELLOW);

  if (gameOver) {
    canvas->setTextColor(RED);
    canvas->setTextSize(4);
    canvas->setCursor(115, 60);
    canvas->println("GAME OVER");

    canvas->setTextColor(WHITE);
    canvas->setTextSize(2);
    canvas->setCursor(115, 120);
    canvas->println("Tap screen to restart");

    canvas->setCursor(115, 150);
    canvas->println("Tap BACK to return");
  }

  canvas->flush();
}

void updateGame() {
  if (millis() - lastFrame < 25) return; // about 40 FPS
  lastFrame = millis();

  if (!gameOver) {
    birdVelocity += 0.65;
    birdY += birdVelocity;

    pipeX -= PIPE_SPEED;

    if (pipeX < -PIPE_W) {
      pipeX = SCREEN_W;
      gapY = GAME_TOP + 30 + random(0, 130);
      score++;
    }

    int hitPipeX = (int)pipeX;

    bool hitPipe =
      BIRD_X + BIRD_R > hitPipeX &&
      BIRD_X - BIRD_R < hitPipeX + PIPE_W &&
      (birdY - BIRD_R < gapY || birdY + BIRD_R > gapY + GAP_H);

    bool hitWall =
      birdY < GAME_TOP + BIRD_R ||
      birdY > SCREEN_H - BIRD_R;

    if (hitPipe || hitWall) {
      gameOver = true;
    }
  }

  drawGame();
}

void setup() {
  Serial.begin(115200);

  pinMode(TOUCH_RST, OUTPUT);
  digitalWrite(TOUCH_RST, LOW);
  delay(50);
  digitalWrite(TOUCH_RST, HIGH);
  delay(200);

  Wire.begin(TOUCH_SDA, TOUCH_SCL);

  if (!gfx->begin()) {
    Serial.println("Display failed!");
    while (1);
  }

  canvas = new Arduino_Canvas(GAME_W, GAME_H, gfx, 0, GAME_TOP);
  canvas->begin();

  Serial.println(psramFound() ? "PSRAM OK" : "NO PSRAM");

  Serial.print("PSRAM Size: ");
  Serial.println(ESP.getPsramSize());

  Serial.print("Free PSRAM: ");
  Serial.println(ESP.getFreePsram());

  randomSeed(analogRead(1));

  drawStopwatchUI();
}

void loop() {
  uint16_t x, y;

  if (mode == STOPWATCH) {
    unsigned long elapsed = savedTime;

    if (running) {
      elapsed += millis() - startTime;
    }

    if (elapsed / 1000 != lastSecond) {
      drawTime(elapsed);
      lastSecond = elapsed / 1000;
    }

    if (getTouch(x, y) && millis() - lastTouch > 350) {
      lastTouch = millis();

      if (x >= 60 && x <= 200 && y >= 250 && y <= 305) {
        if (running) {
          savedTime += millis() - startTime;
          running = false;
        } else {
          startTime = millis();
          running = true;
        }
        drawStartStopButton();
      }

      if (x >= 220 && x <= 330 && y >= 250 && y <= 305) {
        running = false;
        savedTime = 0;
        lastSecond = 999999;
        drawStartStopButton();
        drawTime(0);
      }

      if (x >= 350 && x <= 440 && y >= 250 && y <= 305) {
        mode = GAME;
        resetGame();
      }
    }
  }

  if (mode == GAME) {
    updateGame();

    if (getTouch(x, y) && millis() - lastTouch > 180) {
      lastTouch = millis();

      if (x >= 350 && x <= 470 && y >= 0 && y <= GAME_TOP) {
        mode = STOPWATCH;
        drawStopwatchUI();
        lastSecond = 999999;
        return;
      }

      if (gameOver) {
        resetGame();
      } else {
        birdVelocity = -9.0;
      }
    }
  }
}