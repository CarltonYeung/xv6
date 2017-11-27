#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"


typedef struct {
  int val;
  int sync;
} sync_t;

void futex_wait_bad(void) {
	// Test that the return value of waiting with a bad value is -1
	printf(1, "wait with bad value test\n");

	int res;
	sync_t *locks;

	shmbrk(-1);
	locks = (sync_t *)shmbrk(sizeof(sync_t));
	memset(locks, 0, sizeof(locks));

	// wait with bad value; should not sleep; doesn't need to be woken
	res = futex_wait(&locks->val, 1);
	if (res != -1) {
		printf(1, "res should be -1: %d\n", res);
		exit();
	}

	printf(1, "wait with bad value test OK\n");
}

void futex_wait_good(void) {
	// Test that the return value of waiting with a good value is 0
	printf(1, "wait with good value test\n");

	int pid;
	int res;
	sync_t *locks;

	shmbrk(-1);
	locks = (sync_t *)shmbrk(sizeof(sync_t));
	memset(locks, 0, sizeof(locks));

	// wait with good value; should sleep; needs to be woken
	pid = fork();
	if (0 != pid) {
		res = futex_wait(&locks->val, 0);
		if (res != 0) {
			printf(1, "wait: res should be 0\n");
			exit();
		}
		wait();
	} else {
		locks->val = 1;
		// Also test that wake value is 0
		res = futex_wake(&locks->val);
		if (res != 0) {
			printf(1, "wake: res should be 0\n");
		}
		exit();
	}

	printf(1, "wait with good value test OK\n");
}

int
main(int argc, char *argv[]) {
	printf(1, "original futextest\n");
	int pid;
	int res;

	printf(1, "parent before waiting\n");
	sync_t *my_struct = (sync_t *) shmbrk(4096);
	memset(my_struct, 0, sizeof(my_struct));

	// fork and continue in the child
	pid = fork();
	if (0 != pid) {
		// parent going to sleep
		printf(1, "parent waiting with bad value\n");
		res = futex_wait(&my_struct->val, 2);
		printf(1, "parent res was %d\n", res);

		printf(1, "parent waking sleeping child\n");
		my_struct->sync = 1;
		res = futex_wake(&my_struct->sync);
		printf(1, "parent res was %d\n", res);

		printf(1, "parent waiting with good value\n");
		res = futex_wait(&my_struct->val, 0);
		printf(1, "parent res was %d\n", res);
	}
	else {
		printf(1, "child waiting to be woken up\n");
		res = futex_wait(&my_struct->sync, 0);
		printf(1, "child res was %d\n", res);

		printf(1, "child waking sleeping parent\n");
		my_struct->val = 1;
		res = futex_wake(&my_struct->val);
		printf(1, "child res was %d\n", res);

		printf(1, "child signing off\n");
		exit();
	}
	wait();

	printf(1, "parent signing off\n");
	printf(1, "original futextest OK\n");

	// Additional tests
	futex_wait_bad();
	futex_wait_good();

	exit();
}
