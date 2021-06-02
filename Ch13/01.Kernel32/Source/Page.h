/*
 * Page.h contains macros and structure definition for page entries.
 * Because entries has a lot of bits in common, I decided to use the same
 * structure for all entries. Just keep in mind that when you initialize a
 * entry that contains page frame address, use macros for the entry. The macros
 * is labeled under "uncommon"
 * 
 * every entry consists of 8 bytes, but 8 bytes cannot be initialized at once.
 * This code expects you to give upper part and lower part of entry separately 
 * 
 * reference a picture, IA-32e paging structure entries, in 9-1.md
 */


#ifndef __PAGE_H__
#define __PAGE_H__

#include "Types.h"


// common in all entries, 32 bits lower parts
#define PAGE_FLAGS_P     0x00000001   // Present
#define PAGE_FLAGS_RW    0x00000002   // Read/Write
#define PAGE_FLAGS_US    0x00000004   // User/Supervisor ; 1=user level
#define PAGE_FLAGS_PWT   0x00000008   // Page Level Write-through
#define PAGE_FLAGS_PCD   0x00000010   // PAGE Level Cache Disable
#define PAGE_FLAGS_A     0x00000020   // Accessed

// 32 bits upper part
#define PAGE_FLAGS_EXB   0x80000000   // Execute Disable bit

// end of common


// start of uncommon
// only for entry that has addr to page frame, 32 bits lower parts
#define PAGE_FLAGS_D     0x00000040   // Dirty
#define PAGE_FLAGS_PS    0x00000080   // Page Size
#define PAGE_FLAGS_G     0x00000100   // Global

// exception: 4KB entry's PAT is  bit 7; MINT64OS uses IA32-e mode 2MB-4 stages
#define PAGE_FLAGS_PAT   0x00001000   // Page Attribute Table Index

// end of uncommon


#define PAGE_FLAGS_DEFAULT (PAGE_FLAGS_P | PAGE_FLAGS_RW)
#define PAGE_TABLE_SIZE  0x1000  // each table size (8 bytes * 512)
#define PAGE_MAXENTRYCOUNT 512 // max number of entries in a table

#define PAGE_DEFAULTSIZE 0x200000 // size of page frame: 2MB

#pragma pack(push, 1)

// base structure for all entries
typedef struct kPageTableEntryStruct {
	DWORD dwAttributeAndLowerBaseAddress;
	DWORD dwUpperBaseAddressAndEXB;
} PML4ENTRY, PDPTENTRY, PDENTRY, PTENTRY;

#pragma pack(pop)

// initialize paging tree structure for MINT64OS so you can
// use up to 64GB physical memory.
// The data structure live at 0x100000 and its size is 264KB
void kInitializePageTables(void);

// general function for initializing all page entry
// This function is used in kInitializePageTables function
void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress,
		DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags);

#endif  /*__PAGE_H__*/