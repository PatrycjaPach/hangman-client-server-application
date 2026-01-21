#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //servaddr
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "discovery.h" //multicast

int main(int argc, char **argv){
	int desc, err_inet_pton;
	struct sockaddr_in6 servaddr;
	char buf[4096]; //do echa
	ssize_t n; //do echa


        //obsluga bledu jakby weszlo za duzo argumentow
      /* if(argc !=2){
                fprintf(stderr, "WARNING: invalid number of arguments %s\n", argv[0]);
                return 1;
        }*/ 

	//SOCKET: obsluga bledu i stworzenie gniazda
	if ((desc = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}

	//struktura adresowa
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin6_family=AF_INET6;
	servaddr.sin6_port=htons(1234);
	
	/*tlumaczenie adresu na porzadek sieciowy 
	razem z obsluga bledu (do connect)*/
	/*err_inet_pton=inet_pton(AF_INET6, argv[1], &servaddr.sin6_addr);
	
	if(err_inet_pton==0){
		fprintf(stderr, "Invalid IPv6 addres\n");
		return 1;
	} 
	if(err_inet_pton<0){
		perror("inet_pton");
		return 1;
	}*/
    //zamiast tego co na gorze, do multicastu:
    if (discover_server(&servaddr) < 0) { //wywolanie funkcji do szukania servera przez multicast
        fprintf(stderr, "Nie znaleziono serwera\n");
        close(desc); //zamkniecie gniazda jak sie nie uda wszykuac servera albo connect zrobic, bo inaczej byloyby nie uzyteczne
        return 1;
    }
    /*if (connect(desc, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        return 1;
    }*/
    //zmiana connect pod multicast:
    if (connect(desc, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(desc);
        return 1;
    }
	
	//echo z chata
	while (1) {
		// 1) weź dane od użytkownika
		n = read(STDIN_FILENO, buf, sizeof(buf));
		if (n == 0) break; // EOF (Ctrl+D) -> koniec
		if (n < 0) { perror("read stdin"); break; }

		// 2) wyślij do serwera
		if (write(desc, buf, n) < 0) { perror("write socket"); break; }

		// 3) odbierz echo z serwera
		n = read(desc, buf, sizeof(buf));
		if (n == 0) break; // serwer zamknął połączenie
		if (n < 0) { perror("read socket"); break; }

		// 4) wypisz na ekran
		if (write(STDOUT_FILENO, buf, n) < 0) { perror("write stdout"); break; }
	}
	//koniec echa z chata

    close(desc); 
    return 0;
}
