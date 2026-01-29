#include "discovery.h"
#include <sys/socket.h> /* basic socket definitions */
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define MCAST_ADDR "ff02::1234" //adres do grupy rozgloszeniowej
#define MCAST_PORT 5555 //numer portu do multicastu

int discover_server(struct sockaddr_in6 *servaddr)
{
    int s;
    struct sockaddr_in6 mcast_addr;
    char buf[256];
    socklen_t len;
    
    //tworzenie gniazda ipv6 z udp:
  if ( (s= socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
			return -1;
		}

    //ustawianie struktury adresowej
    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin6_family = AF_INET6;
    mcast_addr.sin6_port   = htons(MCAST_PORT); //port do wyszukiwania uslug
    inet_pton(AF_INET6, MCAST_ADDR, &mcast_addr.sin6_addr);

    /*//struktura adresowa
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family=AF_INET6;
	servaddr.sin6_port=htons(13);*/

    //wysylanie jednego datagramu tylko, dla gniazda s-udp : ODKRYWANIE, bierze adres i port ze struktury
    sendto(s, "ODKRYWANIE", 10, 0,
           (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));
    printf("odkrywanie serwera przez multicast\n");
    fflush(stdout);

   //odbieram jeden datagram, dane->bufora, zapisanie adresy nadawcy do struktury
    //potrzebujemy głownie adresu servera a nie samej wiadomosci
   len = sizeof(*servaddr);
    if (recvfrom(s, buf, sizeof(buf), 0,
                 (struct sockaddr*)servaddr, &len) < 0) {
        perror("recvfrom() error");
        close(s);  //gniazdo potrzebujemy jednorazowo, do znalezienia servera wiec zamykamy w kazdym przypadku
        return -1;
    }
    printf("odebrano pakiet od servera przez multicast\n");
    fflush(stdout);

    servaddr->sin6_port = htons(1234);
    close(s);
    return 0;
}