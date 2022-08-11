#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Utility.h"

// debug
#include "Console.h"


// MINT64OS filesystem manager 
static FILESYSTEMMANAGER gs_stFileSystemManager;

// file system temporary buffer
// can have up to 512 clusters
static BYTE gs_vbTempBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];


// function pointer types related to hard disk control 

fReadHDDInformation gs_pfReadHDDInformation = NULL;

fReadHDDSector gs_fpReadHDDSector = NULL;

fWriteHDDSector gs_pfWriteHDDSector = NULL;


// initialize file system
// return:
//   True on success, False on failure
// info:
//   This function initialize HDD disk controller before initializing filesystem
//   
//   possible failures:
//     when HDD initialization failed
//     when mounting filesystem failed  
BOOL kInitializeFileSystem(void) {

    kMemSet(
        &gs_stFileSystemManager,
        0,
        sizeof(gs_stFileSystemManager)    
    );

    kInitializeMutex(&(gs_stFileSystemManager.stMutex));

    // If HDD initialization is successful, the global static function
    // pointers are set to functions for the hard disk. 
    if (kInitializeHDD() == TRUE) {
        gs_pfReadHDDInformation = kReadHDDInformation;
        gs_fpReadHDDSector = kReadHDDSector;
        gs_pfWriteHDDSector = kWriteHDDSector;
    }
    else {
        return FALSE;
    }

    // connect to filesystem in HDD if HDD has MINT filesystem
    if (kMount() == FALSE) {
        return FALSE;
    }

    // allocate memory for file and dir handler pool
    gs_stFileSystemManager.pstHandlePool = 
        (FILE *) kAllocateMemory(FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE));

    if (gs_stFileSystemManager.pstHandlePool == NULL) {
        gs_stFileSystemManager.bMounted = FALSE;
        return FALSE;
    }

    kMemSet(
        gs_stFileSystemManager.pstHandlePool,
        0,
        FILESYSTEM_HANDLE_MAXCOUNT * sizeof(FILE)
    );


    return TRUE;
}


/* Low Level Functions */

