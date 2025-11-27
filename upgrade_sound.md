# Upgrade-Sound für „PONG OLED“  
_Buzzer-Erweiterung für dein bestehendes Projekt_

Dieses Dokument beschreibt, wie du deinen bestehenden **PONG OLED** Sketch (ohne Sound) um einen **passiven Piezo-Buzzer** erweiterst.  
Ziel: sauberes, nachvollziehbares Upgrade mit klarer Verkabelung und minimal-invasiven Code-Änderungen.

---

## 1. Hardware-Voraussetzungen

### 1.1 Bauteile

- 1 × passiver Piezo-Buzzer (5 V geeignet)
- 1 × Widerstand **220 Ω** (Vorwiderstand, saubere Lösung)
- ein paar Jumper-Kabel (m/w oder m/m)
- 1 × Arduino UNO (bzw. dein bereits verwendetes UNO-kompatibles Board)
- dein vorhandenes Setup:
  - SSD1306 OLED 128×64 (I²C)
  - 3 Taster (UP, DOWN, START)

### 1.2 Wahl des Buzzers

Verwendet werden soll ein **passiver** Buzzer, kein aktiver.

- **Passiver Buzzer**:
  - erzeugt nur einen Ton, wenn er mit einem Rechtecksignal bestromt wird
  - Tonhöhe wird über die Frequenz gesteuert (`tone(pin, freq, duration)`)
  - geeignet für unterschiedliche Sounds / Melodien

- **Aktiver Buzzer**:
  - hat eine interne Elektronik, erzeugt festen Ton, wenn er 5 V bekommt
  - nicht ideal für dieses Upgrade (Tonhöhe nicht fein steuerbar)

Empfehlung: kleiner **Piezo-Buzzer 5 V, passiv**, wie er in üblichen Arduino-Kits enthalten ist.

---

## 2. Elektrischer Anschluss

Wir verwenden:

- **BUZZER_PIN = D11** am Arduino  
- **220 Ω Vorwiderstand** in Serie

### 2.1 Verschaltung

Schema:

```text
Arduino D11  ----[ 220 Ω ]----( + ) Buzzer ( - )---- Arduino GND
```

Praktisch:

1. Ein Ende des 220 Ω Widerstands auf **D11** stecken.
2. Das andere Ende des Widerstands auf den **Plus-Pin (+)** des Buzzers.
3. Den **Minus-Pin (–)** des Buzzers mit **GND** des Arduino verbinden.

Damit ist:

- die Belastung des Pins begrenzt
- Störungen/Spitzen etwas abgefangen
- die Lösung „Elektriker-/IHK-tauglich sauber“

---

## 3. Code-Änderungen im Überblick

Ausgangspunkt ist dein bestehender PONG-Sketch (ohne Buzzer).  
Hinzu kommen:

1. Eine neue Konstante für den Buzzer-Pin  
2. Initialisierung des Pins in `setup()`  
3. Hilfsfunktionen für Sounds (`beep()` und spezialisierte Ton-Funktionen)  
4. Aufrufe dieser Funktionen an passenden Stellen (Paddle-Treffer, Wandtreffer, Score, Game Over)

Der Demo-Modus (`STATE_DEMO`) bleibt **stumm**, damit der Attract-Mode nicht dauerhaft Lärm macht.

---

## 4. Konstante für den Buzzer-Pin

Im Kopfbereich deines Sketches (bei den `#define` für Taster) fügst du hinzu:

```cpp
#define BUZZER_PIN   11   // passiver Piezo-Buzzer über 220-Ohm-Widerstand
```

Strukturell z. B.:

```cpp
#define UP_BUTTON     2
#define DOWN_BUTTON   3
#define START_BUTTON  4

#define BUZZER_PIN   11
```

---

## 5. Setup: Pin als Ausgang initialisieren

In deiner `setup()`-Funktion, nach den `pinMode()`-Zeilen für die Taster:

```cpp
void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  pinMode(UP_BUTTON,     INPUT_PULLUP);
  pinMode(DOWN_BUTTON,   INPUT_PULLUP);
  pinMode(START_BUTTON,  INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT); // Buzzer-Pin vorbereiten

  randomSeed(analogRead(A0));

  drawStartScreen();

  unsigned long now = millis();
  ball_update    = now;
  paddle_update  = now;
  lastInputTime  = now;
}
```

---

## 6. Sound-Hilfsfunktion `beep()`

Am Ende deines Sketches (unterhalb von `renderPlayingFrame()` oder vor den letzten `}`) fügst du einen allgemeinen „Beep“-Wrapper hinzu:

```cpp
void beep(uint16_t freq, uint16_t dur) {
  if (freq == 0 || dur == 0) return;
  tone(BUZZER_PIN, freq, dur);
  // kein noTone() nötig: tone() mit duration stoppt automatisch
}
```

Diese Funktion kapselt den Zugriff auf den Buzzer sauber.  
Falls du später die Lautstärke oder Logik ändern willst, geschieht das an einer Stelle.

---

## 7. Spezifische Sound-Funktionen

### 7.1 Paddle-Treffer

Direkt unter `beep()` definierst du einzelne Sound-Profile:

```cpp
void playerPaddleTone() {
  // heller Klick beim Spieler-Paddle
  beep(900, 35);
}

void cpuPaddleTone() {
  // etwas dunklerer Klick beim CPU-Paddle
  beep(700, 35);
}
```

