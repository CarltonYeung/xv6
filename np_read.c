#include "types.h"
#include "stat.h"
#include "user.h"

int main(void) {
  char* np = 0;

  // Write to np whatever the user enters in stdin
  read(0, np, 4096);

  // Read from np to print whatever the user entered
  write(1, np, 4096);

  exit();
}
