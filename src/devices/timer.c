#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "devices/pit.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* For time_sleep() */
#include <stdbool.h>
  
/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);
static void real_time_delay (int64_t num, int32_t denom);

/* For time_sleep() */
static bool sleep_less (const struct list_elem *a, const struct list_elem *b, void *aux);
static int64_t get_wakeup_tick (const struct list_elem *e);
static void wakeup (void *);

/* Variables Introduced by Project 1 */

/* Task 1: Efficient Alarm Clock */
static struct list sleep_list;			/* List of sleeping threads. */
static struct lock sleep_list_lock;		/* Lock for SLEEP_LIST. */
static int64_t next_wakeup_tick;		/* Most recent tick to wake up any thread. */
static struct semaphore sleep_sema;		/* Semaphore for WAKEUP thread. */

/* WAKEUP thread for time_sleep().
   Wake up all threads that needs to be woken up in SLEEP_LIST. */
static void
wakeup (void * v UNUSED)
{
  while (1)
  {
    sema_down (&sleep_sema);
    sema_up(&sleep_sema);

    /* Make sure WAKEUP thread is always at highest priority. */
    thread_set_priority (PRI_MAX);

    int64_t now = timer_ticks ();
    struct list_elem *e;
    /* Acquire LOCK to access and modify SLEEP_LIST. */
    lock_acquire (&sleep_list_lock);

    /* Wake up all threads with WAKEUP_TICK smaller than NOW by 
       unblocking and remove them from SLEEP_LIST. */
    /* Since the SLEEP_LIST is already sorted, we don't need to sort it anymore. */
    for (e = list_begin (&sleep_list); e != list_end (&sleep_list);)
    {
	    if (get_wakeup_tick (e) <= now)
	    {
	        struct thread *t = list_entry (e, struct thread, sleep_elem);
	        if (t->status != THREAD_BLOCKED)
	    		break;
	        thread_unblock (t);
	        e = list_remove (e);
	    }
	    else
	    	break;
    }

    /* If SLEEP_LIST is empty, WAKEUP will not be unblocked. 
       Otherwise, set NEXT_WAKEUP_TICK to most recent tick to wake up. */
    if (list_empty (&sleep_list))
      next_wakeup_tick = INT64_MAX;
    else
      next_wakeup_tick = get_wakeup_tick (list_front (&sleep_list));

    lock_release (&sleep_list_lock);
    sema_down (&sleep_sema);
  }

}

/* Less compare function for linked list. 
   Return true if thread A's WAKEUP_TICK is smaller than B's. */
bool
sleep_less (const struct list_elem *a,
                             const struct list_elem *b,
                             void *aux UNUSED) 
{
  return (get_wakeup_tick (a) < get_wakeup_tick (b));
}

/* Sets up the timer to interrupt TIMER_FREQ times per second,
   and registers the corresponding interrupt. */
void
timer_init (void) 
{
  pit_configure_channel (0, 2, TIMER_FREQ);
  intr_register_ext (0x20, timer_interrupt, "8254 Timer");

  /* Task 1: Efficient Alarm Clock */
  /* Initialize variables. */
  list_init (&sleep_list);
  lock_init (&sleep_list_lock);
  next_wakeup_tick = INT64_MAX;
  sema_init (&sleep_sema, 0);

  /* Create a WAKEUP thread to wake up sleeping threads when needed. */
  thread_create ("WAKEUP", PRI_MAX, wakeup, NULL);
}

/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. In Task 1, we change timer_sleep() so that it no
   longer implements busy waiting. 
   Instead, it will insert the current thread into SLEEP_LIST. When 
   the wake up time comes, a thread named WAKEUP will be unblocked 
   and unblock the sleeping thread. */
void
timer_sleep (int64_t ticks) 
{
  int64_t start = timer_ticks ();

  ASSERT (intr_get_level () == INTR_ON);

  /* Return immediately if TICKS is non-positive. */
  if (ticks <= 0)
    return;

  struct thread* current_thread = thread_current ();
  current_thread->wakeup_tick = start + ticks;

  /* Acquire LOCK for SLEEP_LIST. */
  lock_acquire (&sleep_list_lock);
  /* Insert current thread into SLEEP_LIST in order. */
  list_insert_ordered (&sleep_list, &(current_thread->sleep_elem), 
  				sleep_less, NULL);
  /* Calculate new value for NEXT_WAKEUP_TICK. */
  next_wakeup_tick = get_wakeup_tick(list_front(&sleep_list));
  /* Release LOCK for SLEEP_LIST. */
  lock_release (&sleep_list_lock);

  /* Block the current thread. */
  intr_disable ();
  thread_block ();
  intr_enable ();
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();

  /* Only call sema_up() when NEXT_WAKEUP_TICK comes. */
  /* Because of pre-computation, it's more efficient and has less delay. */
  if (ticks >= next_wakeup_tick && sleep_sema.value==0)
    sema_up (&sleep_sema);
}

