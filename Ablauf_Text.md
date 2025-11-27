# Textuelles Ablaufdiagramm – PONG OLED

## 1. Ziel des Programms

Das Programm realisiert ein Pong-Spiel auf einem 0,96″-OLED (SSD1306, 128×64) am Arduino UNO.
Gespielt wird Mensch (rechtes Paddle) gegen CPU (linkes Paddle). Zusätzlich gibt es einen Demo-Modus (CPU vs. CPU).

Die Bedienung erfolgt über drei Taster:

* UP (D2)
* DOWN (D3)
* START/PAUSE (D4)

Die Logik arbeitet zustandsbasiert mit einem endlichen Automaten.

---

## 2. Globale Daten und Zustände

### 2.1 Spielzustände (`GameState`)

* `STATE_START`   – Startbildschirm (Titel, Version, Anleitung)
* `STATE_PLAYING` – laufendes Spiel Mensch vs. CPU
* `STATE_PAUSED`  – angehaltenes Spiel (Pausebildschirm)
* `STATE_END`     – Game-Over-Bildschirm mit Gewinneranzeige
* `STATE_DEMO`    – Demo-/Attract-Mode (CPU vs. CPU)

Aktueller Zustand: `gameState`.

### 2.2 Spielobjekte

* Ball:

  * Position: `ball_x`, `ball_y` (Zentrum des 3×3-Ball-Quadrats)
  * Richtung: `ball_dir_x`, `ball_dir_y` (−2 … +2)
* Paddles:

  * Linkes Paddle (CPU): `cpu_y`, x-Position konstant `CPU_X`
  * Rechtes Paddle (Spieler/CPU2): `player_y`, x-Position konstant `PLAYER_X`
* Punktestand:

  * `cpuScore` (0–5)
  * `playerScore` (0–5)

### 2.3 Zeitsteuerung

* `ball_update`   – Zeitpunkt (ms), ab dem der Ball erneut bewegt wird
* `paddle_update` – Zeitpunkt (ms), ab dem die Paddles erneut bewegt werden
* `lastInputTime` – Zeitpunkt der letzten Spieleraktion (für Demo-Timeout)

Konstanten:

* `BALL_RATE`       – Intervall für Ballbewegung (ms)
* `PADDLE_RATE`     – Intervall für Paddlebewegung (ms)
* `DEMO_TIMEOUT_MS` – Inaktivitätsgrenze für Demo-Modus (ms)

### 2.4 Button-Handling

* Rohwerte:

  * `upPressed`, `downPressed`, `startPressed` (LOW = gedrückt)
* Kanten-Erkennung:

  * `lastStartPressed`, `lastDownPressed` (Zustand der Voriteration)
  * daraus abgeleitet:

    * `startClick`   – START wurde losgelassen
    * `comboClick`   – Kombination DOWN+START wurde losgelassen (Hard-Reset)
* Bewegungsflags (Spieler):

  * `up_state`, `down_state` (werden bei Paddle-Update ausgewertet)

### 2.5 Demo-Modus

* `demoMode` (bool) – steuert Darstellung der Punkte / DEMO-Label

---

## 3. Initialisierung (`setup()`)

1. Display initialisieren
   1.1 Aufruf `display.begin(SSD1306_SWITCHCAPVCC, 0x3C)`
   1.2 Display löschen: `display.clearDisplay()`
2. Taster konfigurieren
   2.1 `pinMode(UP_BUTTON, INPUT_PULLUP)`
   2.2 `pinMode(DOWN_BUTTON, INPUT_PULLUP)`
   2.3 `pinMode(START_BUTTON, INPUT_PULLUP)`
3. Zufallsgenerator initialisieren
   3.1 `randomSeed(analogRead(A0))`
4. Startbildschirm anzeigen
   4.1 Funktion `drawStartScreen()` aufrufen
5. Zeitbasis initialisieren
   5.1 `now = millis()`
   5.2 `ball_update   = now`
   5.3 `paddle_update = now`
   5.4 `lastInputTime = now`
6. Spielzustand auf Start setzen
   6.1 `gameState = STATE_START`

Danach folgt die Endlosschleife `loop()`.

---

## 4. Hauptschleife (`loop()`)

Die folgende Sequenz wird permanent wiederholt:

