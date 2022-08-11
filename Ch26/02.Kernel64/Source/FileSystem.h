#ifndef __FILSYSTEM_H__

#define __FILESYSTEM_H__

#include "Types.h"
#include "Synchronization.h"
#include "HardDisk.h"
#include "Task.h"


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

// Maximum number of dir and file handlers in a pool
#define FILESYSTEM_HANDLE_MAXCOUNT          (TASK_MAXCOUNT * 3) 


// types of handlers

#define FILESYSTEM_TYPE_FREE                0
#define FILESYSTEM_TYPE_FILE                1
#define FILESYSTEM_TYPE_DIRECTORY           2


// SEEK options

#define FILESYSTEM_SEEK_SET                 0
#define FILESYSTEM_SEEK_CUR                 1
#define FILESYSTEM_SEEK_END                 2



// overide MINT file system function names with stdio function names

#define fopen                               kOpenFile
#define fread                               kReadFile
#define fwrite                              kWriteFile
#define fseek                               kSeekFile
#define fclose                              kCloseFile
#define remove                              kRemoveFile
#define opendir                             kOpenDirectory
#define readdir                             kReadDirectory
#define rewinddir                           kRewindDirectory
#define closedir                            kCloseDirectory


// overide MINT file system macro names with stdio macro names

#define SEEK_SET                            FILESYSTEM_SEEK_SET
#define SEEK_CUR                            FILESYSTEM_SEEK_CUR
#define SEEK_END                            FILESYSTEM_SEEK_END


// override MINT file system types with stdio types

#define size_t                              DWORD
#define dirent                              DIRECTORYENTRY
#define d_name                              vcFileName


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


// File handler structure that manages a file
typedef struct kFileHandleStruct {
    // Offset of the directory entry where the file exists
    int iDirectoryEntryOffset;
    // actual size of the file
    DWORD dwFileSize;
    // The cluster index where the file starts
    DWORD dwStartClusterIndex;


    // The index of the cluster that is currently performing I/O
    DWORD dwCurrentClusterIndex;
    // The index of the cluster immediately preceding the current cluster
    DWORD dwPreviousClusterIndex;
    // Current position of the file pointer
    DWORD dwCurrentOffset;
} FILEHANDLE;


// Directory handlerstructure that manages a directory
typedef struct kDirectoryHandleStruct {
    // buffer that contains root directory entries (cluster 0 of data area)
    DIRECTORYENTRY *pstDirectoryBuffer;

    
    // Current position of the directory pointer
    int iCurrentOffset;
} DIRECTORYHANDLE;


// File and Dir data structure that is exposed to user
typedef struct kFileDirectoryHandleStruct {
    // type of the handle: file, dir, or free
    BYTE bType;
    
    union {
        FILEHANDLE stFileHandle;
        DIRECTORYHANDLE stDirectoryHandle;
    };
} FILE, DIR;


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

    // file/directory handle pool
    FILE *pstHandlePool;

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


/* high level functions */


// open a FILE handle
// params:
//   pcFileName: name of a file to open
//   pcMode: mode of the file to open
//           r  : read only
//           w  : write only
//           a  : append
//           r+ : read only + extension (write)
//           w+ : write only + extension (read)
//           a+ : append + extension (read)
// return:
//   pointer to a FILE on success
//   null on failure
// note:
//   this is implmenetation of stdio functions.
FILE *kOpenFile(const char *pcFileName, const char *pcMode);


// read content in a file
// params:
//   pvBuffer: pointer to buffer to save the content
//   dwSize: size of data to read at once
//   dwCount: number of data to read
//   pstFile: file handle to read
// return:
//   dwCount * dwSize on success
//   number of data that was read on partial success
//   negative numbers on failure
DWORD kReadFile(void *pvBuffer, DWORD dwSize, DWORD dwCount, FILE *pstFile);


// write data in buffer to the file in storage.
// params:
//   pvBuffer: pointer to a buffer that will be written to storage
//   dwSize: size of data to write at once
//   dwCount: number of data to write
//   pstFIle: file handle to write
// return:
//   -1 on failure
//   dwCount * dwSize on success
//   number of data that was written on partial success
DWORD kWriteFile(
    const void *pvBuffer,
    DWORD dwSize,
    DWORD dwCount,
    FILE *pstFile
);