/* Calibrates loops_per_tick, used to implement brief delays. */
void
timer_calibrate (void) 
{
  unsigned high_bit, test_bit;

  ASSERT (intr_get_level () == INTR_ON);
  printf ("Calibrating timer...  ");

  /* Approximate loops_per_tick as the largest power-of-two
     still less than one timer tick. */
  loops_per_tick = 1u << 10;
  while (!too_many_loops (loops_per_tick << 1)) 
    {
      loops_per_tick <<= 1;
      ASSERT (loops_per_tick != 0);
    }

  /* Refine the next 8 bits of loops_per_tick. */
  high_bit = loops_per_tick;
  for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
    if (!too_many_loops (high_bit | test_bit))
      loops_per_tick |= test_bit;

  printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* Returns the number of timer ticks since the OS booted. */
int64_t
timer_ticks (void) 
{
  enum intr_level old_level = intr_disable ();
  int64_t t = ticks;
  intr_set_level (old_level);
  return t;
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then) 
{
  return timer_ticks () - then;
}

/* Get thread's WAKEUP_TICK by its SLEEP_ELEM. */
int64_t
get_wakeup_tick (const struct list_elem *e) 
{
  return list_entry (e, struct thread, sleep_elem)->wakeup_tick;
}

/* Sleeps for approximately MS milliseconds.  Interrupts must be
   turned on. */
void
timer_msleep (int64_t ms) 
{
  real_time_sleep (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts must be
   turned on. */
void
timer_usleep (int64_t us) 
{
  real_time_sleep (us, 1000 * 1000);
}

/* Sleeps for approximately NS nanoseconds.  Interrupts must be
   turned on. */
void
timer_nsleep (int64_t ns) 
{
  real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Busy-waits for approximately MS milliseconds.  Interrupts need
   not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_msleep()
   instead if interrupts are enabled. */
void
timer_mdelay (int64_t ms) 
{
  real_time_delay (ms, 1000);
}

/* Sleeps for approximately US microseconds.  Interrupts need not
   be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_usleep()
   instead if interrupts are enabled. */
void
timer_udelay (int64_t us) 
{
  real_time_delay (us, 1000 * 1000);
}

/* Sleeps execution for approximately NS nanoseconds.  Interrupts
   need not be turned on.

   Busy waiting wastes CPU cycles, and busy waiting with
   interrupts off for the interval between timer ticks or longer
   will cause timer ticks to be lost.  Thus, use timer_nsleep()
   instead if interrupts are enabled.*/
void
timer_ndelay (int64_t ns) 
{
  real_time_delay (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) 
{
  printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops) 
{
  /* Wait for a timer tick. */
  int64_t start = ticks;
  while (ticks == start)
    barrier ();

  /* Run LOOPS loops. */
  start = ticks;
  busy_wait (loops);

  /* If the tick count changed, we iterated too long. */
  barrier ();
  return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops) 
{
  while (loops-- > 0)
    barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom) 
{
  /* Convert NUM/DENOM seconds into timer ticks, rounding down.
          
        (NUM / DENOM) s          
     ---------------------- = NUM * TIMER_FREQ / DENOM ticks. 
     1 s / TIMER_FREQ ticks
  */
  int64_t ticks = num * TIMER_FREQ / denom;

  ASSERT (intr_get_level () == INTR_ON);
  if (ticks > 0)
    {
      /* We're waiting for at least one full timer tick.  Use
         timer_sleep() because it will yield the CPU to other
         processes. */                
      timer_sleep (ticks); 
    }
  else 
    {
      /* Otherwise, use a busy-wait loop for more accurate
         sub-tick timing. */
      real_time_delay (num, denom); 
    }
}

/* Busy-wait for approximately NUM/DENOM seconds. */
static void
real_time_delay (int64_t num, int32_t denom)
{
  /* Scale the numerator and denominator down by 1000 to avoid
     the possibility of overflow. */
  ASSERT (denom % 1000 == 0);
  busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000)); 
}
