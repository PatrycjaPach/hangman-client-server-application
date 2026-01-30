// cli.c
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // servaddr
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>   // <-- ZMIANA: potrzebne do select()
#include "discovery.h"    // multicast
#include "tlv.h"          // format tlv
#include "protocol.h"

int main(int argc, char **argv){
    int desc;
    struct sockaddr_in6 servaddr;
    char buf[4096];
    ssize_t n;

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

    if (connect(desc, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(desc);
        return 1;
    }


    while (1) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds); //  stdin
        FD_SET(desc, &rfds);         //  socket

        int maxfd = (desc > STDIN_FILENO) ? desc : STDIN_FILENO;

        // select czeka aż coś będzie do czytania na którymś fd
        int ready = select(maxfd + 1, &rfds, NULL, NULL, NULL);
        if (ready < 0) {
            if (errno == EINTR) continue; // przerwane sygnałem, wracamy do pętli
            perror("select");
            break;
        }


        // jeśli serwer wysłał cokolwiek -> od razu odbierz TLV
        if (FD_ISSET(desc, &rfds)) {
            uint16_t type;
            uint8_t rbuf[MAX_TLV_VALUE];

            int rlen = recv_tlv(desc, &type, rbuf, sizeof(rbuf));
            if (rlen <= 0) {
                printf("Server disconnected\n");
                break;
            }


            if (type == TLV_MSG) {
                write(STDOUT_FILENO, rbuf, rlen);
            }
        }


        // jeśli klient wpisał komendę -> czytamy stdin i wysyłamy TLV

        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            n = read(STDIN_FILENO, buf, sizeof(buf)-1);
            if (n == 0) break; 
            if (n < 0) {
                perror("read stdin");
                break;
            }
            buf[n] = '\0';

            // wysyłanie TLV jak wcześniej
            if (strncmp(buf, "LOGIN ", 6) == 0) {
                char *username = buf + 6;
                username[strcspn(username, "\n")] = '\0';
                sendtlv(desc, TLV_LOGIN, username, strlen(username));
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
                sendtlv(desc, TLV_SCORE, NULL, 5);
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
