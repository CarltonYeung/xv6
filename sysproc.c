#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_shmbrk(void)
{
	int n;
	struct proc *curproc = myproc();
	pde_t *pgdir = curproc->pgdir;
	int new_break;
	int old_break = curproc->shm_break;

	if (argint(0, &n) < 0) {
		return -1;
	}

	if (0 == n) {
		return old_break;
	}

	if (n > 0) {
		new_break = PGROUNDUP(curproc->shm_break + n);

//		cprintf("shm_break = %d\n", curproc->shm_break);
//		cprintf("n = %d\n", n);
//		cprintf("new_break = %d\n", new_break);
//		cprintf("shm_first = %d\n", curproc->shm_first);

		if (new_break - curproc->shm_first > MAX_SHM) {
//			cprintf("returning -1\n");
			return -1;
		}
		new_break = allocuvm(pgdir, curproc->shm_break, new_break);

		if (0 == new_break) {
			freevm(pgdir);
			return -1;
		}

		curproc->shm_last = new_break - 1;
		curproc->shm_break = new_break;
	}

	if (n < 0) {
		deallocuvm(pgdir, curproc->shm_break, curproc->shm_first);
		curproc->shm_last = curproc->shm_first;
		curproc->shm_break = curproc->shm_first;
	}

	return old_break;
}

int
sys_futex_wait(void)
{
  // LAB 4: Your Code Here
  return 0xDEADBEAF;
}

int
sys_futex_wake(void)
{
  // LAB 4: Your Code Here
  return 0xDEADBEAF;
}
