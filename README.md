# Operating System Project
This repo contains the Pintos Project 1 & 2 I have written for Operating System (CS302) at SUSTech.

## Report
- [Project 1](reports/project1.md) Threads
- [Project 2](reports/project2.md) User Programs

## Changes
To implement Project 1 & 2, we have made modified the following files.

### Project 1: Threads

- [devices/timer.h](src/devices/timer.h)
- [devices/timer.c](src/devices/timer.c)
- [threads/synch.h](src/threads/synch.h)
- [threads/synch.c](src/threads/synch.c)
- [threads/thread.h](src/threads/thread.h)
- [threads/threads.c](src/threads/threads.c)
- [threads/init.c](src/threads/init.c)
- [tests/threads/Make.tests](src/tests/threads/Make.tests)


#### Raw Log
```shell
$ git diff --stat 33f1019f9b6789b91b3bdcdedf968088471775c2 3f025169e92b6ce73f74cda786ffed26e392634e
 .gitignore                              |   2 +-
 reports/project1.md                     | 389 +++++++++++++++++++++
 sample/PROJECT1-DESIGNDOC.txt           | 677 ++++++++++++++++++++++++++++++++++++
 sample/PROJECT2-DESIGNDOC.txt           | 420 ++++++++++++++++++++++
 src/devices/timer.c                     | 154 +++++++-
 src/devices/timer.h                     |   3 +
 src/tests/threads/Make.tests            |  28 +-
 src/tests/threads/alarm-priority-old.c  |  59 ++++
 src/tests/threads/alarm-priority-old.ck |  19 +
 src/tests/threads/tests.c               |   2 +
 src/tests/threads/tests.h               |   1 +
 src/threads/init.c                      |  11 +-
 src/threads/synch.c                     |  95 ++++-
 src/threads/synch.h                     |  37 +-
 src/threads/thread.c                    | 200 ++++++++++-
 src/threads/thread.h                    |  60 +++-
 src/utils/.gitignore                    |   1 +
 17 files changed, 2107 insertions(+), 51 deletions(-)
```

### Project 2: User Programs

#### Raw Log
```shell
$ git diff --stat 933b1b846ecc8440fedf9423e52d61600e30e3a7
 README.md                |  44 ++++++
 README.txt               |   2 -
 src/threads/thread.c     |  40 ++++-
 src/threads/thread.h     |  51 +++++++
 src/userprog/exception.c |  15 ++
 src/userprog/process.c   | 238 +++++++++++++++++++++++++++--
 src/userprog/process.h   |   2 +
 src/userprog/syscall.c   | 414 ++++++++++++++++++++++++++++++++++++++++++++++++++-
 src/userprog/syscall.h   |   8 +
 9 files changed, 794 insertions(+), 20 deletions(-)
```
