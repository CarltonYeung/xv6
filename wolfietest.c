#include "types.h"
#include "stat.h"
#include "user.h" // Where the user-facing system call is declared

int main(void) {
	uint size = 649;
    char buf[size];

    wolfie(buf, size);
    printf(1, "%s", buf);
    exit();
}
