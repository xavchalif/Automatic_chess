unsigned short myrand(void);
void gameOver(String);
void bkp();
void serialBoard();
void AI_HvsH();
void AI_HvsC(uint8_t);

extern char mov [];
extern byte state;
extern boolean no_valid_move;
extern String winner;
extern uint8_t bot_level[];