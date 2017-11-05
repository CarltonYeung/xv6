#include "types.h"
#include "stat.h"
#include "user.h"

void foo(int);

void stack_overflow_test(void) {
  // 260 * (sizeof(int) * 1024) = ~1MB stack space
  // program should run out of stack space before num == 0
  int num = 0;
  foo(num);
}

void foo(int num) {
  printf(1, "stack page = %d\n", num);

  int x[1024]; // ~PGSIZE stack space
  x[0] = 0; // bypass compiler warnings
  x[0]++; // bypass compiler warnings

  foo(num + 1);
}

int main(void) {
  stack_overflow_test();

  exit();
}
