#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>

int main(const int argc, const char* argv[])
{
    char bytes[2] = {11, 0};

    if (argc == 2)
    {
        bytes[1] = atoi(argv[1]);
        printf("test2222\n");
    }
    else
    {
        fprintf(stderr, "number of arguments should be 2.");
        printf("test111\n");
        exit(1);
    }
    if (ioctl(STDIN_FILENO, TIOCLINUX, bytes) < 0)
    {
        fprintf(stderr, "%s: ioctl(stdin, TIOCLINEX) : %s",
            argv[0], strerror(errno));
        exit(1);
    }
    exit(0);
}