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

char
checksum_single_process(void)
{
	printf(1, "Single process checksum\n");

	int fd;          // README file descriptor
	char checksum;   // byte-level checksum of 4 README files
	char token;      // current byte
	int nread;       // return value of Read() needed to check for EOF
	int i;           // number of README files

	checksum = 0;

	for (i = 0; i < NPRODUCERS; i++) {
		fd = Open("README", O_RDONLY);

		for (nread = Read(fd, &token, 1); nread != EOF; nread = Read(fd, &token, 1))
			checksum += token;

		close(fd);
	}

	printf(1, "Single process checksum OK\n");

	return checksum;
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

char
checksum_producers_consumers(void)
{
	printf(1, "Producers/consumers checksum\n");

	// Parent's local data
	int pid;
	int i;

	// Shared data
	char *checksum;         // global byte-level sum
	queue *q;               // shared buffer
	mutex_t *lock;          // lock used to protect all shared data
	cond_var_t *non_empty;
	cond_var_t *non_full;
	int *ndone;             // number of producers that are done adding to the buffer

	// All shared data should live in SHM area
	checksum = (char *)shmbrk(PGSIZE);
	*checksum = 0;

	q = (queue *)((char *)checksum + sizeof(char));
	init_queue(q);

	lock = (mutex_t *)((char *)q + sizeof(queue));
	mutex_init(lock);

	non_empty = (cond_var_t *)((char *)lock + sizeof(mutex_t));
	cv_init(non_empty);

	non_full = (cond_var_t *)((char *)non_empty + sizeof(cond_var_t));
	cv_init(non_full);
	non_full->done = 1;

	ndone = (int *)((char *)non_full + sizeof(cond_var_t));
	*ndone = 0;

	// Fork producers
	for (i = 0; i < NPRODUCERS; i++) {
		pid = Fork();
		if (pid == 0) {
			// Producers' local data
			int fd;
			char readme[12];   // intermediary between file and shared buffer
			int nread;
			int j;

			fd = Open("README", O_RDONLY);

			// Read and append PCHUNKSZ byte chunks (from README to shared buffer)
			for (nread = Read(fd, readme, PCHUNKSZ); nread != EOF; nread = Read(fd, readme, PCHUNKSZ)) {
				mutex_lock(lock);

				while (nread > BUFSIZE - q->size) {
//					printf(1, "Producer %d waiting\n", i);
					cv_wait(non_full, lock);
//					printf(1, "Producer %d waking up\n", i);
				}

				for (j = 0; j < nread; j++)
					enqueue(q, readme[j]);

//				printf(1, "Producer %d enqueued %d chars\n", i, nread);

				cv_bcast(non_empty);
				mutex_unlock(lock);
			}

			// Reached EOF
			close(fd);
			__sync_fetch_and_add(ndone, 1);
//			printf(1, "Producer %d finished\n", i);
			exit();
		}
	}

	// Fork 4 consumers
	for (i = 0; i < NCONSUMERS; i++) {
		pid = Fork();
		if (pid == 0) {
			// Consumers' local data
			char sum;     // consumer's local byte-level sum
			int nitems;
			int j;

			sum = 0;

			// Read and sum 8 byte chunks (from shared buffer to local sum
			while (1) {
				mutex_lock(lock);

				while (q->size == 0 && *ndone < NPRODUCERS) {
//					printf(1, "Consumer %d waiting\n", i);
					cv_wait(non_empty, lock);
//					printf(1, "Consumer %d waking up\n", i);
				}

				nitems = (q->size > CCHUNKSZ)? CCHUNKSZ : q->size;
				for (j = 0; j < nitems; j++)
					sum = sum + dequeue(q);

//				printf(1, "Consumer %d dequeued %d chars\n", i, nitems);

				if (q->size == 0 && *ndone == NPRODUCERS) {
					mutex_unlock(lock);
					break;
				}

				cv_bcast(non_full);
				mutex_unlock(lock);
			}

			// Buffer is empty and all producers are done
			__sync_fetch_and_add(checksum, sum);
//			printf(1, "Consumer %d finished\n", i);
			exit();
		}
	}

	// Parent continues here
	for (i = 0; i < NPRODUCERS + NCONSUMERS; i++)
		wait();

	printf(1, "Producers/consumers checksum OK\n");

	return *checksum;
}

int
main(void)
{
	char checksum[2];

	checksum[0] = checksum_single_process();
	checksum[1] = checksum_producers_consumers();

	printf(1, "Single process checksum = %d\n", checksum[0]);
	printf(1, "Producers/consumers checksum = %d\n", checksum[1]);

	if (checksum[0] != checksum[1])
		printf(1, "Checksums don't match\n");
	else
		printf(1, "Checksums match OK\n");

	exit();
}
