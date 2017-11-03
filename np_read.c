#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  write(1, (void *)0, 4096);
  exit();
}
