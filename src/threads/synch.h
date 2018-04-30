#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#define LOCK_INIT_PREV_PRIORITY -20
#define LOCK_INIT_DONATION -20

#include <list.h>
#include <stdbool.h>

/* A counting semaphore. */
struct semaphore 
  {
    unsigned value;             /* Current value. */
    struct list waiters;        /* List of waiting threads. */
  };

void sema_init (struct semaphore *, unsigned value);
void sema_down (struct semaphore *);
bool sema_try_down (struct semaphore *);
void sema_self_test (void);

/* Lock. */
struct lock 
  {
    struct thread *holder;      /* Thread holding lock (for debugging). */
    struct semaphore semaphore; /* Binary semaphore controlling access. */

    /* Members Introduced by Project 1 */
    /* Task 3: Priority Scheduler */
    int donation;               /* Donation due to the lock. */
    struct list_elem don_elem;  /* Element for list of donations in struct thread. */
  };


bool lock_try_acquire (struct lock *);
bool lock_held_by_current_thread (const struct lock *);

/* Condition variable. */
struct condition 
  {
    struct list waiters;        /* List of waiting threads. */
  };

void cond_init (struct condition *);
void cond_broadcast (struct condition *, struct lock *);




/* Functions Introduced in Project 1 */
/* Task 3: Priority Scheduler */
bool donation_cmp (const struct list_elem *, const struct list_elem *, void *);
bool waiter_cmp (const struct list_elem *, const struct list_elem *, void *);
int get_donation (const struct list_elem *elem);




/* Functions Modified in Project 1 */
/* Task 3: Priority Scheduler */
void sema_up (struct semaphore *);

void lock_init (struct lock *);
void lock_acquire (struct lock *);
void lock_release (struct lock *);

void cond_wait (struct condition *, struct lock *);
void cond_signal (struct condition *, struct lock *);


/* Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */
