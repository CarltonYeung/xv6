#include "types.h"
#include "defs.h"
#include "syswolfie.h"

int sys_wolfie(void) {
    char *buf;
    int size;
    int wolfie_size = sizeof(WOLFIE);

    // Get arguments from stack and do error checks
    if (argstr(0, &buf) < 0 || argint(1, &size) < 0) {
        return -1;
    }

    if (size < wolfie_size || buf <= 0) {
    	return -1;
    }

    memmove(buf, WOLFIE, wolfie_size);

    return wolfie_size;
}
