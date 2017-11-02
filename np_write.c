#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  char* np = 0;

  // Write wolfie to null pointer
  wolfie(np, 649);

  // Read wolfie from null pointer
  write(1, np, 649);

  exit();
}
