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
#define BUFSIZE 128
#define EOF 0
#define NPRODUCERS 4
#define NCONSUMERS 4
#define PCHUNKSZ 12
#define CCHUNKSZ 8

int
Fork(void)
{
	int pid;

	pid = fork();
	if (pid < 0) {
		printf(2, "fork failed\n");
		exit();
	}

	return pid;
}

int
Open(char *name, int flags)
{
	int fd;

	fd = open(name, flags);
	if (fd < 0) {
		printf(2, "open failed\n");
		exit();
	}

	return fd;
}

int
Read(int fd, void *buf, int size)
{
	int nread;

	nread = read(fd, buf, size);
	if (nread < 0) {
		printf(2, "read failed\n");
		exit();
	}

	return nread;
}

void
checksum_single_process(void)
{
	printf(1, "README (x4) single process checksum\n");

	int fd;
	char checksum;
	char token;
	int nread;
	int i;

	checksum = 0;

	for (i = 0; i < 4; i++) {
		fd = Open("README", O_RDONLY);

		do {
			nread = read(fd, &token, sizeof(char));
//			printf(1, "%c", token);
			checksum += token;
		} while (nread != 0);

		close(fd);
	}

	printf(1, "README (x4) single process checksum = %d\n", checksum);
}

typedef struct {
	char buf[BUFSIZE];
	int head;        // index of first element
	int tail;        // index of last element
	int size;        // number of elements in the queue
} queue;

void init_queue(queue* q) {
	q->head = -1;
	q->tail = -1;
	q->size = 0;
}

void
enqueue(queue *q, char val)
{
	if (q->size == BUFSIZE) {
		printf(2, "queue is full\n");
	} else if (q->head == -1) {
		q->head = 0;
		q->tail = 0;
		q->buf[q->tail] = val;
		q->size++;
	} else {
		q->tail = (q->tail + 1) % BUFSIZE;
		q->buf[q->tail] = val;
		q->size++;
	}
}

char
dequeue(queue *q)
{
	char val;

	if (q->head == -1) {
		printf(2, "queue is empty\n");
		return -1;
	} else {
		val = q->buf[q->head];
		q->head = (q->head + 1) % 128;
		q->size--;

		if (q->size == 0) {
			q->head = -1;
			q->tail = -1;
		}

		return val;
	}
}

// Returns 1 if done[i] == 1 for all i
int
producers_done(int done[], int size)
{
	int val = done[0];
	int i;

	for (i = 1; i < size; i++)
		val &= done[i];

	return val;
}

void
checksum_producers_consumers(void)
{
	printf(1, "README (x4) producers/consumers checksum\n");

	int pid;
	int i;

	// Shared data
	char checksum;
	queue *q;
	mutex_t *lock;          // this lock is used to protect all shared data
	cond_var_t *non_empty;
	cond_var_t *non_full;
	int done[NPRODUCERS];   // consumers need to know when producers are all done

	checksum = 0;

	q = (queue *)shmbrk(PGSIZE);
	init_queue(q);

	lock = (mutex_t *)((char *)q + sizeof(queue));
	mutex_init(lock);

	non_empty = (cond_var_t *)((char *)lock + sizeof(mutex_t));
	cv_init(non_empty);

	non_full = (cond_var_t *)((char *)non_empty + sizeof(cond_var_t));
	cv_init(non_full);

	memset(done, 0, NPRODUCERS * sizeof(int));

	// fork producers
	for (i = 0; i < NPRODUCERS; i++) {
		pid = Fork();
		if (pid == 0) {
			// Local data
			int fd;
			char readme[12];
			int nread;
			int j;

			fd = Open("README", O_RDONLY);

			// Read and append 12 byte chunks (from README to shared buffer)
			for (nread = Read(fd, readme, PCHUNKSZ); nread != EOF; nread = Read(fd, readme, PCHUNKSZ)) {
				mutex_lock(lock);
				while (nread > BUFSIZE - q->size) {
					printf(1, "Producer %d waiting\n", i);
					cv_wait(non_full, lock);
					printf(1, "Producer %d waking up\n", i);
				}

				for (j = 0; j < nread; j++)
					enqueue(q, readme[j]);

				printf(1, "Producer %d enqueued %d chars\n", i, nread);

				cv_bcast(non_empty);
				mutex_unlock(lock);
			}

			// Clean up
			close(fd);

			mutex_lock(lock);
			done[i] = 1;
			mutex_unlock(lock);

			printf(1, "Producer %d finished <--------------------\n", i);
			exit();
		}
	}

	// fork 4 consumers
	for (i = 0; i < NCONSUMERS; i++) {
		pid = Fork();
		if (pid == 0) {
			// Local data
			char sum;
			int nitems;
			int j;

			sum = 0;

			// Read and sum 8 byte chunks (from shared buffer to local sum
			while (1) {
				mutex_lock(lock);
				while (q->size == 0 && !producers_done(done, NPRODUCERS)) {
					printf(1, "Consumer %d waiting\n", i);
					cv_wait(non_empty, lock);
					printf(1, "Consumer %d waking up\n", i);
				}

				nitems = (q->size > CCHUNKSZ)? CCHUNKSZ : q->size;
				for (j = 0; j < nitems; j++)
					sum += dequeue(q);

				printf(1, "Consumer %d dequeued %d chars\n", i, nitems);

				if (q->size == 0 && producers_done(done, NPRODUCERS)) {
					mutex_unlock(lock);
					break;
				}

				cv_bcast(non_full);
				mutex_unlock(lock);
			}

			// Add to global checksum
			mutex_lock(lock);
			checksum += sum;
			mutex_unlock(lock);

			printf(1, "Consumer %d finished <--------------------\n", i);
			exit();
		}
	}

	// Parent continues here
	for (i = 0; i < NPRODUCERS + NCONSUMERS; i++)
		wait();

	printf(1, "README (x4) producers/consumers checksum = %d\n", checksum);
}

int
main(void)
{
	checksum_single_process();
	checksum_producers_consumers();

	exit();
}
