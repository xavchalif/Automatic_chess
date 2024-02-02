#include <Arduino.h>

// Définition des pins du Arduino qui seront utilisées sur mon board
// Multiplexeur
const uint8_t mux_add[4] = { 40, 41, 42, 43 };
const uint8_t mux_out[4] = { A0, A1, A2, A3 };

// Électroaimant
const uint8_t electromagnetPin = 22;

// Moteurs
const uint8_t directionPinMotorA = 24;
const uint8_t stepPinMotorA = 25;
const uint8_t directionPinMotorB = 26;
const uint8_t stepPinMotorB = 27;
const uint8_t motorEnPin = 23;
const uint8_t stepsPerSquare = 192;  // Nombre de pas pour se deplacer de la distance d'un carre (3.8cm)

//Boutons
const uint8_t buttonUpPin = 30;
const uint8_t buttonDownPin = 31;
const uint8_t buttonSelectPin = 32;
const uint8_t buttonWhite = 34;
const uint8_t buttonBlack = 35;

// Défition des variables utiles
bool currentBoard[8][8] = { { 0, 0, 0, 0, 0, 0, 0, 0 },
                            { 0, 0, 0, 0, 0, 0, 0, 0 },
                            { 1, 1, 1, 1, 1, 1, 1, 1 },
                            { 1, 1, 1, 1, 1, 1, 1, 1 },
                            { 1, 1, 1, 1, 1, 1, 1, 1 },
                            { 1, 1, 1, 1, 1, 1, 1, 1 },
                            { 0, 0, 0, 0, 0, 0, 0, 0 },
                            { 0, 0, 0, 0, 0, 0, 0, 0 } };
bool previousBoard[8][8];
extern char b[];
extern char bk[];


// Machine a états
enum { start_up,
       mode_choice,
       level_choice,
       timer_choice,
       increment_choice,
       calibrationXY,
       calibrationBoard,
       turn_white,
       turn_black,
       game_ended,
       wait_start,
       color_choice };
byte state = start_up;

// Interrupteurs fin de course
const uint8_t xEndButtonPin = 10;
const uint8_t yEndButtonPin = 11;

// Timers
uint8_t second_white = 0;
uint8_t minute_white = 0;

int8_t second_black = 0;
uint8_t minute_black = 0;

uint8_t second = 0;
uint8_t minute = 0;

// Déplacements XY
uint8_t pos_x = 10;
uint8_t pos_y = 8;

// Vitesses (Nombre de microsecondes entre chaque step)
uint16_t vitesse_lente = 900;
uint16_t vitesse_rapide = 600;

// mouvements
char mov[4];
String black_move = "";

// Nombre de pieces mangees
uint8_t pieces_mangees = 0;

// Variable pour vérifier si le roi du robot a bougé
bool king_moved = false;

// Direction moteurs (h = haut, b = bas, d = droite, g = gauche)
enum { droite,
       gauche,
       bas,
       haut,
       diag_hd,
       diag_hg,
       diag_bg,
       diag_bd };
double overshoot = 0.05;

// Micro-max
extern char lastH[], lastM[];
extern uint8_t game_finished;
bool no_valid_move = false;

// Selection du mode de jeu
uint8_t currentOption = 0;

uint8_t numGameModeOptions = 3;
uint8_t selectedMode = 0;
String menuOptions[] = {
  "Humain VS Humain   ",
  "Humain VS Robot    ",
  "Humain VS Distance "
};

uint8_t numColorOptions = 2;
uint8_t selectedColor = 0;
String colorOptions[] = {
  "Blanc              ",
  "Noir               "
};

// Selection du niveau
uint8_t numLevelOptions = 3;
uint8_t selectedLevel = 0;
String levelOptions[] = {
  "Facile             ",
  "Intermediaire      ",
  "Difficile          "
};
uint8_t bot_strength[] = { 1, 2, 3 };
uint8_t bot_level[] = { 0x00, 0x00, 0x1F, 0x3F };

// Sélection timer
uint8_t numtimerOptions = 7;
String timerOptions[] = {
  "30 minutes         ",
  "15 minutes         ",
  "10 minutes         ",
  "5 minutes          ",
  "3 minutes          ",
  "2 minutes          ",
  "1 minute           "
};
uint8_t intTimerOptions[] = { 30, 15, 10, 5, 3, 2, 1 };  // en minutes
bool change_player = true;

uint8_t numincrementOptions = 7;
String incrementOptions[] = {
  "30 secondes        ",
  "15 secondes        ",
  "10 secondes        ",
  "5 secondes         ",
  "2 secondes         ",
  "1 secondes         ",
  "Aucun increment    "
};
uint8_t intIncrementOptions[] = { 30, 15, 10, 5, 2, 1, 0 };
uint8_t selectedIncrement = 0;  //En secondes

// Fin de partie
String winner = "";