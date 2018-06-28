#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* Opened files descriptors. */
#define MAX_OPENED_FILES 128

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
    /*  Introduced children_list, processs_ptr for wait() & exit() system calls.
        To see how they are actually used, check init_thread() in threads/thread.c
        and process_execute(), process_wait(), process_thread_exit() in 
        userprog/process.c. */
    /*  When the parent calls process_execute(), it dynamically allocates a "struct 
        child_process" for the child, which is supoosed to store the exit status of the
        child (see "struct child_process" below for more details) and points child's
        process_ptr to this info. */
    /*  Important Notes: When the parent exits, it must clean up all the memory allocated
        for each "struct child_process" in children_list. */
    struct list children_list;          /* List of children process. */
    struct child_process *process_ptr;  /* Pointer to the process's information. */
    /*  Introduced loaded_sema, loaded for exec() system call.
        To see how they are actually used, check init_thread() in threads/thread.c
        and start_process() in userprog/process.c and _exec() in userprog/syscall.c. */
    /*  When the parent calls exec() system call, it calls process_execute(), essentially,
        start_process() first, then downs the semaphore of the child process. Once the 
        child process finish loading, its load result is saved into loaded and loaded_sema 
        is upped, thus waking up the parent in exec(). */
    struct semaphore loaded_sema;       /* Semaphore for load. */
    bool loaded;                        /* Whether the process is successfully loaded. */
    /*  Introduced opened_files for File System Calls, which is an array for 
        storing File Descriptors. It will not been allocated memory until the first time
        _open() is called. 
        See _open(), _write(), _close() in userprog/syscall.c for more deatils. */
    /*  Important Notes: When the process exits, it must make sure that it cleans up the
        memory allocated for opened_files. */
    struct file **opened_files;         /* Array of currently opened files. */
    /*  To ensure that when the process is running, its executable cannot be
        modified, we save the pointer to the file into process_file.
        When the process starts, the process denies write access using 
        file_deny_write(). Check out load() in userprog/process.c.
        When the process exits, the process allows write access using 
        file_allow_write(). Check  out process_thread_exit() in userprog/process.c. */
    /*  Notice that executable could still be not writable if there is another
        process of the same executable running. */
    struct file *process_file;          /* The executable file of the process. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

#ifdef USERPROG
/*  Info of the child process.

    To implement process_wait(), we introduce this structure.

    On one hand, it has a "struct list_elem" for inserting itself
    into the parent's children_list. Thus, the parent can access its
    status (waited, exit_status, etc..) easily.

    On the other hand, the corresponding child process has a pointer 
    named "process_ptr" to this structure. Therefore, the child process
    can safely save its exit status into this structure without worrying
    it might get lost when all the resouces of the process is freed.

    The initail value of "semaphore" is 0. When the child process is being 
    waited, that is, process_wait() is called on the child, the parent downs 
    the semaphore. When the child process exits, process_thread_wait() is 
    called and the semaphore is upped. Then, the parent waiting for the child
    would wake up and can retrieve the child's exit status now. */
struct child_process
{
  struct list_elem children_elem; /* List element for list of children of the process. */
  tid_t tid;                      /* Child process's tid. */
  struct semaphore semaphore;     /* Semaphore. */
  bool waited;                    /* Whether the child process has been waited for. */
  struct thread *thread;          /* Pointer to the child thread. */
  int exit_status;                /* Exit status. */
};
#endif

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

struct thread *thread_find (tid_t);

#endif /* threads/thread.h */
