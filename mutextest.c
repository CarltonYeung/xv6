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

	shared_var *s = (shared_var *)shmbrk(sizeof(shared_var));
	s->count = 0;
	mutex_init(&s->m);

	if ((&s->m)->flag != 0) {
		printf(1, "mutex flag should be initialized to 0: %d\n", (&s->m)->flag);
		exit();
	}

	for (i = 0; i < 2; i++) {
		pid = fork();
		if (0 == pid) {
			for (j = 0; j < 10000000; j++) {
				mutex_lock(&s->m);
				s->count = s->count + 1;
				mutex_unlock(&s->m);
			}
			exit();
		}
	}

//	for (i = 0; i < 2; i++)
//		wait();

	if (s->count != 20000000) {
		printf(1, "count should be 20,000,000: %d\n", s->count);
		exit();
	}

	shmbrk(-1);

	printf(1, "children add to shared int test OK\n");
}

int main(void) {
	children_add_shared_int();

	exit();
}
