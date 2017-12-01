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

void checksum_single_process(void) {
	int fd;
	char checksum;
	char token;
	int ret;
	int nloops;

	checksum = 0;

	for (nloops = 0; nloops < 4; nloops++) {
		fd = open("README", O_RDONLY);
		if (fd < 0) {
			printf(2, "open failed\n");
			exit();
		}

		do {
			ret = read(fd, &token, sizeof(char));
			if (ret < 0) {
				printf(2, "read failed\n");
				exit();
			}

//			printf(1, "%c", token);

			checksum = checksum + token;
		} while (ret != 0);

		close(fd);
	}

	printf(1, "README (x4) single process checksum = %d\n", checksum);
}

typedef struct {
	char buf[128];
	int head;        // index of head
	int tail;        // index of tail
	int nitems;      // number of items in buf
	int nslots;      // number of empty slots in buf
	mutex_t m;       // mutex to protect shared buffer
	cond_var_t cv;   // condition variable to indicate empty/full buf
	char checksum;   // global checksum
	int done[4];     // producers indicate that they are done producing
} shared_objs;

void init_shared_objs(shared_objs *objs) {
	int i;

	objs->head = 0;
	objs->tail = 0;
	objs->nitems = 0;
	objs->nslots = 128;
	mutex_init(&objs->m);
	cv_init(&objs->cv);
	objs->checksum = 0;

	for (i = 0; i < 4; i++)
		objs->done[i] = 0; // 0 = not done; 1 = done
}

// Return 1 if all producers are done producing, 0 otherwise
int done_producing(int done[4]) {
	int ans = 1;
	int i;

	for (i = 0; i < 4; i++)
		ans &= done[i];

	return ans;
}

void checksum_producers_consumers(void) {
	int pid;
	int i;

	shared_objs *so = (shared_objs *)shmbrk(sizeof(shared_objs));
	init_shared_objs(so);

	// fork 4 producers
	for (i = 0; i < 4; i++) {
		pid = fork();
		if (pid < 0) {
			printf(2, "producer fork failed\n");
			exit();
		}

		if (0 == pid) {
			// Producer local variables
			int fd;
			int nread;

			fd = open("README", O_RDONLY);
			if (fd < 0) {
				printf(2, "open failed\n");
				exit();
			}

			do {
				mutex_lock(&so->m);
				while (so->nslots < 12)
					cv_wait(&so->cv, &so->m);

				// Insert to shared buffer
				nread = read(fd, &so->buf[so->tail], 12);
				if (nread < 0) {
					printf(2, "read failed\n");
					exit();
				}

				printf(1, "Producer %d inserted %d chars\n", i, nread);

				so->tail = (so->tail + nread) % 128;
				so->nitems += nread;
				so->nslots -= nread;

				cv_bcast(&so->cv);
				mutex_unlock(&so->m);

			} while (nread != 0);

			printf(1, "Producer %d done\n", i);
			so->done[i] = 1; // indicate that it is done producing
			exit();
		}
	}

	// fork 4 consumers
	for (i = 0; i < 4; i++) {
		pid = fork();
		if (pid < 0) {
			printf(2, "consumer fork failed\n");
			exit();
		}

		if (0 == pid) {
			// Consumer local variables
			char sum = 0;
			int nread;

			do {
				mutex_lock(&so->m);
				while (so->nitems == 0)
					cv_wait(&so->cv, &so->m);

				// Remove at most 8 chars from shared buffer
				while (1) {
					if (so->nitems == 0 || nread == 8)
						break;

					sum += so->buf[so->head];
					so->head = (so->head + 1) % 128;
					so->nitems--;
					so->nslots++;

					nread++;
				}

				printf(1, "Consumer %d removed %d chars\n", i, nread);

				cv_bcast(&so->cv);
				mutex_unlock(&so->m);

			} while (so->nitems > 0 || !done_producing(so->done));

			// Update global checksum
			mutex_lock(&so->m);
			so->checksum += sum;
			mutex_unlock(&so->m);

			printf(1, "Consumer %d done\n", i);
			exit();
		}
	}

	// Wait for all children to finish
	for (i = 0; i < 8; i++)
		wait();

	printf(1, "README (x4) producers/consumers checksum = %d\n", so->checksum);
}

int main(void) {
	checksum_single_process();
	checksum_producers_consumers();

	exit();
}
