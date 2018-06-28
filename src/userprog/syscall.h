#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

/*	Lock for synchronizing the File System Calls.

	Important Notes:
		For ALL operations related to file systems, you need this lock 
		to achieve synchronization. I learnt this lesson in a hard way,
		by spending this whole afternoon debuging (multi-oom test). */
struct lock filesys_lock;

void syscall_init (void);

#endif /* userprog/syscall.h */
