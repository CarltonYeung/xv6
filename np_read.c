#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  char *np = 0;
  printf(1, "%s\n", *np);

  printf(1, "This should never print.\n");

  exit();
}
