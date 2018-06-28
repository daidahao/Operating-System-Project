/*
	In a single process, open and close a file many 
	times (say 1000) to test if there is a page fault 
	when the process has to assign new file descriptors 
	to the newly opened files. 

	This might happen even when currently there are 
	relatively few opened files (say 5), if the array 
	(or other data structure) for storing the file 
	descriptors has no boundary and no reuse.

	A process should be able to call open() as many
	times as it wants, as long as it do not exceed the
	limit of opened files.
	*/
#include <stdio.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

const char *file_name = "sample.txt";
const int times = 1000;
const int num_of_files_each_time = 5;

void
test_main (void) 
{
	int handle[num_of_files_each_time];
	int i, j;
	for (i = 0; i < times; i++)
	{
		/* Open files. */
		for (j = 0; j < num_of_files_each_time; j++)
		{
			CHECK (handle[j] > 1, 
					"open file \"%s\", handle = %d", 
					file_name, (handle[j] = open(file_name)));
		}
		/* Close files. */
		for (j = 0; j < num_of_files_each_time; j++)
		{
			close (handle[j]);
			msg ("close file \"%s\", handle = %d", 
					file_name, handle[j]);
		}
	}
}


