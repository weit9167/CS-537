#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  printf(1, "Hello World!\n");
  printf(1, "pid = %d\n", getpid());
  printf(1, "ppid = %d\n", getppid());
  exit();
}
