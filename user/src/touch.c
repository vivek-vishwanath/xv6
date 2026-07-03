#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

int len(const char *str) {
    const char *s = str;
    while (*++s)
        ;
    return s - str;
}

int
main(int argc, char *argv[]) {
    int fd;
    if (argc <= 1) {
        printf(1, "Specify a file name");
    } else if (argc < 4) {
        fd = open(argv[1], O_CREATE | O_WRONLY);
        if (fd < 0)
            printf(1, "touch: cannot open %s\n", argv[1]);
        if (argc == 3) {
            int length = len(argv[2]);
            int n = 0;
            if ((n = write(fd, argv[2], length)) != length)
                printf(1, "touch: write error %d, != %d\n", length, n);
        }
    } else {
        printf(1, "Usage: touch <file> [\"optional text\"]\n");
    }
    exit();
}
