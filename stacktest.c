#include "types.h"
#include "stat.h"
#include "user.h"

void foo(int);

void stack_overflow_test(void) {
  // program should be terminated after using all the stack space
  int num = 1;
  foo(num);
}

void foo(int num) {
  printf(1, "page %d\n", num);

  int numOfPages = 1; // how many pages to allocate at a time?
  char x[4096 * numOfPages];
  x[0] = 0; // bypass compiler warnings
  x[0]++; // bypass compiler warnings

  foo(num + numOfPages);
}

int main(void) {
  stack_overflow_test();

  exit();
}
