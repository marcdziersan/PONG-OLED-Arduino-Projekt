```text
ASCII-Ablaufdiagramm – PONG OLED (vereinfacht, IHK-geeignet)
===========================================================

                       +------------------------+
                       |   RESET / POWER ON    |
                       +------------------------+
                                   |
                                   v
                       +------------------------+
                       |       setup()         |
                       | - Display init        |
                       | - Pins INPUT_PULLUP   |
                       | - randomSeed(A0)      |
                       | - Startscreen zeichn. |
                       | - Zeiten setzen       |
                       +------------------------+
                                   |
                                   v
                       +------------------------+
                       |     STATE_START        |
                       |  "Startbildschirm"     |
                       |                        |
                       | - Anzeige Titel/Info   |
                       | - lastInputTime=now    |
                       +------------------------+
                         |        |             |
                         |        |             |
          START-Click ---+        |             |--- DOWN+START loslassen
                         |        |             |    (Hard-Reset-Kombi)
                         |        |             |
                         v        v             v
              +----------------+  +------------------------+
              |  STATE_PLAYING |  |      STATE_DEMO        |
              | "Spiel läuft"  |  | "Demo / Attract Mode"  |
              +----------------+  +------------------------+
                         ^                     |
                         |                     |
                         |    Beliebige Taste  |
                         +---------------------+
                               (zurück zu START)

      Inaktivität > DEMO_TIMEOUT_MS (z.B. 15 s)
      ---------------------------------------->
                 von STATE_START nach STATE_DEMO


===========================================================
Detail: STATE_PLAYING (Spielzustand)
===========================================================

                    +---------------------------+
      ENTER         |       STATE_PLAYING       |
 (von STATE_START   |        "Spiel läuft"      |
   nach StartClick) |                           |
                    | Initial:                  |
                    | - Scores evtl. 0:0        |
                    | - Ball zentrieren         |
                    | - Zeiten setzen           |
                    +---------------------------+
                                   |
                                   v
                   +---------------------------------+
                   |   LOOP-LOGIK (jede Iteration)   |
                   +---------------------------------+
                                   |
       +---------------------------+------------------------------+
       |                           |                              |
       v                           v                              v

  (1) Eingaben lesen          (2) Ball-Update               (3) Paddle-Update
  ------------------          -----------------             ------------------
  - upPressed                 WENN time > ball_update:      WENN time > paddle_update
  - downPressed                 - neue Pos (new_x,new_y)      UND gameState==PLAYING:
  - startPressed               - Wände oben/unten prüfen      - CPU-Paddle folgt Ball
  - lastStartPressed           - Kollision CPU-Paddle         - Spieler-Paddle:
  - lastDownPressed            - Kollision Spieler-Paddle        * up_state => y--
                                - Tor links? Spieler++           * down_state => y++
  - startClick / comboClick    - Tor rechts? CPU++            - Paddles clampen
  - up_state / down_state      - evtl. handleScore()          - paddle_update+=PADDLE_RATE
                                - sonst ball_x/ball_y setzen

       |                           |                              |
       +---------------------------+------------------------------+
                                   |
                                   v
                          +------------------+
                          |   ENTSCHEIDUNG   |
                          +------------------+
                                   |
             +---------------------+----------------------+
             |                     |                      |
             v                     v                      v

   A) START-Click im Spiel   B) Score >= 5?         C) Kein Ende / kein Pause
   ------------------------   ------------------    -------------------------
   - Spiel pausieren         - Game Over:          - Normales Weiterspielen
   - gameState=STATE_PAUSED    * STATE_END         - Frame neu zeichnen
   - drawPauseScreen()         * drawEndScreen()     (renderPlayingFrame())
                              (Spielende)


===========================================================
Detail: Übergänge aus STATE_PLAYING
===========================================================

+----------------------------------------+
|        EVENTS IN STATE_PLAYING         |
+----------------------------------------+
|                                        |
| 1) START-Click                         |
|    -> STATE_PAUSED                     |
|    -> "PAUSE"-Screen                   |
|                                        |
| 2) Score eines Spielers >= 5           |
|    (über handleScore())                |
|    -> STATE_END                        |
|    -> "GAME OVER" + Gewinneranzeige    |
|                                        |
| 3) DOWN+START loslassen (comboClick)   |
|    -> Hard-Reset                       |
|    -> Scores=0:0, Positionen reset     |
|    -> STATE_START + Startscreen        |
|                                        |
+----------------------------------------+


===========================================================
Detail: STATE_PAUSED
===========================================================

                        +-----------------------+
                        |     STATE_PAUSED      |
                        |        "Pause"        |
                        +-----------------------+
                                   |
                                   |
                      START-Click (weiter)
                                   |
                                   v
                     +------------------------+
                     |  Zeiten neu setzen     |
                     |  (ball_update, etc.)   |
                     |  gameState=PLAYING     |
                     |  renderPlayingFrame()  |
                     +------------------------+


===========================================================
Detail: STATE_END (Game Over)
===========================================================

                        +-----------------------+
                        |      STATE_END        |
                        |      "Game Over"      |
                        |  (Anzeige Gewinner)   |
                        +-----------------------+
                                   |
                         START-Click (Menu)
                                   |
                                   v
                        +------------------------+
                        |    STATE_START         |
                        |  Startbildschirm      |
                        +------------------------+


===========================================================
Detail: STATE_DEMO (CPU vs. CPU)
===========================================================

             +------------------------------+
             |          STATE_DEMO          |
             |     "Demo / Attract Mode"    |
             +------------------------------+
                           |
                           v
                 +---------------------------+
                 |  LOOP-LOGIK im Demo:      |
                 |  - CPU links folgt Ball   |
                 |  - CPU rechts folgt Ball  |
                 |  - Ball prallt überall    |
                 |  - KEINE Punktewertung    |
                 |  - Anzeige "DEMO" oben    |
                 +---------------------------+
                           |
                           |
               +-----------+-----------+
               |                       |
               v                       v
   Beliebige Taste               DOWN+START loslassen
   (UP / DOWN / START)           (Hard-Reset)
   -> STATE_START                -> STATE_START
   -> Startscreen                -> Scores reset, Startscreen

```
