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
  struct proc *p = myproc();
  acquire(&p->tgo->lk);
  addr = p->tgo->sz;
  release(&p->tgo->lk);
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

extern int preempt(struct proc *op, struct proc *np);
extern void set_runnable(struct proc *p);
extern void add_to_rq(struct proc *np);
extern void remove_from_rq(struct proc *p);

int sys_setscheduler(void) {
  int pid, policy, priority;
  struct proc *curproc = myproc();
  struct proc *p;
  if (argint(0, &pid) < 0 || argint(1, &policy) < 0 || argint(2, &priority) < 0)
    return -1;
  if (policy != SCHED_FIFO && policy != SCHED_RR && policy != SCHED_OTHER) return -1;
  if (priority < 0) return -1;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      if (p->state == ZOMBIE || p->state == UNUSED) return -1;
      if (p != curproc && p->parent != curproc) return -1;
      p->policy = policy;
      p->priority = priority;
      if (p->state == RUNNABLE && preempt(myproc(), p)) {
        remove_from_rq(p);
        add_to_rq(p);
        yield();
      }
      return pid;
    }
  }
  return -1;
}

int sys_clone(void) {
  void *stack;
  int stack_sz;
  if (argint(1, &stack_sz) < 0 || argptr(0, (char **) &stack, stack_sz) < 0) {
    return -1;
  }
  return clone(stack, stack_sz);
}

int sys_park(void) {
  void *chan;
  if (argptr(0, (char **) &chan, sizeof(void *)) < 0)
    return -1;
  struct proc *p = myproc();
  acquire(&ptable.lock);
  // cprintf("park %d\n", myproc()->pid);
  if (p->park == WONT_PARK)
    p->park = WILL_PARK;
  else
    sleep(chan, &ptable.lock);
  release(&ptable.lock);
  return 0;
}

int sys_setpark(void) {
  void *chan;
  if (argptr(0, (char **) &chan, sizeof(void *)) < 0)
    return -1;
  struct proc *p = myproc();
  acquire(&ptable.lock);
  // cprintf("setpark %d\n", myproc()->pid);
  p->park = WILL_PARK;
  p->chan = chan;
  release(&ptable.lock);
  return 0;
}

int sys_unpark(void) {
  void *chan;
  if (argptr(0, (char **) &chan, sizeof(void *)) < 0)
    return -1;
  struct proc *p = myproc();
  acquire(&ptable.lock);
  p->park = DRIVING;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->chan == chan) {
      if (p->state == SLEEPING) {
        set_runnable(p);
        release(&ptable.lock);
        return 1;
      }
      if (p->park == WILL_PARK) {
        p->park = WONT_PARK;
        release(&ptable.lock);
        return 1;
      }
    }
  }
  release(&ptable.lock);
  return 0;
}

int sys_waitpid(void) {
  int pid;
  if (argint(0, &pid) < 0) {
    return -1;
  }
  return waitpid(pid);
}

int sys_waitinfo(void) {
  struct schedinfo *info;
  if (argptr(0, (char **) &info, sizeof(struct schedinfo *)) < 0) {
    return -1;
  }

  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        if (p->tgo == p) {
          acquire(&p->tgo->lk);
          freevm(p->tgo->pgdir);
          release(&p->tgo->lk);
        }
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        *info = p->info;
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}