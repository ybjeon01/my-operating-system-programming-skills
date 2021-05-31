#include "Page.h"

// initialize paging tree structure for MINT64OS so you can
// use up to 64GB physical memory.
// the data structure lives at 0x100000 (1MB) and
// it consists of 1 PML4 table, 1 page directory pointer table,
// 64 page directories => (4KB + 4KB + 4KB * 64 = 264KB)
void kInitializePageTables(void) {
    PML4ENTRY *pstPML4TEntry;
    PDPTENTRY *pstPDPTEntry;
    PDENTRY *pstPDEntry;
    DWORD dwMappingAddress;
    int i;

    // initialize PML4 table

    // initialize first entry 
    pstPML4TEntry = (PML4ENTRY *) 0x100000;

    // size of PML4 table is 4KB so first PDPTENTRY address
    // is (1MB + 4KB) which is 0x101000 in hex
    kSetPageEntryData(&(pstPML4TEntry[0]), 0x00, 0x101000,
    		PAGE_FLAGS_DEFAULT, 0);

    // Since we support up to 64GB, one PML4T entry is enough
    // set with 0 from second entries
    for (i = 1; i < PAGE_MAXENTRYCOUNT; i++) {
    	kSetPageEntryData(&(pstPML4TEntry[i]), 0, 0, 0, 0);
    }
    // end of creating PML4 table

    // initialize one page directory pointer table

    // one PDP table can map up to 512GB, so only one table is necessary
    // for 64 GB mapping 
    pstPDPTEntry = (PDPTENTRY *) 0x101000;
    for (i = 0; i < 64; i++) {
    	// (1MB + 4KB) + 4KB = 0x102000
        kSetPageEntryData(&(pstPDPTEntry[i]), 0,
        		0x102000 + (i * PAGE_TABLE_SIZE), PAGE_FLAGS_DEFAULT, 0);
    }

    // set 0 from 65th entries
    for (i = 64; i < PAGE_MAXENTRYCOUNT; i++) {
        kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);
    }
    // end of creating PDP table

    // start of creating page directory tables

    // first page directory table address and also
    // first page entry address
    pstPDEntry = (PDENTRY *) 0x102000;
    dwMappingAddress = 0;
    for (i = 0; i < 64 * PAGE_MAXENTRYCOUNT; i++) {
    	// since we cannot represent 64 bit address in IA-32 mode,
    	// "(i * PAGE_DEFAULTSIZE) >> 32" cannot be used for upper
    	// base address. so I first right-shifted 20 bits and multiplies
    	// by i and then right-shifted 12 bits
        // remember that only page entry that has pointer to page frame has
        // PAGE_FLAGS_PS.
        kSetPageEntryData(&(pstPDEntry[i]),
        		(i * (PAGE_DEFAULTSIZE >> 20)) >> 12, dwMappingAddress,
				PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
         dwMappingAddress += PAGE_DEFAULTSIZE;
    }
}

// general function for initializing all page entry
// This function is used in kInitializePageTables function
void kSetPageEntryData(
    PTENTRY *pstEntry,
    DWORD dwUpperBaseAddress,
    DWORD dwLowerBaseAddress,
    DWORD dwLowerFlags,
    DWORD dwUpperFlags) {
    pstEntry->dwAttributeAndLowerBaseAddress =
    		dwLowerBaseAddress | dwLowerFlags;

    // and operator is because only 8 bits of upper part 
    // for address
    pstEntry->dwUpperBaseAddressAndEXB =
        (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}