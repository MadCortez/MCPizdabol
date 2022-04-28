#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <ctype.h>
#include <error.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#ifdef DEBUG

    FILE *debug_log;

    #define LOG(...) fprintf(debug_log, __VA_ARGS__)

#else

    #define LOG(...) 

#endif

struct processDirCxt 
{
    size_t chldCount;
    size_t chldLimit;
    pid_t prntPid;
};

void processDir(const char *prevDir, const char *newDir, struct processDirCxt *cxt);
void runChild(const char *dirName, const char *fname);

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <path> <processes_limit>\n", 
                        argv[0]);
        exit(EXIT_FAILURE); 
    }

    char *endptr;
    size_t chldLimit = (size_t) strtol(argv[2], &endptr, 10) - 1;
    if ( (long) chldLimit <= 0 ||  errno == ERANGE || errno == EINVAL || *endptr != '\0')
    {
        fprintf(stderr, "Bad processes limit\n");
        exit(EXIT_FAILURE);
    }

#ifdef DEBUG

    debug_log = fopen("/tmp/dir-scanner.log", "w");
    assert(!setvbuf(debug_log, NULL, _IONBF, 0));
    assert(debug_log);

#endif

    setvbuf(debug_log, NULL, _IONBF, 0);

    char realPath[PATH_MAX];
    if(!realpath(argv[1], realPath))
    {
        fprintf(stderr, "Bad path\n");
        exit(EXIT_FAILURE);
    }

    struct processDirCxt cxt =
    {
        .chldLimit = chldLimit,
        .prntPid = getpid(),
    };

    processDir(realPath, realPath, &cxt);

    errno = 0;
    while (errno != ECHILD)
    {
        wait(NULL);
        if (errno && errno != ECHILD)
            error(EXIT_FAILURE, errno, "wait");
    }

    exit(EXIT_SUCCESS);
}

void processDir(const char *prevDir, const char *newDir, struct processDirCxt *cxt)
{
    assert(prevDir);
    assert(newDir);
    assert(cxt);

    LOG("!#$ CALL processDir $#!\n");

    DIR *dir = opendir(newDir);
    if (!dir)
    {
        fprintf(stderr, "[parrent:%u]: Can't open directory %s/%s: %s\n", 
                        cxt->prntPid, prevDir, newDir, strerror(errno));
        return;
    }

    if (chdir(newDir) == -1)
    {
        fprintf(stderr, "[parrent:%u]: Can't join directory %s/%s: %s\n",
                        cxt->prntPid, prevDir, newDir, strerror(errno));
        return;
    }

    char curDir[PATH_MAX];
    if (!getcwd(curDir, PATH_MAX))
    {
        fprintf(stderr, "[parrent:%u]: Can't get working directory: %s\n", 
                        cxt->prntPid, strerror(errno));
        return;
    }

    struct dirent *dirent = NULL;
    do
    {
        errno = 0;
        dirent = readdir(dir);
        if (errno)
            error(EXIT_FAILURE, errno, "readdir");

        if (!dirent)
            continue;

        LOG("processing %s/%s...\n", curDir, dirent->d_name);

        if (dirent->d_type == DT_DIR && 
            strcmp(dirent->d_name, "..") &&
            strcmp(dirent->d_name, "."))
        {
            processDir(curDir, dirent->d_name, cxt);
            continue;
        }

        if (dirent->d_type != DT_REG)
            continue;

        if (cxt->chldCount == cxt->chldLimit)
        {
            if (wait(NULL) == -1)
                error(EXIT_FAILURE, errno, "wait"); 
            cxt->chldCount--;
        }

        switch(fork())
        {
            case 0:
                runChild(curDir, dirent->d_name);
                exit(EXIT_SUCCESS);
                break;
            case -1:
                error(EXIT_FAILURE, errno, "fork");
                break;
            default:
                cxt->chldCount++;
        }
    }
    while (dirent);

    closedir(dir);

    if (chdir(prevDir) == -1)
        error(EXIT_FAILURE, errno, "[parrent:%u]: Can't join directory %s: %s\n",
              cxt->prntPid, prevDir, strerror(errno));

    LOG("!#$ RET processDir $#!\n");
}

void runChild(const char *dirname, const char *fname)
{
    assert(fname);
    assert(dirname);

    pid_t pid = getpid();

    char name[PATH_MAX];
    snprintf(name, 256, "%s/%s", dirname, fname);

    FILE *stream = fopen(name, "r"); 
    if (!stream)
    {
        fprintf(stderr, "[child:%u]: Can't open file %s: %s\n", 
                        pid, name, strerror(errno) );
        exit(EXIT_FAILURE);
    }

    int symb = 0;
#define STATE_INWORD 1
#define STATE_OUTWORD 0
    int state = STATE_OUTWORD;
    size_t wordCount = 0;
    size_t byteCount = 0;
    while ((symb = getc(stream)) != EOF)
    {
        byteCount++;

        if (!isascii(symb))
            continue;

        if (isspace(symb))
        {
            if (state == STATE_INWORD)
                state = STATE_OUTWORD;
        }
        else
        {
            if (state == STATE_OUTWORD)
            {
                wordCount++;
                state = STATE_INWORD;
            }
        }
    }
#undef STATE_INWORD
#undef STATE_OUTWORD

    fclose(stream);

    printf("[child:%u]: File %s contains %lu bytes and %lu words\n",
           pid, name, byteCount, wordCount);
}
