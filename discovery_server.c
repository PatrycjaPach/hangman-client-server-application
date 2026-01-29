#include "discovery_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define MCAST_ADDR "ff02::1234" //ten sam adres co w kliencie (musi byc ta sama grupa)
#define MCAST_PORT 5555 //ten sam port co klient

void multicast_discovery_server(void)
{
    int s;
    struct sockaddr_in6 addr, client;
    socklen_t len;
    char buf[256];
    ssize_t n;

    //tworzenie gniazda ipv6 z udp: to samo co klient
  if ( (s= socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			fprintf(stderr,"AF_INET6 socket error : %s\n", strerror(errno));
			return;
		}

        //analogicznie do klienta:
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port   = htons(MCAST_PORT);
    addr.sin6_addr   = in6addr_any;// do słuchania na wszystkich adresach

        //bind z konkretnej struktury adresowej: struct sockaddr_in6
    if(bind(s, (struct sockaddr*)&addr, sizeof(addr))<0){
        perror("bind error");
        close(s);
        return;
    }

    //potrzebne do setsockopt:
    struct ipv6_mreq mreq;
    inet_pton(AF_INET6, MCAST_ADDR, &mreq.ipv6mr_multiaddr); //wpisanie danych do struktury
    mreq.ipv6mr_interface = 0; //domyślny interface

    //dołączenie do grupy:
    if(setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP,
               &mreq, sizeof(mreq))<0){
                perror("join group error");
                close(s);
                return;
               }

    for (;;) {
        len = sizeof(client);
        //odebranie od klienta:
        if((n = recvfrom(s, buf, sizeof(buf)-1, 0,
                         (struct sockaddr*)&client, &len))<0) {
                            perror("recvfrom() error");
                            continue;
                         }
        printf("odebrano pakiet od klienta przez multicast\n");
        fflush(stdout);

        buf[n] = 0;
        //aby było kompatybilne z klientem:
        if (strcmp(buf, "ODKRYWANIE") == 0) {
            //odpowiedz:, wysyła do klienta
            sendto(s, "JESTEM", 6, 0,
                   (struct sockaddr*)&client, len);
        }
    }
}