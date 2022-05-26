#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <fcntl.h>

#define STRSIZE 0x10


void proc1();
void proc2();
void proc3();
void proc4();
void proc5();
void proc6();
void proc7();
void proc8();
void runChild(void (*cb)());

void proc1SigHandler(int signum, siginfo_t *info, void *data);
void proc2SigHandler(int signum, siginfo_t *info, void *data);
void proc3SigHandler(int signum, siginfo_t *info, void *data);
void proc4SigHandler(int signum, siginfo_t *info, void *data);
void proc5SigHandler(int signum, siginfo_t *info, void *data);
void proc6SigHandler(int signum, siginfo_t *info, void *data);
void proc7SigHandler(int signum, siginfo_t *info, void *data);
void proc8SigHandler(int signum, siginfo_t *info, void *data);

void writePid();
void readPidlist(pid_t *pidlist);

struct HandlerCxt 
{
    pid_t pidlist[8];
    pid_t pid;
    pid_t ppid; 
} cxt;

size_t recv_counter;
size_t send_counter;
char *name = NULL;
int fd = -1;
sigset_t set;

int main(int argc, char **argv)
{
    name = argv[0];
    assert(name);
    fd = open("pid_list.txt", O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    if (fd == -1)
        error(EXIT_FAILURE, errno, "%u:%s: can't open file", getpid(), name);

    sigset_t old;
    if (sigemptyset(&set) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigemptyset", getpid(), name);
    if (sigaddset(&set, SIGUSR1) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaddset", getpid(), name);
    if (sigaddset(&set, SIGUSR2) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaddset", getpid(), name);

    if (sigprocmask(SIG_SETMASK, &set, &old) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    set = old;

    runChild(proc1); 

    wait(NULL);

    exit(EXIT_SUCCESS);
}

void runChild(void (*cb)())
{
    assert(cb);

    int ret = fork();
    if (!ret)
        cb();
    if (ret < 0)
        error(EXIT_FAILURE, errno, "%u:%s: fork failed\n", getpid(), name); 
}

void proc1()
{
    writePid(1);

    runChild(proc2);
    runChild(proc3);

    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc1SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR2, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("1 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (kill(cxt.pidlist[1], SIGUSR2) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: kill", getpid(), name);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS);
}

void proc1SigHandler(int signum, siginfo_t *info, void *data)
{
    recv_counter++;

    if (recv_counter == 101)
    {
        kill(cxt.pidlist[1], SIGTERM);
        kill(cxt.pidlist[2], SIGTERM);
        kill(cxt.pidlist[3], SIGTERM);
        kill(cxt.pidlist[4], SIGTERM);
        kill(cxt.pidlist[5], SIGTERM);
        kill(cxt.pidlist[6], SIGTERM);
        kill(cxt.pidlist[7], SIGTERM);

        wait(NULL);
        wait(NULL);

        printf("1 %u %u FINISHED WORK AFTER 0 SIGUSR1 AND %lu SIGUSR2 SENT\n", cxt.pid, cxt.ppid, send_counter);

        exit(EXIT_SUCCESS);
    }

    printf("1 %u %u RECEIVED SIGUSR2 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);

    kill(cxt.pidlist[1], SIGUSR2);
    printf("1 %u %u SENT SIGUSR2 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[1]);
    fflush(stdout);

    send_counter++;
}

void proc2()
{
    writePid(2);

    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc2SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR2, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);
    if (sigaction(SIGTERM, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("2 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS);
}

void proc2SigHandler(int signum, siginfo_t *info, void *data)
{
    if (signum == SIGTERM)
    {
        printf("2 %u %u FINISHED WORK AFTER %lu SIGUSR1 AND 0 SIGUSR2 SENT\n", cxt.pid, cxt.ppid, send_counter);
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    printf("2 %u %u RECEIVED SIGUSR2 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);

    kill(cxt.pidlist[2], SIGUSR1);
    printf("2 %u %u SENT SIGUSR1 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[2]);
    fflush(stdout);

    kill(cxt.pidlist[3], SIGUSR1);
    printf("2 %u %u SENT SIGUSR1 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[3]);
    fflush(stdout);

    kill(cxt.pidlist[4], SIGUSR1);
    printf("2 %u %u SENT SIGUSR1 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[4]);
    fflush(stdout);

    send_counter += 3;
}

void proc3()
{
    writePid(3);

    runChild(proc4);

    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc3SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);
    if (sigaction(SIGTERM, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("3 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS);
}

void proc3SigHandler(int signum, siginfo_t *info, void *data)
{
    if (signum == SIGTERM)
    {
        wait(NULL);

        printf("3 %u %u FINISHED WORK AFTER %lu SIGUSR1 AND 0 SIGUSR2 SENT (!@$ LOSS COULD OCCUR $@!)\n", 
               cxt.pid, cxt.ppid, send_counter);
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    printf("3 %u %u RECEIVED SIGUSR1 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);

    kill(cxt.pidlist[6], SIGUSR1);
    printf("3 %u %u SENT SIGUSR1 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[6]);
    fflush(stdout);

    send_counter++;
}

void proc4()
{
    writePid(4);

    runChild(proc5);
    runChild(proc6);

    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc4SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);
    if (sigaction(SIGTERM, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("4 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS);
}

void proc4SigHandler(int signum, siginfo_t *info, void *data)
{
    if (signum == SIGTERM)
    {
        wait(NULL);
        wait(NULL);

        printf("4 %u %u FINISHED WORK AFTER %lu SIGUSR1 AND 0 SIGUSR2 SENT (!@$ LOSS COULD OCCUR $@!)\n", 
               cxt.pid, cxt.ppid, send_counter);
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    printf("4 %u %u RECEIVED SIGUSR1 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);

    kill(cxt.pidlist[5], SIGUSR1);
    printf("4 %u %u SENT SIGUSR1 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[5]);
    fflush(stdout);

    send_counter++;
}

void proc5()
{
    writePid(5);


    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc5SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);
    if (sigaction(SIGTERM, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("5 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS);
}

void proc5SigHandler(int signum, siginfo_t *info, void *data)
{
    if (signum == SIGTERM)
    {
        printf("5 %u %u FINISHED WORK AFTER %lu SIGUSR1 AND 0 SIGUSR2 SENT\n", cxt.pid, cxt.ppid, send_counter);
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    printf("5 %u %u RECEIVED SIGUSR1 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);

    kill(cxt.pidlist[7], SIGUSR1);
    printf("5 %u %u SENT SIGUSR1 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[7]);
    fflush(stdout);

    send_counter++;
}

void proc6()
{
    writePid(6);

    runChild(proc7);

    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc6SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);
    if (sigaction(SIGTERM, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("6 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS);
}

void proc6SigHandler(int signum, siginfo_t *info, void *data)
{
    if (signum == SIGTERM)
    {
        wait(NULL);

        printf("6 %u %u FINISHED WORK AFTER 0 SIGUSR1 AND 0 SIGUSR2 SENT\n", cxt.pid, cxt.ppid);
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    printf("6 %u %u RECEIVED SIGUSR1 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);
}

void proc7()
{
    writePid(7);

    runChild(proc8);

    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc7SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);
    if (sigaction(SIGTERM, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("7 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS);
}

void proc7SigHandler(int signum, siginfo_t *info, void *data)
{
    if (signum == SIGTERM)
    {
        wait(NULL);

        printf("7 %u %u FINISHED WORK AFTER 0 SIGUSR1 AND 0 SIGUSR2 SENT\n", cxt.pid, cxt.ppid);
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    printf("7 %u %u RECEIVED SIGUSR1 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);
}

void proc8()
{
    writePid(8);

    cxt.pid = getpid();
    cxt.ppid = getppid();

    struct sigaction act = 
    {
        .sa_sigaction = proc8SigHandler,
        .sa_flags = SA_SIGINFO,
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);
    if (sigaction(SIGTERM, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    readPidlist(cxt.pidlist);

    printf("8 %u %u\n", getpid(), getppid());
    fflush(stdout);

    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigprocmask", getpid(), name);

    while(1);

    exit(EXIT_SUCCESS); 
}

void proc8SigHandler(int signum, siginfo_t *info, void *data)
{
    if (signum == SIGTERM)
    {
        printf("8 %u %u FINISHED WORK AFTER 0 SIGUSR1 AND %lu SIGUSR2 SENT\n", cxt.pid, cxt.ppid, send_counter);
        fflush(stdout);

        exit(EXIT_SUCCESS);
    }

    printf("8 %u %u RECEIVED SIGUSR1 FROM %u\n", cxt.pid, cxt.ppid, info->si_pid);
    fflush(stdout);

    kill(cxt.pidlist[0], SIGUSR2);
    printf("8 %u %u SENT SIGUSR2 TO %u\n", cxt.pid, cxt.ppid, cxt.pidlist[0]);
    fflush(stdout);

    send_counter++;
}

void writePid(unsigned number)
{
    pid_t pid = getpid();
    char str[STRSIZE];

    snprintf(str, STRSIZE, "%10u:%u\n", pid, number);
    if (write(fd, str, STRSIZE) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: write", pid, name);
}

void readPidlist(pid_t *pidlist)
{
    assert(pidlist);

    pid_t mypid = getpid();

    int i = 0;

    /*
           /|\
          /   \
          | - |
          |   |
          |   |
          |   |
       /---------\
       |    |    |
       |---------|
     */

    while (i < 8)
    { 
        char str[STRSIZE];

        if (pread(fd, str, STRSIZE, STRSIZE * i) == -1)
            error(EXIT_FAILURE, errno, "%u:%s: read", mypid, name);

        unsigned number;
        pid_t pid;
        if (sscanf(str, "%u:%u", &pid, &number) == 2)
        {
            assert(number <= 8);

            if (pidlist[number-1] == pid)
                continue;
            
            pidlist[number-1] = pid;
            i++;
        }
    }
}
