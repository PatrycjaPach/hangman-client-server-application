#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "new_clients.h"
#include "tlv.h"
#include "protocol.h"
#include "game.h" 
#include "score.h"

void handle_client_input(int desc2)
{
    //char buf[4096];
    //ssize_t n;
    //char username[32]; //LOGIN
    //char letter; //litera do GUESS
    //char extra; //do sprawdzania czy jest wiecej niz jedna litera w GUESS
    uint16_t type;
    uint8_t value[MAX_TLV_VALUE];
    //value to buf w tlv.c/.h, zapisuje bajty do tej tablicy recv_tlv
    int len = recv_tlv(desc2, &type, value, sizeof(value));
    if (len < 0) {
        close(desc2);
        client_remove(desc2);
        return;
    }
   // n = read(desc2, buf, sizeof(buf) - 1); //odczyt
  /* if (n <= 0) { //zamiast while
    if (n < 0)
        perror("read");
    close(desc2);
    client_remove(desc2);
    return;
}*/

    //buf[n] = '\0'; //zerowanie
    //char *newline = strchr(buf, '\n');//szukamy znaku nowej linii
    //if (newline) *newline = '\0'; //jesli znajdziemy to zmieniamy na null

    struct client *c = client_get(desc2); //tworzymy strukture, ktora ma zapisanych aktualnych kleintow
    if (!c) return; //jesli by bylo puste to wroc

    //logika logowania:
    switch(type){
        case TLV_LOGIN: {
            char username[MAX_USERNAME];

            if(len>=MAX_USERNAME){
                sendtlv(desc2, TLV_MSG, "ERROR: login is too long\n",25);
                break;
            }

            memcpy(username, value, len);
            username[len]='\0';

            if (c->logged_in){
                sendtlv(desc2, TLV_MSG, "ERROR already logged in\n",24);
            }
            else if(username_taken(username))//sprawdzamy czy wolny login, funkcja z new_clients
                {
                    sendtlv(desc2, TLV_MSG, "ERROR login taken\n", 19);
                }
            else if(client_login(desc2, username) < 0){ //funkcja z new_client
            sendtlv(desc2, TLV_MSG, "ERROR login failed\n", 19);
            //continue;
                }
            else{
                sendtlv(desc2, TLV_MSG,"OK LOGIN\n", 9);
            }
            
            break;
        }
        case TLV_JOIN: {
            if (!c->logged_in)
            {sendtlv(desc2, TLV_MSG, "ERROR: not logged in\n", 22);
                //continue;
            }
            
            else{
               // sendtlv(desc2,TLV_MSG, "WAIT: joining...\n", 18);
                start_game(desc2); //funkcja z game.c zaczynanie gry
                game_set_login(desc2, c->username);
                }
            break;
            }
        case TLV_GUESS:{
            
                if (!c->logged_in){
                sendtlv(desc2,TLV_MSG, "ERROR: not logged in\n", 22);
                break;
            } 
        /*else if (!c->in_game){
                write(desc2, "ERROR: not in game\n", 20);
                //continue; //TO TRZEBA BEDZIE ALE TO W GRZE
            }*/ 
            if (len!=1){
                sendtlv(desc2,TLV_MSG, "ERROR: exactly one letter expected\n", 36);
                break;
            }  
            char letter = value[0];
            
            if(!isalpha((unsigned char)letter)) {
                    sendtlv(desc2,TLV_MSG, "ERROR: must be a letter\n", 25);
                }
                    
            else {
                    letter = tolower((unsigned char)letter);
                    guess(desc2, letter); //funkcja z game.c
                }
                break;
            } 
            case TLV_SCORE: {
                //sendtlv(desc2, TLV_MSG, "DEBUG: SCORE requested\n", 22); // debug
                //score_print_all(desc2);
                if (!c->logged_in) {
                    sendtlv(desc2, TLV_MSG, "ERROR: not logged in\n", 22);
                    break;
                }

                //start_game(desc2);
                score_print_all(desc2);
                // login w grze 
                

                break;
            }

            default: {
                sendtlv(desc2, TLV_MSG,"ERROR: unknown message\n", 24);
            }
        }
       
    }




  