// Check if HDD has MINT filesystem by reading MBR in HDD.
// If it is a MINT file system, this function reads various information
// related to the file system and inserts it into file system data structure.
// return:
//   TRUE on success and FALSE on failure
// info:
//   mount primary slave drive.
BOOL kMount(void) {
    MBR *pstMBR;

    kLock(&(gs_stFileSystemManager.stMutex));


    /* read MBR in primary master HDD */

    if (gs_fpReadHDDSector(TRUE, FALSE, 0, 1, gs_vbTempBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    pstMBR = (MBR *) gs_vbTempBuffer;
    

    /* check if MBR has MINT filesystem signature */

    if (pstMBR->dwSignature != FILESYSTEM_SIGNATURE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }


    /* read file system related information from HDD */

    gs_stFileSystemManager.bMounted = TRUE;

    // reserved area start address
    gs_stFileSystemManager.dwReservedSectorCount =
        pstMBR->dwReservedSectorCount;

    // link area start address
    gs_stFileSystemManager.dwClusterLinkAreaStartAddress = 
        pstMBR->dwReservedSectorCount + 1;

    // link area size
    gs_stFileSystemManager.dwClusterLinkAreaSize =
        pstMBR->dwClusterLinkSectorCount;

    // data area start address
    gs_stFileSystemManager.dwDataAreaStartAddress =
        pstMBR->dwReservedSectorCount + pstMBR->dwClusterLinkSectorCount + 1;

    // total cluster size
    gs_stFileSystemManager.dwTotalClusterCount = pstMBR->dwTotalClusterCount;

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return TRUE;
}


// Create a file system on HDD
// return:
//   TRUE on success, FALSE on failure
BOOL kFormat(void) {
    HDDINFORMATION *pstHDD;
    MBR *pstMBR;
    DWORD dwTotalSectorCount, dwRemainSectorCount;
    DWORD dwMaxClusterCount, dwClusterCount;
    DWORD dwClusterLinkSectorCount;
    DWORD i;

    kLock(&(gs_stFileSystemManager.stMutex));

    
    /* Calculate the starting address and size of data area, link area  */

    pstHDD = (HDDINFORMATION *) gs_vbTempBuffer;

    if (gs_pfReadHDDInformation(TRUE, FALSE, pstHDD) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    dwTotalSectorCount = pstHDD->dwTotalSectors;

    // calculate total number of clusters on HDD
    dwMaxClusterCount = dwTotalSectorCount / FILESYSTEM_SECTORSPERCLUSTER;


    /** Intermediate process for data area calculation **/

    // Calculate the number of sectors in the link area corresponding to
    // the total number of clusterers on the disk. 
    // each link is 4 bytes, so one sector has up to 128 links 
    dwClusterLinkSectorCount = (dwMaxClusterCount + 127) / 128;

    // data area to use = total sector - link area - MBR 
    dwRemainSectorCount = dwTotalSectorCount - dwClusterLinkSectorCount - 1;
    dwClusterCount = dwRemainSectorCount / FILESYSTEM_SECTORSPERCLUSTER;

    // link area to use
    // recalculate link area for the data rea
    // Because this calculation is approximate calculation, Every cluster is 
    // not used
    dwClusterLinkSectorCount = (dwClusterCount + 127) / 128;


    /* write the calculated information to MBR */

    // read MBR
    if (gs_fpReadHDDSector(TRUE, FALSE, 0, 1, gs_vbTempBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    pstMBR = (MBR *) gs_vbTempBuffer;

    // be careful, this will fail booting on acer laptop **
    kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));
    pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
    pstMBR->dwReservedSectorCount = 0;
    pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
    pstMBR->dwTotalClusterCount = dwClusterCount;

    // write the modified MBR to HDD
    if (gs_pfWriteHDDSector(TRUE, FALSE, 0, 1, gs_vbTempBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }


    /* initialize link area and first cluster which is reserved for root
        directory
    */

    kMemSet(gs_vbTempBuffer, 0, 512);

    // initialize link area and first cluster in data area
    // data area is located right after link area
    for (
        i = 0;
        i < dwClusterLinkSectorCount + FILESYSTEM_SECTORSPERCLUSTER;
        i++
    ) {
        // Initialize the first link to the root directory 
        if (i == 0) {
            // mark as assigned 
            ((DWORD *) gs_vbTempBuffer)[0] = FILESYSTEM_LASTCLUSTER;
        }
        else {
            ((DWORD *) gs_vbTempBuffer)[0] = FILESYSTEM_FREECLUSTER;
        }

        if (gs_pfWriteHDDSector(TRUE, FALSE, i+1, 1, gs_vbTempBuffer) == FALSE) {
            kUnlock(&(gs_stFileSystemManager.stMutex));
            return FALSE;
        }
    }

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return TRUE;
}


// Return information on the hard disk connected to the file system
// params:
//   pstInformation: A pointer to a variable that will hold the info
// return:
//   TRUE on success, FALSE on failure
BOOL kGetHDDInformation(HDDINFORMATION *pstInformation) {
    BOOL bResult;

    kLock(&(gs_stFileSystemManager.stMutex));

    bResult = gs_pfReadHDDInformation(TRUE, FALSE, pstInformation);

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return bResult;
}


// Read a sector at an offset in the cluster link table
// params:
//   dwOffset: Offset to read from cluster link table
//   pbBuffer: A pointer to a variable that will hold the data
// return:
//   TRUE on success, FALSE on failure
static BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_fpReadHDDSector(
        TRUE,
        FALSE,
        dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress,
        1,
        pbBuffer
    );
}


// Write a sector at an offset in the cluster link table
// params:
//   dwOffset: Offset to write to cluster link table
//   pbBuffer: A pointer to a variable that will be written to link area
// return:
//   TRUE on success, FALSE on failure
static BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(
        TRUE,
        FALSE,
        dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress,
        1,
        pbBuffer
    );
}


// Read one cluster at an offset in the data area
// params:
//   dwOffset: Offset to read from data area
//   pbBuffer: A pointer to a variable that will hold the data
// return:
//   TRUE on success, and FALSE on failure
static BOOL kReadCluster(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_fpReadHDDSector(
        TRUE,
        FALSE,
        (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) +
            gs_stFileSystemManager.dwDataAreaStartAddress,
        FILESYSTEM_SECTORSPERCLUSTER,
        pbBuffer
    );
}


// Write one cluster to the offset of the data area
// params:
//   dwOffset: Offset to write to data area
//   pbBuffer: A pointer to a variable that will be written to data area
// return:
//   TRUE on success, and FALSE on failure
static BOOL kWriteCluster(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(
        TRUE,
        FALSE,
        (dwOffset * FILESYSTEM_SECTORSPERCLUSTER) +
            gs_stFileSystemManager.dwDataAreaStartAddress,
        FILESYSTEM_SECTORSPERCLUSTER,
        pbBuffer
    );
}


// Search for an empty cluster in the Cluster Links table area
// return:
//   index to empty cluster in data area
static DWORD kFindFreeCluster(void) {
    DWORD dwLinkCountInSector;
    DWORD dwLastSectorOffset;
    DWORD dwCurrentSectorOffset;

    DWORD i, j;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return FILESYSTEM_LASTCLUSTER;
    }

    dwLastSectorOffset =
        gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset;

    for (
        i = 0;
        i < gs_stFileSystemManager.dwClusterLinkAreaSize;
        i++
    ) {

        // the cluster link area cannot always be used up. The number of
        // available links in the last sector is calculated using the number of
        // clusters in the data area. 
        if (
            (dwLastSectorOffset + i) ==
            (gs_stFileSystemManager.dwClusterLinkAreaSize - 1)
        ) {
            dwLinkCountInSector =
                gs_stFileSystemManager.dwTotalClusterCount % 128;
        }
        else {
            dwLinkCountInSector = 128;
        }

        // sector offset of the cluster link table to read this time
        // This is needed because
        //   for loop loops from 0 or
        //   all links of dwLastSectorOffset are used
        dwCurrentSectorOffset = 
            (dwLastSectorOffset + i) %
            gs_stFileSystemManager.dwClusterLinkAreaSize;

        if (
            kReadClusterLinkTable(
                dwCurrentSectorOffset,
                gs_vbTempBuffer
            ) == FALSE
        ) {
            return FILESYSTEM_LASTCLUSTER;
        }


        /* search for empty cluster in the current sector */

        for (j = 0; j < dwLinkCountInSector; j++) {
            if (((DWORD *)gs_vbTempBuffer)[j] == FILESYSTEM_FREECLUSTER) {
                break;
            }
        }

        // found offset of free link in current sector
        if (j != dwLinkCountInSector) {

            // save offset of the current sector in link area to file manager
            gs_stFileSystemManager.dwLastAllocatedClusterLinkSectorOffset =
                dwCurrentSectorOffset;
            
            // calculate index to empty cluster in data area
            return (128 * dwCurrentSectorOffset) + j;
        }
    }

    // means no empty cluster 
    return FILESYSTEM_LASTCLUSTER;
}


// set value in cluster link corresponding to a cluster index
// params:
//   dwClusterIndex: index to a cluster in data area
//   dwData: 
// return:
//   TRUE on success, FALSE on failure
static BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData) {
    DWORD dwSectorOffset;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return FALSE;
    }

    /* find the index for link area from cluster index */

    // find the sector offset corresponding to the cluster in the link area 
    // each link is 4 bytes so each sector contains 128 links
    dwSectorOffset = dwClusterIndex / 128;

    if (dwSectorOffset >= gs_stFileSystemManager.dwClusterLinkAreaSize) {
        return FALSE;
    }

    /* get link data */

    if (kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) {
        return FALSE;
    }

    /* set data */

    ((DWORD *) gs_vbTempBuffer)[dwClusterIndex % 128] = dwData;

    if (kWriteClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) {
        return FALSE;
    }

    return TRUE;
}


