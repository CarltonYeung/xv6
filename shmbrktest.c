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

void shmbrk_multiple_forks(void) {
	// Test nested forks and a second fork directly from parent after nested forks

	printf(1, "shmbrk nested cow fork test\n");

	char *first;
	char *temp;
	int pid;

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

	buf = shmbrk(0);

	ret = read(0, buf, 1);
	if (ret >= 0) {
		printf(1, "read should have returned a negative value\n");
		exit();
	}

	printf(1, "shmbrk invalid read test OK\n");
}

//void shmbrk_valid_read(void) {
//	// Test valid reads from SHM area
//
//	printf(1, "shmbrk valid read test\n");
//
//	char *buf;
//	int ret;
//
//	buf = shmbrk(1);
//
//	ret = read(0, buf, 1);
//	if (ret < 0) {
//		printf(1, "read should have returned a positive value\n");
//		exit();
//	}
//
//	printf(1, "shmbrk valid read test OK\n");
//}

void shmbrk_overflow(void) {
	// Test that trying to allocate past SHM_MAX is not allowed
}

int main(void) {
	shmbrk_allocate();
	shmbrk_deallocate();
	shmbrk_fork();
	shmbrk_multiple_forks();
	shmbrk_invalid_read();
//	shmbrk_valid_read();
	shmbrk_overflow();

	exit();
}
