#include "param.h"
#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"
#include "syscall.h"
#include "traps.h"
#include "memlayout.h"

#define PGSIZE 4096

typedef struct {
  int val;
  int sync;
} sync_t;

typedef struct {
	mutex_t m;
	volatile int count;
} shared_var;

typedef struct {
	mutex_t m1;
	mutex_t m2;
} two_locks;

void
futex_wait_bad(void)
{
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

void
futex_wait_good(void)
{
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

void
futex_wait_not_SHM(void)
{
	printf(1, "wait with lock not in SHM area test\n");

	int res;

	shmbrk(-1);

	sync_t *locks = (sync_t *)shmbrk(1);

	// Before valid SHM area
	locks = (sync_t *)((char *)locks - 1);
	res = futex_wait(&locks->val, 0);
	if (res != -1) {
		printf(1, "lock is before SHM area: return value should be -1\n");
		exit();
	}

	// After valid SHM area
	shmbrk(-1);
	locks = (sync_t *)shmbrk(0);
	res = futex_wait(&locks->val, 0);
	if (res != -1) {
		printf(1, "lock is after SHM area: return value should be -1\n");
		exit();
	}

	shmbrk(-1);

	printf(1, "wait with lock not in SHM area test OK\n");
}

void
futex_wake_not_SHM(void)
{
	printf(1, "wake with lock not in SHM area test\n");

	int res;

	shmbrk(-1);

	sync_t *locks = (sync_t *)shmbrk(1);

	// Before valid SHM area
	locks = (sync_t *)((char *)locks - 1); // 1 byte before SHM area
	res = futex_wake(&locks->val);
	if (res != -1) {
		printf(1, "lock is before SHM area: return value should be -1\n");
		exit();
	}

	// After valid SHM area
	shmbrk(-1);
	locks = (sync_t *)shmbrk(0);
	res = futex_wake(&locks->val);
	if (res != -1) {
		printf(1, "lock is after SHM area: return value should be -1\n");
		exit();
	}

	shmbrk(-1);

	printf(1, "wake with lock not in SHM area test OK\n");
}

void
children_add_shared_int(void)
{
	printf(1, "children add to shared int test\n");

	shmbrk(-1);

	int pid;
	int i, j;
	const int goal = 100000;
	const int children = 5;

	shared_var *s = (shared_var *)shmbrk(PGSIZE);
	s->count = 0;
	mutex_init(&s->m);

	for (i = 0; i < children; i++) {
		pid = fork();
		if (0 == pid) {
			for (j = 0; j < goal; j++) {
				mutex_lock(&s->m);
				s->count++;
				mutex_unlock(&s->m);
			}
			exit();
		}
	}

	for (i = 0; i < children; i++)
		wait();

	if (s->count != 5 * goal) {
		printf(1, "count should be %d: %d\n", children * goal, s->count);
		exit();
	}

	shmbrk(-1);

	printf(1, "children add to shared int test OK\n");
}

void
children_add_shared_int_trylock(void)
{
	printf(1, "children add to shared int trylock test\n");

	shmbrk(-1);

	int pid;
	int i, j;
	const int goal = 100000;
	const int children = 5;

	shared_var *s = (shared_var *)shmbrk(PGSIZE);
	s->count = 0;
	mutex_init(&s->m);

	for (i = 0; i < children; i++) {
		pid = fork();
		if (0 == pid) {
			for (j = 0; j < goal; j++) {
				while (mutex_trylock(&s->m) < 0);
				s->count++;
				mutex_unlock(&s->m);
			}
			exit();
		}
	}

	for (i = 0; i < children; i++)
		wait();

	if (s->count != 5 * goal) {
		printf(1, "count should be %d: %d\n", children * goal, s->count);
		exit();
	}

	shmbrk(-1);

	printf(1, "children add to shared int trylock test OK\n");
}

void
futex_man_pages_demo(void)
{
	printf(1, "futex man pages demo\n");
	printf(1, "Parent and Child should alternate in order\n");

	int pid;
	int j;
	int nloops;

	nloops = 100;

	two_locks *locks = (two_locks *)shmbrk(sizeof(two_locks));

	// Initially unavailable to child
	mutex_init(&locks->m1);
	mutex_lock(&locks->m1);

	// Initially available to parent
	mutex_init(&locks->m2);

	pid = fork();
	if (-1 == pid) {
		printf(1, "fork returned -1\n");
		exit();
	}

	if (0 == pid) {
		for (j = 0; j < nloops; j++) {
			mutex_lock(&locks->m1);
			printf(1, "Child (%d) %d\n", getpid(), j);
			mutex_unlock(&locks->m2);
		}
		exit();
	}

	for (j = 0; j < nloops; j++) {
		mutex_lock(&locks->m2);
		printf(1, "Parent (%d) %d\n", getpid(), j);
		mutex_unlock(&locks->m1);
	}

	wait();

	printf(1, "futex man pages demo OK\n");
}

void
futex_man_pages_demo_trylock(void)
{
	printf(1, "futex man pages demo trylock\n");
	printf(1, "Parent and Child should alternate in order\n");

	int pid;
	int j;
	int nloops;

	nloops = 100;

	two_locks *locks = (two_locks *)shmbrk(sizeof(two_locks));

	// Initially unavailable to child
	mutex_init(&locks->m1);
	while (mutex_trylock(&locks->m1) < 0);

	// Initially available to parent
	mutex_init(&locks->m2);

	pid = fork();
	if (-1 == pid) {
		printf(1, "fork returned -1\n");
		exit();
	}

	if (0 == pid) {
		for (j = 0; j < nloops; j++) {
			while (mutex_trylock(&locks->m1) < 0);
			printf(1, "Child (%d) %d\n", getpid(), j);
			mutex_unlock(&locks->m2);
		}
		exit();
	}

	for (j = 0; j < nloops; j++) {
		while (mutex_trylock(&locks->m2) < 0);
		printf(1, "Parent (%d) %d\n", getpid(), j);
		mutex_unlock(&locks->m1);
	}

	wait();

	printf(1, "futex man pages demo trylock OK\n");
}

int
main(int argc, char *argv[])
{
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

	// Additional futex tests
	futex_wait_bad();
	futex_wait_good();
	futex_wait_not_SHM();
	futex_wake_not_SHM();

	// Mutex tests
	children_add_shared_int();
	children_add_shared_int_trylock();
	futex_man_pages_demo(); // http://man7.org/linux/man-pages/man2/futex.2.html
	futex_man_pages_demo_trylock();

	exit();
}
