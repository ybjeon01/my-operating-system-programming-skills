#include "HardDisk.h"
#include "Synchronization.h"
#include "AssemblyUtility.h"
#include "Utility.h"
#include "Console.h"

static HDDMANAGER gs_stHDDManager;


// initialize hdd controller to ATA PIO MODE
// return:
//   True on success, False on failure (disk not exist)
BOOL kInitializeHDD(void) {
    
    kInitializeMutex(&gs_stHDDManager.stMutex);

    gs_stHDDManager.bPrimaryInterruptOccur = FALSE;
    gs_stHDDManager.bSecondaryInterruptOccur = FALSE;

    // enable interrupts
    kOutPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);
    kOutPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_DIGITALOUTPUT, 0);

    // if disk does not exist
    if (!kReadHDDInformation(TRUE, TRUE, &(gs_stHDDManager.stHDDInformation))) {
        gs_stHDDManager.bHDDDetected = FALSE;
        gs_stHDDManager.bCanWrite = FALSE;

        return FALSE;
    }
    
    gs_stHDDManager.bHDDDetected = TRUE;
    
    // when disk is QEMU disk
    if (
        kMemCmp(
            gs_stHDDManager.stHDDInformation.vwModelNumber,
            "QEMU",
            4
        ) == 0
    ) {
        gs_stHDDManager.bCanWrite = TRUE;
    }
    else {
        gs_stHDDManager.bCanWrite = FALSE;
    }
    return TRUE;
}


// get status of drive 0
// params:
//   bPrimary: select drive 0 of primary controller or secondary controller
// return:
//   status register of drive 0
static BYTE kReadHDDStatus(BOOL bPrimary) {
    // drive 0 of primary controller
    if (bPrimary == TRUE) {
        return kInPortByte(HDD_PORT_PRIMARYBASE + HDD_PORT_INDEX_STATUS);
    }
    // drive 0 of secondary controller
    return kInPortByte(HDD_PORT_SECONDARYBASE + HDD_PORT_INDEX_STATUS);
}


// wait for drive 0 not to be busy
// params:
//   bPrimary: select drive 0 of primary controller or secondary controller
// return:
//   True on not-busy state, otherwise False
static BOOL kWaitForHDDNoBusy(BOOL bPrimary) {
    QWORD qwStartTickCount = kGetTickCount();

    while (kGetTickCount() - qwStartTickCount <= HDD_WAITTIME) {
        if (!(kReadHDDStatus(bPrimary) & HDD_STATUS_BUSY)) {
            return TRUE;
        }
        kSleep(1);
    }
    return FALSE;
}


// wait for drive 0 not to be ready
// params:
//   bPrimary: select drive 0 of primary controller or secondary controller
// return:
//   True on ready state, otherwise False
static BOOL kWaitForHDDReady(BOOL bPrimary) {
    QWORD qwStartTickCount = kGetTickCount();

    while (kGetTickCount() - qwStartTickCount <= HDD_WAITTIME) {
        if (kReadHDDStatus(bPrimary) & HDD_STATUS_READY) {
            return TRUE;
        }
        kSleep(1);
    }
    return FALSE;
}


// set interrupt flag of hdd manager
// params:
//   bPrimary: select drive 0 of primary controller or secondary controller
//   bFlag: flag to set
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag) {
    if (bPrimary) {
        gs_stHDDManager.bPrimaryInterruptOccur = bFlag;
    }
     gs_stHDDManager.bSecondaryInterruptOccur = bFlag;
}


// wait until interrupt occurs
// params:
//   bPrimary: select drive 0 of primary controller or secondary controller
// return:
//   True on interrupt and False on timeout
static BOOL kWaitForHDDInterrupt(BOOL bPrimary) {
    QWORD qwTickCount = kGetTickCount();

    while (kGetTickCount() - qwTickCount <= HDD_WAITTIME) {
        if (bPrimary && gs_stHDDManager.bPrimaryInterruptOccur) {
            return TRUE;
        }
        else if (!(bPrimary) && gs_stHDDManager.bSecondaryInterruptOccur) {
            return TRUE;
        }
        kSleep(1);
    }
    return FALSE;
}