### 4.1 Allgemeine Eingaben & Ereignisse

1. Aktuelle Systemzeit erfassen

   * `time = millis()`
2. Taster einlesen

   * `upPressed   = (digitalRead(UP_BUTTON)   == LOW)`
   * `downPressed = (digitalRead(DOWN_BUTTON) == LOW)`
   * `startPressed= (digitalRead(START_BUTTON)== LOW)`
3. Inaktivitätszeit aktualisieren

   * WENN `upPressed ODER downPressed ODER startPressed`
     DANN `lastInputTime = time`
4. Kanten-Erkennung (Start+Kombi)
   4.1 Standardwerte setzen:
   - `comboClick = false`
   - `startClick = false`
   4.2 Kombi-Loslassen („Hard-Reset-Ereignis“) prüfen
   - WENN `startPressed == false` UND `downPressed == false`
   UND `lastStartPressed == true` UND `lastDownPressed == true`
   DANN `comboClick = true`
   4.3 Einzel-Start-Loslassen („StartClick“) prüfen
   - SONST WENN `startPressed == false` UND `lastStartPressed == true`
   DANN `startClick = true`
5. Spieler-Bewegungsflags deklarieren (nur initial, Details in STATE_PLAYING)

   * `static bool up_state   = false;`
   * `static bool down_state = false;`

### 4.2 Globaler Hard-Reset (zustandsübergreifend)

6. WENN `comboClick == true`
   DANN:
   6.1 Punktestand zurücksetzen
   - `cpuScore = 0`
   - `playerScore = 0`
   6.2 Paddle-Positionen zurücksetzen
   - `cpu_y = 16`
   - `player_y = 16`
   6.3 Ball neu zentrieren mit Start in Spieler-Richtung
   - `resetBall(+1)`
   6.4 Zeitbasis neu setzen
   - `ball_update = time`
   - `paddle_update = time`
   6.5 Spielstatus zurück auf Start
   - `gameState = STATE_START`
   - `demoMode = false`
   6.6 Startbildschirm zeichnen
   - `drawStartScreen()`
   6.7 `lastInputTime = time`
   6.8 Zustände der Taster sichern
   - `lastStartPressed = startPressed`
   - `lastDownPressed  = downPressed`
   6.9 `return;` (Iteration der Hauptschleife beenden)

WENN kein Hard-Reset: weiter mit Zustandsautomat.

---

## 5. Zustandsautomat (Switch über `gameState`)

### 5.1 Zustand `STATE_START` – Startbildschirm

1. `demoMode = false`
2. Prüfe, ob Start-Click vorliegt

   * WENN `startClick == true`
     DANN:
     2.1 Punktestand und Positionen zurücksetzen (neues Spiel)
     - `cpuScore = 0`, `playerScore = 0`
     - `cpu_y = 16`, `player_y = 16`
     2.2 Ball mit Richtung zum Spieler initialisieren
     - `resetBall(+1)`
     2.3 Zeitbasis für Ball und Paddles neu setzen
     - `ball_update   = time`
     - `paddle_update = time`
     2.4 Spielzustand auf „Playing“
     - `gameState = STATE_PLAYING`
     - `demoMode  = false`
     2.5 Aktuellen Spielbildschirm rendern
     - `renderPlayingFrame()`
3. SONST (kein StartClick)

   * Prüfe auf Demo-Timeout:
     WENN `time - lastInputTime > DEMO_TIMEOUT_MS`
     DANN:
     3.1 Punktestand und Positionen zurücksetzen
     - `cpuScore = 0`, `playerScore = 0`
     - `cpu_y = 16`, `player_y = 16`
     3.2 Ball erneut zentrieren
     - `resetBall(+1)`
     3.3 Zeitbasis neu setzen
     - `ball_update   = time`
     - `paddle_update = time`
     3.4 Zustand auf Demo
     - `gameState = STATE_DEMO`
     - `demoMode  = true`
     3.5 Spielfeld zeichnen (DEMO-Anzeige wird über `drawScores()` realisiert)
     - `renderPlayingFrame()`

END-Zustand `STATE_START` → zurück in `loop()`.

---

### 5.2 Zustand `STATE_PLAYING` – laufendes Spiel

#### 5.2.1 Pauseprüfung

