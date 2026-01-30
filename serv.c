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
#include "new_clients.h"
#include "handle_client.h"
//#include <strings.h> jakby krzyczaolo na bzero

#include <signal.h> //obsuga sigchild - juz nie potrzebna, chyba ze do multicastu ale to id
#include <sys/wait.h> //obsuga sigchild

#include <ctype.h> //do liter i gry, zmiana wielkosci np
#include <poll.h> //do poll - wspolbieznosc
#include "game.h" //logika gry
#include <netdb.h> //do getaddrinfo
#include "demon.h" //do demonizacji

#define LISTEN 10 //kolejka dla listen
#define MAXEVENTS 2000 //max liczba deskryptorów w pollu



int main(int argc, char **argv)
{
        int desc1, desc2; //desc1 do bind, listen, a desc2 do obslugi kleinta, potrzebaa dwoch bo mmamy poll
        socklen_t len;
        struct sockaddr_in6 servaddr, cliaddr;

        //do poll
        struct pollfd client[MAXEVENTS]; //tablica opisów deskryptorów dla poll
        int i, maxi, nready;

        if (daemon_init("hangman_server") < 0) {
                exit(1);
        }

        

        clients_init(); //funckja z new clients, czyszczenie tablicy klientow
        game_init(); //z game.h, czyszczenie tablicy gry

         //ignoruemy sigchild zeby nie robic zombie process
        //tworzenie gniazda IPv6 TCP
        if ((desc1 = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
                perror("socket");
                return 1;
        }

        //REUSEADDR-warto
        int one = 1;
        if (setsockopt(desc1, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
                perror("setsockopt SO_REUSEADDR");
                close(desc1);
                return 1;
        }

        //DNS
        struct addrinfo hints, *res = NULL, *p = NULL; //miejsce na wynik dns
        char portstr[6]; //zmiana numeru portu na tekst
        snprintf(portstr, sizeof(portstr), "%u", 1234);

        memset(&hints, 0, sizeof(hints));
        hints.ai_family   = AF_INET6;       // używamy IPv6 jak w Twoim kodzie (AF_INET6)
        hints.ai_socktype = SOCK_STREAM;    // TCP
        hints.ai_flags    = AI_PASSIVE;     // gdy host == NULL -> bind na "::" (wszystkie interfejsy)

        // host do binda: jeśli podano argument, użyj go; jak nie -> NULL (czyli "::")
        const char *bind_host;

        if (argc >= 2) {
                bind_host = argv[1];   //nazwa hosta
        } else {
                bind_host = NULL; //brak argumentu bind na ::
        }


        int gai_rc = getaddrinfo(bind_host, portstr, &hints, &res);
        if (gai_rc != 0) {
                fprintf(stderr, "getaddrinfo %s\n", gai_strerror(gai_rc));
                close(desc1);
                return 1;
        }

        int bind_ok = 0;
        for (p = res; p != NULL; p = p->ai_next) {//for na liste adresów zwróconą przez dns
                memcpy(&servaddr, p->ai_addr, sizeof(servaddr));

                if (bind(desc1, p->ai_addr, p->ai_addrlen) == 0) {
                        bind_ok = 1; //jeśli bind się udał
                        break;
                }
        }

        freeaddrinfo(res);

        if (!bind_ok) { //jak bind się nie udał na żadnym adresie
                perror("bind");
                close(desc1);
                return 1;
        }

        //listen
        if (listen(desc1, LISTEN) < 0) {
                perror("listen");
                return 1;
        }

        //multicast w osobyn procesie 
        if (fork() == 0) {
                multicast_discovery_server();
                exit(0);
        }

        //poll - inicjalizacja
        client[0].fd = desc1; //patrzy na gniazdo nasłuchujące
        client[0].events = POLLIN; //dzieki temu przyjmowanie polaczen bedzie wykrywane

        for (i = 1; i < MAXEVENTS; i++)
                {client[i].fd = -1; //wpisujemy -1 do reszty, zeby wiedziec ze sa wolne
                }

        maxi = 0; //rosnie gdy dodajemy kleintow, na start 0

        while (1) {
                //poll czeka na zdarzenia, blokuje sie do tego momentu
                nready = poll(client, maxi + 1, -1);
                if (nready < 0) {
                        if (errno == EINTR) continue; //jesli przerwanie sygnałem to kontynuuj
                        perror("poll"); //inaczej -> blad
                        exit(1);
                }

                //nowe polaczenie:
                if (client[0].revents & POLLIN) { //jesli zdarzenie na nasluchujacym
                        len = sizeof(cliaddr);
                        desc2 = accept(desc1, (struct sockaddr*)&cliaddr, &len); //tworzymy nowe gniazdo dla klienta

                        if (desc2 >= 0) {
                                for (i = 1; i < MAXEVENTS; i++) {
                                        if (client[i].fd < 0) { //szukanie wolnego miejsca w tablicy
                                                client[i].fd = desc2; //przypisaywanie wartosci
                                                client[i].events = POLLIN;
                                                client_add(desc2); //funkcja z new_clients - dodanie nowego klienta do tablicy
                                                if (i > maxi) maxi = i; //aktaulizacja maxi
                                                break;
                                        }
                                }
                        }
                        if (--nready <= 0) //jesli nie ma wiecej zdarzen to kontynuuj
                                continue;
                }

                // obsluga danych od kleintow
                for (i = 1; i <= maxi; i++) {
                        if (client[i].fd < 0) //jesli doszlismy do wolnych miejsc to wyjdz z petli
                                {
                                        continue;
                                }

                        if (client[i].revents & POLLIN) { //jesli sa dane do odczytu
                                handle_client_input(client[i].fd); //obsluga danych
                                if (--nready <= 0) //jesli nie ma wiecej zdarzen to wyjdz
                                break;
                        }
                        }
        }
}
