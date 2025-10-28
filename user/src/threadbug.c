#include "threads.h"

void print(char *arg) {
  int x = 0;
  while (x++ < 0x7F000000) {}
  printf(1, "\nHello %s\n", arg);
}

int main(void) {
  int pid;
  printf(1, "\nStarting process");
  printf(1, "\n\tPreparing to launch thread:");
  pid = thread_create((void * (*)(void *))print, "World");
  printf(1, "\n\tFinished launching thread with pid: %d", pid);
  wait();
  printf(1, "\n\tEnding thread with pid: %d", pid);
  printf(1, "\nEnding process\n");
  exit();
}