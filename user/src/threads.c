#include "threads.h"

#include <mmu.h>
#include "free_stack_and_exit.h"

#include "atomics.h"
#include "user.h"

int spinlock_id = 0;
int mutex_id = 0;
int condvar_id = 0;

int thread_create(void *(*start_routine)(void *), void *arg) {
  void *stack = malloc(PGSIZE);
  if (stack == 0) return -1;
  int pid = clone(stack, PGSIZE);
  if (pid < 0) {
    free(stack);
    return -1;
  }
  if (pid == 0) {
    start_routine(arg);
    free_stack_and_exit(stack);
  }
  return pid;
}

int thread_wait(int pid) {
  if (waitpid(pid) < 0) return -1;
  return pid;
}

int spinlock_init(struct spinlock* s) {
  if (!s) return -1;
  s->id = ++spinlock_id;
  s->locked = 0;
  return 0;
}

int spinlock_acquire(struct spinlock* s) {
  if (!s || !s->id) return -1;
  while (atomic_exchange_explicit(&s->locked, 1, memory_order_acq_rel))
    ;
  return 0;
}

int spinlock_release(struct spinlock* s) {
  if (!s || !s->id) return -1;
  atomic_store_explicit(&s->locked, 0, memory_order_release);
  return 0;
}

int mutex_init(struct mutex* m) {
  if (!m) return -1;
  m->id = ++mutex_id;
  m->locked = 0;
  return 0;
}

int mutex_acquire(struct mutex* m) {
  if (!m || !m->id) return -1;
  setpark(m);
  while (atomic_exchange_explicit(&m->locked, 1, memory_order_acq_rel)) {
    park(m);
  }
  return 0;
}

int mutex_release(struct mutex* m) {
  if (!m || !m->id || !m->locked) return -1;
  atomic_store_explicit(&m->locked, 0, memory_order_release);
  unpark(m);
  return 0;
}

int cond_init(struct condvar *cond) {
  if (!cond) return -1;
  cond->id = ++condvar_id;
  return 0;
}

int cond_wait(struct condvar *cond, struct mutex *m) {
  if (!cond || !cond->id || !m || !m->id || !m->locked) return -1;
  setpark(cond);
  mutex_release(m);
  park(cond);
  mutex_acquire(m);
  return 0;
}

int cond_signal(struct condvar *cond) {
  if (!cond || !cond->id) return -1;
  unpark(cond);
  return 0;
}