#include "types.h"

BYTE ReadPort(WORD port) {
    BYTE value;
    #pragma aux ReadPort = \
        "in al, dx" \
        parm [dx] \
        value [al] \
        modify [dx];
    return value;
}

void WritePort(WORD port, BYTE value) {
    #pragma aux WritePort = \
        "out dx, al" \
        parm [dx] [al] \
        modify [dx];
}

WORD ReadPort16(WORD port) {
    WORD value;
    #pragma aux ReadPort16 = \
        "in ax, dx" \
        parm [dx] \
        value [ax] \
        modify [dx];
    return value;
}

void WritePort16(WORD port, WORD value) {
    #pragma aux WritePort16 = \
        "out dx, ax" \
        parm [dx] [ax] \
        modify [dx];
}

void DelayLoop(WORD count) {
    #pragma aux DelayLoop = \
        "L1: dec cx" \
        "jnz L1" \
        parm [cx] \
        modify [cx];
}
