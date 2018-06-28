#include <stdio.h>
#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

const int max_range = 1000;

void
test_main (void)
{
	int i, j;
	for (i = 0; i < max_range; i++)
	{
		CHECK (j == i + 1, 
			"practice (%d) = %d", 
				i, j = practice(i));
	}
}

