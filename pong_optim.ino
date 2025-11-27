/*
  Einfaches Pong-Spiel für SSD1306 OLED, mit größerem Ball (3x3 Pixel),
  separatem START-Button, Spin, Mittellinie, Pause-Funktion,
  Hard-Reset (D3+D4) und Demo-/Attract-Mode (CPU vs CPU).

  Verkabelung:
  OLED 128x64 (I2C):
    SDA -> A4
    SCL -> A5
    VCC -> 5V
    GND -> GND

  Taster:
    UP    : GND - Taster - D2
    DOWN  : GND - Taster - D3
    START : GND - Taster - D4

  Logik: INPUT_PULLUP -> LOW = gedrückt
*/

#include <Wire.h>
#include <Adafruit_GFX.h>

#define SSD1306_NO_SPLASH
#include <Adafruit_SSD1306.h>

#define UP_BUTTON     2   // Taster nach GND -> Hoch
#define DOWN_BUTTON   3   // Taster nach GND -> Runter
#define START_BUTTON  4   // Taster nach GND -> Start/Restart/Pause

#define GAME_TITLE    "PONG OLED"
#define GAME_VERSION  "v1.0.0"
const unsigned long PADDLE_RATE    = 33;
const unsigned long BALL_RATE      = 16;
const unsigned long DEMO_TIMEOUT_MS = 15000; // 15 Sekunden bis Demo

const uint8_t PADDLE_HEIGHT = 16;
const uint8_t BALL_SIZE     = 3;   // Ballgröße: 3x3 Pixel

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64

#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Spielzustände
enum GameState {
  STATE_START,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_END,
  STATE_DEMO
};

GameState gameState = STATE_START;

// Ball (Zentrum)
uint8_t ball_x = 64, ball_y = 32;
int8_t  ball_dir_x = 1, ball_dir_y = 1;
unsigned long ball_update;

// Paddles
unsigned long paddle_update;

const uint8_t CPU_X    = 12;
const uint8_t PLAYER_X = 115;

uint8_t cpu_y    = 16;
uint8_t player_y = 16;

// Punkte
int8_t cpuScore    = 0;
int8_t playerScore = 0;

// Demo-Flag & Inaktivitäts-Timer
bool demoMode = false;
unsigned long lastInputTime = 0;

// Vorwärtsdeklarationen
void drawCourt();
void drawStartScreen();
void drawPauseScreen();
void drawScores();
void drawEndScreen(bool playerWon);
void resetBall(int dir);
void handleScore(bool playerScored);
void applySpin(bool fromPlayer);
void renderPlayingFrame();

// Ball als Quadrat zeichnen (um das Zentrum ball_x/ball_y)
void drawBall(uint8_t x, uint8_t y, uint16_t color) {
  int8_t half = BALL_SIZE / 2;
  display.fillRect(x - half, y - half, BALL_SIZE, BALL_SIZE, color);
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  pinMode(UP_BUTTON,     INPUT_PULLUP);
  pinMode(DOWN_BUTTON,   INPUT_PULLUP);
  pinMode(START_BUTTON,  INPUT_PULLUP);

  randomSeed(analogRead(A0));

  drawStartScreen();

  unsigned long now = millis();
  ball_update    = now;
  paddle_update  = now;
  lastInputTime  = now;
}

