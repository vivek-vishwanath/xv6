#include <sched.h>
#include <user.h>

#define N 11

void schedtest(void) {
  printf(1, "sched test\n");
  int policies[] = {SCHED_RR, SCHED_RR, SCHED_FIFO, SCHED_FIFO, SCHED_RR, SCHED_FIFO, SCHED_FIFO, SCHED_FIFO, SCHED_RR, SCHED_RR, SCHED_RR};
  int priorities[] = {1, 1, 1, 2, 2, 3, 3, 1, 4, 4, 1};

  for (int j = 3; j < 100; j += N+1)
  setscheduler(j, SCHED_FIFO, 9);

  int n, pid, start = uptime();

  for (n = 0; n < N; n++) {
    pid = fork();
    if (pid < 0) break;
    if (pid) setscheduler(pid, policies[n], priorities[n]);
    else {
      int x = 1;
      while (x++ < 0x10000000) {}
      printf(1, "exit#%d\n", n + 1);
      exit();
    }
  }

  while (wait() >= 0) {}

  int end = uptime();

  printf(1, "elapsed time = %d ticks\n", end - start);

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