// get value in cluster link corresponding to a cluster index
// params:
//   dwClusterIndex: index to a cluster in data area
//   pdwData: a pointer to a varaible that will contain the link data 
// return:
//   TRUE on success, FALSE on failure
static BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD *pdwData) {
    DWORD dwSectorOffset;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return FALSE;
    }

    /* find the index for link area from cluster index */

    // find the sector offset corresponding to the cluster in the link area 
    // each link is 4 bytes so each sector contains 128 links
    dwSectorOffset = dwClusterIndex / 128;

    if (dwSectorOffset >= gs_stFileSystemManager.dwClusterLinkAreaSize) {
        return FALSE;
    }

    /* get link data */

    if (kReadClusterLinkTable(dwSectorOffset, gs_vbTempBuffer) == FALSE) {
        return FALSE;
    }

    *pdwData = ((DWORD *) gs_vbTempBuffer)[dwClusterIndex % 128];
    return TRUE;
}


// get idex to empty entry in root directory
// return:
//   index to free entry in root directory
//   -1 on failure
static int kFindFreeDirectoryEntry(void) {
    DIRECTORYENTRY *pstEntry;
    int i;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return -1;
    }

    // read cluster containing root directory
    if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
        return -1;
    }

    pstEntry = (DIRECTORYENTRY *) gs_vbTempBuffer;


    /* search for free entry in root directory */

    for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if (pstEntry[i].dwStartClusterIndex == 0) {
            return i;
        }
    }

    return -1;
}


// write directory entry to index of root directory
// params:
//   iIndex: Directory entry index to set in the root directory
//   pstEntry: pointer containing data to write to straoge
// return:
//   TRUE on success, FALSE on failure
static BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return FALSE;
    }

    // check if iIndex is in domain
    if (iIndex < 0 || iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
        return FALSE;
    }


    /* read cluster containing directory */

    if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
        return FALSE;
    }


    /* set entry in  directory */

    pstRootEntry = (DIRECTORYENTRY *) gs_vbTempBuffer;

    kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));

    /* write to stroage */

    if (kWriteCluster(0, gs_vbTempBuffer) == FALSE) {
        return FALSE;
    }

    return TRUE;
}


// read directory entry at index of root directory
// params:
//   iIndex: Directory entry index to read in the root directory
//   pstEntry: pointer that will contain the data 
// return:
//   TRUE on success, FALSE on failure
static BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return FALSE;
    }

    // check if iIndex is in domain
    if (iIndex < 0 || iIndex >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT) {
        return FALSE;
    }


    /* read cluster containing directory */

    if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
        return FALSE;
    }

    pstRootEntry = (DIRECTORYENTRY *) gs_vbTempBuffer;
    kMemCpy(pstEntry, pstRootEntry + iIndex, sizeof(DIRECTORYENTRY));
    return TRUE;
}



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
) {
    DIRECTORYENTRY *pstRootEntry;
    int i;
    int iLength;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return FALSE;
    }

    /* read cluster containing directory */

    if (kReadCluster(0, gs_vbTempBuffer) == FALSE) {
        return FALSE;
    }

    iLength = kStrLen(pcFileName);
    pstRootEntry = (DIRECTORYENTRY *) gs_vbTempBuffer;


    /* search for direcotry entry that contains the pcFileName */

    for (i = 0; i < FILESYSTEM_MAXDIRECTORYENTRYCOUNT; i++) {
        if (kMemCmp(pstRootEntry[i].vcFileName, pcFileName, iLength) == 0) {
            kMemCpy(pstEntry, pstRootEntry + i, sizeof(DIRECTORYENTRY));
            return i;
        }
    }
    return -1;
}


// copy global file manager to pstManager
// params:
//   pstManager: pointer to a variable that will contain global file manager
//               info
void kGetFileSystemInformation(FILESYSTEMMANAGER *pstManager) {
    kMemCpy(
        pstManager,
        &gs_stFileSystemManager,
        sizeof(gs_stFileSystemManager)
    );
}


/* high level functions */