void loop() {
  unsigned long time = millis();

  bool upPressed   = (digitalRead(UP_BUTTON)   == LOW);
  bool downPressed = (digitalRead(DOWN_BUTTON) == LOW);
  bool startPressed = (digitalRead(START_BUTTON) == LOW);

  // Jede Taste = Aktivität (nur relevant im Start-Screen für Demo-Timeout)
  if (upPressed || downPressed || startPressed) {
    lastInputTime = time;
  }

  // START-Button + DOWN für Click-Events
  static bool lastStartPressed = false;
  static bool lastDownPressed  = false;

  bool comboClick = false;
  bool startClick = false;

  // Kombi-Release: beide waren gedrückt, jetzt beide losgelassen
  if (!startPressed && !downPressed && lastStartPressed && lastDownPressed) {
    comboClick = true;
  }
  // Normaler Start-Click: nur START-Button-Release
  else if (!startPressed && lastStartPressed) {
    startClick = true;
  }

  static bool up_state   = false;
  static bool down_state = false;

  // Globaler Hard-Reset: unabhängig vom State
  if (comboClick) {
    cpuScore    = 0;
    playerScore = 0;
    cpu_y       = 16;
    player_y    = 16;
    resetBall(+1);

    ball_update    = time;
    paddle_update  = time;
    gameState      = STATE_START;
    demoMode       = false;

    drawStartScreen();
    lastInputTime = time;

    // Zustände merken und Loop beenden
    lastStartPressed = startPressed;
    lastDownPressed  = downPressed;
    return;
  }

  switch (gameState) {

    case STATE_START:
      demoMode = false;

      // Start nur über START-Click (Loslassen)
      if (startClick) {
        cpuScore    = 0;
        playerScore = 0;
        cpu_y       = 16;
        player_y    = 16;
        resetBall(+1);

        ball_update    = time;
        paddle_update  = time;
        gameState      = STATE_PLAYING;
        demoMode       = false;

        renderPlayingFrame();
      } else {
        // Wenn lange keine Eingabe im Startscreen: Demo/Attract-Mode
        if (time - lastInputTime > DEMO_TIMEOUT_MS) {
          cpuScore    = 0;
          playerScore = 0;
          cpu_y       = 16;
          player_y    = 16;
          resetBall(+1);

          ball_update    = time;
          paddle_update  = time;
          gameState      = STATE_DEMO;
          demoMode       = true;

          renderPlayingFrame();
        }
      }
      break;

    case STATE_PLAYING: {
      demoMode = false;

      // START im Spiel = Pause (auf Click, nicht Dauer-LOW)
      if (startClick) {
        gameState = STATE_PAUSED;
        drawPauseScreen();
        break;
      }

      bool update = false;

      // Bewegung nur über UP/DOWN (hier darf gedrückt-bleiben wirken)
      up_state   |= upPressed;
      down_state |= downPressed;

      // Ball-Logik
      if (time > ball_update) {
        int16_t new_x = (int16_t)ball_x + ball_dir_x;
        int16_t new_y = (int16_t)ball_y + ball_dir_y;

        // obere/untere Wand
        if (new_y <= 0 || new_y >= 63) {
          ball_dir_y = -ball_dir_y;
          new_y     += ball_dir_y + ball_dir_y;

          // Sicherheit: innerhalb Spielfeld halten
          if (new_y < 1)  new_y = 1;
          if (new_y > 62) new_y = 62;
        }

        // CPU-Paddle
        if (new_x == CPU_X &&
            new_y >= cpu_y &&
            new_y <= cpu_y + PADDLE_HEIGHT) {
          ball_dir_x = -ball_dir_x;
          new_x     += ball_dir_x + ball_dir_x;
          applySpin(false);  // Spin bei CPU-Kontakt (seltener)
        }

        // Spieler-Paddle
        if (new_x == PLAYER_X &&
            new_y >= player_y &&
            new_y <= player_y + PADDLE_HEIGHT) {
          ball_dir_x = -ball_dir_x;
          new_x     += ball_dir_x + ball_dir_x;
          applySpin(true);   // Spin bei Spieler-Kontakt (etwas häufiger)
        }

        // Tor links (CPU verfehlt) -> Spielerpunkt
        if (new_x <= 1) {
          handleScore(true);
          ball_update = time + BALL_RATE;
          update = true;
        }
        // Tor rechts (Spieler verfehlt) -> CPU-Punkt
        else if (new_x >= 126) {
          handleScore(false);
          ball_update = time + BALL_RATE;
          update = true;
        } else {
          ball_x = (uint8_t)new_x;
          ball_y = (uint8_t)new_y;

          ball_update += BALL_RATE;
          update = true;
        }
      }

      // Paddles-Logik
      if (time > paddle_update && gameState == STATE_PLAYING) {
        paddle_update += PADDLE_RATE;

        // CPU-Paddle (immer direkt dem Ball folgen)
        const uint8_t half_paddle = PADDLE_HEIGHT >> 1;
        int16_t center = cpu_y + half_paddle;
        if (center > ball_y) {
          cpu_y--;
        } else if (center < ball_y) {
          cpu_y++;
        }

        if (cpu_y < 1) cpu_y = 1;
        if (cpu_y + PADDLE_HEIGHT > 63) cpu_y = 63 - PADDLE_HEIGHT;

        // Spieler-Paddle
        if (up_state)   player_y--;
        if (down_state) player_y++;
        up_state = down_state = false;

        if (player_y < 1) player_y = 1;
        if (player_y + PADDLE_HEIGHT > 63) player_y = 63 - PADDLE_HEIGHT;

        update = true;
      }

      if (update && gameState == STATE_PLAYING) {
        renderPlayingFrame();
      }
      break;
    }

    case STATE_PAUSED:
      demoMode = false;

      // In Pause: nur START-Click reagiert, Up/Down ignorieren
      if (startClick) {
        // Zeitbasis neu setzen, damit der Ball nicht "vorsprungen" ist
        {
          unsigned long now2 = millis();
          ball_update   = now2;
          paddle_update = now2;
        }
        gameState = STATE_PLAYING;
        renderPlayingFrame();
      }
      break;

    case STATE_END:
      demoMode = false;

      // Neustart nur über START-Click -> zurück zum Startscreen
      if (startClick) {
        drawStartScreen();
        gameState = STATE_START;
        lastInputTime = time;
      }
      break;

    case STATE_DEMO: {
      // Demo-Mode: CPU vs CPU
      demoMode = true;

      // Jede beliebige Taste beendet den Demo-Mode
      if (upPressed || downPressed || startPressed || startClick) {
        gameState = STATE_START;
        demoMode  = false;
        drawStartScreen();
        lastInputTime = time;
        break;
      }

      bool update = false;

      // Ball-Logik (ohne Scoring, Ball prallt an allen Wänden ab)
      if (time > ball_update) {
        int16_t new_x = (int16_t)ball_x + ball_dir_x;
        int16_t new_y = (int16_t)ball_y + ball_dir_y;

        // obere/untere Wand
        if (new_y <= 0 || new_y >= 63) {
          ball_dir_y = -ball_dir_y;
          new_y     += ball_dir_y + ball_dir_y;
          if (new_y < 1)  new_y = 1;
          if (new_y > 62) new_y = 62;
        }

        // linkes Paddle (CPU)
        if (new_x == CPU_X &&
            new_y >= cpu_y &&
            new_y <= cpu_y + PADDLE_HEIGHT) {
          ball_dir_x = -ball_dir_x;
          new_x     += ball_dir_x + ball_dir_x;
          applySpin(false);
        }

        // rechtes Paddle (zweite CPU)
        if (new_x == PLAYER_X &&
            new_y >= player_y &&
            new_y <= player_y + PADDLE_HEIGHT) {
          ball_dir_x = -ball_dir_x;
          new_x     += ball_dir_x + ball_dir_x;
          applySpin(false);
        }

        // links/rechts: nur abprallen, keine Punkte
        if (new_x <= 1 || new_x >= 126) {
          ball_dir_x = -ball_dir_x;
          new_x     += ball_dir_x + ball_dir_x;
        }

        ball_x = (uint8_t)new_x;
        ball_y = (uint8_t)new_y;

        ball_update += BALL_RATE;
        update = true;
      }

      // Beide Paddles folgen dem Ball
      if (time > paddle_update) {
        paddle_update += PADDLE_RATE;

        const uint8_t half_paddle = PADDLE_HEIGHT >> 1;

        // Linkes Paddle
        int16_t centerL = cpu_y + half_paddle;
        if (centerL > ball_y) cpu_y--;
        else if (centerL < ball_y) cpu_y++;

        if (cpu_y < 1) cpu_y = 1;
        if (cpu_y + PADDLE_HEIGHT > 63) cpu_y = 63 - PADDLE_HEIGHT;

        // Rechtes Paddle (CPU2)
        int16_t centerR = player_y + half_paddle;
        if (centerR > ball_y) player_y--;
        else if (centerR < ball_y) player_y++;

        if (player_y < 1) player_y = 1;
        if (player_y + PADDLE_HEIGHT > 63) player_y = 63 - PADDLE_HEIGHT;

        update = true;
      }

      if (update && gameState == STATE_DEMO) {
        renderPlayingFrame();
      }
      break;
    }
  }

  // Button-Zustände für die nächste Runde merken
  lastStartPressed = startPressed;
  lastDownPressed  = downPressed;
}

