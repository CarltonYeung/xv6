#include "types.h"
#include "defs.h"
#include "syswolfie.h"

int sys_wolfie(void) {
    char *buf = 0;
    int size = 0;
    int wolfie_size = sizeof(WOLFIE);

    // Get arguments from stack and do error checks
    if (argint(1, &size) < 0 || argptr(0, &buf, size) < 0) {
        return -1;
    }

    if (size < wolfie_size) {
    	return -1;
    }

    memmove(buf, WOLFIE, wolfie_size);

    return wolfie_size;
}
