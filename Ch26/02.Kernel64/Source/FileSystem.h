#ifndef __FILSYSTEM_H__

#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"


// This macros contain information related to Mint FileSystem header and cluters

// MINT File System Signature
// This information is recorded in the MBR and this method is not standard. 
#define FILESYSTEM_SIGNATURE                0x7E38CF10

// size of cluster (unit: sector)
// A cluster is the smallest unit of a file system that can be written
// to or read by a user. 
#define FILESYSTEM_SECTORSPERCLUSTER        8

#define FILESYSTEM_LASTCLUSTER              0xFFFFFFFF

// sign of free cluster
#define FILESYSTEM_FREECLUSTER              0x00

// Maximum number of entries in the root directory
// MINT64OS have only one directory, root. this means that MINT Filesystem 
// do not support directories in directories and can only save files in the
// root directory.  
#define FILESYSTEM_MAXDIRECTORYENTRYCOUNT   ( \
    FILESYSTEM_SECTORSPERCLUSTER * 512 / sizeof(DIRECTORYENTRY) \
)

// size of cluster (unit: byte)
#define FILESYSTEM_CLUSTERSIZE              ( \
    FILESYSTEM_SECTORSPERCLUSTER * 512 \
)

// Maximum length of file name: 24 characters
#define FILESYSTEM_MAXFILENAMELENGTH        24


// function pointer types related to hard disk control 
typedef BOOL (* fReadHDDInformation) (
    BOOL bPrimary,
    BOOL bMaster,
    HDDINFORMATION *pstHDDInformation
);

typedef int (* fReadHDDSector) (
    BOOL bPrimary,
    BOOL bMaster,
    DWORD dwLBA,
    int iSectorCount,
    char *pcBuffer
);

typedef int (* fWriteHDDSector) (
    BOOL bPrimary,
    BOOL bMaster,
    DWORD dwLBA,
    int iSectorCount,
    char *pcBuffer
);


// structures aligned by one byte 

#pragma pack(push, 1)

// partition structure in MBR
typedef struct kPartitionStruct {
    // Boot flag
    // 0x80 means bootable, 0x00 means non-bootable
    BYTE bBootableFlag;

    // start address of a partition. Currently, CHS addresse is rarely used,
    // and LBAs are used instead.
    BYTE vbStartingCHSAddress[3];

    // partition type
    BYTE bPartitionType;

    // last address of a partition. Currently CHS address is rarely used
    BYTE vbEndingCHSAddress[3];

    // start address of a partition in LBA.
    DWORD dwStartingLBAAddress;
    
    // number of sectors in a partition
    DWORD dwSizeInSector;
} PARTITION;


// MBR structure
typedef struct kMBRStruct {
    // The area where the bootloader code is located
    // According to MBR, 446 bytes are used as bootloader.
    // However, in MINT64OS the last 16 bytes are used for MINT file system. 
    BYTE vbBootCode[430];

    // signature of Mint file system, 0x7E38CF10
    DWORD dwSignature;

    // number of sectors in the reserved area in MINT file system
    DWORD dwReservedSectorCount;

    // Number of sectors in the cluster link table area 
    // MINT file system consists of cluster link table and data area
    // the table is used to efficitiently manage files in data area
    // and it is located at the beginning of a partition
    DWORD dwClusterLinkSectorCount;

    // total number of clusters in data area
    DWORD dwTotalClusterCount;

    // partition tables
    PARTITION vstPartition[4];

    // MBR signature, 0x55, 0xAA
    BYTE vbBootLoaderSignature[2];
} MBR;

// data structure for directory entries
typedef struct kDirectoryEntryStruct {
    // file name
    char vcFileName[FILESYSTEM_MAXFILENAMELENGTH];

    // actual size of the file 
    DWORD dwFileSize;
    
    // the cluster index where the file begins
    DWORD dwStartClusterIndex;
} DIRECTORYENTRY;


