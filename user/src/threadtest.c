#include "threads.h"

struct mutex *lock;

uint execute(char *arg) {
    mutex_acquire(lock);
    printf(1, "<%s --", arg);
    uint x = 0;
    uint z = 2;
    while (x++ < 0x800000) {
        z++;
        z %= 0xABCD;
    }
    printf(1, "-- %s", arg);
    printf(1, ">\n");
    mutex_release(lock);
    return z;
}

int main() {
    int start = uptime();
    printf(1, "Hello, threadtest!\n");
    lock = malloc(sizeof(struct mutex));
    mutex_init(lock);
    for (char c = 'A'; c <= 'z'; c++) {
        char *d = malloc(2);
        d[0] = c;
        d[1] = '\0';
        thread_create((void * (*)(void *)) execute, d);
    }
    while (wait() >= 0) {}
    int stop = uptime();
    printf(1, "elapsed time = %d - %d = %d; global = 0x%x\n", stop, start, stop - start);
    exit();
}
