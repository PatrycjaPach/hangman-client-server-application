#include "score.h"
#include "protocol.h"
#include "tlv.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    char login[LOGIN_MAX];// nazwa gracza
    uint32_t best;// jego najlepszy wynik
    int used;// czy to miejsce w tablicy jest zajęte
} score_entry_t;

#define MAX_PLAYERS 1024

static score_entry_t g_db[MAX_PLAYERS]; //baza wyników
static pthread_mutex_t verification = PTHREAD_MUTEX_INITIALIZER; //tylko jeden wątek może wejśc
static char g_path[256] = {0}; //ścieżka do pliku z wynikami

static int find_login(const char *login) //szukanie loginu w bazie
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (g_db[i].used == 0) //czy miejsce tablicy jest zajęte
            continue; //przechodzi do następnego
            
        if (strcmp(g_db[i].login, login) == 0) //czy login użytkownika już istnieje
            return i; //zwraca indeks w tablicy
    }

    return -1; //nie znaleziono loginu
}

static int player_entry(const char *login) //dodawanie nowego gracza do bazy
{
    // tablica graczy
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        // sprawdzamy czy miejsce jest wolne
        if (g_db[i].used == 0){
            g_db[i].used = 1; // oznaczamy miejsce jako zajęte
            strcpy(g_db[i].login, login); // kopiujemy login do struktury
            g_db[i].best = 0; //najlepszy wynik na 0

            return i; // zwracamy numer miejsca w tablicy

        }
    }

    return -1;
}

void score_init(const char *path)
{
    pthread_mutex_lock(&verification); //zabezpieczenie dla kilku wątków
    memset(g_db, 0, sizeof(g_db)); //czyszczenie tablicy

    // jeśli podano ścieżkę do pliku
    if (path != NULL)
    {
        strcpy(g_path, path);//zapis ścieżki do zmiennej globalnej
        FILE *f = fopen(g_path, "rb"); //otwieramy plik

        // jeśli plik istnieje
        if (f != NULL)
        {
            fread(g_db, sizeof(g_db), 1, f); //wczytywani
            fclose(f); // zamykamy plik
        }
    }
    pthread_mutex_unlock(&verification); //otworzenie żeby inni mogli korzystać
}

static void save_nolock(void)
{
    if (g_path[0] == '\0') return; // czy znamy nazwę pliku
    FILE *f = fopen(g_path, "wb"); // otwórz plik do zapisu binarnego
    if (f == NULL) return; // jeśli nie udało się otworzyć
    fwrite(g_db, sizeof(g_db), 1, f); // zapisz całą tablicę do pliku
    fclose(f); // zamknij plik
}


uint32_t score_get_best(const char *login) // pobieranie najlepszego wyniku gracza
{
    if (login == NULL) return 0; // jeśli nie podano loginu
    pthread_mutex_lock(&verification);// blokada dostępu do bazy
    int idx = find_login(login);// szukamy loginu w bazie
    uint32_t best = 0;           

    if (idx >= 0){ // jeśli znaleziono
        best = g_db[idx].best; // pobieramy najlepszy wynik
    } 

    pthread_mutex_unlock(&verification); // odblokowanie dostępu
    return best;                                 // zwróć wynik
}

void score_update_best(const char *login, uint32_t score)
{
    if (login == NULL) return; // jeśli brak loginu
    if (login[0] == '\0') return; // jeśli login pusty
    pthread_mutex_lock(&verification); // blokada bazy
    int idx = find_login(login); // szukamy gracza w bazie

    if (idx < 0)// jeśli nie znaleziono
        idx = player_entry(login); // dodajemy nowego gracza

    if (idx >= 0)// jeśli mamy poprawny indeks
    {
        if (score > g_db[idx].best)// jeśli nowy wynik lepszy
        {
            g_db[idx].best = score;// zapisz nowy rekord
            save_nolock();// zapisz bazę do pliku
        }
    }
    pthread_mutex_unlock(&verification);
}

uint32_t score_calc(int word_len, int wrong_guesses) {
    if (word_len <= 0) return 0; // zabezpieczenie przed dzieleniem przez 0
    int point = word_len * 150; // punkty za długość słowa
    int wrong = wrong_guesses * 10; // punkty ujemne za błędne zgadywanie
    int result = point - wrong; // obliczenie wyniku
    if (result < 0) result = 0; // wynik nie może być ujemny, jeśli tak to ustawiamy na 0
    return (uint32_t)result; // zwracamy wynik 
}

void score_print_all(int fd) // wysyłanie wszystkich wyników do klienta
{
    pthread_mutex_lock(&verification); // blokada bazy

    int any = 0; //czy jest jakis wynik

    for (int i = 0; i < MAX_PLAYERS; i++) {//przechodzenie po tablicy wyników
        if (!g_db[i].used) continue; //puste miejsce w tablicy

        any = 1; //czy jest jakis wynik

        char buf[128]; //bufor na wiadomość
        snprintf(buf, sizeof(buf), "%s %u\n", g_db[i].login, g_db[i].best); //tworzenie wiadomości z loginem i wynikiem
        sendtlv(fd, TLV_MSG, buf, (int)strlen(buf)); //wysyłanie wiadomości do klienta
    }

      if (!any) { // brak wpisów
        sendtlv(fd, TLV_MSG, "No scores yet.\n", 15); //wysyłanie informacji o braku wyników
    }
    pthread_mutex_unlock(&verification); // odblokowanie bazy
}