1. `demoMode = false`
2. Prüfe, ob Start-Click vorliegt (Pausefunktion)

   * WENN `startClick == true`
     DANN:
     2.1 `gameState = STATE_PAUSED`
     2.2 `drawPauseScreen()`
     2.3 Zustand beendet (kein Bewegungsupdate)

Nur wenn keine Pause ausgelöst wird:

#### 5.2.2 Spieler-Eingaben sammeln

3. Bewegungsflags aktualisieren

   * WENN `upPressed == true` → `up_state   = true`
   * WENN `downPressed == true` → `down_state = true`
     (Ein kurzes Antippen bleibt bis zum nächsten Paddle-Update erhalten.)

#### 5.2.3 Ball-Bewegung

4. `update = false`
5. WENN `time > ball_update`
   DANN:
   5.1 Neue Position berechnen
   - `new_x = ball_x + ball_dir_x`
   - `new_y = ball_y + ball_dir_y`
   5.2 Kollisionsprüfung oben/unten
   - WENN `new_y <= 0 ODER new_y >= 63`
   DANN:
   - `ball_dir_y = -ball_dir_y`
   - `new_y = new_y + 2 * ball_dir_y`
   - Begrenzung auf `[1, 62]`
   5.3 Kollision mit CPU-Paddle (links)
   - WENN `new_x == CPU_X` UND `new_y` innerhalb [`cpu_y` … `cpu_y + PADDLE_HEIGHT`]
   DANN:
   - `ball_dir_x = -ball_dir_x`
   - `new_x = new_x + 2 * ball_dir_x`
   - `applySpin(false)` (geringere Spin-Chance)
   5.4 Kollision mit Spieler-Paddle (rechts)
   - WENN `new_x == PLAYER_X` UND `new_y` innerhalb [`player_y` … `player_y + PADDLE_HEIGHT`]
   DANN:
   - `ball_dir_x = -ball_dir_x`
   - `new_x = new_x + 2 * ball_dir_x`
   - `applySpin(true)` (höhere Spin-Chance)
   5.5 Torprüfung links (CPU verfehlt)
   - WENN `new_x <= 1`
   DANN:
   - `handleScore(true)`    (Spieler erhält Punkt)
   - `ball_update = time + BALL_RATE`
   - `update = true`
   5.6 Torprüfung rechts (Spieler verfehlt)
   - SONST WENN `new_x >= 126`
   DANN:
   - `handleScore(false)`   (CPU erhält Punkt)
   - `ball_update = time + BALL_RATE`
   - `update = true`
   5.7 Kein Tor
   - SONST:
   - `ball_x = new_x`
   - `ball_y = new_y`
   - `ball_update = ball_update + BALL_RATE`
   - `update = true`

> `handleScore()` kann den Zustand auf `STATE_END` setzen, wenn eine Seite 5 Punkte erreicht.

#### 5.2.4 Paddle-Bewegung

6. WENN `time > paddle_update` UND `gameState == STATE_PLAYING`
   DANN:
   6.1 `paddle_update = paddle_update + PADDLE_RATE`
   6.2 CPU-Paddle (links)
   - `center = cpu_y + PADDLE_HEIGHT / 2`
   - WENN `center > ball_y` → `cpu_y--`
   - WENN `center < ball_y` → `cpu_y++`
   - Begrenzung: `cpu_y` in [1, 63 − PADDLE_HEIGHT]
   6.3 Spieler-Paddle (rechts)
   - WENN `up_state == true`  → `player_y--`
   - WENN `down_state == true`→ `player_y++`
   - `up_state = false`, `down_state = false` (Flags zurücksetzen)
   - Begrenzung: `player_y` in [1, 63 − PADDLE_HEIGHT]
   6.4 `update = true`

#### 5.2.5 Rendering

7. WENN `update == true` UND `gameState == STATE_PLAYING`
   DANN:
   7.1 `renderPlayingFrame()`
   - Display löschen
   - `drawCourt()` (Rahmen + Mittellinie)
   - `drawScores()` (CPU/Spieler-Punkte)
   - linkes Paddle zeichnen
   - rechtes Paddle zeichnen
   - Ball zeichnen
   - `display.display()`

END-Zustand `STATE_PLAYING`.

---