void drawCourt() {
  // Außenrand
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);

  // Mittellinie: 2 Pixel breit, durchgehend
  uint8_t centerX1 = (SCREEN_WIDTH / 2) - 1;  // 63
  uint8_t centerX2 = (SCREEN_WIDTH / 2);      // 64

  for (uint8_t y = 1; y < SCREEN_HEIGHT - 1; y++) {
    display.drawPixel(centerX1, y, WHITE);
    display.drawPixel(centerX2, y, WHITE);
  }
}

void drawStartScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println(F("PONG OLED"));

  display.setTextSize(1);
  display.setCursor(15, 28);
  display.println(F("Version v1.0.0"));

  display.setCursor(42, 44);
  display.println(F("UP/DOWN"));
  display.setCursor(42, 54);
  display.println(F("START"));
  display.display();
}

void drawPauseScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(25, 15);
  display.println(F("PAUSE"));

  display.setTextSize(1);
  display.setCursor(10, 40);
  display.println(F("START = weiter"));
  display.display();
}

void drawScores() {
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);

  if (demoMode) {
    // Im Demo-Mode nur ein kleines "DEMO" anzeigen
    int16_t x = (SCREEN_WIDTH / 2) - 12;
    display.setCursor(x, 2);
    display.print(F("DEMO"));
  } else {
    // CPU-Score links
    display.setCursor(5, 2);
    display.print(cpuScore);

    // Spieler-Score rechts
    display.setCursor(SCREEN_WIDTH - 10, 2);
    display.print(playerScore);
  }
}

