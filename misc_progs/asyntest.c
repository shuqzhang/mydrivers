#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#undef __USE_MISC // fix for conflict of types in different included headers
#include <signal.h>
#include <fcntl.h>

# define FASYNC		O_ASYNC // missing __USE_MISC macro which controls the definition of FASYNC

int gotdata = 0;
char buffer[4096];

void input_handler(int signo)
{
    if (signo == SIGIO)
    {
        gotdata++;
    }
}

int main(int argc, char *argv[])
{
    unsigned int oflags = 0;
    int count = 0;
    struct sigaction action;

    int fd = open("/dev/scull_p", O_RDWR);

    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = input_handler;
    action.sa_flags = 0;

    sigaction(SIGIO, &action, NULL);
    fcntl(fd, F_SETOWN, getpid());
    oflags = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, oflags | FASYNC);

    while (1)
    {
        sleep(20);
        if (!gotdata)
        {
            continue;
        }
        count = read(fd, buffer, 4096);
        write(0, buffer, count);
        gotdata = 0;
    }

    return 0;
}
