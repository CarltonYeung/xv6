#include "types.h"
#include "stat.h"
#include "user.h"

void vdso_getpid_test(void) {
  int i;
  int pid;

  printf(1, "vdso_getpid test\n");

  pid = vdso_getpid();
//  printf(1, "PID = %d\n", pid);

  for (i = 0; i < 1000000; i++) {
    if (vdso_getpid() != pid) {
      printf(1, "pid changed");
      exit();
    }
  }

  printf(1, "vdso_getpid test OK\n");
}

void vdso_getticks_test(void) {
  int i;
  uint cur, prev;

  printf(1, "vdso_getticks test\n");

  cur = vdso_getticks();
//  printf(1, "ticks = %d\n", cur);

  for (i = 0; i < 1000000; i++) {
    prev = cur;
    cur = vdso_getticks();
//    printf(1, "ticks = %d\n", cur);

    if (cur < prev) {
      printf(1, "ticks did not monotonically increase");
      exit();
    }
  }

  printf(1, "vdso_getticks test OK\n");
}

int main(void) {
  vdso_getpid_test();
  vdso_getticks_test();

  exit();
}
