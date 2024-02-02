//***********************************************
//  Jeu d'Échecs Automatique
//   Par Xavier Chalifoux
//  Date: 6 décembre 2023
//***********************************************

#include <Arduino.h>
#include "Micro_Max.h"
#include "global.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Global variables
// LCD lignes de 20 caractères, 4 lignes
LiquidCrystal_I2C lcd(0x27, 20, 4);

uint8_t i = 0;

// Timer
uint64_t timer = millis();

// Pin setup et initialisation
void setup() {
  Serial.begin(9600);

  // Setup LCD
  display();

  // Setup Moteurs
  pinMode(directionPinMotorA, OUTPUT);
  pinMode(stepPinMotorA, OUTPUT);
  pinMode(directionPinMotorB, OUTPUT);
  pinMode(stepPinMotorB, OUTPUT);
  pinMode(motorEnPin, OUTPUT);
  disableMotors();

  // Setup boutons
  pinMode(buttonUpPin, INPUT_PULLUP);
  pinMode(buttonDownPin, INPUT_PULLUP);
  pinMode(buttonSelectPin, INPUT_PULLUP);
  pinMode(buttonWhite, INPUT_PULLUP);
  pinMode(buttonBlack, INPUT_PULLUP);

  // Setup Limit switch
  pinMode(xEndButtonPin, INPUT_PULLUP);
  pinMode(yEndButtonPin, INPUT_PULLUP);

  // Setup multiplexeur
  for (int i = 0; i < 4; i++) {
    pinMode(mux_add[i], OUTPUT);
    digitalWrite(mux_add[i], LOW);
    pinMode(mux_out[i], INPUT_PULLUP);
  }

  // Setup électroaimant
  pinMode(electromagnetPin, OUTPUT);

  //  MicroMax
  lastH[0] = 0;
}

