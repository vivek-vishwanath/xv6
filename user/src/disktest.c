#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

int main() {
    int fd = open("sample.txt", O_CREATE | PROT_W);
    int pid = fork();
    if (pid == 0) {
        setuid(1);
        char *text = "Hello, xv6! :(\n";
        uint len = strlen(text);
        printf(1, "About to write:\n");
        int w = write(fd, text, len);
        printf(1, "*Result of write = %d\n", w);
        close(fd);
        exit();
    }
    wait();

    char *text = "Hello, xv6 from %! :)\n";
    text[16] = (char) pid + '0';
    uint len = strlen(text);
    printf(1, "&Result of write = %d\n", write(fd, text, len));
    close(fd);

    chmod("sample.txt", PROT_R | PROT_W);
    exit();
}
