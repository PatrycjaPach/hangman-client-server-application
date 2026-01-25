#ifndef NEW_CLIENTS_H
#define NEW_CLIENTS_H

#include <stddef.h>

#define MAX_CLIENTS 100
#define USERNAME_LEN 32

//to bedzie stan klienta:
struct client {
    int fd;                         // deskryptor gniazda
    int used;                       
    int logged_in;                  
    int ready;                      
    char username[USERNAME_LEN];    
};

//inicjalizacja i czysczenie tablicy
void clients_init(void);

//dodaje nowego klienta 
int client_add(int fd);

//usuwa klienta
void client_remove(int fd);

/* Zwraca wskaźnik do klienta albo NULL */
struct client *client_get(int fd);

/* ===== Logowanie ===== */

/* Sprawdza, czy login jest już zajęty */
int username_taken(const char *username);

/* Ustawia login klienta */
int client_login(int fd, const char *username);

/* Wylogowuje klienta */
void client_logout(int fd);

/* ===== JOIN (bez gry) ===== */

/* Ustawia flagę ready */
int client_set_ready(int fd);

#endif
