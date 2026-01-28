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


        clients_init(); //funckja z new clients, czyszczenie tablicy klientow

        //tworzenie gniazda IPv6 TCP
        if ((desc1 = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
                perror("socket");
                return 1;
        }
        //struktura adresowa
        bzero(&servaddr, sizeof(servaddr));
        servaddr.sin6_family = AF_INET6;
        servaddr.sin6_addr   = in6addr_any;
        servaddr.sin6_port   = htons(1234);

        //bind
        if (bind(desc1, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
                perror("bind");
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
