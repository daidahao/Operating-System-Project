Final Report for Project 1: Threads
===================================
## Group Members

* Zhihao DAI <11510415@mail.sustc.edu.cn>
* Ziqiang LI <11510352@mail.sustc.edu.cn>

## Task 1: Scheduler based on time slice

### Data structures and functions

#### timer.c

- `static struct list sleep_list`  
    Holding the threads which is sleeping.
- `static struct lock sleep_list_lock`  
    Required lock for modifying the `sleep_list`.
- `static int64_t next_wakeup_tick`  
    Holding the next wake up tick since OS booted.
- `static struct semaphore sleep_sema`  
    Semaphore for synchronization. When a thread is to sleep. Another thread will know that.
- `static bool sleep_less (const struct list_elem *a, const struct list_elem *b, void *aux)`  
    Less comparation method for wake up tick.
- `static int64_t get_wakeup_tick (const struct list_elem *e)`  
    Get the thread wake up tick.
- `static void wakeup (void *)`  
    Wake up all due thread.
- `void timer_init (void)`  
    Initialize the timer.
- `void timer_sleep (int64_t ticks)`  
    Block current thread, add it to the `sleep_list`.
- `static void timer_interrupt (struct intr_frame *args UNUSED)`  
    Handle the interruption, if now tick equals `next_wakeup_tick`, modify `sleep_sema`, then `wakeup()` will be called.


#### thread.h

- `void timer_sleep (int64_t ticks) `
  Instead of the while, yield, we instead disable interrupts, block the thread and add the thread to the linked list

* `struct thread`
    - `int64_t wakeup_tick`  
        Records the ticks to wake up after OS booted.
    - `struct list_elem sleep_elem`  
        As a list_elem in the sleep list.

#### thread.c

- `static int64_t ticks`  
  Number of timer ticks since OS booted.

- `void thread_block (void)`
  Blocks a thread, requires interrupts off

- `void thread_unblock (struct thread *t)`
  Unblocks a thread

### Algorithms

When a thread sleeps, we block it and add it into the sleep list.
We insert the items in the increasing order of wake-up ticks, so the first one of the sleep list is the most recently thread to wake up. We check current tick every time in the `timer_interrupt()`. If current tick is greater than or equal `next_wakeup_tick`, wake up procedure will start which includes remove the due threads from `sleep_list` and add it into `ready_list`, then kernel will schedule the threads.

### Synchronization

We have a daemon thread to handle thread wake up event. In the most time the thread is blocked by semaphore. When a new thread wants to sleep, it will insert itself into the sleep list and update `next_wakeup_tick` after it have got the lock to modify the sleep list and then block itself.

`timer_interrupt()` checks if it is the time to wake up the most recent sleep thread. If so, semaphore will notice the daemon thread. the daemon thread, `wakeup` goes through the sleep list, moves the due threads to the ready list and update `next_wakeup_tick`. After doing above, it will block itself again.

### Rationale

We chose to have a sleep list to avoid busy waiting. In this way, we avoid the thread continually yielding and do the little work required to unblock ready threads at the due time. Insertion and waking up takes linear time.

## Task 2: Round-Robin scheduler

### Data structures and functions

#### thread.h

* `struct thread`
    - `int time_slice`  
        Records the time slice which is set to `(priority % 7) + 2` during thread initialization.

#### thread.c

* `thread_tick()`  
    This function is called by `timer_interrupt()`, so every tick it will check the time slice of the current thread. If it runs out, the thread will be thrown back to the ready list, set a new priority, and yield for running other thread.
*  `void thread_set_priority (int new_priority)`  
    Set a new priority.

### Algorithms

Originally, the time slice is defined in macro with 4 and the current implementation of the scheduler don’t respect the time slice. Now, we need to reform the scheduler to Round-Robin mode. When a thread run out it time slice which is determine by its original priority, it need to decrease its own priority by 3 or set it to 0 if priority is going to be negative and move to the tail of ready list. Noticing that if a thread no longer has the highest priority, it must immediately yield the CPU to the highest-priority thread.

### Synchronization

Because all the function call is under `timer_interrupt()` which is already disable the interaption, we don't have to worry about synchronization issues. So, the most vital problem is to simplify the code segment in `timer_interrupt()` to reduce the interrupt time.

### Rationale

We add `time_slice` property in struct `thread`, which help us save the time for calculating the time slice by original priority.

## Task 3: Priority scheduler based on time slice

### Data structures and functions

### Algorithms

#### Priority donation algorithm

### Synchronization