//Main
void loop() {
  switch (state) {
    case mode_choice:
      display();
      read_buttons(numGameModeOptions);
      break;

    case level_choice:
      display();
      read_buttons(numLevelOptions);
      break;

    case color_choice:
      display();
      read_buttons(numColorOptions);
      break;

    case timer_choice:
      display();
      read_buttons(numtimerOptions);
      break;

    case increment_choice:
      display();
      read_buttons(numincrementOptions);
      break;

    case calibrationXY:
      digitalWrite(motorEnPin, LOW);
      calibrateXY();
      digitalWrite(motorEnPin, HIGH);
      state = wait_start;
      break;
    case wait_start:
      display();
      if (digitalRead(buttonSelectPin) == LOW) {
        if (selectedColor == 0) {
          state = turn_white;
        } else {state = turn_black;}
      }
      break;
    case turn_white:
      if ((millis() - timer) > 990) {  //Deduit par essaie erreur
        countdown();
        display();
      }
      readMove();
      if (digitalRead(buttonWhite) == LOW) {
        AI_HvsH();  // Check is movement is valid
        if (no_valid_move == false) state = turn_black;
        else { break; }
        change_player = true;
        second_white = second_white + selectedIncrement;
        if (second_white > 59) {
          second_white = second_white % 60;
          minute_white = minute_white + 1;
        }
      }
      break;

    case turn_black:
      if (selectedMode == 0) {
        if ((millis() - timer) > 990) {
          countdown();
          display();
        }
        readMove();
        if (digitalRead(buttonBlack) == LOW) {
          AI_HvsH();  // Check is movement is valid
          if (no_valid_move == false) state = turn_white;
          else { break; }
          change_player = true;
          second_black = second_black + selectedIncrement;
          if (second_black > 59) {
            second_black = second_black % 60;
            minute_black = minute_black + 1;
          }
        }
      } else if (selectedMode == 1) {
        display();
        uint64_t bot_timer = millis();
        state = turn_white;
        AI_HvsC(bot_strength[selectedLevel]);
        if (game_finished == 0) {
          bot_move();
        }
        uint32_t second_taken = (millis() - bot_timer) / 1000;
        second_black = second_black + selectedIncrement - second_taken;
        if (second_black < 0) {
          int minutes_to_subtract = (-second_black - 1) / 60 + 1;
          second_black = (minutes_to_subtract * 60) + second_black;
          minute_black -= minutes_to_subtract;
        } else if (second_black >= 60) {
          minute_black += second_black / 60;
          second_black %= 60;
        }
      } else if (selectedMode == 2) {
        display();
        uint64_t bot_timer = millis();
        Serial.println("Entrez le mouvement que vous désirez faire sous la forme: e2e4 (e2: coordonnée initiale, e4: coordonnée finale)");
        while (black_move == "") {
          black_move = Serial.readStringUntil('\n');
        }
        if (black_move.length() == 4) {
          mov[0] = black_move.charAt(0);
          mov[1] = black_move.charAt(1);
          mov[2] = black_move.charAt(2);
          mov[3] = black_move.charAt(3);
          if (mov[0] >= 'a' && mov[0] <= 'h' && mov[1] >= '1' && mov[1] <= '8' && mov[2] >= 'a' && mov[2] <= 'h' && mov[3] >= '1' && mov[3] <= '8') {
            AI_HvsH();  // Check is movement is valid
            if (no_valid_move == false) {
              state = turn_white;
            } else {
              black_move = "";
              no_valid_move = false;
              break;
            }
            change_player = true;
            if (game_finished == 0) {
              bot_move();
            }
            uint32_t second_taken = (millis() - bot_timer) / 1000;
            second_black = second_black + selectedIncrement - second_taken;
            if (second_black < 0) {
              int minutes_to_subtract = (-second_black - 1) / 60 + 1;
              second_black = (minutes_to_subtract * 60) + second_black;
              minute_black -= minutes_to_subtract;
            } else if (second_black >= 60) {
              minute_black += second_black / 60;
              second_black %= 60;
            }
            black_move = "";
          } else {
            Serial.println("Coup invalide! Recommencez!");
          }
        }
      }
      break;

    case game_ended:
      display();
      if (digitalRead(buttonSelectPin) == LOW) {
        currentOption = 0;
        state = mode_choice;
        change_player = true;
        winner = "";
        second_white = 0;
        second_black = 0;
        king_moved = false;
        pieces_mangees = 0;
        lastH[0] = 0;
        lastM[0] = 0;
        delay(200);
        while (digitalRead(buttonSelectPin) == LOW)
          ;
      }
      break;
  }
}

