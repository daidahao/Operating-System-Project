Final Report for Project 1: Threads
===================================
## Group Members

* Zhihao DAI <11510415@mail.sustc.edu.cn>

    Task 1 implement and Task 3 design and implement.

* Ziqiang LI <11510352@mail.sustc.edu.cn>

    Task1 and Task2 implement and Task 3 design, report writing.

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
We insert the items in the increasing order of wake-up ticks, so the first one of the sleep list is the most recently thread to wake up. We check current tick every time in the `timer_interrupt()`.

If current tick is greater than or equal `next_wakeup_tick`, wake up procedure will start which includes remove the due threads from `sleep_list` and add it into `ready_list`, then kernel will schedule the threads.

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

+ `void thread_tick (void) `

    Called by the timer interrupt handler at each timer tick. If the current thread has used up its time slice, its priority will decrease by 3 until 0 and then yield if necessary.

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

#### Priority decreasing

1. In `thread_tick (void)` in `thread.c`, we modified this function such that the priority of current thread will decrease by 3 if it have used up its `time_slice`.

2. Also, if the new priority is smaller than the highest one in threads `ready_list`, the current thread will yield once returned from the timer interrupt.

This function is called by timer interrupt handler per time tick.

#### Priority donation algorithm

##### `donate_priority (struct lock *)`

1. The donor acquires a lock and calls `lock_acquire (struct lock *)`. If the lock is currently held by another thread, the donor calls `donate_priority (struct lock *)` in `thread.c`.

1. The donor thread will donate its priority only when the donee's priority is lower.

2. If donee's `donated` is `false`, that is, it hasn't received any donation, store donee's `priority` into `ori_priority` and set `donated` to `true`.

3. Set donee's `priority` to current thread's `priority`.

4. Insert lock to donee's `donations` list and set lock's `donation` to current thread's `priority`.

5. When a donee thread's is blocked by another lock, held by another thread B, the donor thread should donate its priority to thread B.

##### `undo_donate_priority (struct lock *)`

1. When the donee releases the lock, it calls `undo_donate_priority (struct lock *)` in `thread.c`.

2. Remove lock from the thread's `donations` list.

3. If `donations` list is empty, set the thread's `priority` to `ori_priority` and `donated` to false.

4. If not, set the thread's `priority` to the highest priority in the donations list.

### Synchronization

Most of the modifications are in places protected by disabled interrupts or semaphores, so they will not cause race conditions.

### Rationale

Picking a thread to run with maximum priority takes linear time. This was selected for simplicity of coding and to prevent changing the data structure containing ready threads. A priority queue or some other structure may be faster, and may depend on the details of the scheduling algorithm.

When changing priority, we considered searching the other ready threads in order to determine if the current thread should yield. However, this would require disabling interrupts to prevent race conditions with `ready_list`, and the scan could be large. It is simpler and probably faster in most cases to yield no matter what.

The priority donation algorithm forces us to iterate through the lock holder chain and donor list in donee for several operations. It is okay that a single thread own a small number of locks at once, but the thread will keep the highest priority among the donations.

## Design Changes

In Task 1 and 2, we fully implemented what we first proposed during the design review process. However, we did change our implementation of Task 3.

In our initial design, we did not use linked list to store information about priority donations. Instead, we modified `struct lock` as followed.

```c
struct lock
{
  struct thread *holder;       /* Thread holding lock. */
  struct semaphore semaphore;  /* Binary semaphore controlling access. */

  /* Task 3 */
  int prev_priority;          /* Previous priority before donation. */
}
```

To donate a priority, the donor thread must have higher priority than the donee's. When donating, a function `donate_priority()` in `thread.c` is called and the donor thread will set lock's `prev_priority` attribute to donee's priority. If donee's current status is `THREAD_BLOCK`, the donor will run `donate_priority()` recursively on the lock (if any) that blocks the donee.

On releasing the lock, the donee thread will check if the lock has resulted in a donation by checking the value of `prev_priority`. If yes and `prev_priority` is smaller than the current priority, the current thread's priority shall be `prev_priority`. That is, the thread will only restore to the lowest priority it ever had on releasing the lock.

This design comes with the advantage of simper data structures and algorithms. However, it loses track of current effective donations of threads. Consider the following scenario.

