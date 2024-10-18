#include "asm/x86.h"
#include "types.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

extern struct proc *rq;
extern void set_runnable(struct proc *np);
extern void add_to_rq(struct proc *np);
extern void remove_from_rq(struct proc *p);

void rq_dump() {
  if (!rq) {
    cprintf("<<empty ready queue>>");
    return;
  }
  cprintf("{");
  struct proc *p = rq;
  while (p) {
    cprintf("%d", p->pid);
    p = p->next;
    if (p) {
      cprintf(" --> ");
    }
  }
    cprintf("}\n");

}

int
sys_setscheduler(void) {
  int pid, policy, priority;
  struct proc *p;
  if (argint(0, &pid) < 0 || argint(1, &policy) < 0 || argint(2, &priority) < 0)
    return -1;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      p->policy = policy;
      p->priority = priority;
      break;
    }
  }
  remove_from_rq(p);
  add_to_rq(p);
  yield();
  return pid;
}

int sys_clone(void) {
  return -1;
}
int sys_park(void) {
  return -1;
}
int sys_setpark(void) {
  return -1;
}
int sys_unpark(void) {
  return -1;
}
int sys_waitpid(void) {
  return -1;
}