//Fonctions:
void display() {
  if (no_valid_move == true) {
    lcd.setCursor(0, 0);
    lcd.print("    Coup illegal    ");
    lcd.setCursor(0, 2);
    lcd.print(" Recommencez  votre ");
    lcd.setCursor(0, 3);
    lcd.print("        coup        ");
    lcd.setCursor(0, 1);
    lcd.print("        ");
    lcd.print(mov[0]);
    lcd.print(mov[1]);
    lcd.print(mov[2]);
    lcd.print(mov[3]);
    lcd.print("        ");
    no_valid_move = false;
    delay(2000);
  }
  switch (state) {
    case start_up:
      lcd.init();
      lcd.setBacklight((uint8_t)1);
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("    Jeu d'echecs    ");
      lcd.setCursor(0, 1);
      lcd.print("      AUTONOME      ");
      lcd.setCursor(0, 2);
      lcd.print("        Par:        ");
      lcd.setCursor(0, 3);
      lcd.print("  Xavier Chalifoux  ");
      delay(2000);
      state = mode_choice;
      break;

    case mode_choice:
      lcd.setCursor(0, 0);
      lcd.print("Selectionnez un mode");
      lcd.setCursor(1, 1);
      lcd.print(menuOptions[0]);
      lcd.setCursor(1, 2);
      lcd.print(menuOptions[1]);
      lcd.setCursor(1, 3);
      lcd.print(menuOptions[2]);
      for (int i = 0; i < numGameModeOptions; i++) {
        lcd.setCursor(0, 1 + i);
        if (i == currentOption) {
          lcd.print(">");
        } else {
          lcd.print(" ");
        }
      }
      break;

    case level_choice:
      lcd.setCursor(0, 0);
      lcd.print("Choisissez le niveau");
      lcd.setCursor(1, 1);
      lcd.print(levelOptions[0]);
      lcd.setCursor(1, 2);
      lcd.print(levelOptions[1]);
      lcd.setCursor(1, 3);
      lcd.print(levelOptions[2]);

      for (int i = 0; i < numLevelOptions; i++) {
        lcd.setCursor(0, 1 + i);
        if (i == currentOption) {
          lcd.print(">");
        } else {
          lcd.print(" ");
        }
      }
      break;

    case color_choice:
      lcd.setCursor(0, 0);
      lcd.print("Choisissez la       ");
      lcd.setCursor(1, 1);
      lcd.print("de vos pions:       ");
      lcd.setCursor(1, 2);
      lcd.print(colorOptions[0]);
      lcd.setCursor(1, 3);
      lcd.print(colorOptions[1]);

      for (int i = 0; i < 2; i++) {
        lcd.setCursor(0, 1 + i);
        if (i == currentOption) {
          lcd.print(">");
        } else {
          lcd.print(" ");
        }
      }
      break;
    case timer_choice:
      lcd.setCursor(0, 0);
      lcd.print("Choisissez le timer:");
      lcd.setCursor(1, 1);
      lcd.print(timerOptions[currentOption]);
      lcd.setCursor(0, 1);
      lcd.print(">");
      lcd.setCursor(0, 2);
      lcd.print("Decidez l'increment:");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      lcd.print(incrementOptions[6]);
      break;

    case increment_choice:
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 3);
      lcd.print(">");
      lcd.setCursor(0, 2);
      lcd.print("Decidez l'increment:");
      lcd.setCursor(1, 3);
      lcd.print(incrementOptions[currentOption]);
      break;

    case calibrationXY:
      lcd.setCursor(0, 0);
      lcd.print("Calibration du plan ");
      lcd.setCursor(0, 1);
      lcd.print("         XY         ");
      lcd.setCursor(0, 2);
      lcd.print("                    ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      break;
    case wait_start:
      lcd.setCursor(0, 0);
      lcd.print("   Appuyez sur le   ");
      lcd.setCursor(1, 1);
      lcd.print("bouton de selection ");
      lcd.setCursor(1, 2);
      lcd.print("   pour commencer   ");
      lcd.setCursor(1, 3);
      lcd.print("     la partie!     ");

      break;
    case turn_white:
      lcd.setCursor(0, 1);
      lcd.print("Blanc: ");
      lcd.print(String(minute_white, DEC).length() == 1 ? "0" + String(minute_white) : String(minute_white));
      lcd.print(":");
      lcd.print(String(second_white, DEC).length() == 1 ? "0" + String(second_white) : String(second_white));
      lcd.print("        ");
      lcd.setCursor(0, 2);
      lcd.print("Noir:  ");
      lcd.print(String(minute_black, DEC).length() == 1 ? "0" + String(minute_black) : String(minute_black));
      lcd.print(":");
      lcd.print(String(second_black, DEC).length() == 1 ? "0" + String(second_black) : String(second_black));
      lcd.print("        ");
      lcd.setCursor(0, 0);
      lcd.print("   Tour des blancs  ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      break;

    case turn_black:
      lcd.setCursor(0, 1);
      lcd.print("Blanc: ");
      lcd.print(String(minute_white, DEC).length() == 1 ? "0" + String(minute_white) : String(minute_white));
      lcd.print(":");
      lcd.print(String(second_white, DEC).length() == 1 ? "0" + String(second_white) : String(second_white));
      lcd.print("        ");
      lcd.setCursor(0, 2);
      lcd.print("Noir:  ");
      lcd.print(String(minute_black, DEC).length() == 1 ? "0" + String(minute_black) : String(minute_black));
      lcd.print(":");
      lcd.print(String(second_black, DEC).length() == 1 ? "0" + String(second_black) : String(second_black));
      lcd.print("        ");
      lcd.setCursor(0, 0);
      lcd.print("   Tour des noirs   ");
      lcd.setCursor(0, 3);
      lcd.print("                    ");
      break;

    case game_ended:
      lcd.setCursor(0, 0);
      lcd.print("Les " + winner + " ont gagne");
      lcd.setCursor(0, 1);
      lcd.print("   Appuyez sur le   ");
      lcd.setCursor(0, 2);
      lcd.print("bouton de selection ");
      lcd.setCursor(0, 3);
      lcd.print(" pour recommencer!  ");
      break;
  }
}

void countdown() {
  if (change_player == true) {
    change_player = false;
    if (state == turn_white) {
      second = second_white;
      minute = minute_white;
    } else if (state == turn_black) {
      second = second_black;
      minute = minute_black;
    }
  }

  second = second - 1;
  if (second == 255) {
    second = 59;
    minute = minute - 1;
  }

  if (state == turn_white) {
    if (minute == 0 & second == 0) {
      winner = "noirs ";
      state = game_ended;
    }
    second_white = second;
    minute_white = minute;
  } else if (state == turn_black) {
    if (minute == 0 & second == 0) {
      winner = "blancs";
      state = game_ended;
    }
    second_black = second;
    minute_black = minute;
  }
  timer = millis();
}

void enableMotors() {
  digitalWrite(motorEnPin, LOW);
}

void disableMotors() {
  digitalWrite(motorEnPin, HIGH);
}

void read_buttons(int numOptions) {
  if (digitalRead(buttonUpPin) == LOW) {
    if (currentOption != 0) {
      currentOption--;
    }
    delay(200);
    while (digitalRead(buttonUpPin) == LOW)
      ;
  }
  if (digitalRead(buttonDownPin) == LOW) {
    if (currentOption != numOptions - 1) {
      currentOption++;
    }
    delay(200);
    while (digitalRead(buttonDownPin) == LOW)
      ;
  }
  if (digitalRead(buttonSelectPin) == LOW) {
    switch (state) {
      case mode_choice:
        selectedMode = currentOption;
        while (digitalRead(buttonSelectPin) == LOW)
          ;
        currentOption = 2;
        state = timer_choice;
        break;

      case level_choice:
        selectedLevel = currentOption;
        currentOption = 0;
        while (digitalRead(buttonSelectPin) == LOW)
          ;
        state = calibrationXY;
        break;

      case color_choice:
        selectedColor = currentOption;
        currentOption = 2;
        while (digitalRead(buttonSelectPin) == LOW)
          ;
        state = calibrationXY;
        break;

      case timer_choice:
        minute_white = intTimerOptions[currentOption];
        minute_black = intTimerOptions[currentOption];
        currentOption = 6;
        while (digitalRead(buttonSelectPin) == LOW)
          ;
        state = increment_choice;
        break;

      case increment_choice:
        selectedIncrement = intIncrementOptions[currentOption];
        currentOption = 0;
        while (digitalRead(buttonSelectPin) == LOW)
          ;
        if (selectedMode == 0) {
          state = wait_start;
        } else if (selectedMode == 1) {
          state = level_choice;
        } else if (selectedMode == 2) {
          state = calibrationXY;
        }
        break;
    }
    delay(200);
  }
}

void calibrateXY() {
  display();
  while (digitalRead(xEndButtonPin) == LOW) {
    moveMotor(5.0 / stepsPerSquare, vitesse_rapide, droite);  // equivalent a 5 pas
  }
  pos_x = 10;  // Position ou le chariot arrive dans le coin en haut a droite
  while (digitalRead(yEndButtonPin) == LOW) {
    moveMotor(5.0 / stepsPerSquare, vitesse_rapide, haut);
  }
  pos_y = 8;
}


void saveBoard() {
  // Serial.println("BOARD:");
  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t k = 0; k < 8; k++) {
      previousBoard[i][k] = currentBoard[i][k];
      // Serial.print(currentBoard[i][k]);
    }
    // Serial.println();
  }
  // delay(1000);
}