Thread A initialy has priority 20 and holds Lcok 1 and Lock 2. Thread B with priority 30 requests to acquire Lcok 1, thus donating its priority 30 to Thread A. Thread C with priority 40 requests to acquire Lock 2, thus donating its priority 40 to Thread A. Assuming all threads' priorities do not decrease during the whole process.

Now, Thread A releases Lcok 1. It should have a new priority 40 since Thread C with priority 40 is still waiting for Lock 2. However, with our initial design, Thread A will simply be assigned with priority 20 after releasing the lock. Thread C may have to wait for a long period of time before Thread A is able to run again and release Lock 2.

Later on, we realized that simply adding `prev_priority` in `struct lock` cannot fully achieve priority donations. In order to set its priority corretly on releasing the lock, every thread must keep track of priority donations it's currently receiving. Therefore, in our final implementation, we assign a donation list for every thread to achieve robust priority donations.

## Reflection

### What did each memebr do

In this project, Zhihao DAI is responsible for implementing Task 1 and designing as well as implementing Task 3. Ziqiang LI is responsible for implementing Task 1 and Task 2 and designing Task 3.

Both members spent a great amount of time debugging the Pintos system with `printf()` and gdb.

### What went well and wrong

As stated in Design Changes, we made great changes on Task 3 due to our carefulness in initial designs.

Luckily, we did not make any changes on initial design of data structures and algorithms for Task 1 and 2. We easily pass the first 4 tests for Task 1 after several hours of coding.

For Task 3, however, we did not consider the conflicts of undoing priority donations when the thread has received more than one donation.

We assumed simply reseting its priority to the lowest value can motivate threads who donated higher priorities before will donate again. Yet, we ignore the fact that those threads are already blcoked after donations and will not be unblocked unless they have acquired the lock. It was only when we finished coding and started running the tests when we realized this defect.

The important lesson we've leant from Task 3 is not how to fix those conflicts and keep track of each thread's received donations, but how to avoid the same kind of carefulness to rise again and waste our precious time.

For Project 2, we've decided we need to spend greater amount of time on reading Pintos source and testing code before any design or coding. Reading the code will help us better understand how the system works and what the exact requirements are.

Also, the correctness and robustness of our initial design clearly impact the total amount of time we will spend on coding and debugging. To avoid debugging, we need to spend more time on devising, questioning and sometimes arguing over our design of data structures and algorithms.

As Project 1 has taught us, a "good" design is never enough and we should always look for a better one.

<!-- ## Additional Questions

1. Expected: `GOOD`. Actual: `BAD`.

  Start with a lock `L`, a semaphore `S = 0`, and a thread `A` with priority `1`.

  Code for thread A:

  ```
  spawn thread D with priority 0
  acquire L
  spawn thread C with priority 3 (C will donate its priority of 3 to A)
  spawn thread B with priority 2
  call down on S
  print GOOD
  ```

  Code for thread B:

  ```
  call down on S
  print BAD
  ```

  Code for thread C:

  ```
  try to acquire L
  ```

  Code for thread D:
  ```
  call up on S (A and B are waiting. A has eff. 3 and base 1, while B has both 2)
  ```

2. MLFQS table:

  timer ticks | R(A) | R(B) | R(C) | P(A) | P(B) | P(C) | thread to run
  ------------|------|------|------|------|------|------|--------------
  0           |     0|     0|     0|    63|    61|    59| A
  4           |     4|     0|     0|    62|    61|    59| A
  8           |     8|     0|     0|    61|    61|    59| A
  12          |    12|     0|     0|    60|    61|    59| B
  16          |    12|     4|     0|    60|    60|    59| A
  20          |    16|     4|     0|    59|    60|    59| B
  24          |    16|     8|     0|    59|    59|    59| A
  28          |    20|     8|     0|    58|    59|    59| B
  32          |    20|    12|     0|    58|    58|    59| C
  36          |    20|    12|     4|    58|    58|    58| A

3. The question didn’t specify the `PRI_MAX` and the number of ticks per second so we assumed their values based on other parts of the project spec. We assumed `PRI_MAX = 63`, and that there are 100 ticks per second (meaning we do not consider any formulas containing `load_avg` since there are only 36 ticks). We also assumed that threads with the same priority are ordered in a round-robin fashion when picking which one to run. In this case we assume "round-robin" is alphabetical order. -->
