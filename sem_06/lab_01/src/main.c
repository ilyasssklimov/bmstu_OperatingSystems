#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <time.h>
#include <syslog.h>
#include <unistd.h>


#define LOCKFILE "/var/run/daemon.pid"
#define LOCKMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

#define TIMEOUT 3

sigset_t mask;


void daemonize(const char *cmd)
{
    int i, fd0, fd1, fd2;
    pid_t pid;
    struct rlimit rl;
    struct sigaction sa;

    // сбросить маску режима создания файла
    umask(0);  
    
    // получить максимально возможный номер дескриптора файла
    if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
        printf("%s: unable to get maximum descriptor number", cmd);
    
    // стать лидером новой сессии, чтобы утратить управляющий терминал
    if ((pid = fork()) < 0)
        printf("%s: fork error", cmd);
    else if (pid != 0)
        exit(0);
    setsid();
    
    // обеспечить невозможность обретения управляющего терминала в будущем
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        printf("%s: impossible to ignore the signal SIGHUP", cmd);
    if (chdir("/") < 0)
        printf("%s: impossible to make current working directory /", cmd);

    // закрыть все открытые файловые дескприпторы
    if (rl.rlim_max == RLIM_INFINITY)
        rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++)
        close(i);

    // присоединить файловые дескрипторы 0, 1, 2 к /dev/null
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    // инициализировать файл журнала
    openlog(cmd, LOG_CONS, LOG_DAEMON);
    if (fd0 != 0 || fd1 != 1 || fd2 != 2)
    {
        syslog(LOG_ERR, "Invalid file descriptors %d %d %d", fd0, fd1, fd2);
        exit(1);
    }
}


int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;

    return fcntl(fd, F_SETLK, &fl);
}


int already_running(void)
{
    int fd;
    char buf[16];

    fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0)
    {
        syslog(LOG_ERR, "Impossible to open %s: %s", LOCKFILE, strerror(errno));
        exit(1);
    }

    if (lockfile(fd) < 0)
    {
        syslog(LOG_INFO, "Impossible to lock %s: %s (already locked)", LOCKFILE, strerror(errno));
        
        if (errno == EACCES || errno == EAGAIN)
        {
            close(fd);
            return 1;
        }
        
        exit(1);
    }

    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);

    return 0;
}


void *thr_fn(void *arg)
{
    int err, signo;

    while (1)
    {
        err = sigwait(&mask, &signo);
        
        if (err != 0)
        {
            syslog(LOG_ERR, "Sigwait call failed");
            exit(1);
        }

        switch (signo)
        {
        case SIGHUP:
            syslog(LOG_INFO, "Reading configuration file, getlogin: %s", getlogin());
            break;
        case SIGTERM:
            syslog(LOG_INFO, "Got SIGTERM; exiting");
            remove(LOCKFILE);
            exit(0);
        default:
            syslog(LOG_INFO, "Got unexpected signal %d\n", signo);
        }
    }

    return (void *)0;
}

int main(int argc, char *argv[])
{
    int err;
    pthread_t tid;
    char *cmd;
    struct sigaction sa;

    if ((cmd = strrchr(argv[0], '/')) == NULL)
        cmd = argv[0];
    else
        cmd++;

    // перейти в режим демона
    daemonize(cmd);
    
    // убедиться, что ранее не было запущено другой копии демона
    if (already_running())
    {
        syslog(LOG_ERR, "Daemon is already running");
        exit(1);
    }

    // восстановить действие по умолчанию для сигнала SIGHUP  
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0)
        syslog(LOG_SYSLOG, "Impossible to restore action SIG_DFL for SIGHUP");
    sigfillset(&mask);
    if ((err = pthread_sigmask(SIG_BLOCK, &mask, NULL)) != 0)
        printf("SIG_BLOCK execution error");
    
    // создать поток, который будет заниматься обработкой сигналов SIGHUP и SIGTERM
    err = pthread_create(&tid, NULL, thr_fn, NULL);
    if (err != 0)
        syslog(LOG_ERR, "Impossible to create the thread");

    time_t raw_time;
    struct tm *timeinfo;
    
    while (1)
    {
        sleep(TIMEOUT);
        time(&raw_time);
        timeinfo = localtime(&raw_time);
        syslog(LOG_INFO, "Current date and time: %s", asctime(timeinfo));
    }
}