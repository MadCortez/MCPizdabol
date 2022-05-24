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

struct HandlerCxt 
{
    pid_t pidlist[8];
    pid_t pid;
    pid_t ppid; 
};

void proc1();
void proc2();
void proc3();
void proc4();
void proc5();
void proc6();
void proc7();
void proc8();
void runChild(void (*cb)());
pid_t *readPids();

void writePid();
void readPidlist(pid_t *pidlist);

void proc1SigHandler(int signum);
void proc2SigHandler(int signum);
void proc3SigHandler(int signum);
void proc4SigHandler(int signum);
void proc5SigHandler(int signum);
void proc6SigHandler(int signum);
void proc7SigHandler(int signum);
void proc8SigHandler(int signum);

char *name = NULL;
int fd = -1;

int main(int argc, char **argv)
{
    name = argv[0];
    assert(name);
    fd = open("pid_list.txt", O_RDWR | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    if (fd == -1)
        error(EXIT_FAILURE, errno, "%u:%s: can't open file", getpid(), name);

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

    printf("1 %u %u\n", getpid(), getppid());

    runChild(proc2);
    runChild(proc3);

    struct sigaction act = 
    {
        .sa_handler = proc1SigHandler();
    };
    if (sigaction(SIGUSR2, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    if (kill(pidlist[1], SIGUSR2) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: kill", getpid(), name);

    wait(NULL);
    wait(NULL);

    exit(EXIT_SUCCESS);
}

void proc1SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("1 %u %u RECEIVED SIGUSR2\n", cxt->pid, cxt->ppid);
    printf("1 %u %u SENT SIGUSR2\n", cxt->pid, cxt->ppid);
    fflush(stdout);

    kill(cxt->pidlist[1], SIGUSR2);
}

void proc2()
{
    writePid(2);

    printf("2 %u %u\n", getpid(), getppid());

    struct sigaction act = 
    {
        .sa_handler = proc2SigHandler();
    };
    if (sigaction(SIGUSR2, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    exit(EXIT_SUCCESS);
}

void proc2SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("2 %u %u RECEIVED SIGUSR2\n", cxt->pid, cxt->ppid);
    printf("2 %u %u SENT SIGUSR1\n", cxt->pid, cxt->ppid);
    fflush(stdout);

    kill(cxt->pidlist[2], SIGUSR1);
    kill(cxt->pidlist[3], SIGUSR1);
    kill(cxt->pidlist[4], SIGUSR1);
}

void proc3()
{
    writePid(3);

    printf("3 %u %u\n", getpid(), getppid());

    runChild(proc4);

    struct sigaction act = 
    {
        .sa_handler = proc3SigHandler();
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    if (kill(pidlist[1], SIGUSR2) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: kill", getpid(), name);

    wait(NULL);

    exit(EXIT_SUCCESS);
}

void proc3SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("3 %u %u RECEIVED SIGUSR1\n", cxt->pid, cxt->ppid);
    printf("3 %u %u SENT SIGUSR1\n", cxt->pid, cxt->ppid);
    fflush(stdout);

    kill(cxt->pidlist[6], SIGUSR1);
}

void proc4()
{
    writePid(4);

    printf("4 %u %u\n", getpid(), getppid());

    runChild(proc5);
    runChild(proc6);

    struct sigaction act = 
    {
        .sa_handler = proc4SigHandler();
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    wait(NULL); 
    wait(NULL);

    exit(EXIT_SUCCESS);
}

void proc4SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("4 %u %u RECEIVED SIGUSR1\n", cxt->pid, cxt->ppid);
    printf("4 %u %u SENT SIGUSR1\n", cxt->pid, cxt->ppid);
    fflush(stdout);

    kill(cxt->pidlist[5], SIGUSR1);
}

void proc5()
{
    writePid(5);

    printf("5 %u %u\n", getpid(), getppid());

    struct sigaction act = 
    {
        .sa_handler = proc5SigHandler();
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    exit(EXIT_SUCCESS);
}

void proc5SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("5 %u %u RECEIVED SIGUSR1\n", cxt->pid, cxt->ppid);
    printf("5 %u %u SENT SIGUSR1\n", cxt->pid, cxt->ppid);
    fflush(stdout);

    kill(cxt->pidlist[7], SIGUSR1);
}

void proc6()
{
    writePid(6);

    printf("6 %u %u\n", getpid(), getppid());

    runChild(proc7);

    struct sigaction act = 
    {
        .sa_handler = proc6SigHandler();
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    wait(NULL);

    exit(EXIT_SUCCESS);
}

void proc6SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("6 %u %u RECEIVED SIGUSR1\n", cxt->pid, cxt->ppid);
    fflush(stdout);
}

void proc7()
{
    writePid(7);

    printf("7 %u %u\n", getpid(), getppid());

    runChild(proc8);

    struct sigaction act = 
    {
        .sa_handler = proc7SigHandler();
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    wait(NULL);

    exit(EXIT_SUCCESS);
}

void proc7SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("7 %u %u RECEIVED SIGUSR1\n", cxt->pid, cxt->ppid);
    fflush(stdout);
}

void proc8()
{
    writePid(8);

    printf("8 %u %u\n", getpid(), getppid());

    struct sigaction act = 
    {
        .sa_handler = proc8SigHandler();
    };
    if (sigaction(SIGUSR1, &act, NULL) == -1)
        error(EXIT_FAILURE, errno, "%u:%s: sigaction", getpid(), name);

    pid_t pidlist[8] = {};
    readPidlist(pidlist);

    exit(EXIT_SUCCESS); 
}

void proc8SigHandler(int signum, siginfo_t *info, void *data)
{
    assert(data);
    struct HandlerCxt *cxt = (struct HandlerCxt *) data;

    printf("8 %u %u RECEIVED SIGUSR1\n", cxt->pid, cxt->ppid);
    printf("8 %u %u SENT SIGUSR2\n", cxt->pid, cxt->ppid);
    fflush(stdout);

    kill(cxt->pidlist[0], SIGUSR2);
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