void readBoard() {
  saveBoard();
  for (uint8_t j = 0; j < 16; j++) {
    //Pour écrire les bonnes adresses pour lire les 16 interrupteurs dans l'ordre
    digitalWrite(mux_add[0], j & 0x01);
    digitalWrite(mux_add[1], (j >> 1) & 0x01);
    digitalWrite(mux_add[2], (j >> 2) & 0x01);
    digitalWrite(mux_add[3], (j >> 3) & 0x01);
    //Lecture des interrupteurs
    for (uint8_t i = 0; i < 4; i++) {
      if (j >= 8) {  // En fonction de la maniere que mes connections sont faites
        currentBoard[2 * i][j - 8] = digitalRead(mux_out[i]);
      } else {
        currentBoard[2 * i + 1][j] = digitalRead(mux_out[i]);
      }
    }
  }
}

void readMove() {
  readBoard();
  char coordX[] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h' };
  char coordY[] = { '1', '2', '3', '4', '5', '6', '7', '8' };

  for (uint8_t i = 0; i < 8; i++) {
    for (uint8_t j = 0; j < 8; j++) {
      if (previousBoard[i][j] != currentBoard[i][j]) {
        Serial.print((int)b[16 * abs(i - 7) + j]);
        Serial.print(" ");
        if (i == 7) { Serial.println(); }
        if (currentBoard[i][j] == 1 && ((b[16 * abs(i - 7) + j] & 8 && state == turn_white) || (b[16 * abs(i - 7) + j] & 16 && state == turn_black))) {
          mov[0] = coordX[j];  // coord en x (a a h)
          mov[1] = coordY[i];  // coord en y (1 a 8)
        } else if (currentBoard[i][j] == 0) {
          mov[2] = coordX[j];
          mov[3] = coordY[i];
        }
      }
    }
  }
}

