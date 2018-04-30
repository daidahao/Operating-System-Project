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
    - `unsigned time_slice`  
        Records the time slice which is set to `(priority % 7) + 2` during thread initialization.

#### thread.c

* `thread_tick()`  
    This function is called by `timer_interrupt()`, so every tick it will check the time slice of the current thread. If it runs out, the thread will be thrown back to the ready list, update a new priority, and yield for running other thread.
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

#### thread.h

+ `struct thread`
    + `struct lock *blocked_lock`  
    Records the lock which block the thread. 
    + `struct list donations`  
    As a list containing all the lock requesters or donors, who have higher priority than current lock holder.
    + `int ori_priority`  
    Records the original priority after it receives other thread donation. 
    + `bool donated`  
    Indicate if the current thread is received other thread donation. 

+ `void set_blocked_lock (struct lock *)`  
    Records the current thread is blocked by the specific lock.
+ `void donate_priority(struct lock *)`  
    Donate priority to the lock holder.
+ `void undo_donate_priority (struct lock *)`  
    Undo priority donation after the thread releases the lock.
+ `bool thread_cmp_by_priority (const struct list_elem *, const struct list_elem *, void * UNUSED)`  
    Compare 2 thread's priority. 

#### thread.c

+ `tid_t thread_create (const char *name, int priority, thread_func *function, void *aux) `  
    After thread creation, current thread must yield if it has lower priority than the new one.

+ `void thread_set_priority (int new_priority)`  
    Set the current thread's priority to NEW_PRIORITY. If the current thread's has received donation, we store NEW_PRIORITY into ORI_PRIORITY so that once all donations are revoked, NEW_PRIORITY will take effect.

+ `static struct thread * next_thread_to_run (void) `  
    Get the thread in the ready list with the highest priority.
 
#### synch.h

+ `struct lock`
    + `int donation`  
    Records lock holders priority before receiving donation.
    + `struct list_elem don_elem`  
    As a list_elem in the donation list.

+ `bool donation_cmp (const struct list_elem *, const struct list_elem *, void *)`  
    Compare donation property in `struct lock`.

+ `bool waiter_cmp (const struct list_elem *, const struct list_elem *, void *)`  
    Compare waiting thread priority property in `struct semaphore_elem`.

+ `int get_donation (const struct list_elem *elem)`  
    Get method of donation property in `struct lock`.

#### synch.c

+ `void sema_up (struct semaphore *sema)`  
    Increments SEMA's value and wakes up the highest priority thread of those waiting for SEMA.
+ `void lock_init (struct lock *lock)`  
    Add donation property initialization procedure.
+ `void lock_acquire (struct lock *lock)`  
    Firstly, current thread try donate priority to the lock holder if it exsists, then the thread acquire the lock if it is available. Otherwise, the thread keeps sleep until no one else who have higher priority acquire the lock.
+ `void lock_release (struct lock *lock)`  
    Release the holding lock and undo the received priority.

### Algorithms

#### Priority donation algorithm

Idea

1. A donee thread's priority is always lower than donor thread's priority.

2. When a donee thread's is blocked by another thread B who held an another lock, the donor thread should donate its priority to thread B.

3. When a thread releases a lock, it should wake up the waiting thread having the highest priority.


### Synchronization

Most of the modifications are in places protected by disabled interrupts or semaphores, so they will not cause race conditions.

### Rationale

Picking a thread to run with maximum priority takes linear time. This was selected for simplicity of coding and to prevent changing the data structure containing ready threads. A priority queue or some other structure may be faster, and may depend on the details of the scheduling algorithm.

When changing priority, we considered searching the other ready threads in order to determine if the current thread should yield. However, this would require disabling interrupts to prevent race conditions with `ready_list`, and the scan could be large. It is simpler and probably faster in most cases to yield no matter what.

The priority donation algorithm forces us to iterate through the lock holder chain and donor list in donee for several operations. It is okay that a single thread own a small number of locks at once, but the thread will keep the highest priority among the donations.
