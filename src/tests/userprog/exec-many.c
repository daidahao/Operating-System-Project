/*	Execute a "bad" child and a "good" one many times 
	to test whether the kernel frees all the resouces 
	of the thread (process) when it exits.

	A parent should be able to repeat this process as
	many as times as it wants, being assured that the
	kernel will free all the resouces allocated for 
	the previous children.
	*/
#include <stdio.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

const char *bad_child = "child-bad";
const char *good_child = "child-good";
const int times = 1000;

void
test_main (void) 
{
	int i;
	for (i = 0; i < times; i++)
	{
		/* Execute bad child. */
		int exit_status;
		pid_t child_pid = exec (bad_child);
		msg ("exec (%s) = %d", bad_child, child_pid);
		CHECK(exit_status == -1, "wait (%d) = %d", 
			child_pid, exit_status = wait (child_pid));
		/* Execute good child. */
		child_pid = exec (good_child);
		msg ("exec (%s) = %d", good_child, child_pid);
		CHECK(exit_status == 0, "wait (%d) = %d", 
			child_pid, exit_status = wait (child_pid));
	}
}


