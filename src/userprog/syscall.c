#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <debug.h>
#include "devices/shutdown.h"
#include "devices/input.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);

uint32_t dereference (uint32_t *addr);
void pop1 (void *esp, uint32_t *a1);
void pop2 (void *esp, uint32_t *a1, uint32_t *a2);
void pop3 (void *esp, uint32_t *a1, uint32_t *a2, uint32_t *a3);

/* Project 2: Task 2 Process System Calls. */
void _exit (void *esp);
void _halt (void *esp);
int _exec (void *esp);
int _wait (void *esp);

/* Project 2: Task 3 File System Calls. */
struct file **init_opened_files (void);
bool is_fd_valid (int fd, struct file **file);
bool _create (void *esp);
bool _remove (void *esp);
int _open (void *esp);
int _filesize (void *esp);
int _read (void *esp);
int _write (void *esp);
void _seek (void *esp);
unsigned _tell (void *esp);
void _close (void *esp);


/* Lock for synchronizing the File System Calls. */
struct lock filesys_lock;

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&filesys_lock);
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

/* Pop 1 arguments off the stack by ESP. */
void
pop1 (void *esp, uint32_t *a1)
{
	uint32_t *esp_ = (uint32_t *)esp;
	*a1 = dereference(++esp_);
}

/* Pop 2 arguments off the stack by ESP. */
void
pop2 (void *esp, uint32_t *a1, uint32_t *a2)
{
	uint32_t *esp_ = (uint32_t *)esp;
	*a1 = dereference(++esp_);
	*a2 = dereference(++esp_);
}

/* Pop 3 arguments off the stack by ESP. */
void
pop3 (void *esp, uint32_t *a1, uint32_t *a2, uint32_t *a3)
{
	uint32_t *esp_ = (uint32_t *)esp;
	*a1 = dereference(++esp_);
	*a2 = dereference(++esp_);
	*a3 = dereference(++esp_);
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

bool
_create (void *esp)
{
	const char *file;
	unsigned initial_size;
	pop2 (esp, (uint32_t *)&file, (uint32_t *)&initial_size);
	if (file == NULL)
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}
	lock_acquire (&filesys_lock);
	bool success = filesys_create (file, initial_size);
	lock_release (&filesys_lock);
	return success;
}

bool
_remove (void *esp)
{
	const char *file;
	pop1 (esp, (uint32_t *)&file);
	if (file == NULL)
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}
	lock_acquire (&filesys_lock);
	bool success = filesys_remove (file);
	lock_release (&filesys_lock);
	return success;
}

/* 	Helper function for file system calls.
	Allocate a new page to "struct file **opened_files" 
	in "struct thread". */
struct file **
init_opened_files (void)
{
	return ((struct file **) palloc_get_page (PAL_ZERO));
}

int
_open (void *esp)
{
	const char *file_name;
	pop1 (esp, (uint32_t *)&file_name);
	if (file_name == NULL)
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}

	int file_descriptor = -1;
	/* We do not initialize opened_files in struct thread unitl
	the first time a file is opened. */
	struct file **opened_files = thread_current ()->opened_files;
	if (opened_files == NULL)
		opened_files =  thread_current ()->opened_files 
					= init_opened_files ();

	lock_acquire (&filesys_lock);
	struct file *file = filesys_open (file_name);
	lock_release (&filesys_lock);

	if (file == NULL)
		goto result;

	/* Find the first NULL element in opened_files such that we can 
	allocate the index as a file descriptor to the file. If such element
	does not exist, return -1. */
	int i;
	for (i = 2; i < MAX_OPENED_FILES + 2; i++)
	{
		if (opened_files[i] == NULL)
		{
			opened_files[i] = file;
			file_descriptor = i;
			break;
		}
	}

	result:
		return file_descriptor;
}

/* 	Helper function for file system calls.
	Determine whether the fd is valid for the current process. */
bool
is_fd_valid (int fd, struct file **file)
{
	if (fd < 2 || fd > MAX_OPENED_FILES + 1)
		return false;
	struct file **opened_files = thread_current ()->opened_files;
	if (opened_files == NULL || opened_files[fd] == NULL)
		return false;
	if (file != NULL)
		*file = opened_files[fd];
	return true;
}

