#ifndef __PIT_H__
#define __PIT_H__

#include "Types.h"


/* PIT frequency related macros */

#define PIT_FREQUENCY 1193182
#define MSTOCOUNT(x) (PIT_FREQUENCY * (x) / 1000)
#define USTOCOUNT(x) (PIT_FREQUENCY * (x) / 1000000)


/* PIT I/O ports */

#define PIT_PORT_CONTROL    0x43
#define PIT_PORT_COUNTER0   0x40
#define PIT_PORT_COUNTER1   0x41
#define PIT_PORT_COUNTER2   0x42


/* PIT control/mode register bits */

// common to mode and command
#define PIT_CONTROL_COUNTER0    0x00
#define PIT_CONTROL_COUNTER1    0x40
#define PIT_CONTROL_COUNTER2    0x80


/** command related macros **/

// latch command: read 2 bytes without sync problem
#define PIT_CONTROL_LATCH       0x00


/** mode related macros **/

// read or write low byte and high byte
#define PIT_CONTROL_LSBMSBRW    0x30

// two modes used in MINT64OS
#define PIT_CONTROL_MODE0       0x00
#define PIT_CONTROL_MODE2       0x04 

// binary or BCD for reset counter
#define PIT_CONTROL_BINARYCOUNTER   0x00
#define PTI_CONTROL_BCDCOUNTER      0x01


/** setting for MINT64OS **/

#define PIT_COUNTER0_ONCE ( \
    PIT_CONTROL_COUNTER0 | \
    PIT_CONTROL_LSBMSBRW | \
    PIT_CONTROL_MODE0 | \
    PIT_CONTROL_BINARYCOUNTER \
)

#define PIT_COUNTER0_PERIODIC ( \
    PIT_CONTROL_COUNTER0 | \
    PIT_CONTROL_LSBMSBRW | \
    PIT_CONTROL_MODE2 | \
    PIT_CONTROL_BINARYCOUNTER \
)

// command: read 2 bytes of counter0 without sync problem
#define PIT_COUNTER0_LATCH ( \
    PIT_CONTROL_COUNTER0 | \
    PIT_CONTROL_LATCH \
)


/* PIT related functions */

// initialize PIT counter0 mode and its reset value
// params:
//   wCount: reset value of counter0
//   bPeriodic: mode of counter0
//     True: mode2
//     False: mode0
// info:
//   this function does not disable interrupt, so if other program call
//   PIT-related functions, the result is unpredictable 
void kInitializePIT(WORD wCount, BOOL bPeriodic);


// read two bytes of counter0 by using latch command
// return:
//   current counter value of counter0 register
// info:
//   this function does not disable interrupt, so if other program call
//   PIT-related functions, the result is unpredictable 
WORD kReadCounter0(void);


// wait busily as long as the count
// params:
//   wCount: reset value of counter0
// info:
//   this function does not disable interrupt, so if other program call
//   PIT-related functions, the result is unpredictable
void kWaitUsingDirectPIT(WORD wCount);

#endif /* __PIT_H__ */