// Set the current pointer of a file to the desired location
// params:
//   pstFile: pointer to FILE data structure that will be modified
//   iOffset: offset count from iOrigin
//   iOrigin: reference position to move the file pointer
// return:
//   0 on success
//   -1 on failure
// note:
//  For iOrigin, there are three options
//    FILESYSTEM_SEEK_SET: move pointer from the start of the file
//    FILESYSTEM_SEEK_CUR: move pointer from the current of the file
//    FILESYSTEM_SEEK_END: move pointer from the end of the file
int kSeekFile(FILE *pstFile, int iOffset, int iOrigin);


// Close a open file
// params:
//   pstFile: pointer to a opened file to close
// return:
//   0 on success
//   -1 on failure (when pstFile is not valid)
int kCloseFile(FILE *pstFile);


// Remove a file from root directory
// params:
//   pcFileName: string of a file to delete
// return:
//   0 on Success
//   -1 on Failure
// notes:
//   this function does not wipe data of a file in the storage. Instead,
//   unlink all clusters in the link area.
int kRemoveFile(const char *pcFileName);


// Open a directory named pcDirectoryName
// params:
//   pcDirectoryName: name of a directory to open
// return:
//   NULL on failure
//   pointer to a directory handle on success
// notes:
//   In MINT64OS, only root directory is supported, so whatever directory name
//   is given, root directory will be opened.
DIR *kOpenDirectory(const char *pcDirectoryName);


// Iterate to the next directory entry in the pstDirectory
// params:
//   pstDirectory: point to a directory data structure to iterate
// return:
//   NULL on failure
//   NULL if there is no more dir entry to iterate
//   dir entry if there is dir entry to iterate
DIRECTORYENTRY *kReadDirectory(DIR *pstDirectory);


// set the current offset of pstDirectory to 0, so it points to the first
// directory entry.
// params:
//   pstDirectory: point to a directory data structure to rewind
// notes:
//   if pstDirectory is invalid data structure or NULL, nothing occurs
void kRewindDirectory(DIR *pstDirectory);


// Close a opened directory data structure
// params:
//   pstDirectory: pointer to a directory data structure to close
// return:
//  0 on success
// -1 on failure
int kCloseDirectory(DIR *pstDirectory);


// Writes 0 as many as dwCount to file
// params:
//   pstFile: file data structure that 0 will be written
//   dwCount: number of 0 to write
// return:
//   TRUE on success
//   FALSE on failure
// note:
//    partial writing also return false
BOOL kWriteZero(FILE *pstFile, DWORD dwCount);


// Check whether an entry (file) in directory is open or not
// params:
//   pstEntry: entry of a directory to check
// return:
//   TRUE on success
//   FALSE on failure
// note:
//   Opened files are not managed in efficient way, so this function is not
//   efficient.
BOOL kIsFileOpened(const DIRECTORYENTRY *pstEntry);


// get a free handle from file/dir handle pool
// return:
//   on success, pointer to a avaiable FILE handle.
//   on failure, NULL (pool has no available handles)
static void *kAllocateFileDirectoryHandle();


// free a used handle
// note:
//   pstFile must be a valid file handle. Otherwise, unexpected behavior occurs
static void kFreeFileDirectoryHandle(FILE *pstFile);


// create a file and fill it to root directory entry.
// params:
//   pcFileName: name of a file to create
//   pstEntry: pointer to a directory entry that will have the file info
//   piDirectoryEntryIndex: index of the entry in the root directory
// return:
//   TRUE on success
//   FALSE on failure
static BOOL kCreateFile(
    const char *pcFileName,
    DIRECTORYENTRY *pstEntry,
    int *piDirectoryEntryIndex
);


// free all connected clusters from the given cluster index
// params:
//   dwClusterIndex: start index of a cluster to free
// return:
//   TRUE on success
//   FALSE on failure 
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex);


// flush meta data in FILE handle in memory to directory entry in storage
// params:
//   pstFileHandle: file handle
// return:
//   TRUE on success
//   FALSE on failure 
static BOOL kUpdateDirectoryEntry(FILEHANDLE *pstFileHandle);

#endif /* __FILESYSTEM_H__ */