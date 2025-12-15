#include "asm/x86.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"

struct ptable_struct ptable;

static struct proc *initproc;
struct proc *rq;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);
extern int sys_uptime(void);

extern void wakeup1(void *chan);

int preempt(struct proc *op, struct proc *np) {
  if (op->policy == np->policy) {
    if (np->policy == SCHED_OTHER) return np->vruntime < op->vruntime;
    if (np->policy == SCHED_FIFO && np->priority == op->priority)
      return np->pid < op->pid;
    return np->priority > op->priority;
  }
  return op->policy > np->policy;
}

void add_to_rq(struct proc *np) {
  if (!rq)
    rq = np;
  else if (preempt(rq, np)) {
    np->next = rq;
    rq = np;
  } else {
    struct proc *p = rq;
    for(;;) {
      struct proc *q = p->next;
      if (!q || preempt(q, np)) {
        p->next = np;
        np->next = q;
        break;
      }
      p = q;
    }
  }
}

void remove_from_rq(struct proc *p) {
  struct proc *q = rq;
  if (rq && rq->pid == p->pid) {
    rq = rq->next;
    q->next = 0;
  } else if (rq) {
    struct proc *target = q->next;
    while (target) {
      if (target->pid == p->pid) {
        q->next = target->next;
        target->next = 0;
        break;
      }
      q = target;
      target = q->next;
    }
  }
}

int since_transition(struct proc *p) {
  return ticks - p->info.execution_time - p->info.wait_time - p->info.io_time - p->info.creation_time;
}

void update_vruntime(struct proc *p) {
  if (p->state != RUNNING) return;
  uint runtime = since_transition(p);
  int nice = p->priority > 10 ? 10 : p->priority;
  int load_weight = 1;
  for (int i = 0; i < nice; i++) load_weight *= 1.25;
  p->vruntime += runtime * load_weight;
}

void set_runnable(struct proc *p) {
  update_vruntime(p);
  if (p->state == SLEEPING)
    p->info.io_time += since_transition(p);
  else if (p->state == RUNNING)
    p->info.execution_time += since_transition(p);
  p->state = RUNNABLE;
  add_to_rq(p);
}
void set_sleeping(struct proc *p) {
  update_vruntime(p);
  p->info.execution_time += since_transition(p);
  p->state = SLEEPING;
}

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->tgo = p;
  p->policy = SCHED_RR;
  p->info.creation_time = sys_uptime();
  p->info.execution_time = 0;
  p->info.wait_time = 0;
  p->info.io_time = 0;
  p->info.response_time = 0;
  p->info.exit_time = 0;
  initlock(&p->lk, "tgo lock");

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  acquire(&p->tgo->lk);
  if((p->tgo->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->tgo->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->tgo->sz = PGSIZE;
  release(&p->tgo->lk);
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));

  acquire(&p->tgo->lk);
  p->tgo->cwd = namei("/");
  release(&p->tgo->lk);

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  set_runnable(p);

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  acquire(&curproc->tgo->lk);
  sz = curproc->tgo->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->tgo->pgdir, sz, sz + n)) == 0) {
      release(&curproc->tgo->lk);
      return -1;
    }
  } else if(n < 0){
    if((sz = deallocuvm(curproc->tgo->pgdir, sz, sz + n)) == 0) {
      release(&curproc->tgo->lk);
      return -1;
    }
  }
  curproc->tgo->sz = sz;
  release(&curproc->tgo->lk);
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  acquire(&curproc->tgo->lk);
  if((np->tgo->pgdir = copyuvm(curproc->tgo->pgdir, curproc->tgo->sz)) == 0){
    release(&curproc->tgo->lk);
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->tgo->sz = curproc->tgo->sz;
  release(&curproc->tgo->lk);
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  acquire(&curproc->tgo->lk);
  for(i = 0; i < NOFILE; i++)
    if(curproc->tgo->ofile[i])
      np->tgo->ofile[i] = filedup(curproc->tgo->ofile[i]);
  np->tgo->cwd = idup(curproc->tgo->cwd);
  release(&curproc->tgo->lk);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  set_runnable(np);

  release(&ptable.lock);
  return pid;
}

