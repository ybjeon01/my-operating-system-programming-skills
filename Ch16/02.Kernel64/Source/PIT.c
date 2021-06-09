#include "PIT.h"
#include "AssemblyUtility.h"

// initialize PIT counter0 mode and its reset value
// params:
//   wCount: reset value of counter0
//   bPeriodic: mode of counter0
//     True: mode2
//     False: mode0
// info:
//   this function does not disable interrupt, so if other program call
//   PIT-related functions, the result is unpredictable 
void kInitializePIT(WORD wCount, BOOL bPeriodic) {
    // initialize counter0 mode
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_ONCE);

    if (bPeriodic) {
        kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_PERIODIC);
    }

    // pass 2 bytes of reset value 
    kOutPortByte(PIT_PORT_COUNTER0, (BYTE) wCount);
    kOutPortByte(PIT_PORT_COUNTER0, (BYTE) (wCount >> 8));
}


// read two bytes of counter0 by using latch command
// return:
//   current counter value of counter0 register
// info:
//   this function does not disable interrupt, so if other program call
//   PIT-related functions, the result is unpredictable 
WORD kReadCounter0(void) {
    BYTE bHighByte, bLowByte;
    WORD wTemp = 0;

    // latch command on counter0
    // reading counter without latch command can cause
    // reading wrong value of high byte
    kOutPortByte(PIT_PORT_CONTROL, PIT_COUNTER0_LATCH);

    bLowByte = kInPortByte(PIT_PORT_COUNTER0);
    bHighByte = kInPortByte(PIT_PORT_COUNTER0);

    wTemp = (WORD) (bHighByte << 8);
    wTemp |= bLowByte;
    return wTemp;
}


// wait busily as long as the count
// params:
//   wCount: reset value of counter0
// info:
//   this function does not disable interrupt, so if other program call
//   PIT-related functions, the result is unpredictable
void kWaitUsingDirectPIT(WORD wCount) {
    WORD wLastCounter0;
    WORD wCurrentCounter0;

    // In original code, 0 is used. However, in really, fast PC
    // reading counter register after initializing reset counter
    // can return 0. In this case, (wLastCounter0 - wCurrentCounter0)
    // does not work as intended
    kInitializePIT(0xFFFF, TRUE);

    int i = 0;

    wLastCounter0 = kReadCounter0();
    while (TRUE) {
        wCurrentCounter0 = kReadCounter0();
        if ((wLastCounter0 - wCurrentCounter0) > wCount) {
            break;
        }
    }
}