int
_filesize (void *esp)
{
	int fd;
	pop1 (esp, (uint32_t *)&fd);

	struct file *file;
	if (!is_fd_valid (fd, &file))
		return -1;

	lock_acquire (&filesys_lock);
	int file_size = file_length (file);
	lock_release (&filesys_lock);

	return file_size;
}

int
_read (void *esp)
{
	int fd;
	void *buffer;
	unsigned size;
	pop3 (esp, (uint32_t *)&fd, (uint32_t *)&buffer, (uint32_t *)&size);

	/* 	Check whether buffer is valid user address. If not, exit the 
		thread with status -1. */
	if (!is_user_vaddr (buffer))
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}
	
	/* Fd 0 reads from the keyboard using input_getc(). */
	if (fd == 0)
	{
		char *buffer__ = (char *) buffer;
		while (size-- > 0)
			*(buffer__++) = input_getc ();
		return size;
	}

	/* Otherwise, reads from the corresponding file using file_read(). */
	struct file *file;
	if (!is_fd_valid (fd, &file))
		return -1;

	lock_acquire (&filesys_lock);
	int bytes_read =  file_read (file, buffer, size);
	lock_release (&filesys_lock);

	return bytes_read;
}

int
_write (void *esp)
{
	int fd;
	const void *buffer;
	unsigned size;
	pop3 (esp, (uint32_t *)&fd, (uint32_t *)&buffer, 
				(uint32_t *)&size);

	/* 	Check whether buffer is valid user address. If not, exit the 
		thread with status -1. */
	if (!is_user_vaddr (buffer))
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}

	/* Fd 1 writes to the console using putbuf(). */
	if (fd == 1)
	{
		putbuf (buffer, size);
		return size;
	}

	/* Otherwise, writes to the corresponding file using file_write(). */
	struct file *file;
	if (!is_fd_valid (fd, &file))
		return -1;

	lock_acquire (&filesys_lock);
	int bytes_written =  file_write (file, buffer, size);
	lock_release (&filesys_lock);

	return bytes_written;
}

void
_seek (void *esp)
{
	int fd;
	unsigned position;
	pop2 (esp, (uint32_t *)&fd, (uint32_t *)&position);

	/* Retrieve pointer to the file using fd. */
	struct file *file;
	if (!is_fd_valid (fd, &file))
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}

	/* 	Changes the next byte to be read or written in open file fd 
		to position using file_seek(). */
	lock_acquire (&filesys_lock);
	file_seek (file, position);
	lock_release (&filesys_lock);
}

unsigned
_tell (void *esp)
{
	int fd;
	pop1 (esp, (uint32_t *)&fd);

	/* Retrieve pointer to the file using fd. */
	struct file *file;
	if (!is_fd_valid (fd, &file))
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}

	return file_tell (file);
}

void
_close (void *esp)
{
	int fd;
	pop1 (esp, (uint32_t *)&fd);

	/* Retrieve pointer to the file using fd. */
	struct file *file;
	if (!is_fd_valid (fd, &file))
	{
		process_thread_exit (-1);
		NOT_REACHED ();
	}

	/* Closes file descriptor fd using file_close(). */
	lock_acquire (&filesys_lock);
	file_close (file);
	(thread_current ()->opened_files)[fd] = NULL;
	lock_release (&filesys_lock);
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
  	case SYS_CREATE: return_value = _create (esp); break;
  	case SYS_REMOVE: return_value = _remove (esp); break;
  	case SYS_OPEN: return_value = _open (esp); break;
  	case SYS_FILESIZE: return_value = _filesize (esp); break;
  	case SYS_READ: return_value = _read (esp); break;
  	case SYS_WRITE: return_value = _write (esp); break;
  	case SYS_SEEK: _seek (esp); break;
  	case SYS_CLOSE: _close (esp); break;
  	case SYS_TELL: return_value =  _tell (esp); break;
  	default: 
  	{
  		printf ("system call!\n");
  		process_thread_exit (-1);
  	}
  }

  /* Place return value into eax. */
  f->eax = return_value;
}
