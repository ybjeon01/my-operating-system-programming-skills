#include "FileSystem.h"
#include "HardDisk.h"
#include "DynamicMemory.h"
#include "Utility.h"

// MINT64OS filesystem manager 
static FILESYSTEMMANAGER gs_stFileSystemManager;

// file system temporary buffer
// can have up to 512 clusters
static BYTE gs_vbTemBuffer[FILESYSTEM_SECTORSPERCLUSTER * 512];


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

    return TRUE;
}


/* Low Level Functions */

// Check if HDD has MINT filesystem by reading MBR in HDD.
// If it is a MINT file system, this function reads various information
// related to the file system and inserts it into file system data structure.
// return:
//   TRUE on success and FALSE on failure
BOOL kMount(void) {
    MBR *pstMBR;

    kLock(&(gs_stFileSystemManager.stMutex));


    /* read MBR in primary master HDD */

    if (gs_fpReadHDDSector(TRUE, FALSE, 0, 1, gs_vbTemBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    pstMBR = (MBR *) gs_vbTemBuffer;
    

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

    pstHDD = (HDDINFORMATION *) gs_vbTemBuffer;

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
    if (gs_fpReadHDDSector(TRUE, FALSE, 0, 1, gs_vbTemBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }

    pstMBR = (MBR *) gs_vbTemBuffer;

    // be careful, this will fail booting on acer laptop **
    kMemSet(pstMBR->vstPartition, 0, sizeof(pstMBR->vstPartition));
    pstMBR->dwSignature = FILESYSTEM_SIGNATURE;
    pstMBR->dwReservedSectorCount = 0;
    pstMBR->dwClusterLinkSectorCount = dwClusterLinkSectorCount;
    pstMBR->dwTotalClusterCount = dwClusterCount;

    // write the modified MBR to HDD
    if (gs_pfWriteHDDSector(TRUE, FALSE, 0, 1, gs_vbTemBuffer) == FALSE) {
        kUnlock(&(gs_stFileSystemManager.stMutex));
        return FALSE;
    }


    /* initialize link area and first cluster which is reserved for root
        directory
    */

    kMemSet(gs_vbTemBuffer, 0, 512);

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
            ((DWORD *) gs_vbTemBuffer)[0] = FILESYSTEM_LASTCLUSTER;
        }
        else {
            ((DWORD *) gs_vbTemBuffer)[0] = FILESYSTEM_FREECLUSTER;
        }

        if (gs_pfWriteHDDSector(TRUE, FALSE, i+1, 1, gs_vbTemBuffer) == FALSE) {
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
BOOL kReadClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer) {
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
BOOL kWriteClusterLinkTable(DWORD dwOffset, BYTE *pbBuffer) {
    return gs_pfWriteHDDSector(
        TRUE,
        FALSE,
        dwOffset + gs_stFileSystemManager.dwClusterLinkAreaStartAddress,
        1,
        pbBuffer
    );
}


// Read one cluster to the offset of the data area
// params:
//   dwOffset: Offset to read from data area
//   pbBuffer: A pointer to a variable that will hold the data
// return:
//   TRUE on success, and FALSE on failure
BOOL kReadCluster(DWORD dwOffset, BYTE *pbBuffer) {
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
BOOL kWriteCluster(DWORD dwOffset, BYTE *pbBuffer) {
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
DWORD kFindFreeCluster(void) {
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
                gs_vbTemBuffer
            ) == FALSE
        ) {
            return FILESYSTEM_LASTCLUSTER;
        }


        /* search for empty cluster in the current sector */

        for (j = 0; j < dwLinkCountInSector; j++) {
            if (((DWORD *)gs_vbTemBuffer)[j] == FILESYSTEM_FREECLUSTER) {
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
BOOL kSetClusterLinkData(DWORD dwClusterIndex, DWORD dwData) {
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

    if (kReadClusterLinkTable(dwSectorOffset, gs_vbTemBuffer) == FALSE) {
        return FALSE;
    }

    /* set data */

    ((DWORD *) gs_vbTemBuffer)[dwClusterIndex % 128] = dwData;

    if (kWriteClusterLinkTable(dwSectorOffset, gs_vbTemBuffer) == FALSE) {
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
BOOL kGetClusterLinkData(DWORD dwClusterIndex, DWORD *pdwData) {
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

    if (kReadClusterLinkTable(dwSectorOffset, gs_vbTemBuffer) == FALSE) {
        return FALSE;
    }

    *pdwData = ((DWORD *) gs_vbTemBuffer)[dwClusterIndex % 128];
    return TRUE;
}


// get idex to empty entry in root directory
// return:
//   index to free entry in root directory
//   -1 on failure
int kFindFreeDirectoryEntry(void) {
    DIRECTORYENTRY *pstEntry;
    int i;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return -1;
    }

    // read cluster containing root directory
    if (kReadCluster(0, gs_vbTemBuffer) == FALSE) {
        return -1;
    }

    pstEntry = (DIRECTORYENTRY *) gs_vbTemBuffer;


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
BOOL kSetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
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

    if (kReadCluster(0, gs_vbTemBuffer) == FALSE) {
        return FALSE;
    }


    /* set entry in  directory */

    pstRootEntry = (DIRECTORYENTRY *) gs_vbTemBuffer;

    kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));

    /* write to stroage */

    if (kWriteCluster(0, gs_vbTemBuffer) == FALSE) {
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
BOOL kGetDirectoryEntryData(int iIndex, DIRECTORYENTRY *pstEntry) {
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

    if (kReadCluster(0, gs_vbTemBuffer) == FALSE) {
        return FALSE;
    }

    pstRootEntry = (DIRECTORYENTRY *) gs_vbTemBuffer;
    kMemCpy(pstRootEntry + iIndex, pstEntry, sizeof(DIRECTORYENTRY));
    return TRUE;
}



// find an entry in the root directory with a matching file name
// params:
//   pcFileName: filename to search
//   pstEntry: pointer to a variable that will contain the found entry
// return:
//   index of found entry in root directory
//   or -1 on failure 
int kFindDirectoryEntry(const char *pcFileName, DIRECTORYENTRY *pstEntry) {
    DIRECTORYENTRY *pstRootEntry;
    int i;
    int iLength;

    // check if filesystem is mounted;
    if (gs_stFileSystemManager.bMounted == FALSE) {
        return FALSE;
    }

    /* read cluster containing directory */

    if (kReadCluster(0, gs_vbTemBuffer) == FALSE) {
        return FALSE;
    }

    iLength = kStrLen(pcFileName);
    pstRootEntry = (DIRECTORYENTRY *) gs_vbTemBuffer;


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
