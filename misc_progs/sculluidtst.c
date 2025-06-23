#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>

char buffer[4096];

int main(int argc, char *argv[])
{
    int count = 0;

    int fd = open("/dev/scull_uid", O_RDWR);
    printf("Am i running ?\n");

    while (1)
    {
        printf("Begin reading ...\n");
        count = read(fd, buffer, 4096);
        write(0, buffer, count);
        sleep(60);
    }

    close(fd);

    return 0;
}