void electromagnet(bool electromagnetON) {
  if (electromagnetON == true) {
    // On active l'Électroaimant
    digitalWrite(electromagnetPin, HIGH);
    delay(500);
  } else if (electromagnetON == false) {
    // On désactive l'Électroaimant
    digitalWrite(electromagnetPin, LOW);
    delay(500);
  }
}

void moveMotor(double distance, double speed, uint8_t direction) {
  if (direction == droite || direction == bas || direction == diag_bd) { digitalWrite(directionPinMotorA, HIGH); }  // Les mouvements ou le moteur A tourne dans le sens horaire
  else {
    digitalWrite(directionPinMotorA, LOW);
  }
  if (direction == gauche || direction == bas || direction == diag_bg) { digitalWrite(directionPinMotorB, HIGH); }  // Les mouvements ou le moteur B tourne dans le sens horaire
  else {
    digitalWrite(directionPinMotorB, LOW);
  }
  // Le nombre de pas pour faire un déplacement d'une case en diagonal est le double
  if (direction == diag_bd || direction == diag_bg || direction == diag_hd || direction == diag_hg) {
    distance = distance * 2;
  }
  for (int i = 0; i < distance * stepsPerSquare; i++) {
    if (direction == diag_bg || direction == diag_hd) { digitalWrite(stepPinMotorA, LOW); }  // La diagonale ou seulement le moteur B fonctionne
    else {
      digitalWrite(stepPinMotorA, HIGH);
    }
    if (direction == diag_bd || direction == diag_hg) { digitalWrite(stepPinMotorB, LOW); }  // La diagonale ou seulement le moteur A fonctionne
    else {
      digitalWrite(stepPinMotorB, HIGH);
    }
    delayMicroseconds(speed);
    digitalWrite(stepPinMotorA, LOW);
    digitalWrite(stepPinMotorB, LOW);
    delayMicroseconds(speed);
  }
}

