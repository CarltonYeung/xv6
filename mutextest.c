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
	int goal = 100000;

	shared_var *s = (shared_var *)shmbrk(PGSIZE);
	s->count = 0;
	mutex_init(&s->m);

	for (i = 0; i < 2; i++) {
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

	for (i = 0; i < 2; i++)
		wait();

	if (s->count != 2 * goal) {
		printf(1, "count should be %d: %d\n", 2 * goal, s->count);
		exit();
	}

	shmbrk(-1);

	printf(1, "children add to shared int test OK\n");
}

int main(void) {
	children_add_shared_int();

	exit();
}