// read disk information
// params:
//   bPrimary: select primary controller or secondary controller
//   bMaster: select master or slave drive
//   pstHDDInformation: pointer to save hdd info
// return:
//   TRUE on success, FALSE on failure (busy, not ready, error)
BOOL kReadHDDInformation(
    BOOL bPrimary,
    BOOL bMaster,
    HDDINFORMATION *pstHDDInformation
) {
    WORD wPortBase;
    QWORD qwLastTIckCount;
    BYTE bStatus;
    BYTE bDriveFlag;
    int i;
    WORD wTemp;
    BOOL bWaitResult;

    
    /* primary or secondary bus */

    if (bPrimary) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    // lock hdd manager for futher use
    kLock(&(gs_stHDDManager.stMutex));


    // wait for previous command to end
    if (kWaitForHDDNoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    /* master or slave drive */

    if (bMaster) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }

    kOutPortByte(wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD, bDriveFlag);

    /* send command and wait for hdd interrupt */

    // wait for controller to be ready to accept command
    if (kWaitForHDDReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    // initialize interrupt variable
    kSetHDDInterruptFlag(bPrimary, FALSE);

    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_IDENTIFY);

    // interrupt occurs when command completes
    bWaitResult = kWaitForHDDInterrupt(bPrimary);
    bStatus = kReadHDDStatus(bPrimary);

    // interrupt timeout means that error occurred or something is wrong
    if (!bWaitResult || (bStatus & HDD_STATUS_ERROR)) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return FALSE;
    }

    /* read hdd info */

    // read one sector
    // read two bytes at each I/O operation
    for (i = 0; i < 512 / 2; i++) {
        ((WORD *) pstHDDInformation)[i] = 
            kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
    }
    
    // rearrange string in sorted order
    kSwapByteInWord(
        pstHDDInformation->vwModelNumber,
        sizeof(pstHDDInformation->vwModelNumber) / 2
    );
    kSwapByteInWord(
        pstHDDInformation->vwSerialNumber,
        sizeof(pstHDDInformation->vwSerialNumber) / 2
    );
    kUnlock(&(gs_stHDDManager.stMutex));
    return TRUE;
}


// change the order of bytes within a WORD
// params:
//   pwData: pointer to a string to sort
//   iWordCount: number of words in the string
static void kSwapByteInWord(WORD *pwData, int iWordCount) {
    int i;
    WORD wTemp;

    for (i = 0; i < iWordCount; i++) {
        wTemp = pwData[i];
        pwData[i] = (wTemp >> 8) | (wTemp << 8);
    }
}


// read hard disk sectors
// params:
//   bPrimary: select primary controller or secondary controller
//   bMaster: select master or slave drive
//   dwLBA: logical block address
//   iSectorCount: number of sectors to read
//   pcBuffer: pointer to buffer which saves the data from hdd
// info:
//   can read up to 256 sectors
// return:
//   number of sectors actually read
int kReadHDDSector(
    BOOL bPrimary,
    BOOL bMaster,
    DWORD dwLBA,
    int iSectorCount,
    char *pcBuffer
) {
    WORD wPortBase;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lReadCount = 0;
    BOOL bWaitResult;


    /* check if parameters are valid */

    if (
        (gs_stHDDManager.bHDDDetected == FALSE) ||
        (iSectorCount <= 0) || (256 < iSectorCount) ||
        (
            (dwLBA + iSectorCount) >= 
            gs_stHDDManager.stHDDInformation.dwTotalSectors
        )
    ) {
        return 0;
    }


    if (bPrimary) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    kLock(&(gs_stHDDManager.stMutex));

    if (kWaitForHDDNoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return 0;
    }

    
    /* set registers for LBA */

    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    
    if (bMaster) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }

    kOutPortByte(
        wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD,
        bDriveFlag | ((dwLBA >> 24) & 0x0F)
    );


    /* send read command */

    if (kWaitForHDDReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return 0;
    }

    kSetHDDInterruptFlag(bPrimary, FALSE);

    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_READ);


    /* copy data to buffer */

    for (i = 0; i < iSectorCount; i++) {
        bStatus = kReadHDDStatus(bPrimary);
        if ((bStatus & HDD_STATUS_ERROR) == HDD_STATUS_ERROR) {
            kPrintf("HardDisk: Error Occur\n");
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }

        if ((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);

            if (bWaitResult == FALSE) {
                kPrintf("HardDisk: Interrupt not occurred\n");
                kUnlock(&(gs_stHDDManager.stMutex));
                return i;
            }
        }

        for (j = 0; j < 512 / 2; j++) {
            ((WORD *) pcBuffer)[lReadCount++] =
                kInPortWord(wPortBase + HDD_PORT_INDEX_DATA);
        }
    }

    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}


