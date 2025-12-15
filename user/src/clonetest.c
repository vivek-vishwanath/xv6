// Test that fork fails gracefully.
// Tiny executable so that the limit can be filling the proc table.

#include <sched.h>

#include "types.h"
#include "stat.h"
#include "user.h"

#define N  5

//void
//printf(int fd, const char *s, ...)
//{
//  write(fd, s, strlen(s));
//}

int global = 0;

void
clonetest(void)
{
    int n, pid;

    int pre_clone = 0;

    printf(1, "clone test\n");
    for(n=0; n<N; n++){
        printf(1, "Cloning thread $%d\n", n);
        pid = clone(malloc(0x1000), 0x1000);
        if(pid < 0) {
            printf(1, "clone() failed, %d\n", pid);
            break;
        }
        if(pid == 0) {
            if (n != 2)
                setscheduler(6, SCHED_FIFO, 1);
            int post_clone = 0;
            while (post_clone < 0x100000) {
                post_clone++;
                pre_clone++;
                global++;
            }
            printf(1, "%d returned\n", n+1);
            printf(1, "child thread #%d returned\tpre-clone: 0x%x;\tpost-clone: 0x%x;\tglobal: 0x%x\n", n+1, pre_clone, post_clone, global);
            exit();
        }
        // if (pid == 7) {
        //   setscheduler(6, SCHED_FIFO, 1);
        // }
        printf(1, "parent returned\t\t\tpre-clone: 0x%x;\t\t\t\t\tglobal: 0x%x\n", pre_clone, global);
    }

    while(wait() >= 0);

    if(n == N){
        printf(1, "clone claimed to work %d times!\n", N);
        exit();
    }

    for(; n > 0; n--){
        if(wait() < 0){
            printf(1, "wait stopped early\n");
            exit();
        }
    }

    if(wait() != -1){
        printf(1, "wait got too many\n");
        exit();
    }

    printf(1, "clone test OK\n");
}

void ctest(void) {
    int x = 0x123456;
    int y[16];
    (void) x;
    (void) y;
    clonetest();
}

int
main(void)
{
    ctest();
    exit();
}