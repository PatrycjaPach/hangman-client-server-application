/*
struct game {
    int active;                 // czy gra trwa
    int player1_fd;             // deskryptor 1. gracza
    int player2_fd;             // deskryptor 2. gracza

    char word[MAX_WORD_LEN];    // hasło
    char guessed[MAX_WORD_LEN]; // aktualny stan (_ a _ _)
    int wrong_guesse;          // liczba błędów
};*/

#include "game.h"
#include "tlv.h"
#include "protocol.h"
#include "score.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


static struct game games; //pojedyncza gra 

static int other_player_fd(int fd) { //zwraca deskryptor drugiego gracza
    if (fd == games.player1_fd) return games.player2_fd;
    if (fd == games.player2_fd) return games.player1_fd;
    return 0;
}

static void send_turn_info(void) { //wysyła informację o ruchu
    char msg[64];
    snprintf(msg, sizeof(msg), "Turn: fd=%d\n", games.turn_fd);
    sendtlv(games.player1_fd, TLV_MSG, msg, (int)strlen(msg));
    sendtlv(games.player2_fd, TLV_MSG, msg, (int)strlen(msg));
}

static void switch_turn(void) { //zmienia ruch na drugiego gracza
    int other = other_player_fd(games.turn_fd);
    if (other != 0) games.turn_fd = other;
}

//stan logiczny graczy
static char g_p1_login[LOGIN_MAX] = {0}; 
static char g_p2_login[LOGIN_MAX] = {0};   


void game_set_login(int fd, const char *login) { //podpina login do gracza
    if (!login || login[0] == '\0') return;      
    if (fd == games.player1_fd) {                
        strncpy(g_p1_login, login, LOGIN_MAX - 1); 
        g_p1_login[LOGIN_MAX - 1] = '\0';        
    } else if (fd == games.player2_fd) {         
        strncpy(g_p2_login, login, LOGIN_MAX - 1);
        g_p2_login[LOGIN_MAX - 1] = '\0';        
    }
} 

static const char *word_list[] = {
    "gra",
    "programowanie", 
    "sieci",
    "myszka",
    "drzewo",
    "papier",
    "klawisz",
    "kursor",
    "kubek",
    "lustro",
    "telefon"
};

void game_init(void) {
    memset(&games, 0, sizeof(games));
    srand(time(NULL)); //inicjalizacja rand
    score_init("scores.dat"); //inicjalizacja score z pliku
}

int start_game(int desc2){

    if(games.active){ //jesli trwa inna gra
        sendtlv(desc2, TLV_MSG, "Wait: game already in progress\n", 32);
        return 0;
    }

    if(games.player1_fd==0){
        games.player1_fd=desc2;
        sendtlv(desc2, TLV_MSG, "Wait: waiting for another player...\n", 36);
        return 0;
    }
    else if(games.player2_fd==0 && desc2 != games.player1_fd){
        games.player2_fd=desc2;
       
        //wybór słowa
          int index = rand() % (sizeof(word_list) / sizeof(word_list[0]));
        const char *selected_word = word_list[index];
        strncpy(games.word, selected_word,MAX_WORD_LEN- 1);  //kopiowanie slowa MAX_WORD_LEN ????????
        games.word[MAX_WORD_LEN-1]='\0'; //dodanie na koncu null

        for(size_t i=0; i<strlen(games.word); i++){
            games.guessed[i]='_'; //wstawienie wszedzie pustyc miejsc
        }
        games.guessed[strlen(games.word)]='\0'; //dodanie na koncu null
        games.wrong_guesse=0;
        games.active=1; //ustawienie ze gra trwa

        games.turn_fd = games.player1_fd; //ustawienie ruchu na 1 gracza
        send_turn_info(); //wyslanie info o ruchu

        games.wrong_count = 0;
        games.wrong_letters[0] = '\0';

        sendtlv(games.player1_fd, TLV_MSG, "Starting game!!!\n", 18);
        sendtlv(games.player2_fd, TLV_MSG, "Starting game!!!\n", 18);
        
        return 0;
    }
    else{
        sendtlv(desc2, TLV_MSG, "ERROR: cannot join game\n", 24);
        return -1;
    }



}

