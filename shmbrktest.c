#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE 4096
#define MAX_SHM 0x1000000

void shmbrk_allocate(void) {
	printf(1, "shmbrk allocate test\n");

	char *first;
	char *last;

	shmbrk(-1);

	// shmbrk(positive) and shmbrk(0)
	first = shmbrk(1);
	if ((int)first < 0) {
		printf(1, "shmbrk returned negative value\n");
		exit();
	}

	last = shmbrk(0);
	if ((int)last < 0) {
		printf(1, "shmbrk returned negative value\n");
		exit();
	}

	// Make sure one page is allocated
	if (last - first != PGSIZE) {
		printf(1, "shmbrk did not allocate PGSIZE bytes: %d\n", last - first);
		exit();
	}

	shmbrk(-1);

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

	shmbrk(-1);

	printf(1, "shmbrk deallocate test OK\n");
}

void shmbrk_fork(void) {
	// Test SHM area forking
	printf(1, "shmbrk cow fork test\n");

	char *first;
	char *temp;
	int pid;

	shmbrk(-1);

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

void shmbrk_multiple_forks(void) {
	// Test nested forks and a second fork directly from parent after nested forks
	printf(1, "shmbrk nested cow fork test\n");

	char *first;
	char *temp;
	int pid;

	shmbrk(-1);

	first = shmbrk(1);

	// Parent write
	*(int *)first = PGSIZE;

	pid = fork();

	if (0 == pid) {
		// Child read
		if (*(int *)first != PGSIZE) {
			printf(1, "child should be able to read what parent wrote\n");
			exit();
		}

		// Child write
		temp = first + sizeof(int);
		*(int *)temp = 306 * PGSIZE;

		pid = fork();

		if (0 == pid) {
			// Grandchild read
			if (*(int *)temp != 306 * PGSIZE) {
				printf(1, "grandchild should be able to read what parent wrote\n");
				exit();
			}

			if (*(int *)first != PGSIZE) {
				printf(1, "grandchild should be able to read what grandparent wrote\n");
				exit();
			}

			// Grandchild write
			temp = temp + sizeof(int);
			*(int *)temp = PGSIZE - 306;

			exit();
		}

		wait();

		// Child check that its write hasn't been tampered with
		if (*(int *)temp != 306 * PGSIZE) {
			printf(1, "child's write should not have changed\n");
			exit();
		}

		exit();
	}

	wait();

	// Test forking from original parent after nested forks
	pid = fork();

	if (0 == pid) {
		// Child read all previous writes
		temp = first;
		if (*(int *)temp != PGSIZE) {
			printf(1, "brother should be able to read parent's write\n");
			exit();
		}

		temp = temp + sizeof(int);
		if (*(int *)temp != 306 * PGSIZE) {
			printf(1, "brother should be able to read sibling's write\n");
			exit();
		}

		temp = temp + sizeof(int);
		if (*(int *)temp != PGSIZE - 306) {
			printf(1, "brother should be able to read nephew's write\n");
			exit();
		}

		// Brother write
		temp = temp + sizeof(int);
		*(int *)temp = PGSIZE * PGSIZE;

		exit();
	}

	wait();

	// Parent check that its write hasn't been tampered with
	temp = first;
	if (*(int *)temp != PGSIZE) {
		printf(1, "parent's write should not have changed\n");
		exit();
	}

	// Parent read child's write
	temp = temp + sizeof(int);
	if (*(int *)temp != 306 * PGSIZE) {
		printf(1, "parent should be able to read child's write\n");
		exit();
	}

	// Parent read grandchild's write
	temp = temp + sizeof(int);
	if (*(int *)temp != PGSIZE - 306) {
		printf(1, "parent should be able to read grandchild's write\n");
		exit();
	}

	// Parent read second child's write
	temp = temp + sizeof(int);
	if (*(int *)temp != PGSIZE * PGSIZE) {
		printf(1, "parent should be able to read second child's write\n");
		exit();
	}

	// Deallocate
	shmbrk(-1);

	printf(1, "shmbrk nested cow fork test OK\n");
}

void shmbrk_invalid_read(void) {
	// Test that read should return negative value for unallocated SHM area
	printf(1, "shmbrk invalid read test\n");

	char *buf;
	int ret;

	shmbrk(-1);

	buf = shmbrk(0);

	ret = read(0, buf, 1);
	if (ret >= 0) {
		printf(1, "read should have returned a negative value\n");
		exit();
	}

	shmbrk(-1);

	printf(1, "shmbrk invalid read test OK\n");
}

void shmbrk_max_allocate(void) {
	// Test that user can allocate up to MAX_SHM space
	printf(1, "shmbrk max allocate test\n");

	char *first;
	char *last;
	char *temp;
	int i;

	shmbrk(-306);

	// Allocate MAX_SHM all at once
	first = shmbrk(MAX_SHM);
	if ((int)first < 0) {
		printf(1, "1: shmbrk should have returned a positive value: %d\n", first);
		exit();
	}

	last = shmbrk(0);
	if (last - first != MAX_SHM) {
		printf(1, "1: shmbrk should have allocated a total of MAX_SHM bytes: %d\n", last - first);
		exit();
	}

	shmbrk(-306);

	// Allocate MAX_SHM over multiple calls
	for (i = 0; i < PGSIZE; i++) {
		temp = shmbrk(MAX_SHM / PGSIZE);
		if ((int)temp < 0) {
			printf(1, "2: shmbrk should have returned a positive value: %d\n", temp);
			exit();
		}
	}

	last = shmbrk(0);
	if (last - first != MAX_SHM) {
		printf(1, "2: shmbrk should have allocated a total of MAX_SHM bytes: %d\n", last - first);
		exit();
	}

	shmbrk(-306);

	printf(1, "shmbrk max allocate test OK\n");
}

void shmbrk_overflow(void) {
	// Test that trying to allocate past MAX_SHM causes a negative return value
	printf(1, "shmbrk overflow test\n");

	char *first;

	// Deallocate SHM to reset shm_break
	shmbrk(-1);

	first = shmbrk(MAX_SHM + 1);

	if ((int)first >= 0) {
		printf(1, "shmbrk should have returned a negative value: %d\n", first);
		exit();
	}

	shmbrk(-1);

	printf(1, "shmbrk overflow test OK\n");
}

int main(void) {
	shmbrk_allocate();
	shmbrk_deallocate();
	shmbrk_fork();
	shmbrk_multiple_forks();
	shmbrk_invalid_read();
	shmbrk_max_allocate();
	shmbrk_overflow();

	exit();
}
