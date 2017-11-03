#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  char* np = 0;

  // Read the first page of user virtual memory
  write(1, np, 4096);

  exit();
}
