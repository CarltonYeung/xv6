#include <string.h>
#include "syswolfie.h"
#include "types.h"

int sys_wolfie(void) {
    void *buf;
    uint size;
    uint wolfie_size = sizeof(WOLFIE);

    if (argstr(0, &buf) < 0 || argint(1, &size) < 0 || size < wolfie_size) {
        return -1;
    }

    memmove(buf, WOLFIE, wolfie_size);

    return wolfie_size;
}