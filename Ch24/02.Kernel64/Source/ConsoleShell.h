// This header file should be included only in ConsoleShell.c and Main.c files.
// This header file contains static function declaration, so if you include
// this file, compiler warns you to define the static functions.
//
// The way that the functions are defined does not look good.
// After reading all chapters, split the commands part from ConsoleShell.c

#ifndef __CONSOLESHELL_H__
#define __CONSOLESHELL_H__

#include "Types.h"


#define CONSOLESHELL_MAXCOMMANDBUFFERCOUNT  300
#define CONSOLESHELL_PROMPTMESSAGE          "MINT64>"


typedef void (*CommandFunction) (const char* pcParameter);

#pragma pack(push, 1)


// structure for each command
// currently, task or process is not implemented, so commands are stored in
// memory
typedef struct kShellCommandEntryStruct {
    char *pcCommand;
    char *pcHelp;
    CommandFunction pfFunction;
} SHELLCOMMANDENTRY;

// structure for each command parameter list
typedef struct kParameterListStruct {
    const char *pcBuffer;
    int iLength;
    int iCurrentPosition;
} PARAMETERLIST;

#pragma pack(pop)


/* shell main function */

// main loop of shell
void kStartConsoleShell(void);


// function that search command and execute
// params:
//   pcCommandBuffer: buffer that contains command string 
static void kExecuteCommand(const char *pcCommandBuffer);


/* parameter related functions which are used inside command */


// function that initializes parameter list
// params:
//   pstList: parameter list
//   pcParmater: string buffer that contains parameters
// info:
//   this function is used inside command function to extract parameters
//   from string buffer
static void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter);


// function that gets next parameter from parameter list
// params:
//   pstList: parameter list
//   pcParmater: string buffer that will hold next parameter string
// return:
//   length of next parameter string
//   if no more paramters, zero is return
// info:
//   this function is used inside command function to extract parameters
//   from string buffer
int kGetNextParameter(PARAMETERLIST *pstList, char *pcParameter);


/* shell commands */

// shell command that prints available commands
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kHelp does have any parameters
static void kHelp(const char* pcParameterBuffer);


// clear screen
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCls does have any parameters
static void kCls(const char* pcParameterBuffer);


// show total ram size of the computer
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCls does have any parameters
static void kShowTotalRAMSize(const char *pcParameterBuffer);


// check if given parameter is hex or decimal
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   parameters: strings only with number
static void kStringToDecimalHexTest(const char *pcParameterBuffer);


// reboot computer
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShutdown does have any parameters
static void kShutdown(const char *pcParamegerBuffer);


// set timer using PIT counter0 register
// params:
//   pcCommandBuffer: parameters passed to command by shell
//     time: counter reset value in ms unit
//     periodic: boolean value to decide repeat
//       1: true
//       0: false
static void kSetTimer(const char *pcParameterBuffer);


// busy-wait for a period by using PIT controller
// params:
//   pcCommandBuffer: parameters passed to command by shell
//     time: counter reset value in ms unit
//     periodic: boolean value to decide repeat
//       1: true
//       0: false
static void kWaitUsingPIT(const char *pcParameterBuffer);


// read time stamp counter
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
static void kReadTimeStampCounter(const char *pcParameterBuffer);


// measure processor maximum clock for 10 seconds
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
static void kMeasureProcessorSpeed(const char *pcParameterBuffer);


// print date and time stored in RTC controller
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
static void kShowDateAndTime(const char *pcParameterBuffer);


// create a simple task and switch to the task
// params:
//   pcCommandBuffer: parameters passed to command by shell
//     type: type of task
//       1 (first type task) 
//       2 (second type task)
//     count: number of tasks to create
static void kCreateTestTask(const char *pcParameterBuffer);


// change priority of a task
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     taskID: id of a task to change priority
//     priority: priority that task will have
//       range: 0 ~ 4
static void kChangeTaskPriority(const char *pcParameterBuffer);


// show information about currently created tasks
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShowTaskList does have any parameters
static void kShowTaskList(const char *pcParameterBuffer);



// kill a task by ID
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     id: task id
//         if id is 0xFFFFFFFF, tasks whose id above 0x200000001 are killed
static void kKillTask(const char *pcParameterBuffer);


// show processor usage in percentage
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCPULoad does have any parameters
static void kCPULoad(const char *pcParameterBuffer);


// creates two tasks that adds 1 to the same variable. and check if
// mutex synchronization technique works
// params:
//   kTestMutex: parameters passed to command by shell
// info:
//   kTestMutex does have any parameters
static void kTestMutex(const char *pcCommandBuffer);


// creates a process task that makes three threads
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kTestThread does have any parameters
static void kTestThread(const char *pcParameterBuffer);


// prints strings like matrix movie
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShowMatrix does have any parameters
static void kShowMatrix(const char *pcParameterBuffer);


// creates kernel threads that tests whether FPU is working and 
// whether FPU context switching is working
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kTestPIE does have any parameters
static void kTestPIE(const char *pcParameterBuffer);


// show information about dynamic memory module 
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShowDynamicMemoryInforation does have any parameters
static void kShowDynamicMemoryInformation(const char *pcParameterBuffer);


// test dynamic memory allocation by allocating memory
// sequentially, writing to the memory and deallocating the memory
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kTestSequentialAllocation does have any parameters
static void kTestSequentialAllocation(const char *pcParameterBuffer);


// test if  memory allocation of random size works well
// by create a lot of thread tasks that asks memory of random size
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kTestRandomAllocation does have any parameters
static void kTestRandomAllocation(const char *pcParameterBuffer);


// a thread that asks memory of random size, writes to the memory, and
// dellocate the memory
static void kRandomAllocationTask(void);


// print hdd primary-master hdd info to screen
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShowHDDInformation does have any parameters
static void kShowHDDInformation(const char *pcParameterBuffer);


// read sectors from hdd
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     address: address of data to read in decimal format
//     sectorCount: number of sectors to read
static void kReadSector(const char *pcParameterBuffer);

// write sectors to hdd
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     address: address to write data in decimal format
//     sectorCount: number of sectors to write
static void kWriteSector(const char *pcParameterBuffer);


/* custom shell commands */

// check if memory at specific address is readable and writable
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   parmater: address in hex (unit: byte) or address in decimal (unit: MB)
//   this function actually tries to write data at address given by paramter
//   If the area is hardware reserved, reboot might happen
// example:
//   access 0x12345
//   access 1024
static void access(const char *pcParameterBuffer);


// print computer memory map that shows hardware reserved area and
// free area
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   printMemoryMap does have any parameters
static void printMemoryMap(const char *pcParameterBuffer);


// print MINT64OS banner
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   banner does have any parameters
static void banner(const char *pcParameterBuffer);


// show information about scheduler lists
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   params:
//     queueName: name of queue to print
//       values: [waiting, ready]
//     priority: priority of ready queue (valid only if queueName is ready)
//       range: [0, TASK_MAXREADYLISTCOUNT)
static void kShowSchedulerList(const char *pcParameterBuffer);


static void kReadHDDRegisters(const char *pcParameterBuffer);

static void kWriteToHDDReg(const char *pcParameterBuffer);

#endif /*__CONSOLESHELL_H__*/


