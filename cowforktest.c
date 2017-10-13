#include "types.h"
#include "stat.h"
#include "user.h"

void
cowforktest1(void)
{
  int n, pid;
  const int number = 306;

  int my_number = number;

  printf(1, "cow fork test #1\n");
  printf(1, "pre-fork: parent's number is %d\n", my_number);

    for(n=1; n<=10; n++){
      pid = fork();
      if(pid < 0)
        break;
      if(pid == 0){
        my_number += n;
        printf(1, "child changes its number to %d\n", my_number);
        exit();
      }
      wait();
    }

    printf(1, "post-fork: parent's number is %d\n", my_number);

    if(my_number != number){
      printf(1, "child process modified parent address space\n");
      exit();
    }

    printf(1, "cow fork test #1 OK\n");
}

int
main(void)
{
  cowforktest1();
  exit();
}
