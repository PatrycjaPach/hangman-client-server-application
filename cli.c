#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //servaddr
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "discovery.h" //multicast
#include "tlv.h" //format tlv
#include "protocol.h"



int main(int argc, char **argv){
	int desc, err_inet_pton;
	struct sockaddr_in6 servaddr;
	char buf[4096]; //do echa
	ssize_t n; //do echa

	if ((desc = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}

	//struktura adresowa
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family=AF_INET6;
	servaddr.sin6_port=htons(1234);
	
    //zamiast tego co na gorze, do multicastu:
    if (discover_server(&servaddr) < 0) { //wywolanie funkcji do szukania servera przez multicast
        fprintf(stderr, "Nie znaleziono serwera\n");
        close(desc); //zamkniecie gniazda jak sie nie uda wszykuac servera albo connect zrobic, bo inaczej byloyby nie uzyteczne
        return 1;
    }

    //zmiana connect pod multicast:
    if (connect(desc, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(desc);
        return 1;
    }
	

	while (1) {
		// 1) weź dane od użytkownika
		n = read(STDIN_FILENO, buf, sizeof(buf)-1); //o
		if (n == 0) break; // EOF (Ctrl+D) -> koniec
		if (n < 0) { perror("read stdin"); break; }//o
		buf[n] = '\0';//o

		
		//zamiast write, pod tlv:
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
		else {
			printf("Unknown command\n");
			continue;
		}


		//zamaist read pod tlv: (pełni podobne funkcje)
		uint16_t type;
		uint8_t rbuf[MAX_TLV_VALUE];

		int rlen = recv_tlv(desc, &type, rbuf, sizeof(rbuf));
		if (rlen <= 0) {
			printf("Server disconnected\n");
			break;
		}

		//zamiast 4)

		
		if (type == TLV_MSG) {
			write(STDOUT_FILENO, rbuf, rlen);
		}


	}


    close(desc); 
    return 0;
}
