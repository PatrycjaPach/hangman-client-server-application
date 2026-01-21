#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include                <unistd.h>
#include        <time.h>                /* old system? */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include "discovery_server.h"
//#include <strings.h> jakby krzyczaolo na bzero

#include <signal.h> //obsuga sigchild
#include <sys/wait.h> //obsuga sigchild

#include <ctype.h> //do liter i gry, zmiana wielkosci np


#define LISTEN 10 //ile klientw ma czekac w kolejce

void sigchild(int s){
        (void)s; /*zawsze kernel przekazuje numer sygnau jako argument alr my go nie 
        uzywamy*/ 
        while(waitpid(-1, NULL, WNOHANG)>0){} 
        /*
        -1-zabierz dowolne dziecko
        NULL- nie interesuje nas kod zakoczenia dziecka
        WHOHANG-jak nie ma dzieci do zabrania wr od razu
        */

}

int main(int argc, char **argv){
        int desc1, desc2; //desktyptory
        socklen_t len; //rozmiar adresu klienta
        struct sockaddr_in6 servaddr, cliaddr; //do struktury adresowej
        char buf[4096]; //bufor na dane
        ssize_t n;

        //logowanie i komunikaty, commit:1 - komunikaty
        int logged_in=0;
        int in_game=0;
        char username[32]; //LOGIN
        char letter; //litera do GUESS
        char extra; //do sprawdzania czy jest wiecej niz jedna litera w GUESS

        //socket
        if((desc1=socket(AF_INET6, SOCK_STREAM, 0))<0){
                perror("socket: ");//komunikat o bledzie
                return 1;
        }
         
        //struktura adresowa
        bzero(&servaddr, sizeof(servaddr)); //czyszczenie struktury adresowej
        servaddr.sin6_family =AF_INET6;
        servaddr.sin6_addr = in6addr_any;
        servaddr.sin6_port= htons(1234); //zmiana na inny port bo 13 to do daytime typowo

        //bind
        if(bind(desc1, (struct sockaddr*)&servaddr, sizeof(servaddr))<0){
                perror("bind: ");
                return 1;
        }
         
        //listen
        if(listen(desc1, LISTEN)<0){
                perror("listen: ");
                return 1;
        }
        if (fork() == 0) { //tylko w procesie potomnym
            multicast_discovery_server();   // multicast discovery
            exit(0);              // nigdy nie wraca
        }

        //sigaction 
        struct sigaction sigact;
        bzero(&sigact, sizeof(sigact));
        sigact.sa_handler=sigchild; //odwolanie do voida na gorze do obslugi waitpid
        sigemptyset(&sigact.sa_mask); //przy wykonywaniu hendlera nie blokuje dod. syg.
        sigact.sa_flags=SA_RESTART; //jesli sygnal przerwie accept to system ma je wzmowic
        sigaction(SIGCHLD, &sigact, NULL); //sigaction

        while(1){
                len=sizeof(cliaddr); //rozmiar bufora do ktrego wpiszemy adres klienta

                if((desc2=accept(desc1, (struct sockaddr*)&cliaddr, &len))<0){
                        if (errno == EINTR) continue; //Jakby nastapil Sigchild w trakcie
                        perror("accept: ");
                        continue; //jeli bd to idz na pocztek ptli
                }

                if(fork()==0){
                        close(desc1);
                        /*Na razie robie echo*/
                       /* while((n=read(desc2, buf, sizeof(buf)))>0){
                                write(desc2, buf, n); //odeslij to co jest w buforze
                        }*/ //zmiana echo na komunikaty:
                        //dodane, commit:1 - komunikaty 
                        while((n=read(desc2, buf, sizeof(buf) -1 ))>0){
                                buf[n]='\0'; //string terminator

                                char *newline=strchr(buf, '\n'); //szukamy znaku nowej linii
                              if(newline) *newline='\0'; //jesli znajdziemy to zmieniamy na null
                             
                              if (strncmp(buf, "LOGIN", 5) == 0) {
                                if(logged_in){
                                        write(desc2, "ERROR already logged in\n", 24);
                                        continue; //nie wiem czy dodac czy nie
                                }
                                else if(sscanf(buf, "LOGIN %31s", username)==1){    //sscanf sprawdza czy to co jest w buf pasuje do formatu "LOGIN %21s" i jesli tak to zapisuje do username
                                        logged_in=1;
                                        write(desc2, "OK LOGIN\n", 9); //korzystamy z write 
                                }
                                else{
                                        write(desc2, "ERROR invalid login\n",21);
                                        continue;
                                }
                                
                                }
                                else if (strcmp(buf, "JOIN") == 0) {
                                        if(!logged_in){
                                                write(desc2, "ERROR: not logged in\n", 22);
                                                continue;
                                        }
                                        else if(in_game){
                                                write(desc2, "ERROR: already in game\n",23);
                                                continue;
                                        }
                                        else{
                                                in_game=1;
                                                write(desc2, "WAIT: joining...\n", 18);
                                        }
                                        
                                }
                                else if (strncmp(buf, "GUESS ", 6) == 0) {
                                        if(!logged_in){
                                                write(desc2, "ERROR: not logged in\n", 22);
                                                continue;
                                        }
                                        else if(!in_game){
                                                write(desc2, "ERROR: not in game\n",20);
                                                continue;
                                        }
                                        
                                        if(sscanf(buf, "GUESS %c %c", &letter, &extra)>1){ //sprawdzanie czy nie jest wiecej literek
                                                write(desc2, "TO MANY LETTERS: write only one\n", 32);
                                                continue;
                                        } 

                                        else if(sscanf(buf, "GUESS %c", &letter)==1){
                                                //tutaj logika gry, zapisalismy literke do zmiennej na razie
                                               if(!isalpha((unsigned char)letter)){
                                                        write(desc2, "ERROR: must be a letter\n", 25);
                                                        continue;
                                               }
                                               letter=tolower((unsigned char)letter); //zmiana na mala litere
                                               //na razie zawsze ok, pozniej logika gry i sprawdzanie
                                                write(desc2, "OK GUESS\n", 9);
                                        }
                                        else{
                                                write(desc2, "ERROR invalid guess\n",21);
                                        }
                                }
                               /* else if (strcmp(buf, "QUIT") == 0) {SSSS
                                        write(desc2, "BYE\n", 4);
                                        break;
                                }*/
                                else {
                                        write(desc2, "ERROR unknown command\n", 22);
                                }
                        } //end of commit:1 komunikaty
                        if(n<0){
                                perror("read: ");
                                return 1;
                        }

                        /*Koniec echo*/
                        close(desc2);
                        exit(0);
                }
                close(desc2);
        }

}
