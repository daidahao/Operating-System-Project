#include <syscall.h>
#include "tests/lib.h"
#include "tests/main.h"

const char *test_name = "child-good";

int
main (void)
{
  int handle;
  CHECK ((handle = open ("sample.txt")) > 1, "open \"sample.txt\"");
  msg ("close \"sample.txt\"");
  close (handle);
  return 0;
}