void drawEndScreen(bool playerWon) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(10, 10);
  display.println(F("GAME OVER"));

  display.setTextSize(1);
  display.setCursor(10, 35);
  display.println(playerWon ? F("Du gewinnst") : F("CPU gewinnt"));

  display.setCursor(10, 50);
  display.println(F("START = Menu"));
  display.display();
}

void resetBall(int dir) {
  ball_x = SCREEN_WIDTH  / 2;
  ball_y = SCREEN_HEIGHT / 2;

  ball_dir_x = (dir >= 0) ? 1 : -1;
  ball_dir_y = (random(0, 2) == 0) ? -1 : 1;
}

// Spin-Logik: vertikale Geschwindigkeit leicht ändern
// fromPlayer = true  -> höhere Chance auf Spin (Vorteil Spieler)
// fromPlayer = false -> geringere Chance (CPU)
void applySpin(bool fromPlayer) {
  // Chance: Spieler ~ 1/3, CPU ~ 1/6
  uint8_t chance = fromPlayer ? 3 : 6;
  if (random(chance) != 0) {
    return;  // kein Spin diesmal
  }

  // delta = +1 oder -1
  int8_t delta = (random(0, 2) == 0) ? -1 : 1;
  ball_dir_y += delta;

  // Begrenzen: Ball nicht zu extrem machen
  if (ball_dir_y >  2) ball_dir_y =  2;
  if (ball_dir_y < -2) ball_dir_y = -2;

  // Nie 0, sonst fliegt er exakt horizontal
  if (ball_dir_y == 0) {
    ball_dir_y = (delta > 0) ? 1 : -1;
  }
}

void handleScore(bool playerScored) {
  if (playerScored) {
    playerScore++;
  } else {
    cpuScore++;
  }

  if (playerScore >= 5 || cpuScore >= 5) {
    drawEndScreen(playerScore > cpuScore);
    gameState = STATE_END;
  } else {
    // neue Runde: Ball Richtung Verlierer-Seite
    int dir = playerScored ? -1 : +1;
    resetBall(dir);
  }
}

// kompletten Frame im Spielzustand zeichnen
void renderPlayingFrame() {
  display.clearDisplay();
  drawCourt();
  drawScores();

  // CPU-Paddle
  display.drawFastVLine(CPU_X, cpu_y, PADDLE_HEIGHT, WHITE);
  // Spieler-/rechter Paddle (Player oder CPU, je nach Modus)
  display.drawFastVLine(PLAYER_X, player_y, PADDLE_HEIGHT, WHITE);
  // Ball
  drawBall(ball_x, ball_y, WHITE);

  display.display();
}
