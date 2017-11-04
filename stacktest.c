#include "types.h"
#include "stat.h"
#include "user.h"

int fibonacci(int);

void stack_overflow_test(void) {
  int num = 2147483647;

  fibonacci(num);
}

int fibonacci(int num) {
  if (0 == num)
    return 0;
  if (1 == num)
    return 1;
  return fibonacci(num - 1) + fibonacci(num - 2);
}

int main(void) {
  stack_overflow_test();

  exit();
}
