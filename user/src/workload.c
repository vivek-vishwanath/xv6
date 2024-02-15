#include "fs.h"
#include "sched.h"
#include "stat.h"
#include "types.h"
#include "user.h"

void
mmult_workload (int num_processes)
{
  char matrix_size[16];
  char *mmult_args[] = { "mmul", matrix_size, 0 };

  for (int n = 0; n < num_processes; n++)
    {
      int pid = fork ();
      if (pid < 0)
        {
          printf (1, "fork failed\n");
          exit ();
        }
      if (pid == 0)
        {
          itoa (32 * n, (char *)&matrix_size);
          exec ("mmult", mmult_args);
          exit ();
        }
    }

  for (int n = 0; n < num_processes; n++)
    {
      wait ();
    }
}

int
main (int argc, char *argv[])
{
  uint start_time, end_time;

  /********** FIFO SCHEDULER **********/
  printf (1, "FIFO\n");
  // remove comment once setscheduler is implemented
  // setscheduler (getpid (), SCHED_FIFO, 0);

  start_time = uptime ();
  mmult_workload (16);
  end_time = uptime ();

  printf (1, "FIFO total time ticks: %d\n", end_time - start_time);

  /********** ROUND ROBIN SCHEDULER **********/
  printf (1, "ROUND ROBIN\n");

  // remove comment once setscheduler is implemented
  // setscheduler (getpid (), SCHED_FIFO, 0);

  start_time = uptime ();
  mmult_workload (16);
  end_time = uptime ();

  printf (1, "RR total time ticks: %d\n", end_time - start_time);

  exit ();
}