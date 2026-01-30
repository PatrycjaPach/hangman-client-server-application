// cli.c
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // servaddr
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>   // potrzebne do select()
#include "discovery.h"    // multicast
#include "tlv.h"          // format tlv
#include "protocol.h"
#include <signal.h> 

int main(int argc, char **argv){
    int desc;
    struct sockaddr_in6 servaddr;
    char buf[4096];
    ssize_t n;

    signal(SIGPIPE, SIG_IGN);//ignorowanie sygnału SIGPIPE
    
    if ((desc = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port   = htons(1234);

    // multicast discovery
    if (discover_server(&servaddr) < 0) {
        fprintf(stderr, "Nie znaleziono serwera\n");
        close(desc);
        return 1;
    }

    if (connect(desc, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) { //uzupelnia adres serwera - wziety z mulitcastu
        perror("connect");
        close(desc);
        return 1;
    }


    while (1) {
        fd_set rfds; //typ danych z select() (zbiór deskryptorów do czytania)
        FD_ZERO(&rfds); //czyści zbiór
        FD_SET(STDIN_FILENO, &rfds); //kiedy użytkownaik coś wpisze czli deskryptor stdin
        FD_SET(desc, &rfds); //kiedy serwer coś wyśle

        int maxfd;

        if (desc > STDIN_FILENO) { //select potrzebuje max deskryptora bo inkrementuje od 0 do maxfd
            maxfd = desc;
        } else {
            maxfd = STDIN_FILENO;
        }

        /* select czeka aż coś będzie do czytania na którymś fd
        maxfd-+ - bo select sprawdza od 0 do maxfd włącznie
        &rfds - zbiór deskryptorów do czytania
        */
        int ready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        
        if (ready < 0) {// sprawdzamy czy select się nie zepsuł
            if (errno == EINTR) continue; // przerwane sygnałem, wracamy do pętli, jeśli sygnał nas przerwał, jest zapisywane do EITER
            perror("select");
            break;
        }


        // jeśli serwer wysłał cokolwiek - od razu odbierz TLV
        if (FD_ISSET(desc, &rfds)) { //FD_ISSET sprawdza czy dany deskryptor jest gotowy do czytania i jest w zbioerze
            uint16_t type; // typ odebranego TLV
            uint8_t rbuf[MAX_TLV_VALUE]; // bufor na wartość TLV

            int rlen = recv_tlv(desc, &type, rbuf, sizeof(rbuf)); //czytamy jedną ramkę TLV
            if (rlen <= 0) { //błąd odebrania TLV
                printf("Server disconnected\n");
                break;
            }


            if (type == TLV_MSG) { //jeśli typ TLV to wiadomość - wyświetlamy ją
                write(STDOUT_FILENO, rbuf, rlen); //wyświetlamy wiadomość od serwera
            }
        }


        // jeśli klient wpisał komendę czytamy i wysyłamy TLV

        if (FD_ISSET(STDIN_FILENO, &rfds)) { //jeśli coś jest do czytania na stdin
            n = read(STDIN_FILENO, buf, sizeof(buf)-1); //czytamy dane ze stdin
            if (n == 0) break; // jeśli stdin zamknięty to zamykamu
            if (n < 0) {   //błąd czytania
                perror("read stdin");
                break;
            }
            buf[n] = '\0'; // znak końca stringa

            // wysyłanie TLV jak wcześniej
            if (strncmp(buf, "LOGIN ", 6) == 0) { //sprawdzamy czy komenda to LOGIN
                char *username = buf + 6; //wskaźnik na nazwę użytkownika
                username[strcspn(username, "\n")] = '\0'; //usuwamy znak nowej linii z końca nazwy użytkownika
                sendtlv(desc, TLV_LOGIN, username, strlen(username)); //wysyłamy TLV_LOGIN z nazwą użytkownika
            }
            else if (strncmp(buf, "JOIN", 4) == 0) {
                sendtlv(desc, TLV_JOIN, NULL, 0);
            }
            else if (strncmp(buf, "GUESS ", 6) == 0) {
                char letter = buf[6];
                sendtlv(desc, TLV_GUESS, &letter, 1);
            }
            else if (strncmp(buf, "WRONG", 5) == 0) {
                sendtlv(desc, TLV_WRONG, NULL, 0);
            }
            else if (strncmp(buf, "SCORE", 5) == 0) {
                sendtlv(desc, TLV_SCORE, NULL, 0);
            }
            else {
                printf("Unknown command\n");
                continue;
            }
        }
    }

    close(desc);
    return 0;
}
