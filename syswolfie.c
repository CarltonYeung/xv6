#include "types.h"
#include "defs.h"
#include "syswolfie.h"

int sys_wolfie(void) {
    char *buf;
    int size;
    int wolfie_size = sizeof(WOLFIE);

    if (argstr(0, &buf) < 0 || argint(1, &size) < 0 || size < wolfie_size) {
        return -1;
    }

    memmove(buf, WOLFIE, wolfie_size);

    return wolfie_size;
}