// Structure that manages the file system
// MINT64OS kernel uses this structure to manage file system
typedef struct kFileSystemManagerStruct {
    // Check whether kernel can use file system
    BOOL bMounted;

    // Number of sectors and starting LBA address in each area
    DWORD dwReservedSectorCount;
    DWORD dwClusterLinkAreaStartAddress;
    DWORD dwClusterLinkAreaSize;

    // starting LBA address in data area
    DWORD dwDataAreaStartAddress;
    // total number of clusters in data area
    DWORD dwTotalClusterCount;

    // sector offset of the cluster link table to which the last
    // cluster was allocated.
    // this variable is used to efficitiently write file to clusters
    DWORD dwLastAllocatedClusterLinkSectorOffset;

    // file system synchronization object
    MUTEX stMutex;

} FILESYSTEMMANAGER;


// functions


// initialize file system
// return:
//   True on success, False on failure
// info:
//   This function initialize HDD disk controller before initializing filesystem
//   
//   possible failures:
//     when HDD initialization failed
//     when mounting filesystem failed  
BOOL kInitializeFileSystem(void);


// Check if HDD has MINT filesystem by reading MBR in HDD.
// If it is a MINT file system, this function reads various information
// related to the file system and inserts it into file system data structure.
// return:
//   TRUE on success and FALSE on failure
BOOL kFormat(void);


// Create a file system on HDD
// return:
//   TRUE on success, FALSE on failure
BOOL kMount(void);


// Return information on the hard disk connected to the file system
// params:
//   pstInformation: A pointer to a variable that will hold the info
// return:
//   TRUE on success, FALSE on failure
BOOL kGetHDDInformation(HDDINFORMATION *pstInformation);


/* low level functions */


// Read a sector at an offset in the cluster link table
// params:
//   dwOffset: Offset to read from cluster link table
//   pbBuffer: A pointer to a variable that will hold the data
// return:
//   TRUE on success, FALSE on failure
static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer);


// Write a sector at an offset in the cluster link table
// params:
//   dwOffset: Offset to write to cluster link table
//   pbBuffer: A pointer to a variable that will be written to link area
// return:
//   TRUE on success, FALSE on failure
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer);


// Read one cluster to the offset of the data area
// params:
//   dwOffset: Offset to read from data area
//   pbBuffer: A pointer to a variable that will hold the data
// return:
//   TRUE on success, and FALSE on failure
static BOOL kReadCluster(DWORD dwOffset, BYTE *pbBuffer);


// Write one cluster to the offset of the data area
// params:
//   dwOffset: Offset to write to data area
//   pbBuffer: A pointer to a variable that will be written to data area
// return:
//   TRUE on success, and FALSE on failure
static BOOL kWriteCluster(DWORD dwOffset, BYTE *pbBuffer);


// Search for an empty cluster in the Cluster Links table area
// return:
//   index to empty cluster in data area
static DWORD kFindFreeCluster(void);


// set value in cluster link corresponding to a cluster index
// params:
//   dwClusterIndex: index to a cluster in data area
//   dwData: 
// return:
//   TRUE on success, FALSE on failure
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData);


// get value in cluster link corresponding to a cluster index
// params:
//   dwClusterIndex: index to a cluster in data area
//   pdwData: a pointer to a varaible that will contain the link data 
// return:
//   TRUE on success, FALSE on failure
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD *pdwData);


// get idex to empty entry in root directory
// return:
//   index to free entry in root directory
//   -1 on failure
static int kFindFreeDirectoryEntry(void);


// write directory entry to index of root directory
// params:
//   iIndex: Directory entry index to set in the root directory
//   pstEntry: pointer containing data to write to straoge
// return:
//   TRUE on success, FALSE on failure
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry);


// read directory entry at index of root directory
// params:
//   iIndex: Directory entry index to read in the root directory
//   pstEntry: pointer that will contain the data 
// return:
//   TRUE on success, FALSE on failure
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry);


// find an entry in the root directory with a matching file name
// params:
//   pcFileName: filename to search
//   pstEntry: pointer to a variable that will contain the found entry
// return:
//   index of found entry in root directory
//   or -1 on failure 
static int kFindDirectoryEntry(
    const char *pcFileName,
    DIRECTORYENTRY *pstEntry
);


// copy global file manager to pstManager
// params:
//   pstManager: pointer to a variable that will contain global file manager
//               info
void kGetFileSystemInformation(FILESYSTEMMANAGER *pstManager);


#endif /* __FILESYSTEM_H__ */