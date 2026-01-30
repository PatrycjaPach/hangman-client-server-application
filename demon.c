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

    /* 1. fork */
    pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        exit(0);   /* parent exits */

    /* 2. new session */
    if (setsid() < 0)
        return -1;

    /* 3. ignore SIGHUP */
    signal(SIGHUP, SIG_IGN);

    /* 4. second fork */
    pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        exit(0);

    /* 5. change working directory */
    chdir("/");

    /* 6. close file descriptors */
    for (i = 0; i < MAXFD; i++)
        close(i);

    /* 7. redirect stdin/stdout/stderr */
    open("/dev/null", O_RDONLY);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

    /* 8. syslog */
    openlog(pname, LOG_PID | LOG_CONS, LOG_DAEMON);

    return 0;
}
