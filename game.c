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

//FD do deskryptora drugiego gracza
static int other_player_fd(int fd) { //zwraca deskryptor drugiego gracza
    if (fd == games.player1_fd) return games.player2_fd; //jeśli fd to player1 to zwraca player2
    if (fd == games.player2_fd) return games.player1_fd; //jeśli fd to player2 to zwraca player1
    return 0;
}

//wysyła informację o ruchu
static void send_turn_info(void) { 
    char msg[64]; //bufor na wiadomość
    snprintf(msg, sizeof(msg), "the turn of the player with the descriptor fd=%d\n", games.turn_fd); //tworzenie wiadomości z deskryptorem gracza
    sendtlv(games.player1_fd, TLV_MSG, msg, (int)strlen(msg));//wysyłanie wiadomości do player1
    sendtlv(games.player2_fd, TLV_MSG, msg, (int)strlen(msg)); //wysyłanie wiadomości do player2
}

//zmienia ruch na drugiego gracza
static void switch_turn(void) { 
    int other = other_player_fd(games.turn_fd); //pobiera deskryptor drugiego gracza z wcześniejszej funkcji
    if (other != 0) games.turn_fd = other; //jeśli drugi gracz istnieje to zmienia ruch na niego
}

//stan logiczny graczy
static char g_p1_login[LOGIN_MAX] = {0}; //login pierwszego gracza
static char g_p2_login[LOGIN_MAX] = {0};  //login drugiego gracza


void game_set_login(int fd, const char *login) { //podpina login do gracza
    if (!login || login[0] == '\0') return;    //jeśli login jest pusty to wychodzi   
    if (fd == games.player1_fd) { //jeśli fd to player1 to ustawia login 1 gracza        
        strncpy(g_p1_login, login, LOGIN_MAX - 1); //kopiuje login do g_p1_login (wcześniejsza funkcja)
        g_p1_login[LOGIN_MAX - 1] = '\0'; //dodaje null na koniec
    } else if (fd == games.player2_fd) { //to samo dla drugiego gracza         
        strncpy(g_p2_login, login, LOGIN_MAX - 1);
        g_p2_login[LOGIN_MAX - 1] = '\0';        
    }
} 

static const char *word_list[] = { //lista słów do gry
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

void game_init(void) { //inicjalizacja gry
    memset(&games, 0, sizeof(games));
    srand(time(NULL)); //inicjalizacja rand
    score_init("scores.dat"); //inicjalizacja score z pliku
}

int start_game(int desc2){

    if(games.active){ //jesli trwa inna gra
        sendtlv(desc2, TLV_MSG, "Wait: game already in progress\n", 32);
        return 0;
    }

    if(games.player1_fd==0){ //jesli nie ma 1 gracza to dodajemy
        games.player1_fd=desc2;
        sendtlv(desc2, TLV_MSG, "Wait: waiting for another player...\n", 36);
        return 0;
    }
    else if(games.player2_fd==0 && desc2 != games.player1_fd){ //jesli jest 1 gracz a nie ma 2 to dodajemy 2 gracza
        games.player2_fd=desc2;
       
        //wybór słowa
          int index = rand() % (sizeof(word_list) / sizeof(word_list[0]));
        const char *selected_word = word_list[index];
        strncpy(games.word, selected_word,MAX_WORD_LEN- 1);  //kopiowanie slowa MAX_WORD_LEN 
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

    letter = (char)tolower((unsigned char)letter);// zamiana na małą literę
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
    if(!games.active){ //sprawdzenie czy gra trwa
        sendtlv(desc2, TLV_MSG, "ERROR: not in game?\n", 20);
        return;
    }
    if(desc2 != games.player1_fd && desc2 != games.player2_fd){ //sprawdzenie czy gracz jest w grze
        sendtlv(desc2, TLV_MSG, "ERROR: you didnt start the game\n", 32);
        return;
    }

    if(desc2 != games.turn_fd){ //sprawdzenie czy ruch jest tego gracza
        sendtlv(desc2, TLV_MSG, "ERROR: not your turn\n", 21);
        return;
    }

    int correct = 0; //czy poprawna litera
    int already = 0; //czy już była użyta


    for(size_t i=0; i<strlen(games.word); i++){ //sprawdzanie czy litera jest w słowie
        if(games.word[i]==letter && games.guessed[i]=='_'){ //jeśli litera jest w słowie i nie była wcześniej zgadnięta
            games.guessed[i]=letter; 
            correct=1; //ustawianie na 1, bo pozniej boolem sprawdzamy
        }
    }

        if (!correct) { //jeśli litera niepoprawna

        // czy litera już została użyta (zła)
        for (int j = 0; j < games.wrong_count; j++) {
            if (games.wrong_letters[j] == letter) {
                already = 1;
                break;
            }
        }

        if (already) { //jeśli już była użyta
            sendtlv(desc2, TLV_MSG, "You already tried this letter!\n", 30);
            return;
        }

        games.wrong_guesse++; //zwiększenie liczby błędów

        //dodanie litery do tablicy złych liter
        if (games.wrong_count < (int)sizeof(games.wrong_letters) - 1) {
            games.wrong_letters[games.wrong_count++] = letter; //dodanie litery i zwiększenie licznika
            games.wrong_letters[games.wrong_count] = '\0'; //dodanie null na koniec
        }

        sendtlv(desc2, TLV_MSG, "Wrong letter!\n", 14); //wysłanie info o złej literze
    }
    else {
        sendtlv(desc2, TLV_MSG, "Good guess!\n", 12); //wysłanie info o dobrej literze
    }

    //wyslanie stanu gry
    send_game();
    if(all_guessed()){
        //zapis score
        uint32_t s = score_calc((int)strlen(games.word), games.wrong_guesse); //obliczenie score
        if (g_p1_login[0]) score_update_best(g_p1_login, s); //zapis score dla 1 gracza
        if (g_p2_login[0]) score_update_best(g_p2_login, s); //zapis score dla 2 gracza

        sendtlv(games.player1_fd, TLV_MSG, "end of the game \n", 21); //wyslanie info o wygranej
        sendtlv(games.player2_fd, TLV_MSG, "end of the game \n", 21);
        game_reset();
        return;
    }
    if(games.wrong_guesse >= MAX_WRONG_GUESSES){
        //zapisz score nawet jak przegrasz
        uint32_t s = score_calc((int)strlen(games.word), games.wrong_guesse); 
        if (g_p1_login[0]) score_update_best(g_p1_login, s);                 
        if (g_p2_login[0]) score_update_best(g_p2_login, s); 

        sendtlv(games.player1_fd, TLV_MSG, "game over! too many wrong guesses\n", 36);
        sendtlv(games.player2_fd, TLV_MSG, "game over! too many wrong guesses\n", 36);
        game_reset();
        return;
    }
    switch_turn(); //zmiana ruchu
    send_turn_info(); //wyslanie info o ruchu
}

int all_guessed(void){//sprawdzenie czy wszystkie litery zostały zgadnięte
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




