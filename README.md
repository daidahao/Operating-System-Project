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
- [threads/thread.h](src/threads/thread.h)
- [threads/thread.c](src/threads/threads.c)
- [threads/synch.h](src/threads/synch.h)
- [threads/synch.c](src/threads/synch.c)
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

- [userprog/syscall.h](src/userprog/syscall.h)
- [userprog/syscall.c](src/userprog/syscall.c)
- [userprog/process.h](src/userprog/process.h)
- [userprog/process.c](src/userprog/process.c)
- [threads/thread.h](src/threads/thread.h)
- [threads/thread.c](src/threads/thread.c)
- [userprog/exception.c](src/userprog/exception.c)
- [lib/syscall-nr.h](src/lib/syscall-nr.h)
- [lib/user/syscall.h](src/lib/user/syscall.c)
- [lib/user/syscall.c](src/lib/user/syscall.c)
- [tests/tests.pm](src/tests/tests.pm)
- [tests/userprog/Make.tests](src/tests/userprog/Make.tests)
- [tests/userprog/child-good.c](src/tests/userprog/child-good.c)
- [tests/userprog/exec-many.c](src/tests/userprog/exec-many.c)
- [tests/userprog/exec-many.ck](src/tests/userprog/exec-many.ck)
- [tests/userprog/open-many.c](src/tests/userprog/open-many.c)
- [tests/userprog/open-many.ck](src/tests/userprog/open-many.ck)
- [tests/userprog/practice.c](src/tests/userprog/practice.c)
- [tests/userprog/practice.ck](src/tests/userprog/practice.ck)

#### Raw Log
```shell
$ git diff --stat 933b1b846ecc8440fedf9423e52d61600e30e3a7 ffc0abd008ec0f5506fec4bb31e16eb0b43135d6
 .travis.yml                        |  15 +
 README.md                          |  71 ++++
 README.txt                         |   2 -
 install.sh                         |  18 +
 reports/pro2/exec-many1.png        | Bin 0 -> 593502 bytes
 reports/pro2/exec-many2.png        | Bin 0 -> 697311 bytes
 reports/pro2/open-many2.png        | Bin 0 -> 838010 bytes
 reports/pro2/open_many1.png        | Bin 0 -> 844416 bytes
 reports/pro2/push_stack.png        | Bin 0 -> 83212 bytes
 reports/project2.md                | 663 +++++++++++++++++++++++++++++++++++++
 reports/project2.pdf               | Bin 0 -> 2939889 bytes
 src/lib/syscall-nr.h               |   1 +
 src/lib/user/syscall.c             |   6 +
 src/lib/user/syscall.h             |   1 +
 src/tests/tests.pm                 |   4 +-
 src/tests/threads/alarm-priority.c |   8 +-
 src/tests/userprog/Make.tests      |  14 +-
 src/tests/userprog/child-good.c    |  15 +
 src/tests/userprog/exec-many.c     |  39 +++
 src/tests/userprog/exec-many.ck    |   8 +
 src/tests/userprog/open-many.c     |  49 +++
 src/tests/userprog/open-many.ck    |   8 +
 src/tests/userprog/practice.c      |  19 ++
 src/tests/userprog/practice.ck     |   8 +
 src/threads/thread.c               |  40 ++-
 src/threads/thread.h               |  73 ++++
 src/userprog/exception.c           |  15 +
 src/userprog/process.c             | 247 +++++++++++++-
 src/userprog/process.h             |   2 +
 src/userprog/syscall.c             | 425 +++++++++++++++++++++++-
 src/userprog/syscall.h             |   8 +
 31 files changed, 1731 insertions(+), 28 deletions(-)
```