// get a free handle from file/dir handle pool
// return:
//   on success, pointer to a avaiable FILE handle.
//   on failure, NULL (pool has no available handles)
// note:
//   returned handle has the type of FILE. If you want directory handle,
//   change the type to FILESYSTEM_TYPE_DIRECTORY
static void *kAllocateFileDirectoryHandle() {
    int i;
    FILE *pstFile;

    pstFile = gs_stFileSystemManager.pstHandlePool;

    for (int i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT; i++) {
        if (pstFile->bType == FILESYSTEM_TYPE_FREE) {
            pstFile->bType = FILESYSTEM_TYPE_FILE;
            return pstFile;
        }
        pstFile++;
    }

    return NULL;
}


// free a used handle
// note:
//   pstFile must be a valid file handle. Otherwise, unexpected behavior occurs.
static void kFreeFileDirectoryHandle(FILE *pstFile) {
    kMemSet(pstFile, 0, sizeof(FILE));
    pstFile->bType = FILESYSTEM_TYPE_FREE;
}


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
) {
    DWORD dwCluster;

    dwCluster = kFindFreeCluster();
    
    if (dwCluster == FILESYSTEM_LASTCLUSTER)
        return FALSE;
    
    // set the cluster to used one
    if (kSetClusterLinkData(dwCluster, FILESYSTEM_LASTCLUSTER) == FALSE)
        return FALSE;
    
    // find an empty entry in the root directory.
    *piDirectoryEntryIndex = kFindFreeDirectoryEntry();

    if (*piDirectoryEntryIndex == -1)
        goto ERROR;



    /* create a file */

    kMemCpy(pstEntry->vcFileName, pcFileName, kStrLen(pcFileName) + 1);
    pstEntry->dwStartClusterIndex = dwCluster;
    pstEntry->dwFileSize = 0;

    if (kSetDirectoryEntryData(*piDirectoryEntryIndex, pstEntry) == FALSE)
        goto ERROR;

    return TRUE;

ERROR:
    kSetClusterLinkData(dwCluster, FILESYSTEM_FREECLUSTER);
    return FALSE;
}


// free all connected clusters from the given cluster index
// params:
//   dwClusterIndex: start index of a cluster to free
// return:
//   TRUE on success
//   FALSE on failure 
static BOOL kFreeClusterUntilEnd(DWORD dwClusterIndex) {
    DWORD dwCurrentClusterIndex;
    DWORD dwNextClusterIndex;

    dwCurrentClusterIndex = dwClusterIndex;

    while (dwCurrentClusterIndex != FILESYSTEM_LASTCLUSTER) {
        // get index of the next cluster
        if (!kGetClusterLinkData(dwCurrentClusterIndex, &dwNextClusterIndex))
            return FALSE;

        // free cluster
        if (!kSetClusterLinkData(dwCurrentClusterIndex, FILESYSTEM_FREECLUSTER))
            return FALSE;

        dwCurrentClusterIndex = dwNextClusterIndex;
    }

    return TRUE;
}


// flush meta data in FILE handle in memory to directory entry in storage
// params:
//   pstFileHandle: file handle
// return:
//   TRUE on success
//   FALSE on failure 
static BOOL kUpdateDirectoryEntry(FILEHANDLE *pstFileHandle) {
    DIRECTORYENTRY stEntry;

    if (pstFileHandle == NULL)
        return FALSE;
    if (
        kGetDirectoryEntryData(
            pstFileHandle->iDirectoryEntryOffset,
            &stEntry
        ) == FALSE
    )
        return FALSE;

    stEntry.dwFileSize = pstFileHandle->dwFileSize;
    stEntry.dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;

    if (
        kSetDirectoryEntryData(
            pstFileHandle->iDirectoryEntryOffset,
            &stEntry
        ) == FALSE
    )
        return FALSE;

    return TRUE;
}


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
//   every stdio function in this source code is incomplete.
//   For example, writing to a file opened with 'r' should not be allowed.
//   but writing is not banned.
//   Currently, no matter what options is given, reading and writing is
//    possible to all files  
FILE *kOpenFile(const char *pcFileName, const char *pcMode) {
    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset;
    int iFileNameLength;
    DWORD dwSecondCluster;
    FILE *pstFile;

    kLock(&(gs_stFileSystemManager.stMutex));


    /* check if pcFileName is less than the maximum file length */

    iFileNameLength = kStrLen(pcFileName);

    if (iFileNameLength > (sizeof(stEntry.vcFileName) - 1))
        return NULL;
    
    if (iFileNameLength == 0)
        return NULL;


    /* check if file exists in the storage */

    iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);

    // file does not exist
    if (iDirectoryEntryOffset == -1) {
        // 'r' and 'r+' mode 
        if (pcMode[0] == 'r')
            goto ERROR;

        // 'w', 'w+', 'a', 'a+' mode: create new file
        if (kCreateFile(pcFileName, &stEntry, &iDirectoryEntryOffset) == FALSE)
            goto ERROR;
        
    }
    else {
        // 'w' and 'w+' mode
        if (pcMode[0] == 'w') {
            // get the next cluster index to be freed until the end
            if (
                kGetClusterLinkData(
                    stEntry.dwStartClusterIndex,
                    &dwSecondCluster
                ) == FALSE
            )
                goto ERROR;

            if (kFreeClusterUntilEnd(dwSecondCluster) == FALSE)
                goto ERROR;

            // set the start index as the last cluster of the file
            if (
                kSetClusterLinkData(
                    stEntry.dwStartClusterIndex,
                    FILESYSTEM_LASTCLUSTER
                ) == FALSE
            )
                goto ERROR;

            // update the cluster to the stoage 
            stEntry.dwFileSize = 0;
            if (
                kSetDirectoryEntryData(
                    iDirectoryEntryOffset,
                    &stEntry
                ) == FALSE
            )
                goto ERROR;
        }
    }


    /* allocate a new FILE HANDLE */

    if ((pstFile = kAllocateFileDirectoryHandle()) == NULL)
        goto ERROR;
    
    pstFile->bType = FILESYSTEM_TYPE_FILE;
    pstFile->stFileHandle.iDirectoryEntryOffset = iDirectoryEntryOffset;
    pstFile->stFileHandle.dwFileSize = stEntry.dwFileSize;
    pstFile->stFileHandle.dwStartClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwPreviousClusterIndex = stEntry.dwStartClusterIndex;
    pstFile->stFileHandle.dwCurrentOffset = 0;

    // 'a' and 'a+' mode
    if (pcMode[0] == 'a')
        kSeekFile(pstFile, 0, FILESYSTEM_SEEK_END);
    
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return pstFile;

