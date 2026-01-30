#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <syslog.h>
#include <fcntl.h>
#include <sys/types.h>

#define MAXFD 64

int daemon_init(const char *pname)
{
    pid_t pid;
    int i;


    pid = fork();
    if (pid < 0) // fork error
        return -1;
    if (pid > 0) // rodzic dostaje PID dziecka
        exit(0); // rodzic wychodzi demon odkleja się od procesu


    if (setsid() < 0) // tworzenie nowej sesji
        return -1; // błąd

    signal(SIGHUP, SIG_IGN); // ignorowanie sygnału SIGHUP, bo po zamknięciu terminala może ubić proces

    // drugi fork, żeby demon nie był liderem sesji i nie mógł odzyskać terminala
    pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        exit(0);

    //zmiana katalogu roboczego na /
    chdir("/");

    //zamykanie wszystkich deskryptorów plików/
    for (i = 0; i < MAXFD; i++)
        close(i);

    // przekierowanie stdin, stdout, stderr do /dev/null żeby nie był przypadkowo użyty przez socket
    open("/dev/null", O_RDONLY); //stdin-czytanie
    open("/dev/null", O_RDWR); //stdout-wypisanie na output
    open("/dev/null", O_RDWR); //stderr-wypisanie błędów

    // inicjalizacja sysloga bo demon przez niego wysyła komunikaty
    openlog(pname, LOG_PID | LOG_CONS, LOG_DAEMON);

    return 0;
}
