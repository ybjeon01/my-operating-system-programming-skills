#include "Task.h"
#include "Descriptor.h"
#include "Utility.h"

// create a initial task structure based on the parameters
// params:
//   pstTCB: an unused TCB struct
//   qwID: the task unique id
//   qwFlags: a flag that determines if other task can interrupt this task
//   qwEntryPointAddress: start address of a code
//   pvStackAddress: start address of a stack used for the task
//   qwStackSize:  size of the stack
void kSetUpTask(
    TCB *pstTCB,
    QWORD qwID,
    QWORD qwFlags,
    QWORD qwEntryPointAddress,
    void *pvStackAddress,
    QWORD qwStackSize
) {
    // initialize TCB struct
    kMemSet(
        pstTCB->stContext.vqRegister,
        0,
        sizeof(pstTCB->stContext.vqRegister)
    );


    /* set rsp and rbp */
    
    pstTCB->stContext.vqRegister[TASK_RSPOFFSET] = 
        (QWORD) pvStackAddress + qwStackSize;
    
    pstTCB->stContext.vqRegister[TASK_RBPOFFSET] = 
        (QWORD) pvStackAddress + qwStackSize;


    /* set segment selectors */

    pstTCB->stContext.vqRegister[TASK_CSOFFSET] = GDT_KERNELCODESEGMENT;

    pstTCB->stContext.vqRegister[TASK_DSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_ESOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_FSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_GSOFFSET] = GDT_KERNELDATASEGMENT;
    pstTCB->stContext.vqRegister[TASK_SSOFFSET] = GDT_KERNELDATASEGMENT;


    /* set IP */
    
    pstTCB->stContext.vqRegister[TASK_RIPOFFSET] = qwEntryPointAddress;


    /* set interrupt flag in RFLAGS by default */

    pstTCB->stContext.vqRegister[TASK_RFLAGSOFFSET] |= 0x200;

    /*  ID, stack, and flag */

    pstTCB->qwID = qwID;
    pstTCB->pvStackAddress = pvStackAddress;
    pstTCB->qwStackSize = qwStackSize;
    pstTCB->qwFlags = qwFlags;
}