### 5.3 Zustand `STATE_PAUSED` – Pausebildschirm

1. `demoMode = false`
2. Tasten UP und DOWN werden ignoriert
3. WENN `startClick == true`
   DANN:
   3.1 Zeitbasis neu setzen
   - `now2 = millis()`
   - `ball_update   = now2`
   - `paddle_update = now2`
   3.2 Spielzustand wechseln
   - `gameState = STATE_PLAYING`
   3.3 Spielfeld neu zeichnen
   - `renderPlayingFrame()`

END-Zustand `STATE_PAUSED`.

---

### 5.4 Zustand `STATE_END` – Game Over

1. `demoMode = false`
   (Game-Over-Screen wurde bereits durch `handleScore()` gezeichnet.)
2. WENN `startClick == true`
   DANN:
   2.1 Startbildschirm anzeigen
   - `drawStartScreen()`
   2.2 Spielzustand zurücksetzen
   - `gameState = STATE_START`
   - `lastInputTime = time`

END-Zustand `STATE_END`.

---

### 5.5 Zustand `STATE_DEMO` – Demo-/Attract-Mode

#### 5.5.1 Demo-Abbruch

1. `demoMode = true`
2. WENN irgendeine Taste aktiv ist
   (`upPressed` ODER `downPressed` ODER `startPressed` ODER `startClick`)
   DANN:
   2.1 `gameState = STATE_START`
   2.2 `demoMode  = false`
   2.3 `drawStartScreen()`
   2.4 `lastInputTime = time`
   2.5 Zustand beendet

Nur wenn keine Taste gedrückt wurde:

#### 5.5.2 Ball-Bewegung im Demo-Modus

3. `update = false`
4. WENN `time > ball_update`
   DANN:
   4.1 Neue Position berechnen
   - `new_x = ball_x + ball_dir_x`
   - `new_y = ball_y + ball_dir_y`
   4.2 Kollision oben/unten wie im Spiel
   4.3 Kollision mit linkem Paddle (CPU1)
   - wie in STATE_PLAYING, jedoch `applySpin(false)`
   4.4 Kollision mit rechtem Paddle (CPU2)
   - wie Spieler-Paddle, aber ebenfalls `applySpin(false)`
   4.5 Abprallen an den Seiten statt Tor
   - WENN `new_x <= 1 ODER new_x >= 126`
   DANN:
   - `ball_dir_x = -ball_dir_x`
   - `new_x = new_x + 2 * ball_dir_x`
   4.6 Ballposition übernehmen
   - `ball_x = new_x`
   - `ball_y = new_y`
   - `ball_update = ball_update + BALL_RATE`
   - `update = true`

#### 5.5.3 Paddle-Bewegung im Demo-Modus

5. WENN `time > paddle_update`
   DANN:
   5.1 `paddle_update = paddle_update + PADDLE_RATE`
   5.2 Linkes Paddle (CPU1) an Ball ausrichten
   - `centerL = cpu_y + PADDLE_HEIGHT / 2`
   - WENN `centerL > ball_y` → `cpu_y--`
   - WENN `centerL < ball_y` → `cpu_y++`
   - Begrenzung auf gültigen Bereich
   5.3 Rechtes Paddle (CPU2) an Ball ausrichten
   - `centerR = player_y + PADDLE_HEIGHT / 2`
   - WENN `centerR > ball_y` → `player_y--`
   - WENN `centerR < ball_y` → `player_y++`
   - Begrenzung auf gültigen Bereich
   5.4 `update = true`

#### 5.5.4 Rendering im Demo-Modus

6. WENN `update == true` UND `gameState == STATE_DEMO`
   DANN:
   6.1 `renderPlayingFrame()`
   - Anzeige von „DEMO“ über `drawScores()`
   - Paddles und Ball werden wie im Spiel gerendert

END-Zustand `STATE_DEMO`.

---

## 6. Abschluss jedes `loop()`-Durchlaufs

Unabhängig vom Zustand, nach dem Switch:

1. Aktuelle Tasterzustände merken

   * `lastStartPressed = startPressed`
   * `lastDownPressed  = downPressed`
2. Rücksprung an den Beginn von `loop()`
   (Endlosschleife läuft weiter, alle Timings und Zustände werden im nächsten Durchlauf erneut bewertet.)