// write data to hard disk
// params:
//   bPrimary: select primary controller or secondary controller
//   bMaster: select master or slave drive
//   dwLBA: logical block address
//   iSectorCount: number of sectors to read
//   pcBuffer: pointer to buffer which have data to write
// info:
//   can write up to 256 sectors
// return:
//   number of sectors actually written
int kWriteHDDSector(
    BOOL bPrimary,
    BOOL bMaster,
    DWORD dwLBA,
    int iSectorCount,
    char *pcBuffer
) {
    WORD wPortBase;
    WORD wTemp;
    int i, j;
    BYTE bDriveFlag;
    BYTE bStatus;
    long lWriteCount = 0;
    BOOL bWaitResult;


    if (
        (gs_stHDDManager.bHDDDetected == FALSE) ||
        (iSectorCount <= 0) || (256 <iSectorCount) ||
        (
            (dwLBA + iSectorCount) >= 
            gs_stHDDManager.stHDDInformation.dwTotalSectors
        )
    ) {
        return 0;
    }


    if (bPrimary) {
        wPortBase = HDD_PORT_PRIMARYBASE;
    }
    else {
        wPortBase = HDD_PORT_SECONDARYBASE;
    }

    kLock(&(gs_stHDDManager.stMutex));

    if (kWaitForHDDNoBusy(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return 0;
    }

    
    /* set registers for LBA */

    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORCOUNT, iSectorCount);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_SECTORNUMBER, dwLBA);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERLSB, dwLBA >> 8);
    kOutPortByte(wPortBase + HDD_PORT_INDEX_CYLINDERMSB, dwLBA >> 16);
    
    if (bMaster) {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA;
    }
    else {
        bDriveFlag = HDD_DRIVEANDHEAD_LBA | HDD_DRIVEANDHEAD_SLAVE;
    }

    kOutPortByte(
        wPortBase + HDD_PORT_INDEX_DRIVEANDHEAD,
        bDriveFlag | ((dwLBA >> 24) & 0x0F)
    );


    /* send write command */

    if (kWaitForHDDReady(bPrimary) == FALSE) {
        kUnlock(&(gs_stHDDManager.stMutex));
        return 0;
    }

    kOutPortByte(wPortBase + HDD_PORT_INDEX_COMMAND, HDD_COMMAND_WRITE);

    // wait until sending data is possible
    while (TRUE) {
        bStatus = kReadHDDStatus(bPrimary);
    
        if (bStatus & HDD_STATUS_ERROR) {
            kUnlock(&(gs_stHDDManager.stMutex));
            return 0;
        }

        if (bStatus & HDD_STATUS_DATAREQUEST) {
            break;
        }

        kSleep(1);
    }


    // send data to hdd
    for (i = 0; i < iSectorCount; i++) {
        kSetHDDInterruptFlag(bPrimary, FALSE);
        for (j = 0; j < 512 / 2; j++) {
            kOutPortWord(
                wPortBase + HDD_PORT_INDEX_DATA,
                ((WORD *) pcBuffer)[lWriteCount++]
            );
        }

        bStatus = kReadHDDStatus(bPrimary);
        if (bStatus & HDD_STATUS_ERROR) {
            kUnlock(&(gs_stHDDManager.stMutex));
            return i;
        }

        if ((bStatus & HDD_STATUS_DATAREQUEST) != HDD_STATUS_DATAREQUEST) {
            bWaitResult = kWaitForHDDInterrupt(bPrimary);
            kSetHDDInterruptFlag(bPrimary, FALSE);
            if (!bWaitResult) {
                kUnlock(&(gs_stHDDManager.stMutex));
                return i;
            }
        }
    }

    kUnlock(&(gs_stHDDManager.stMutex));
    return i;
}