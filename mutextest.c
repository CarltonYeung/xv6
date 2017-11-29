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
	mutex_t m;
	volatile int count;
} shared_var;

void children_add_shared_int(void) {
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

void children_add_shared_int_trylock(void) {
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

typedef struct {
	mutex_t m1;
	mutex_t m2;
} two_locks;

void futex_man_pages_demo(void) {
	printf(1, "futex man pages demo\n");

	int pid;
	int j;
	int nloops;

	nloops = 5;

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

int main(void) {
	children_add_shared_int();
	children_add_shared_int_trylock();
	futex_man_pages_demo(); // http://man7.org/linux/man-pages/man2/futex.2.html

	exit();
}
