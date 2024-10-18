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

void
forktest(void)
{
  int n, pid;

  printf(1, "fork test\n");

  int policies[] = {0, 0, SCHED_FIFO, SCHED_RR, SCHED_RR, SCHED_FIFO, SCHED_FIFO, SCHED_RR};
  int priorities[] = {0, 0, 9, 1, 1, 1, 2, 2};

  setscheduler(3, SCHED_FIFO, 9);

  for(n=0; n<N; n++){
    pid = fork();
    if (pid)
      setscheduler(pid, policies[pid-1], priorities[pid-1]);
    // if (pid == 7) setscheduler(7, SCHED_FIFO, 2);
    // if (pid == 5) setscheduler(5, SCHED_FIFO, 1);
    // if (pid == 6) setscheduler(6, SCHED_FIFO, 1);
    if(pid < 0)
      break;
    if(pid == 0) {
      int x = 1;
      while (x < 0x10000000) x++;
      exit();
    }
  }

  if(n == N){
    printf(1, "fork claimed to work %d times!\n", N);
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

  printf(1, "fork test OK\n");
}

int
main(void)
{
  forktest();
  exit();
}
