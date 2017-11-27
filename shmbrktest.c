#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4096

void shmbrk_allocate(void) {
	printf(1, "shmbrk allocate test\n");

	char *first;
	char *last;

	// shmbrk(positive) and shmbrk(0)
	first = shmbrk(1);
	if (first < 0) {
		printf(1, "shmbrk returned negative value\n");
		exit();
	}

	last = shmbrk(0);
	if (last < 0) {
		printf(1, "shmbrk returned negative value\n");
		exit();
	}

	// Make sure one page is allocated
	if (last - first != PGSIZE) {
		printf(1, "shmbrk didn not allocate the correct amount on memory\n");
		exit();
	}

	printf(1, "shmbrk allocate test OK\n");
}

void shmbrk_deallocate(void) {
	printf(1, "shmbrk deallocate test\n");

	char *first;
	char *last;
	char *temp;

	// deallocate everything from previous tests
	shmbrk(-1);

	// test deallocation again
	first = shmbrk(1);
	last = shmbrk(0);

	temp = shmbrk(-1);
	if (temp != last) {
		printf(1, "shmbrk should have returned previous value of break\n");
		exit();
	}

	temp = shmbrk(0);
	if (temp != first) {
		printf(1, "shmbrk should have reset break to its original, unallocated location\n");
		exit();
	}

	printf(1, "shmbrk deallocate test OK\n");
}

void shmbrk_fork(void) {
	// Test SHM area forking

	printf(1, "shmbrk cow fork test\n");

	char *first;
	char *temp;
	int pid;

	first = shmbrk(1);

	// Parent write to SHM area
	*(int *)first = PGSIZE;

	pid = fork();

	if (0 == pid) {
		// Child read from SHM area
		if (*(int *)first != PGSIZE) {
			printf(1, "child should be able to read what parent wrote\n");
			exit();
		}

		// Child write to SHM area
		temp = first + sizeof(int);
		*(int *)temp = 306 * PGSIZE;
		exit();
	}

	wait();

	// Parent check that its write hasn't been tampered with
	if (*(int *) first != PGSIZE) {
		printf(1, "parent's write should not have changed\n");
		exit();
	}

	// Parent read from SHM area
	temp = first + sizeof(int);
	if (*(int *)temp != 306 * PGSIZE) {
		printf(1, "parent should be able to read what child wrote\n");
		exit();
	}

	// Deallocate
	shmbrk(-1);

	printf(1, "shmbrk cow fork test OK\n");
}

void shmbrk_nested_forks(void) {
	// Test that nested forks allow inter-process communication
}

void shmbrk_overflow(void) {
	// Test that trying to allocate past SHM_MAX is not allowed
}

int main(void) {
	shmbrk_allocate();
	shmbrk_deallocate();
	shmbrk_fork();

	exit();
}
