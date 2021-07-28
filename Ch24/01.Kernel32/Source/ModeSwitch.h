#ifndef __MODESWITCH_H__
#define __MODESWITCH_H__

#include "Types.h"

// execute CPUID instruction
// params:
//   dwEAX: information to look up
//     possible values: [0x00000000(basic info), 0x80000001(extension info)]
//   pdwEAX: address where EAX returned by CPUID instruction will resides
//   pdwEBX: address where EBX returned by CPUID instruction will resides
//   pdwECX: address where ECX returned by CPUID instruction will resides
//   pdwEDX: address where EDX returned by CPUID instruction will resides 
void kReadCPUID(
    DWORD dwEAX,
	DWORD *pdwEAX,
    DWORD *pdwEBX,
    DWORD *pdwECX,
    DWORD *pdwEDX
);

// switch protected mode to long mode
// this function modifies CR0, CR3, CR4, and IA32_EFER so cpu switches to IA-32e
// with cache and paging enabled. The Cache is write-through cache mode, and
// other paging-specific features are disabled.
// Before calling this function, it is required to initialize IA-32e paging
// tree structure at 0x100000(1MB)
void kSwitchAndExecute64bitKernel(void);

#endif /* 01_KERNEL32_SOURCE_MODESWITCH_H_ */