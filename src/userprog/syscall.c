#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <debug.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

static void syscall_handler (struct intr_frame *);
uint32_t dereference (uint32_t *addr);
void pop1 (void *esp, uint32_t *a1);
void pop3 (void *esp, uint32_t *a1, uint32_t *a2, uint32_t *a3);
int _write (void *esp);
void _exit (void *esp);
void _halt (void *esp);
int _exec (void *esp);
int _wait (void *esp);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

uint32_t
dereference (uint32_t *addr)
{
	if (!is_user_vaddr (addr))
		goto exit;

	struct thread *current_thread = thread_current ();
	void *page = pagedir_get_page (current_thread->pagedir, addr);
	if (page == NULL)
		goto exit;

	return (*((uint32_t *)page));

	exit:
		process_thread_exit (-1);
		NOT_REACHED ();
}

void
pop3 (void *esp, uint32_t *a1, uint32_t *a2, uint32_t *a3)
{
	uint32_t *esp_ = (uint32_t *)esp;
	*a1 = dereference(++esp_);
	*a2 = dereference(++esp_);
	*a3 = dereference(++esp_);
}

void
pop1 (void *esp, uint32_t *a1)
{
	uint32_t *esp_ = (uint32_t *)esp;
	*a1 = dereference(++esp_);
}

int
_write (void *esp)
{
	int fd;
	const void *buffer;
	unsigned size;
	pop3 (esp, (uint32_t *)&fd, (uint32_t *)&buffer, 
				(uint32_t *)&size);
	if (fd == 1)
	{
		putbuf (buffer, size);
		return size;
	}
	return 0;
}

void
_halt (void *esp UNUSED)
{
	shutdown_power_off ();
}

void
_exit (void *esp)
{
	int status;
	pop1 (esp, (uint32_t *)&status);
	process_thread_exit (status);
	NOT_REACHED ();
}

int
_exec (void *esp)
{
	const char *file;
	pop1 (esp, (uint32_t *)&file);
	tid_t tid = process_execute (file);
	if (tid == TID_ERROR)
		return -1;

	enum intr_level old_level = intr_disable ();
	struct thread *thread = thread_find (tid);
    intr_set_level (old_level);

	sema_down (&(thread->loaded_sema));
	if (!(thread->loaded))
		return -1;
	return tid;
}

int
_wait (void *esp)
{
	int pid;
	pop1 (esp, (uint32_t *)&pid);
	/* In our implementation, tid & pid is one-to-one correspondance. */
	return process_wait (pid);
}

static void
syscall_handler (struct intr_frame *f) 
{
  void *esp = f->esp;
  int syscall_number = *(int *)esp;
  int return_value = 0;

  switch (syscall_number)
  {
  	case SYS_HALT: _halt (esp); NOT_REACHED ();
  	case SYS_EXIT: _exit (esp); NOT_REACHED ();
  	case SYS_EXEC: return_value = _exec (esp); break;
  	case SYS_WAIT: return_value = _wait (esp); break;
  	case SYS_WRITE: return_value = _write (esp); break;
  	default: 
  	{
  		printf ("system call!\n");
  		process_thread_exit (-1);
  	}
  }

  /* Place return value into eax. */
  f->eax = return_value;
}
