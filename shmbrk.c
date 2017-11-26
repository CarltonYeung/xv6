#include "types.h"
#include "defs.h"
#include "shmbrk.h"
#include "memlayout.h"
#include "mmu.h"

char* shmbrk(void) {
	cprintf("??\n");
	int n;
	struct proc *curproc = myproc();
	pde_t *pgdir = curproc->pgdir;
	uint new_break;
	uint old_break = curproc->shm_break;
	cprintf("??\n");
	if (argint(0, &n) < 0) {
		cprintf("??\n");
		return -1;
	}

	if (0 == n) {
		cprintf("??\n");
		return (char *)old_break;
	}

	if (n > 0) {
		cprintf("??\n");
		new_break = PGROUNDUP(curproc->shm_break + n);
		if (curproc->shm_first + MAX_SHM == new_break)
			return -1;

		if (0 == allocuvm(pgdir, curproc->shm_break, new_break)) {
			freevm(pgdir);
			return -1;
		}

		curproc->shm_last = new_break - 1;
		curproc->shm_break = new_break;
		return (char *)old_break;
	}

	if (n < 0) {
		cprintf("??\n");
		deallocuvm(pgdir, curproc->shm_break, curproc->shm_first);
		curproc->shm_last = curproc->shm_first;
		curproc->shm_break = curproc->shm_first;
		return (char *)old_break;
	}
}
