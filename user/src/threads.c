#include "threads.h"
#include "atomics.h"
#include "user.h"

int thread_create(void *(*start_routine)(void *), void *arg) {
  return -1;
}

int thread_wait(int pid) {
  return -1;
}

int spinlock_init(struct spinlock* s) {
  return -1;
}

int spinlock_acquire(struct spinlock* s) {
  return -1;
}

int spinlock_release(struct spinlock* s) {
  return -1;
}

int mutex_init(struct mutex* m) {
  return -1;
}

int mutex_acquire(struct mutex* m) {
  return -1;
}

int mutex_release(struct mutex* m) {
  return -1;
}

int cond_init(struct condvar *cond) {
  return -1;
}

int cond_wait(struct condvar *cond, struct mutex *m) {
  return -1;
}

int cond_signal(struct condvar *cond) {
  return -1;
}
