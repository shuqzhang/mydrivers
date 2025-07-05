#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>

char buffer[4096];

void print(const char* buf)
{
    write(0, buf, strlen(buf));
}

int main(int argc, char *argv[])
{
    int count = 0;

    int fd = open("/dev/scull_uid", O_RDWR);
    print("Am i running ?\n");

    while (1)
    {
        print("Begin reading ...\n");
        count = read(fd, buffer, 4096);
        write(0, buffer, count);
        sleep(20);
    }

    close(fd);

    return 0;
}
