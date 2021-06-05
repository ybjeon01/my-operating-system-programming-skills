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
void kExecuteCommand(const char *pcCommandBuffer);


/* parameter related functions which are used inside command */


// function that initializes parameter list
// params:
//   pstList: parameter list
//   pcParmater: string buffer that contains parameters
// info:
//   this function is used inside command function to extract parameters
//   from string buffer
void kInitializeParameter(PARAMETERLIST *pstList, const char *pcParameter);


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
void kHelp(const char* pcParameterBuffer);


// clear screen
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCls does have any parameters
void kCls(const char* pcParameterBuffer);


// show total ram size of the computer
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kCls does have any parameters
void kShowTotalRAMSize(const char *pcParameterBuffer);


// check if given parameter is hex or decimal
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   parameters: strings only with number
void kStringToDecimalHexTest(const char *pcParameterBuffer);


// reboot computer
// params:
//   pcCommandBuffer: parameters passed to command by shell
// info:
//   kShutdown does have any parameters
void kShutdown(const char *pcParamegerBuffer);

#endif /*__CONSOLESHELL_H__*/


