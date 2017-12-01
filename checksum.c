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

			printf(1, "%c", token);

			checksum = checksum + token;
		} while (ret != 0);

		close(fd);
	}

	printf(1, "README (x4) checksum = %d\n", checksum);
}

int main(void) {
	checksum_single_process();

	exit();
}