int clone(void *stack, int new_stack_sz) {
  int pid;
  struct proc *nt;
  struct proc *curproc = myproc();

  // Allocate process.
  if((nt = allocproc()) == 0){
    return -1;
  }

  acquire(&curproc->tgo->lk);
  // Copy process state from proc.
  nt->tgo->pgdir = curproc->tgo->pgdir;
  // Change proc->tgo->sz to uint*
  nt->tgo->sz = curproc->tgo->sz;
  release(&curproc->tgo->lk);

  nt->parent = curproc;
  *nt->tf = *curproc->tf;
  nt->tgo = curproc->tgo;

  // Clear %eax so that fork returns 0 in the child.
  nt->tf->eax = 0;

  // Find bottom of stack (where %ra = 0xFFFFFFFF)
  uint *base = (uint *) curproc->tf->ebp;
  while (*(base + 1) != -1)
    base = (uint *) *base;
  uint old_stack_sz = (uint) (base + 2) - curproc->tf->esp;
  if (old_stack_sz > new_stack_sz) {
    kfree(nt->kstack);
    nt->kstack = 0;
    nt->state = UNUSED;
    return -1;
  }
  nt->tf->esp = (uint) stack + new_stack_sz - old_stack_sz;
  memmove((int *) nt->tf->esp, (int *) curproc->tf->esp, old_stack_sz);

  base = (uint *) curproc->tf->ebp;
  uint offset = nt->tf->esp - curproc->tf->esp;

  // Remap base pointers
  nt->tf->ebp = curproc->tf->ebp + offset;
  while (*(base + 1) != -1) {
    *(uint *) ((int) base + offset) = *base + offset;
    base = (uint *) *base;
  }

  safestrcpy(nt->name, curproc->name, sizeof(curproc->name));

  pid = nt->pid;
  acquire(&ptable.lock);
  set_runnable(nt);
  release(&ptable.lock);
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  curproc->info.exit_time = ticks;
  curproc->info.response_time = curproc->info.exit_time - curproc->info.creation_time;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  if (curproc->tgo == curproc) {
    acquire(&curproc->tgo->lk);
    for(fd = 0; fd < NOFILE; fd++) {
      if(curproc->tgo->ofile[fd]){
        fileclose(curproc->tgo->ofile[fd]);
        curproc->tgo->ofile[fd] = 0;
      }
    }
    begin_op();
    iput(curproc->tgo->cwd);
    end_op();
    curproc->tgo->cwd = 0;
    release(&curproc->tgo->lk);
  }

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
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
          acquire(&curproc->tgo->lk);
          freevm(p->tgo->pgdir);
          release(&curproc->tgo->lk);
        }
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
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

int
waitpid(int pid)
{
  struct proc *p;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      break;
    }
  }
  if (p == &ptable.proc[NPROC] || p->parent != curproc || curproc->killed){
    release(&ptable.lock);
    return -1;
  }
  for(;;){
    if(p->state == ZOMBIE){
      // Found one.
      kfree(p->kstack);
      p->kstack = 0;
      if (p->tgo == p) {
        acquire(&curproc->tgo->lk);
        freevm(p->tgo->pgdir);
        release(&curproc->tgo->lk);
      }
      p->pid = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;
      p->state = UNUSED;
      release(&ptable.lock);
      return 0;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    p = rq;
    if (p) {
      int time = sys_uptime();
      p->info.wait_time = time - p->info.io_time - p->info.execution_time - p->info.creation_time;
      p->state = RUNNING;
      c->proc = p;
      rq = p->next;
      p->next = 0;
      switchuvm(p);
      swtch(&c->scheduler, p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  set_runnable(myproc());
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  struct spinlock *lock = &myproc()->tgo->lk;
  int tgo_lk = holding(lock);
  if (tgo_lk) release(lock);
  // Go to sleep.
  p->chan = chan;
  set_sleeping(p);

  sched();

  // Tidy up.
  p->chan = 0;
  p->park = DRIVING;

  if (tgo_lk) acquire(lock);
  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      set_runnable(p);
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        set_runnable(p);
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
