#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  *(char *)0 += 1;
  exit();
}
