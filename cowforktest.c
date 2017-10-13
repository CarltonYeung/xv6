#include "types.h"
#include "stat.h"
#include "user.h"

const int number = 306;

/*
 * Test that a child cannot write to its parent's physical page.
 */
void
cowforktest1(void)
{
  int n, pid;
  int my_number = number;

  printf(1, "cow fork test #1\n");
  printf(1, "pre-fork: parent's number is %d\n", my_number);

  for(n = 1; n <= 10; n++){
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
    printf(1, "child modified parent address space\n");
    exit();
  }

  printf(1, "cow fork test #1 OK\n");
}

/*
 * Nested version of cowforktest1.
 * Test that a grandchild cannot write to its parent's physical page.
 * i.e. Test that nested forking preserves the parent's physical pages.
 */
void
cowforktest2(void)
{
  int pid;
  int my_number = number;

  printf(1, "cow fork test #2\n");
  printf(1, "parent's number is %d\n", my_number);

  pid = fork();
  if(pid == 0){
    my_number++;
    printf(1, "child changes its number to %d\n", my_number);

    pid = fork();
    if(pid == 0){
      my_number++;
      printf(1, "grandchild changes its number to %d\n", my_number);
      exit();
    }

    wait();

    printf(1, "post-fork: child's number is %d\n", my_number);
    if(my_number != number + 1){
      printf(1, "grand child modified child's address space\n");
      exit();
    }

    exit();
  }

  wait();

  printf(1, "post-fork: parent's number is %d\n", my_number);
  if(my_number != number){
    printf(1, "child modified parent address space\n");
    exit();
  }

  printf(1, "cow fork test #2 OK\n");
}

int
main(void)
{
  cowforktest1();
  cowforktest2();
  exit();
}
