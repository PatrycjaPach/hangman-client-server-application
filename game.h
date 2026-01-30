#ifndef GAME_H
#define GAME_H

#include <stdint.h> 

#define MAX_WORD_LEN 32
#define MAX_WRONG_GUESSES 7

struct game {
    int active;                 //czy jset ktos w grze i jest aktywna - 2 osoby
    int player1_fd;             // gniazdo 1
    int player2_fd;             // gniazdo 2

    char word[MAX_WORD_LEN];    //haslo do zgadniecia
    char guessed[MAX_WORD_LEN]; //zgadywane 
    int wrong_guesse;          //liczba błędów
    
    int wrong_count;          // liczba błędów
    char wrong_letters[32];   // tablica na błędy
    int turn_fd;              // gniazdo gracza, który ma ruch

};

void game_init(void);
void send_game(void);
void game_reset(void);

int start_game(int desc2);
void guess(int desc2, char letter);
int all_guessed(void);

void game_set_login(int fd, const char *login);



#endif