void guess(int desc2, char letter){

    if (!isalpha((unsigned char)letter)) {   // ignoruj nie-litery
        return;
    }

    letter = (char)tolower((unsigned char)letter);
    // czy litera już została użyta (dobra)
    for (size_t i = 0; i < strlen(games.guessed); i++) {
        if (games.guessed[i] == letter) {
            sendtlv(desc2, TLV_MSG, "You already tried this letter!\n", 30);
            return;
        }
    }

    // czy litera już została użyta (zła)
    for (int j = 0; j < games.wrong_count; j++) {
        if (games.wrong_letters[j] == letter) {
            sendtlv(desc2, TLV_MSG, "You already tried this letter!\n", 30);
            return;
        }
    }
    if(!games.active){
        sendtlv(desc2, TLV_MSG, "ERROR: not in game?\n", 20);
        return;
    }
    if(desc2 != games.player1_fd && desc2 != games.player2_fd){
        sendtlv(desc2, TLV_MSG, "ERROR: you didnt start the game\n", 32);
        return;
    }

    if(desc2 != games.turn_fd){ //sprawdzenie czy ruch jest tego gracza
        sendtlv(desc2, TLV_MSG, "ERROR: not your turn\n", 21);
        return;
    }

    int correct = 0;
    int already = 0;


    for(size_t i=0; i<strlen(games.word); i++){
        if(games.word[i]==letter && games.guessed[i]=='_'){
            games.guessed[i]=letter;
            correct=1; //ustawianie na 1, bo pozniej boolem sprawdzamy
        }
    }

        if (!correct) {

        for (int j = 0; j < games.wrong_count; j++) {
            if (games.wrong_letters[j] == letter) {
                already = 1;
                break;
            }
        }

        if (already) {
            sendtlv(desc2, TLV_MSG, "You already tried this letter!\n", 30);
            return;
        }

        games.wrong_guesse++;

        if (games.wrong_count < (int)sizeof(games.wrong_letters) - 1) {
            games.wrong_letters[games.wrong_count++] = letter;
            games.wrong_letters[games.wrong_count] = '\0';
        }

        sendtlv(desc2, TLV_MSG, "Wrong letter!\n", 14);
    }
    else {
        sendtlv(desc2, TLV_MSG, "Good guess!\n", 12);
    }

    //wyslanie stanu gry
    send_game();
    if(all_guessed()){
        //zapis score
        uint32_t s = score_calc((int)strlen(games.word), games.wrong_guesse); 
        if (g_p1_login[0]) score_update_best(g_p1_login, s);      
        if (g_p2_login[0]) score_update_best(g_p2_login, s);           

        sendtlv(games.player1_fd, TLV_MSG, "GAME OVER!! winner\n", 21);
        sendtlv(games.player2_fd, TLV_MSG, "GAME OVER!! winner\n", 21);
        game_reset();
        return;
    }
    if(games.wrong_guesse >= MAX_WRONG_GUESSES){
        //zapisz score nawet jak przegrasz
        uint32_t s = score_calc((int)strlen(games.word), games.wrong_guesse); 
        if (g_p1_login[0]) score_update_best(g_p1_login, s);                 
        if (g_p2_login[0]) score_update_best(g_p2_login, s); 

        sendtlv(games.player1_fd, TLV_MSG, "GAME OVER!! too many wrong guesses\n", 36);
        sendtlv(games.player2_fd, TLV_MSG, "GAME OVER!! too many wrong guesses\n", 36);
        game_reset();
        return;
    }
    switch_turn(); //zmiana ruchu
    send_turn_info(); //wyslanie info o ruchu
}

int all_guessed(void){
    for(size_t i=0; i<strlen(games.word); i++){
        if(games.guessed[i]=='_'){
            return 0; //jesli jest gdzies _ to nie wszysrtkie zgadlismy
        }
    }
    return 1; 
}

void game_reset(void){
    memset(&games, 0, sizeof(games)); //czyszcenie tablicy
    g_p1_login[0] = '\0';
    g_p2_login[0] = '\0'; 
}

void send_game(){
    char buff[256];


    snprintf(buff, sizeof(buff),
             "Word: %s Wrong guesses: %d/%d WRONG: %s\n",
             games.guessed,
             games.wrong_guesse,
             MAX_WRONG_GUESSES,
             (games.wrong_count > 0) ? games.wrong_letters : "-");
    sendtlv(games.player1_fd,TLV_MSG, buff,strlen(buff));
    sendtlv(games.player2_fd,TLV_MSG, buff,strlen(buff));
}