void bot_move() {
  // Je considere que les positions des cases en x vont de 1 a 10 et de 1 a 8 en y
  // Le overshoot a ete implementer afin de compenser la distance de la piece derriere l'electroaimant lorsqu'elle deplace la piece. Le overshoot se fait seulement lorsque la piece atteint sa destination
  int departure_coord_x = lastM[0] - 'a' + 3;  // Le +3 compense les deux colonnes a gauche de l'echiquier pour ranger les pieces eliminee
  int departure_coord_y = lastM[1] - '0';
  int arrival_coord_x = lastM[2] - 'a' + 3;
  int arrival_coord_y = lastM[3] - '0';

  if (departure_coord_x < 0 || departure_coord_x > 10 || departure_coord_y < 0 || departure_coord_y > 8 || arrival_coord_x < 0 || arrival_coord_x > 10 || arrival_coord_y < 0 || arrival_coord_y > 8) {
    return;
  }

  uint8_t deplacementX = 0;
  uint8_t deplacementY = 0;
  enableMotors();

  bool white_capture = false;
  if (bk[16 * abs(arrival_coord_y - 8) + arrival_coord_x - 3] != 0) { white_capture = true; }
  // if (currentBoard[arrival_coord_y - 1][arrival_coord_x - 3] == 0) { white_capture = true; }
  // Algorithme pour placer les pieces dans la zone morte sans interference
  if (white_capture) {
    deplacementX = abs(arrival_coord_x - pos_x);
    deplacementY = abs(arrival_coord_y - pos_y);
    if (arrival_coord_x > pos_x) {
      moveMotor(deplacementX, vitesse_rapide, droite);
    } else if (arrival_coord_x < pos_x) {
      moveMotor(deplacementX, vitesse_rapide, gauche);
    }
    if (arrival_coord_y > pos_y) {
      moveMotor(deplacementY, vitesse_rapide, haut);
    } else if (arrival_coord_y < pos_y) {
      moveMotor(deplacementY, vitesse_rapide, bas);
    }
    electromagnet(true);
    if (arrival_coord_y == 8) {
      moveMotor(0.5, vitesse_lente, bas);
    } else {
      moveMotor(0.5, vitesse_lente, haut);
    }
    if (arrival_coord_y > pieces_mangees) {
      moveMotor(arrival_coord_x - 1, vitesse_lente, gauche);
      pos_x = 1;
    } else if (pieces_mangees >= 8 && arrival_coord_y > pieces_mangees - 8) {
      moveMotor(arrival_coord_x - 2, vitesse_lente, gauche);
      pos_x = 2;
    } else if (pieces_mangees >= 8) {
      moveMotor(arrival_coord_x - 2.5, vitesse_lente, gauche);
      pos_x = 2;
    } else {
      moveMotor(arrival_coord_x - 1.5, vitesse_lente, gauche);
      pos_x = 1;
    }
    if ((pieces_mangees >= 8 && arrival_coord_y <= pieces_mangees - 8) || (pieces_mangees < 8 && arrival_coord_y <= pieces_mangees)) {
      if (pieces_mangees >= 8) {
        moveMotor(pieces_mangees - 7.5 - arrival_coord_y, vitesse_lente, haut);  // Pour aller placer la piece au dessus de la derniere piece mangee
      } else {
        moveMotor(pieces_mangees - arrival_coord_y + 0.5, vitesse_lente, haut);  // Pour aller placer la piece au dessus de la derniere piece mangee
      }
      moveMotor(0.5, vitesse_lente, gauche);
    } else if (pieces_mangees >= 8) {
      if (arrival_coord_y != 8) {
        moveMotor(7.5 + arrival_coord_y - pieces_mangees, vitesse_lente, bas);  // Pour aller placer la piece au dessus de la derniere piece mangee
      } else {
        moveMotor(6.5 + arrival_coord_y - pieces_mangees, vitesse_lente, bas);
      }
    } else if (pieces_mangees < 8) {
      if (arrival_coord_y != 8) {
        moveMotor(arrival_coord_y - pieces_mangees - 0.5, vitesse_lente, bas);
      } else {
        moveMotor(arrival_coord_y - pieces_mangees - 1.5, vitesse_lente, bas);
      }
    }
    pos_y = pieces_mangees % 8 + 1;
    pieces_mangees++;
    electromagnet(false);
  }
  // Deplacement vers la position initiale de la piece
  deplacementX = abs(departure_coord_x - pos_x);
  deplacementY = abs(departure_coord_y - pos_y);

  //DEPLACEMENT AVEC DIAGONALE POUR OPTIMISER LE CHEMIN
  if (departure_coord_y == pos_y) {
    if (departure_coord_x > pos_x) {
      moveMotor(deplacementX, vitesse_rapide, droite);
    } else {
      moveMotor(deplacementX, vitesse_rapide, gauche);
    }
  } else if (departure_coord_x == pos_x) {
    if (departure_coord_y > pos_y) {
      moveMotor(deplacementY, vitesse_rapide, haut);
    } else {
      moveMotor(deplacementY, vitesse_rapide, bas);
    }
  } else if ((departure_coord_x > pos_x) && (departure_coord_y > pos_y)) {  // diag hd
    if (deplacementX > deplacementY) {
      moveMotor(deplacementY, vitesse_rapide, diag_hd);
      moveMotor(deplacementX - deplacementY, vitesse_rapide, droite);
    } else {
      moveMotor(deplacementX, vitesse_rapide, diag_hd);
      moveMotor(deplacementY - deplacementX, vitesse_rapide, haut);
    }
  } else if ((departure_coord_x > pos_x) && (departure_coord_y < pos_y)) {  // diag bd
    if (deplacementX > deplacementY) {
      moveMotor(deplacementY, vitesse_rapide, diag_bd);
      moveMotor(deplacementX - deplacementY, vitesse_rapide, droite);
    } else {
      moveMotor(deplacementX, vitesse_rapide, diag_bd);
      moveMotor(deplacementY - deplacementX, vitesse_rapide, bas);
    }
  } else if ((departure_coord_x < pos_x) && (departure_coord_y > pos_y)) {  // diag hg
    if (deplacementX > deplacementY) {
      moveMotor(deplacementY, vitesse_rapide, diag_hg);
      moveMotor(deplacementX - deplacementY, vitesse_rapide, gauche);
    } else {
      moveMotor(deplacementX, vitesse_rapide, diag_hg);
      moveMotor(deplacementY - deplacementX, vitesse_rapide, haut);
    }
  } else {  // diag bg
    if (deplacementX > deplacementY) {
      moveMotor(deplacementY, vitesse_rapide, diag_bg);
      moveMotor(deplacementX - deplacementY, vitesse_rapide, gauche);
    } else {
      moveMotor(deplacementX, vitesse_rapide, diag_bg);
      moveMotor(deplacementY - deplacementX, vitesse_rapide, bas);
    }
  }

  // Implementation du code pour deplacer les pieces
  deplacementX = abs(arrival_coord_x - departure_coord_x);
  deplacementY = abs(arrival_coord_y - departure_coord_y);
  electromagnet(true);

  // Deplacements diagonals (Pion, reine, roi, fou)
  if (deplacementX == deplacementY) {
    if ((arrival_coord_x > departure_coord_x) && (arrival_coord_y > departure_coord_y)) {  // deplacement diag haut droite
      moveMotor(deplacementY + overshoot, vitesse_lente, diag_hd);
    } else if ((arrival_coord_x > departure_coord_x) && (arrival_coord_y < departure_coord_y)) {  // deplacement diag bas droite
      moveMotor(deplacementY + overshoot, vitesse_lente, diag_bd);
    } else if ((arrival_coord_x < departure_coord_x) && (arrival_coord_y > departure_coord_y)) {  // deplacement diag haut gauche
      moveMotor(deplacementY + overshoot, vitesse_lente, diag_hg);
    } else {  // deplacement diag bas gauche
      moveMotor(deplacementY + overshoot, vitesse_lente, diag_bg);
    }

  }
  // Deplacements droits (Tours, pions, reine, roi)
  else if (deplacementX == 0 || deplacementY == 0) {
    if (arrival_coord_x > departure_coord_x) {  // deplacement droite
      moveMotor(deplacementX + overshoot, vitesse_lente, droite);
    } else if (arrival_coord_x < departure_coord_x) {  // deplacement gauche
      moveMotor(deplacementX + overshoot, vitesse_lente, gauche);
    } else if (arrival_coord_y > departure_coord_y) {  // deplacement haut
      moveMotor(deplacementY + overshoot, vitesse_lente, haut);
    } else {  // deplacement bas
      moveMotor(deplacementY + overshoot, vitesse_lente, bas);
    }
  }

  // Deplacements cavaliers
  else if ((deplacementX == 1 && deplacementY == 2) || (deplacementX == 2 && deplacementY == 1)) {
    if ((arrival_coord_x > departure_coord_x) && (arrival_coord_y > departure_coord_y)) {  // deplacement diag haut droite
      moveMotor(0.5, vitesse_lente, diag_hd);
      if (deplacementY == 2) {
        moveMotor(1, vitesse_lente, haut);
      } else {
        moveMotor(1, vitesse_lente, droite);
      }
      moveMotor(0.5 + overshoot, vitesse_lente, diag_hd);
    } else if ((arrival_coord_x > departure_coord_x) && (arrival_coord_y < departure_coord_y)) {  // deplacement diag bas droite
      moveMotor(0.5, vitesse_lente, diag_bd);
      if (deplacementY == 2) {
        moveMotor(1, vitesse_lente, bas);
      } else {
        moveMotor(1, vitesse_lente, droite);
      }
      moveMotor(0.5 + overshoot, vitesse_lente, diag_bd);
    } else if ((arrival_coord_x < departure_coord_x) && (arrival_coord_y > departure_coord_y)) {  // deplacement diag haut gauche
      moveMotor(0.5, vitesse_lente, diag_hg);
      if (deplacementY == 2) {
        moveMotor(1, vitesse_lente, haut);
      } else {
        moveMotor(1, vitesse_lente, gauche);
      }
      moveMotor(0.5 + overshoot, vitesse_lente, diag_hg);
    } else {  // deplacement diag bas gauche
      moveMotor(0.5, vitesse_lente, diag_bg);
      if (deplacementY == 2) {
        moveMotor(1, vitesse_lente, bas);
      } else {
        moveMotor(1, vitesse_lente, gauche);
      }
      moveMotor(0.5 + overshoot, vitesse_lente, diag_bg);
    }
  }

  // Roque côté du roi
  // Il est interessant de noter que le roi aura deja bougé avec la fonction de mouvement droit précédente, donc seulement le mouvement de la tour est effectué ici
  if (departure_coord_x == 7 && departure_coord_y == 8 && arrival_coord_x == 9 && arrival_coord_y == 8 && !king_moved) {
    electromagnet(false);
    moveMotor(1 - overshoot, vitesse_rapide, droite);
    electromagnet(true);
    moveMotor(0.5, vitesse_lente, diag_bg);
    moveMotor(1, vitesse_lente, gauche);
    moveMotor(0.5 + overshoot, vitesse_lente, diag_hg);
  }
  // Roque côté de la reine:
  else if (departure_coord_x == 7 && departure_coord_y == 8 && arrival_coord_x == 5 && arrival_coord_y == 8 && !king_moved) {
    electromagnet(false);
    moveMotor(2 - overshoot, vitesse_rapide, gauche);
    electromagnet(true);
    moveMotor(0.5, vitesse_lente, diag_bd);
    moveMotor(2, vitesse_lente, droite);
    moveMotor(0.5 + overshoot, vitesse_lente, diag_hd);
  }
  // Pour s'assurer que le roque s'effectue une seule fois
  if (departure_coord_x == 7 && departure_coord_y == 8) {
    king_moved = true;
  }
  electromagnet(false);
  // Je recalibre apres chaque coup pour meilleure precision
  calibrateXY();  // Remet pos_x et pos_y a 10 et 8
  disableMotors();
}