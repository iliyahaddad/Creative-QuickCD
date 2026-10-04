#include "types.h"
#include "cdrom.h"

/*
 * Open Watcom inline-assembly helpers.
 * "#pragma aux name = <code>" must be at file scope and the function is only
 * *declared* (no body): the pragma supplies the code that is inlined.
 */

#pragma aux ReadPort = \
    "in al, dx" \
    parm [dx] \
    value [al];

#pragma aux WritePort = \
    "out dx, al" \
    parm [dx] [al];

#pragma aux ReadPort16 = \
    "in ax, dx" \
    parm [dx] \
    value [ax];

#pragma aux WritePort16 = \
    "out dx, ax" \
    parm [dx] [ax];