ERROR:
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return NULL;
}


// read content in a file
// params:
//   pvBuffer: pointer to buffer to save the content
//   dwSize: size of data to read at once
//   dwCount: number of data to read
//   pstFile: file handle to read
// return:
//   dwCount on success
//   negative numbers on failure
DWORD kReadFile(void *pvBuffer, DWORD dwSize, DWORD dwCount, FILE *pstFile) {
    DWORD dwTotalCount;
    DWORD dwReadCount;
    DWORD dwOffsetInCluster;
    DWORD dwCopySize;
    FILEHANDLE *pstFileHandle;
    DWORD dwNextClusterIndex;

    if (pstFile == NULL || pstFile->bType != FILESYSTEM_TYPE_FILE)
        return -1;

    pstFileHandle = &(pstFile->stFileHandle);

    // end of file
    if (pstFileHandle->dwCurrentOffset == pstFileHandle->dwFileSize)
        return 0;

    // end of file
    if (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER)
        return 0;

    // calculate actually readable bytes
    dwTotalCount = MIN(
        dwSize * dwCount,
        pstFileHandle->dwFileSize - pstFileHandle->dwCurrentOffset
    );

    kLock(&(gs_stFileSystemManager.stMutex));


    /* read data from storage */

    dwReadCount = 0;
    while (dwReadCount != dwTotalCount) {
        if (
            kReadCluster(
                pstFileHandle->dwCurrentClusterIndex,
                gs_vbTempBuffer
            ) == FALSE
        )
            break;

        dwOffsetInCluster = 
            pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;

        // If it spans multiple clusters, read what's left from the current
        // cluster and move on to the next cluster.
        dwCopySize = MIN(
            FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster,
            dwTotalCount - dwReadCount
        );

        kMemCpy(
            (char *) pvBuffer + dwReadCount,
            gs_vbTempBuffer + dwOffsetInCluster, dwCopySize
        );

        dwReadCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;


        /* if necessary, move to the next cluster */

        // case to think: 
        //   offset in cluster = 0, data to read = 5 bytes
        // in this case, next cluster is not needed.
        if ((pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) == 0) {
            
            // temporary solution for giving weired cluster index
            kSleep(300);
            if (
                kGetClusterLinkData(
                    pstFileHandle->dwCurrentClusterIndex,
                    &dwNextClusterIndex
                ) == FALSE
            )
                break;
            
            pstFileHandle->dwPreviousClusterIndex =
                pstFileHandle->dwCurrentClusterIndex;
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return dwReadCount;
}


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
) {
    DWORD dwWriteCount;
    DWORD dwTotalCount;
    DWORD dwOffsetInCluster;
    DWORD dwCopySize;
    DWORD dwAllocatedClusterIndex;
    DWORD dwNextClusterIndex;
    FILEHANDLE *pstFileHandle;

    if (pstFile == NULL || (pstFile->bType != FILESYSTEM_TYPE_FILE))
        return -1;
    
    pstFileHandle = &(pstFile->stFileHandle);
    dwTotalCount = dwSize * dwCount;

    kLock(&(gs_stFileSystemManager.stMutex));

    /* write data to file in storage */

    dwWriteCount = 0;
    while (dwWriteCount != dwTotalCount) {
        // if current cluster is the end of file, allocate new cluster
        if (pstFileHandle->dwCurrentClusterIndex == FILESYSTEM_LASTCLUSTER) {

            dwAllocatedClusterIndex = kFindFreeCluster();
            if (dwAllocatedClusterIndex == FILESYSTEM_LASTCLUSTER) {
                // debug
                kPrintf("Error1");
                break;
            }

            // connect null to cur link
            if (
                kSetClusterLinkData(
                    dwAllocatedClusterIndex,
                    FILESYSTEM_LASTCLUSTER
                ) == FALSE
            ) {
                // debug
                kPrintf("Error2");
                break;
            }

            // connect cur link to prev link
            if (
                kSetClusterLinkData(
                    pstFileHandle->dwPreviousClusterIndex,
                    dwAllocatedClusterIndex
                ) == FALSE
            ) {
                kSetClusterLinkData(
                    dwAllocatedClusterIndex,
                    FILESYSTEM_FREECLUSTER
                );
                // debug
                kPrintf("Error3");
                break;
            }

            pstFileHandle->dwCurrentClusterIndex = dwAllocatedClusterIndex;

            // Since 1 cluster is the smallest unit to write at a time,
            // clear temp buffer before writing.
            kMemSet(gs_vbTempBuffer, 0, sizeof(gs_vbTempBuffer));
        }
        // functions in this file system cannot write data smaller than the
        // cluster size at once. So it is necessary to read a cluster and modify
        // the cluster and then write it back to storage.
        else if (
            (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE) != 0 ||
            (dwTotalCount - dwWriteCount) < FILESYSTEM_CLUSTERSIZE
        ) {
            if (
                kReadCluster(
                    pstFileHandle->dwCurrentClusterIndex,
                    gs_vbTempBuffer
                ) == FALSE
            ) {
                // debug
                kPrintf("Error4");
                break;
            }
        }

        dwOffsetInCluster =
            pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE;

        dwCopySize =
            MIN(
                FILESYSTEM_CLUSTERSIZE - dwOffsetInCluster,
                dwTotalCount - dwWriteCount
            );

        kMemCpy(
            gs_vbTempBuffer + dwOffsetInCluster,
            (char *) pvBuffer + dwWriteCount,
            dwCopySize
        );


        /* write buffer to storage */

        if (
            kWriteCluster(
                pstFileHandle->dwCurrentClusterIndex,
                gs_vbTempBuffer
            ) == FALSE
        ) {
            // debug
            kPrintf("Error5");
            break;
        }
        dwWriteCount += dwCopySize;
        pstFileHandle->dwCurrentOffset += dwCopySize;


        /* If current cluster is full, move to the next cluster */

        if (pstFileHandle->dwCurrentOffset % FILESYSTEM_CLUSTERSIZE == 0)
        {
            if (
                kGetClusterLinkData(
                    pstFileHandle->dwCurrentClusterIndex,
                    &dwNextClusterIndex
                ) == FALSE
            ) {
                // debug
                kPrintf("Error6");
                break;
            }

            pstFileHandle->dwPreviousClusterIndex =
                pstFileHandle->dwCurrentClusterIndex;
            
            pstFileHandle->dwCurrentClusterIndex = dwNextClusterIndex;
        }
    }

    // if some data was written to the disk, write the updated meta data to
    // disk 
    if (pstFileHandle->dwFileSize < pstFileHandle->dwCurrentOffset) {
        pstFileHandle->dwFileSize = pstFileHandle->dwCurrentOffset;
        kUpdateDirectoryEntry(pstFileHandle);
    }

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return dwWriteCount;
}


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
int kSeekFile(FILE *pstFile, int iOffset, int iOrigin) {
    DWORD dwRealOffset;

    DWORD dwClusterOffsetToMove;
    DWORD dwCurrentClusterOffset;
    DWORD dwLastClusterOffset;
    DWORD dwMoveCount;

    DWORD i;
    DWORD dwStartClusterIndex;
    DWORD dwPreviousClusterIndex;
    DWORD dwCurrentClusterIndex;
    
    FILEHANDLE *pstFileHandle;

    if (pstFile == NULL || pstFile->bType != FILESYSTEM_TYPE_FILE)
        return -1;

    pstFileHandle = &(pstFile->stFileHandle);


    /* calcuate linear offset to be moved based on the iOrigin */

    switch (iOrigin) {
    
    case FILESYSTEM_SEEK_SET:
        if (iOffset <= 0)
            dwRealOffset = 0;
        else
            dwRealOffset = iOffset;
        break;
    
    case FILESYSTEM_SEEK_CUR:
        // If the offset to move is negative and greater than the value of the
        // current file pointer, it cannot go any further, so move to the
        // beginning of the file.
        if (iOffset < 0 && pstFileHandle->dwCurrentOffset <= (DWORD) -iOffset)
            dwRealOffset = 0;
        else
            dwRealOffset = pstFileHandle->dwCurrentOffset + iOffset;
        break;

    case FILESYSTEM_SEEK_END:
        // If the offset to move is negative and greater than the value of the
        // current file size, it cannot go any further, so move to the
        // beginning of the file.
        if (iOffset < 0 && pstFileHandle->dwFileSize <= (DWORD) -iOffset)
            dwRealOffset = 0;
        else
            dwRealOffset = pstFileHandle->dwFileSize + iOffset;
        break;
    }


    /* calculate cluster index to be moved */

    dwLastClusterOffset = pstFileHandle->dwFileSize / FILESYSTEM_CLUSTERSIZE;
    dwClusterOffsetToMove = dwRealOffset / FILESYSTEM_CLUSTERSIZE;
    dwCurrentClusterIndex =
        pstFileHandle->dwCurrentOffset / FILESYSTEM_CLUSTERSIZE;

    // ex:
    // <s>    <c>    <e>   <d>
    //  |------|------|00000|
    // <s>: start cluster idx
    // <c>: current cluster idx
    // <d>: dest cluster idx
    // <e>: end cluster idx
    if (dwLastClusterOffset < dwClusterOffsetToMove) {
        // Although <d> beyond <e>, set the dest cluster index to the <e>
        // the code at the last section will fill 0s and go to <d> 
        dwMoveCount = dwLastClusterOffset - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    }
    // ex:
    // <s>    <c>    <d>   <e>
    //  |------|------|-----|
    else if (dwCurrentClusterOffset <= dwClusterOffsetToMove) {
        dwMoveCount = dwClusterOffsetToMove - dwCurrentClusterOffset;
        dwStartClusterIndex = pstFileHandle->dwCurrentClusterIndex;
    }
    // ex:
    // <s>    <d>    <c>   <e>
    //  |------|------|-----|
    else {
        dwMoveCount = dwClusterOffsetToMove;
        dwStartClusterIndex = pstFileHandle->dwStartClusterIndex;
    }

    kLock(&(gs_stFileSystemManager.stMutex));


    /* iterate and find the cluster to be moved */

    dwCurrentClusterIndex = dwStartClusterIndex;
    for (i = 0; i < dwMoveCount; i++) {
        dwPreviousClusterIndex = dwCurrentClusterIndex;

        if (
            kGetClusterLinkData(
                dwPreviousClusterIndex,
                &dwCurrentClusterIndex
            ) == FALSE
        ) {
            kUnlock(&(gs_stFileSystemManager.stMutex));
            return -1;
        }
    }
       
    if (dwMoveCount > 0) {
        pstFileHandle->dwPreviousClusterIndex = dwPreviousClusterIndex;
        pstFileHandle->dwCurrentClusterIndex = dwCurrentClusterIndex;
    }
    else if (dwStartClusterIndex == pstFileHandle->dwStartClusterIndex) {
        pstFileHandle->dwPreviousClusterIndex =
            pstFileHandle->dwStartClusterIndex;
        
        pstFileHandle->dwCurrentClusterIndex = 
            pstFileHandle->dwStartClusterIndex;
    }

    // when <d> is beyond <e>
    if (dwLastClusterOffset < dwClusterOffsetToMove) {
        pstFileHandle->dwCurrentOffset = pstFileHandle->dwFileSize;
        kUnlock(&(gs_stFileSystemManager.stMutex));

        if (
            kWriteZero(
                pstFile,
                dwRealOffset - pstFileHandle->dwFileSize
            ) == FALSE
        )
            return -1;
    }

    pstFileHandle->dwCurrentOffset = dwRealOffset;
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return 0;
}


// Close a open file
// params:
//   pstFile: pointer to a opened file to close
// return:
//   0 on success
//   -1 on failure (when pstFile is not valid)
int kCloseFile(FILE *pstFile) {
    if (pstFile == NULL || pstFile->bType != FILESYSTEM_TYPE_FILE)
        return -1;

    kFreeFileDirectoryHandle(pstFile);
    return 0;
}


// Remove a file from root directory
// params:
//   pcFileName: string of a file to delete
// return:
//   0 on Success
//   -1 on Failure
// notes:
//   this function does not wipe data of a file in the storage. Instead,
//   unlink all clusters in the link area.
int kRemoveFile(const char *pcFileName) {
    DIRECTORYENTRY stEntry;
    int iDirectoryEntryOffset;
    int iFileNameLength;

    iFileNameLength = kStrLen(pcFileName);

    if (iFileNameLength > (sizeof(stEntry.vcFileName) - 1))
        return -1;
    if (iFileNameLength == 0)
        return -1;

    kLock(&(gs_stFileSystemManager.stMutex));

    iDirectoryEntryOffset = kFindDirectoryEntry(pcFileName, &stEntry);
    
    // file does not exists
    if (iDirectoryEntryOffset == -1)
        goto ERROR;

    // file is opened
    if (kIsFileOpened(&stEntry))
        goto ERROR;
    
    
    /* unlick all clusters that consists of the file */

    if (kFreeClusterUntilEnd(stEntry.dwStartClusterIndex) == FALSE)
        goto ERROR;


    /* set the directory entry to free entry */

    kMemSet(&stEntry, 0, sizeof(stEntry));
    if (kSetDirectoryEntryData(iDirectoryEntryOffset, &stEntry) == FALSE)
        goto ERROR;

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return 0;

ERROR:
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return -1;
}


// Open a directory named pcDirectoryName
// params:
//   pcDirectoryName: name of a directory to open
// return:
//   NULL on failure
//   pointer to a directory handle on success
// notes:
//   In MINT64OS, only root directory is supported, so whatever directory name
//   is given, root directory will be opened.
DIR *kOpenDirectory(const char *pcDirectoryName) {
    DIR *pstDirectory;
    DIRECTORYENTRY *pstDirectoryBuffer;

    kLock(&(gs_stFileSystemManager.stMutex));

    // allocate a DIR handle
    pstDirectory = kAllocateFileDirectoryHandle();
    if (pstDirectory == NULL)
        goto UNLOCK;

    // buffer for root directory cluster
    // remember that a cluster is allocated to a directory
    pstDirectoryBuffer = 
        (DIRECTORYENTRY *) kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if (pstDirectoryBuffer == NULL)
        goto FREE_HANDLE;

    // read root directory cluster
    if (kReadCluster(0, (BYTE *) pstDirectoryBuffer) == FALSE)
        goto FREE_BUFFER;


    pstDirectory->bType = FILESYSTEM_TYPE_DIRECTORY;
    pstDirectory->stDirectoryHandle.iCurrentOffset = 0;
    pstDirectory->stDirectoryHandle.pstDirectoryBuffer = pstDirectoryBuffer;

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return pstDirectory;

FREE_BUFFER:
    kFreeMemory(pstDirectoryBuffer);

FREE_HANDLE:
    kFreeFileDirectoryHandle(pstDirectory);

UNLOCK:
    kUnlock(&(gs_stFileSystemManager.stMutex));
    
    return NULL; 
}


// Iterate to the next directory entry in the pstDirectory
// params:
//   pstDirectory: point to a directory data structure to iterate
// return:
//   NULL on failure
//   NULL if there is no more dir entry to iterate
//   dir entry if there is dir entry to iterate
DIRECTORYENTRY *kReadDirectory(DIR *pstDirectory) {
    DIRECTORYHANDLE *pstDirectoryHandle;
    DIRECTORYENTRY *pstEntry;

    if (pstDirectory == NULL)
        return NULL;
    if (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)
        return NULL;

    pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

    if (pstDirectoryHandle->iCurrentOffset < 0)
        return NULL;
    if (pstDirectoryHandle->iCurrentOffset >= FILESYSTEM_MAXDIRECTORYENTRYCOUNT)
        return NULL;

    
    kLock(&(gs_stFileSystemManager.stMutex));


    /* iterate cluster 0 to return the next directory entry */ 
    
    pstEntry = pstDirectoryHandle->pstDirectoryBuffer;
    while (
        pstDirectoryHandle->iCurrentOffset < FILESYSTEM_MAXDIRECTORYENTRYCOUNT
    ) {
        if (
            pstEntry[pstDirectoryHandle->iCurrentOffset].dwStartClusterIndex !=
            FILESYSTEM_FREECLUSTER
        ) {
            kUnlock(&(gs_stFileSystemManager.stMutex));
            return (pstEntry + pstDirectoryHandle->iCurrentOffset++);
        }

        pstDirectoryHandle->iCurrentOffset++;
    }

    // end of directory entry
    kUnlock(&(gs_stFileSystemManager.stMutex));
    return NULL;
}


// Set the current offset of pstDirectory to 0, so it points to the first
// directory entry.
// params:
//   pstDirectory: pointer to a directory data structure to rewind
// notes:
//   if pstDirectory is invalid data structure or NULL, nothing occurs
void kRewindDirectory(DIR *pstDirectory) {
    DIRECTORYHANDLE *pstDirectoryHandle;

    if (pstDirectory == NULL)
        return;
    if (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)
        return;

    pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

    kLock(&(gs_stFileSystemManager.stMutex));

    pstDirectoryHandle->iCurrentOffset = 0;
    kUnlock(&(gs_stFileSystemManager.stMutex));
}


// Close a opened directory data structure
// params:
//   pstDirectory: pointer to a directory data structure to close
// return:
//  0 on success
// -1 on failure
int kCloseDirectory(DIR *pstDirectory) {
    DIRECTORYHANDLE *pstDirectoryHandle;

    if (pstDirectory == NULL)
        return -1;
    if (pstDirectory->bType != FILESYSTEM_TYPE_DIRECTORY)
        return -1;

    pstDirectoryHandle = &(pstDirectory->stDirectoryHandle);

    kLock(&(gs_stFileSystemManager.stMutex));

    // free buffer  for cluster 0 (root directory entries) 
    kFreeMemory(pstDirectoryHandle->pstDirectoryBuffer);

    // free directory handle
    kFreeFileDirectoryHandle(pstDirectory);

    kUnlock(&(gs_stFileSystemManager.stMutex));
    return 0;
}



// Writes 0 as many as dwCount to file
// params:
//   pstFile: file data structure that 0 will be written
//   dwCount: number of 0 to write
// return:
//   TRUE on success
//   FALSE on failure
// note:
//    partial writing also return false
BOOL kWriteZero(FILE *pstFile, DWORD dwCount) {
    BYTE *pbBuffer;
    DWORD dwRemainCount;
    DWORD dwWriteCount;

    if (pstFile == NULL)
        return FALSE;

    pbBuffer = (BYTE *) kAllocateMemory(FILESYSTEM_CLUSTERSIZE);
    if (pbBuffer == NULL)
        return FALSE;
    
    kMemSet(pbBuffer, 0, FILESYSTEM_CLUSTERSIZE);
    dwRemainCount = dwCount;

    while (dwRemainCount != 0) {
        dwWriteCount = MIN(dwRemainCount, FILESYSTEM_CLUSTERSIZE);
        if (kWriteFile(pbBuffer, 1, dwWriteCount, pstFile) != dwWriteCount)
            goto ERROR;
        
        dwRemainCount -= dwWriteCount;
    }

    kFreeMemory(pbBuffer);
    return TRUE;

ERROR:
    kFreeMemory(pbBuffer);
    return FALSE;
}


// Check whether an entry (file) in directory is open or not
// params:
//   pstEntry: entry of a directory to check
// return:
//   TRUE on success
//   FALSE on failure
// note:
//   Opened files are not managed in efficient way, so this function is not
//   efficient.
BOOL kIsFileOpened(const DIRECTORYENTRY *pstEntry) {
    int i;
    FILE *pstFile;

    pstFile = gs_stFileSystemManager.pstHandlePool;
    for (i = 0; i < FILESYSTEM_HANDLE_MAXCOUNT; i++) {
        if (pstFile[i].bType != FILESYSTEM_TYPE_FILE)
            continue;
        if (
            pstFile[i].stFileHandle.dwStartClusterIndex !=
            pstEntry->dwStartClusterIndex
        )
            continue;

        return TRUE;
    }
    return FALSE;
}