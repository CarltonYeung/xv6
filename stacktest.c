#include "types.h"
#include "stat.h"
#include "user.h"

void foo(int num) {
  printf(1, "page %d\n", num);

  int numOfPages = 1; // how many pages to allocate at a time?
  char x[4096 * numOfPages];
  x[0] = 0; // bypass compiler warnings
  x[0]++; // bypass compiler warnings

  foo(num + numOfPages);
}

void stack_overflow_test(void) {
  printf(1, "\nstack_overflow_test\n");

  // program should be terminated after using all the stack space
  int num = 1;
  foo(num);
  printf(1, "This should never print\n");
}

int main(void) {
  stack_overflow_test();

  exit();
}