### 7.2 Wandtreffer

```cpp
void wallTone() {
  // kurzer tiefer Klick an der Wand
  beep(400, 30);
}
```

### 7.3 Score (Tor) für Spieler und CPU

```cpp
void playerScoreTone() {
  // kurzer "positiver" Doppelton
  beep(800, 70);
  delay(90);
  beep(1000, 90);
}

void cpuScoreTone() {
  // kurzer "negativer" Doppelton
  beep(500, 70);
  delay(90);
  beep(350, 90);
}
```

### 7.4 Game-Over-Melodie

```cpp
void gameOverTone(bool playerWon) {
  if (playerWon) {
    // kleine Siegesfanfare
    beep(900, 120);
    delay(140);
    beep(1100, 160);
    delay(180);
    beep(1300, 200);
  } else {
    // "traurigere" Niederlagenfolge
    beep(400, 160);
    delay(180);
    beep(300, 200);
  }
}
```

---

## 8. Sound an die Spielereignisse hängen

### 8.1 Wandtreffer (oben/unten)

Im `STATE_PLAYING` Block, innerhalb der Ball-Logik, hast du bereits:

```cpp
if (new_y <= 0 || new_y >= 63) {
  ball_dir_y = -ball_dir_y;
  new_y     += ball_dir_y + ball_dir_y;

  if (new_y < 1)  new_y = 1;
  if (new_y > 62) new_y = 62;
}
```

Erweitere diesen Block um:

```cpp
  wallTone();
```

also:

```cpp
if (new_y <= 0 || new_y >= 63) {
  ball_dir_y = -ball_dir_y;
  new_y     += ball_dir_y + ball_dir_y;

  if (new_y < 1)  new_y = 1;
  if (new_y > 62) new_y = 62;

  wallTone();
}
```

> Hinweis: Im `STATE_DEMO` wird bewusst **kein** `wallTone()` aufgerufen.

### 8.2 Paddle-Treffer CPU / Spieler

CPU-Paddle-Kollision:

```cpp
if (new_x == CPU_X &&
    new_y >= cpu_y &&
    new_y <= cpu_y + PADDLE_HEIGHT) {
  ball_dir_x = -ball_dir_x;
  new_x     += ball_dir_x + ball_dir_x;
  applySpin(false);  // Spin bei CPU-Kontakt (seltener)

  cpuPaddleTone();   // HIER: Sound hinzufügen
}
```

Spieler-Paddle-Kollision:

```cpp
if (new_x == PLAYER_X &&
    new_y >= player_y &&
    new_y <= player_y + PADDLE_HEIGHT) {
  ball_dir_x = -ball_dir_x;
  new_x     += ball_dir_x + ball_dir_x;
  applySpin(true);   // Spin bei Spieler-Kontakt (etwas häufiger)

  playerPaddleTone(); // HIER: Sound hinzufügen
}
```

### 8.3 Tore / Punkte (Score) und Game Over

In deiner `handleScore(bool playerScored)`-Funktion ergänzt du Sound-Aufrufe:

```cpp
void handleScore(bool playerScored) {
  if (playerScored) {
    playerScore++;
    playerScoreTone();   // Sound Spieler-Tor
  } else {
    cpuScore++;
    cpuScoreTone();      // Sound CPU-Tor
  }

  if (playerScore >= 5 || cpuScore >= 5) {
    bool playerWon = (playerScore > cpuScore);
    gameOverTone(playerWon);     // Game-Over-Melodie
    drawEndScreen(playerWon);
    gameState = STATE_END;
  } else {
    // neue Runde: Ball Richtung Verlierer-Seite
    int dir = playerScored ? -1 : +1;
    resetBall(dir);
  }
}
```

Im `STATE_DEMO` wird `handleScore()` nicht verwendet, daher bleibt der Demo-Mode stumm.

---

## 9. Demo-Mode explizit stumm lassen

Im `STATE_DEMO` behandelst du Kollisionen (Wand / Paddles) bereits separat.  
Wichtig: **Keine** der Sound-Funktionen (`wallTone()`, `cpuPaddleTone()`, `playerPaddleTone()`, `playerScoreTone()`, `cpuScoreTone()`, `gameOverTone()`) dort aufrufen.

Damit bleibt:

- Im Demo-Modus nur visuelles „CPU vs. CPU“
- Im echten Spiel volle Audio-Rückmeldung

---

## 10. Kurze Checkliste

1. Hardware
   - [ ] Passiver 5-V-Piezo-Buzzer vorhanden  
   - [ ] 220-Ω-Widerstand in Serie auf D11 geschaltet  
   - [ ] Buzzer-„–“ sauber mit GND verbunden  

2. Code
   - [ ] `#define BUZZER_PIN 11` gesetzt  
   - [ ] `pinMode(BUZZER_PIN, OUTPUT);` in `setup()`  
   - [ ] `beep()` und Soundfunktionen eingefügt  
   - [ ] `wallTone()` bei Wandtreffer im `STATE_PLAYING` eingebaut  
   - [ ] `cpuPaddleTone()` / `playerPaddleTone()` bei Paddle-Kollision eingebaut  
   - [ ] `playerScoreTone()` / `cpuScoreTone()` + `gameOverTone()` in `handleScore()` integriert  
   - [ ] Im Demo-Mode (`STATE_DEMO`) keine Sound-Aufrufe

Wenn alle Häkchen sitzen, ist dein Upgrade sauber.

---
