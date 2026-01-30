#include "score.h"
#include "protocol.h"
#include "tlv.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

typedef struct {
    char login[LOGIN_MAX];   // nazwa gracza
    uint32_t best;           // jego najlepszy wynik
    int used;                // czy to miejsce w tablicy jest zajęte
} score_entry_t;

#define MAX_PLAYERS 1024

static score_entry_t g_db[MAX_PLAYERS];
static pthread_mutex_t g_mtx = PTHREAD_MUTEX_INITIALIZER; //tylko jeden wątek może wejśc
static char g_path[256] = {0}; //ścieżka do pliku z wynikami

static int find_login(const char *login)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (g_db[i].used == 0) //czy miejsce tablicy jest zajęte
            continue;

        if (strcmp(g_db[i].login, login) == 0) //czy login użytkownika już istnieje
            return i;
    }

    return -1;
}

static int player_entry(const char *login)
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
    pthread_mutex_lock(&g_mtx); //zabezpieczenie dla kilku wątków
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
    pthread_mutex_unlock(&g_mtx); //otworzenie żeby inni mogli korzystać
}

static void save_nolock(void)
{
    if (g_path[0] == '\0') return; // czy znamy nazwę pliku
    FILE *f = fopen(g_path, "wb"); // otwórz plik do zapisu binarnego
    if (f == NULL) return; // jeśli nie udało się otworzyć
    fwrite(g_db, sizeof(g_db), 1, f); // zapisz całą tablicę do pliku
    fclose(f); // zamknij plik
}


uint32_t score_get_best(const char *login)
{
    if (login == NULL) return 0; // jeśli nie podano loginu
    pthread_mutex_lock(&g_mtx);// blokada dostępu do bazy
    int idx = find_login(login);// szukamy loginu w bazie
    uint32_t best = 0;           

    if (idx >= 0){
        best = g_db[idx].best; // pobieramy najlepszy wynik
    } 

    pthread_mutex_unlock(&g_mtx); // odblokowanie dostępu
    return best;                                 // zwróć wynik
}

void score_update_best(const char *login, uint32_t score)
{
    if (login == NULL) return; // jeśli brak loginu
    if (login[0] == '\0') return; // jeśli login pusty
    pthread_mutex_lock(&g_mtx); // blokada bazy
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
    pthread_mutex_unlock(&g_mtx);
}

uint32_t score_calc(int word_len, int wrong_guesses) {
    if (word_len <= 0) return 0;
    int point = word_len * 100;
    int wrong = wrong_guesses * 30;
    int result = point - wrong;
    if (result < 0) result = 0;
    return (uint32_t)result;
}

void score_print_all(int fd)
{
    pthread_mutex_lock(&g_mtx);

    int any = 0; //czy jest jakis wynik

    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!g_db[i].used) continue;

        any = 1; //czy jest jakis wynik

        char buf[128];
        snprintf(buf, sizeof(buf), "%s %u\n", g_db[i].login, g_db[i].best);
        sendtlv(fd, TLV_MSG, buf, (int)strlen(buf));
    }

    if (!any) { // brak wpisów
        sendtlv(fd, TLV_MSG, "No scores yet.\n", 15);
    }
    pthread_mutex_unlock(&g_mtx);
}
