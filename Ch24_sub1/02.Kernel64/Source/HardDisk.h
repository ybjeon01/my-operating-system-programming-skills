#ifndef __HARDDISK_H__
#define __HARDDISK_H__

#include "Types.h"
#include "Synchronization.h"

// first and second PATA IO port base
#define HDD_PORT_PRIMARYBASE    0x1F0
#define HDD_PORT_SECONDARYBASE  0x170


/* IO port index */

#define HDD_PORT_INDEX_DATA             0x00
#define HDD_PORT_INDEX_ERROR            0x01
#define HDD_PORT_INDEX_STATUS           0x07
#define HDD_PORT_INDEX_COMMAND          0x07
#define HDD_PORT_INDEX_DIGITALOUTPUT    0x206
#define HDD_PORT_INDEX_DRIVEADDRESS     0x207

/** CHS or LBA related address **/

#define HDD_PORT_INDEX_SECTORCOUNT  0x02
#define HDD_PORT_INDEX_SECTORNUMBER 0x03
#define HDD_PORT_INDEX_CYLINDERLSB  0x04
#define HDD_PORT_INDEX_CYLINDERMSB  0x05
#define HDD_PORT_INDEX_DRIVEANDHEAD 0x06


/* macros for command register */

#define HDD_COMMAND_READ        0x20
#define HDD_COMMAND_WRITE       0x30
#define HDD_COMMAND_IDENTIFY    0xEC


/* macros for status register */

#define HDD_STATUS_ERROR            0x01
#define HDD_STATUS_INDEX            0x02
#define HDD_STATUS_CORRECTEDDATA    0x04
#define HDD_STATUS_DATAREQUEST      0x08
#define HDD_STATUS_SEEKCOMPLETE     0x10
#define HDD_STATUS_WRITEFAULT       0x20
#define HDD_STATUS_READY            0x40
#define HDD_STATUS_BUSY             0x80


/* macros for drive/head register */

#define HDD_DRIVEANDHEAD_LBA    0xE0
#define HDD_DRIVEANDHEAD_SLAVE  0x10


/* macros for digitaloutput register */

#define HDD_DIGITALOUTPUT_RESET             0x04
#define HDD_DIGITALOUTPUT_DISABLEINTERRUPT  0x01


/* MISC macros */

// time to wait for harddisk response (millisecond)
#define HDD_WAITTIME        500

// maximum number of sectors to read or write
#define HDD_MAXBULKSECTORCOUNT  256


/* structs for managing harddisk */ 

#pragma pack(push, 1)

// struct for hdd information
typedef struct kHDDInformationStruct {
    WORD wConfiguration;


    /* cylinder */

    WORD wNumberOfCylinder;
    WORD wReserved1;

    /* head */

    WORD wNumberOfHead;
    WORD wUnformattedBytesPerTrack;
    WORD wUnformattedBytesPerSector;


    /* sector */

    WORD wNumberOfSectorPerCylinder;
    WORD wInterSectorGap;
    WORD wBytesinPhaseLock;
    WORD wNumberOfVendorUniqueStatusWord;

    
    /* harddisk serial number */

    WORD vwSerialNumber[10];
    WORD wControllerType;
    WORD wBufferSize;
    WORD wNumberOfECCBytes;
    WORD vwFirmwareRevision[4];

    
    /* harddisk model number */

    WORD vwModelNumber[20];
    WORD vwReserved2[13];

    /* total number of sectors */
    DWORD dwTotalSectors;
    WORD vwReserved3[196];

} HDDINFORMATION;

#pragma pack(pop)


// struct for managing harddisks
typedef struct kHDDMamagerStruct {
    BOOL bHDDDetected; // check if hdd exist
    BOOL bCanWrite;    // help to write only on QEMU disk

    /* check if interrupt occurs */

    volatile BOOL bPrimaryInterruptOccur;
    volatile BOOL bSecondaryInterruptOccur;

    // sync object for one process to access hdd at time
    MUTEX stMutex;

    HDDINFORMATION stHDDInformation;
} HDDMANAGER;


/* hdd related functions */


// initialize hdd controller to ATA PIO MODE
// return:
//   True on success, False on failure (disk not exist)
BOOL kInitializeHDD(void);


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
);


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
);


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
);


// set interrupt flag of hdd manager
// params:
//   bPrimary: select drive 0 of primary controller or secondary controller
//   bFlag: flag to set
void kSetHDDInterruptFlag(BOOL bPrimary, BOOL bFlag);


// set interrupt flag of hdd manager
// params:
//   bPrimary: select drive 0 of primary controller or secondary controller
//   bFlag: flag to set
void kSetHDDInterruptFlagclear(BOOL bPrimary, BOOL bFlag);

static void kSwapByteInWord(WORD *pwData, int iWordCount);
static BYTE kReadHDDStatus(BOOL bPrimary);
static BOOL kIsHDDBusy(BOOL bPrimary);
static BOOL kIsHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDNoBusy(BOOL bPrimary);
static BOOL kWaitForHDDReady(BOOL bPrimary);
static BOOL kWaitForHDDInterrupt(BOOL bPrimary);

#endif /* __HARDDISK_H__ */