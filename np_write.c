#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  int *np = 0;
//  *np = 306;
  wolfie(np, 649);

  printf(1, "This should never print.\n");

  exit();
}
