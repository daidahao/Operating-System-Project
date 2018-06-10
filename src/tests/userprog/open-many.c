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
			msg ("close file \"%s\", handle = %d", file_name, handle[j]);
		}
	}
}


