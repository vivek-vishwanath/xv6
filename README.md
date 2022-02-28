# Lab 3 -- Threading

The purpose of this lab is to introduce you to the concepts of concurrency.

This is a *large* lab, larger than the labs you've done so far. You must
complete the parts in order.

To help you stay organized, we have split it into two main checkpoints:
- Checkpoint 1 (Parts 1 and 2)
- Checkpoint 2 (Full lab)

To give a sense of how long it may take to complete each part, we've marked
them with the following labels:
- easy: 30m to 1hr
- moderate: 1-5hrs
- hard: 5-9hrs

Note that there are just estimates. Your completion time will vary based on
your grasp of the course material and proficiency with navigating xv6. We
highly recommend that you read chapters 3 to 5 of the xv6 manual.

## General notes

- We are providing a minimum interface specification, you must build
at least these system calls.  However, if needed, you may build additional
system calls to aid you in your lab construction (although you may not modify
the interface of these system calls).
- Throughout this lab, you will have to test the concurrency of your
system.  You will have to run the `xv6-qemu` script with multiple cpus (default
is 1 cpu) using the `-c <CPUS>` or `--num-cpus=<CPUS>` flags.  We recommend
initial debugging with 1 cpu, to make things easier to parse, then further
debugging with additional cpus.

## Part 1 (hard) -- Threading

What is a thread, and how do we build it?  Like a process, a thread represents
an independent execution context (all processes execute independently), however,
where processes have memory isolation, a thread shares its address space with
all of its peer threads.

In this part of lab3 you will be adding threading support to xv6.  Like Linux,
you'll be treating threads as processes, however they share memory with their
neighboring threads.

### The birth of `clone`

First, you'll need a way to create a new thread (a process that shares address
space with its parent).  For this lab, we'll be accomplishing this with our
version of the classic system call `clone()`:

```
int clone(void *stack, int stack_size)

Arguments:
  stack -- a pointer to the beginning of a memory region of size stack_size, to
           be used as the new thread's stack
  stack_size -- the size of the new thread's stack in bytes

Return:
  As with fork, clone returns twice on success (in the child and the parent).
  The returned values are:
    Parent - the pid of the child.
    Child - 0

  On error clone returns exactly once, with the value -1 (no child is created).

Behavior:
  On success clone creates a new process which shares its address space with its
  parent.  Additionally, clone sets up the child's stack to be logically
  equivalent to the parent's stack.  On clone the child's register state is
  equivalent to that of the parent, with the exception of registers used for 
  the return value from clone (recall eax is the return value of a system call), 
  or holding information about the stack.
```

`clone()` creates a new process, and adds it to the caller's "thread group".
Processes within a "thread group" all share the same address space.  Thread
groups are created via either the `fork` or `exec` system calls.  The first
process within a thread group (the `fork`d or `exec`d process) is the thread
group's owner.  If the owner of a thread group terminates before the other
threads in the group, the behavior for those threads is undefined.

Clone should additionally follow these rules:

- Clone should fail cleanly on errors. If clone cannot run (for instance, if its
  passed a stack that's too small), it should return with an error.

- Cloned processes share several resources with their parent, namely:
   - Virtual address space (shared memory)
   - File descriptor table
   - Current working directory

When any thread makes a change to a shared resource (such as writing to memory,
allocating new memory, or changing the directory) that change should be visible
to all threads in that thread group. 

**NOTE:** Clone sets up its stack to be logically equivalent to its parents, however it
cannot just `memcpy` the stack.  What do you know about stacks that limits you
from doing this (think back to lab1's backtrace)?  How must clone adjust?

### Waiting on specific processes

xv6 has a `wait` system call that waits on any child process. This is useful
when we don't know which children we want to wait on, but can result in
non-deterministic results. As you will see below, we sometimes need to be able
to wait on specific processes.

You will add support for this by implementing the `waitpid()` system call:
```
int waitpid(int pid)

Arguments:
  pid -- pid of the process/thread to wait on

Return:
  -1 on error, 0 on success

Behavior:
  Wait on the process/thread with process id = pid. If the pid doesn't exist
  you must return -1 without waiting.
```

### Nits

- All threads within a thread group share all shared resources.
- If a thread finishes before its children, the behavior of those children
  (threads spawned by this thread) is undefined.

## Part 2 (moderate) -- Beginnings of a userspace threading library

We now have sufficient support from the kernel to start building a userspace
threading library.

You will now implement the following userspace library functions to allow users
to easily create and wait on threads. These functions are defined in
`user/src/threads.c`.
```
Function: thread_create
Arguments:
  - start_routine -- A function pointer to the routine that the child thread will run
  - arg -- the argument passed to start_routine
Return Value:
  - -1 on failure, pid of the created thread on success
Description:
Creates a new child thread.  That thread will immediately begin running start_routine,
as though invoked with start_routine(arg).

Definition:
int thread_create(void *(*start_routine)(void *), void *arg);


Function: thread_wait
Arguments:
  - pid -- pid of the thread to wait on
Return Value:
  - -1 on failure, pid of the joined thread on success
Description:
Waits for a child thread of process id = pid to finish.

Definition:
int thread_wait(int pid);
```

These functions are declared in `user/include/threads.h`. You will implement
them in `user/include/threads.c`. Be warned, despite this simple interface,
these functions actually have tricky implementations, particularly when
attempting to safely avoid memory leaks.

**Important: Lab3's thread library has some rather tricky behavior related to
deallocating its stack. We provide you with a small assembly segment which
atomically calls "free" of the stack of currently running thread, then calls
exit safely. You may use this code in your project. (To use
`free_stack_and_exit`, include the header "free_stack_and_exit.h". The source
code for it can be found in `user/asm/free_stack_and_exit.S`)**

### Nits

- Since `thread_wait` takes in a pid, users can pass in the pid of the process
  itself (and not that of a thread created by it). This is fine.

## Part 3 (easy) -- Userspace spinlocks

Now you will extend your userspace library by implementing spinlocks.

You will implement the following functions:
```
Function: spinlock_init
Arguments:
  - s -- a pointer to the spinlock to initialize
Return value:
  - -1 on failure, 0 on success
Description:
Initializes a spinlock.

Definition:
int spinlock_init(struct spinlock *s);


Function: spinlock_acquire
Arguments:
  - s -- a pointer to the spinlock to acquire
Return value:
  - -1 on failure, 0 on success
Description:
Acquires a spinlock. If the spinlock is already locked, the calling thread spin
until the lock is available.

Definition:
int spinlock_acquire(struct spinlock *s);


Function: spinlock_release 
Arguments:
  - s -- a pointer to the spinlock to release
Return value:
  - -1 on failure, 0 on success
Description:
Releases a spinlock.

Definition:
int spinlock_release(struct spinlock *s);
```

These functions are declared in `user/include/threads.h`. You will implement
them in `user/include/threads.c`. As discussed in class, implementing
synchronization primitives is tricky. In particular, you will need to use
atomic instructions to avoid data races. To that end, we've patched C11 atomics
to the userspace implementation of xv6. You can find the corresponding header
file at `user/include/atomics.h` (which you can subsequently include using
`#include "atomics.h"` in userspace). 

## Part 4 (hard) -- Userspace mutexes

With spinlocks you can now write multi-threaded code that protects its critical
sections.  Spinlocks, however, can be inefficient if the lock is heavily
contended because you waste CPU cycles by having multiple threads contending
for the lock spin in a loop.

Instead of spinning we can put the process to sleep if the lock has been
acquired by some other process and wake it up when released. Locks that exhibit
this behavior are commonly referred to as mutexes.

For this part we will be implementing a userspace mutex. Implementing a mutex
in userspace is tricky due to the "lost wakeup" problem in which a process is
notified to wakeup right before it goes to sleep, thereby losing the
notification and sleeping indefinitely.

To address this issue, we will be implementing the following system calls:
```
Function: park
Arguments:
  - chan -- The channel to sleep on
Return value:
  - -1 on failure, 0 on success
Description:
Puts a process to sleep on channel chan

Definition:
int park(void *chan);


Function: setpark
Arguments:
  - chan -- The channel to sleep on
Return value:
 - -1 on failure, 0 on success
Description:
Signals that a process intends to sleep. It doesn't put the process to sleep
however.

Definition:
int setpark(void *chan);


Function: unpark
Arguments:
  - chan -- The channel to sleep on
Return value:
 - -1 on failure, number of processes woken up on success
Description:
Wake up at most one process sleeping on channel chan.

Definition:
int unpark(void *chan);
```

Once you have these system calls in place, use them to implement the following
functions in `user/src/threads.c`:
```
Function: mutex_init
Arguments:
  - m -- a pointer to the mutex to initialize
Return value:
  - -1 on failure, 0 on success
Description:
Initializes a mutex.

Definition:
int mutex_init(struct mutex *m);


Function: mutex_acquire
Arguments:
  - m -- a pointer to the mutex to acquire
Return value:
  - -1 on failure, 0 on success
Description:
Acquires a mutex. If the mutex is already locked, the calling thread will
sleep until the mutex is available.


Definition:
int mutex_acquire(struct mutex *m);

Function: mutex_release
Arguments:
  - m -- a pointer to the mutex to release
Return value:
  - -1 on failure, 0 on success
Description:
Releases a mutex. If there are any threads waiting on the mutex, one of them
will be woken up

Definition:
int mutex_release(struct mutex *m);
```

### Nits

- `setpark(void *chan)` doesn't put the process to sleep. It only informs the
  kernel that the process is _about to go to sleep_ in the near future. You
  will need this to solve the "lost wakeup" problem
- `unpark(void *chan)` doesn't specify which process to wake up. We leave this
  choice to you.

## Part 5 (easy) -- Userspace conditional variables

While spinlock and mutex synchronization work well, sometimes we need a
synchronization pattern similar to a producer-consumer queue. Instead of
spinning on a spinlock or yielding the CPU in a mutex, we would like the thread
to sleep until certain condition is met. Condition variables give us this
abstraction.

Implement the following functions in `user/src/threads.c`:
```
Function: cond_init 
Arguments:
  - cond -- a pointer to the condition variable to initialize
Return value:
  - -1 on failure, 0 on success
Description:
Initializes a condition variable.

Definition:
int cond_init(struct condvar *cond);


Function: cond_wait 
Arguments:
  - cond -- a pointer to the condition variable to wait on
  - m -- a pointer to the mutex to acquire
Return value:
  - -1 on failure, 0 on success
Description:
Atomically blocks the current thread waiting on the condition variable cond,
and releases the mutex m. The waiting thread unblocks only after another thread
calls cond_signal. After being woken up the current thread reacquires the mutex
m.


Definition:
int cond_wait(struct condvar *cond, struct mutex *m);


Function: cond_signal
Arguments:
  - cond -- a pointer to the condition variable to signal
Return value:
  - -1 on failure, 0 on success
Description:
Unblocks one thread waiting for the condition variable cond.

Definition:
int cond_signal(struct condvar *cond);
```

## General Guidance

Recall, a kernel's responsibility is to provide high-level abstractions to the
user-space. Any user-behavior shouldn't be able to break the abstractions
provided by the kernel. As such, user-state and user input to the kernel should
not allow the user-space to execute arbitrary code on the user's behalf, modify
arbitrary kernel memory, or crash the kernel. The autograder will try to crash
your kernel by providing unexpected user-space input! You should protect against
bad input that comes from user-space, just as a real kernel must protect against
malicious users.

Also, the kernel persists throughout the lifetime of the machine. As a result,
any OS code should be free of memory leaks and data-races. Your code should
error out correctly when given bad inputs, and shouldn't leak resources (memory,
process table entries, or fds, etc), even in the instance of failures.

Lastly, we encourage you to have fun while implementing it. It may seem daunting
at first, but know full-well that you have all that you need to do well in this
lab. Make good use of lectures, Piazza, and office hours: we're there to help.

## Leaderboard

We are working on setting this up. Stay tuned!

## Autograder

As usual, you will submit this lab to the autograder.  As this lab is larger
than prior labs, we will give you some guidance as to what the autograder is
testing for.

- Clone Functionality
  - Tests 1-5
- Clone error / security
  - Tests 6-11
- Thread library general testing
  - Tests 12-16
- Thread library error / security testing
  - Tests 17-21
- Spinlock
  - Test 22
- Park, setpark, unpark
  - Tests 23-25
- Mutex
  - Test 26
- Waitpid
  - Test 27
- Condvar
  - Tests 28-31

**IMPORTANT**: Since this lab has a good deal of concurrency involved, you may
be able to pass some tests without correctly implementing some of these
primitives. We will be looking over your submission when hand-grading, so
please thoroughly test your implementation.

On Gradescope you will find two assignments:
- Lab 3 - Checkpoint 1
- Lab 3 - Checkpoint 2

Your final Lab 3 score is equal to the score you get for checkpoint 2 (autograded + hand-graded). This means that you can continue working on checkpoint 1 after the due date. If you are able to pass all the autograder tests for checkpoint 1 by the due date, **five bonus points** will be added to your final Lab 3 score.
