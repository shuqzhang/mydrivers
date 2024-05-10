#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
/* #include <unistd.h> */ /* conflicting on the alpha */
#define __LIBRARY__ /* _syscall3 and friends are only available through this */
#include <linux/unistd.h>
#include <sys/klog.h>

int main(int argc, char** argv)
{
    int level = 8;
    if (argc == 2)
    {
        level = atoi(argv[1]);
    }
    else
    {
        fprintf(stderr, "%s need a single argument!\n", argv[0]);
    }
    if (klogctl(8, NULL, level))
    {
        fprintf(stderr, "%s :syslog(setlevel) failed due to %s",
            argv[0], strerror(errno));
        exit(1);
    }
    exit(0);
}