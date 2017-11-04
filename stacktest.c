#include "types.h"
#include "stat.h"
#include "user.h"

void foo(int);

void stack_overflow_test(void) {
  foo(10000);
}

void foo(int num) {
  if (num > 0) {
    int x[1024];
    x[0] = 0;
    x[0]++;

    foo(num - 1);
  }
}

int main(void) {
  stack_overflow_test();

  exit();
}
