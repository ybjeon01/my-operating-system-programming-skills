#ifndef __ISR_H__
#define __ISR_H__

/* 
 * Interrupt handling functions 
 *
 * This functions are referenced by IDT gate descriptors. Their role is
 * to save context such as registers to a stack referenced by IDT and to restore
 * the context after calling C function
 * You can think these functions as intermediate functions
 * 
 * Every ISR passes a exception/interrupt number parameter to C code and
 * some ISRs pass error number too
 * 
 */


/* 0 to 19 reserved exception/interrupt handling functions */

void kISRDivideError( void );
void kISRDebug( void );
void kISRNMI( void );
void kISRBreakPoint( void );
void kISROverflow( void );
void kISRBoundRangeExceeded( void );
void kISRInvalidOpcode();
void kISRDeviceNotAvailable( void );
void kISRDoubleFault( void );
void kISRCoprocessorSegmentOverrun( void );
void kISRInvalidTSS( void );
void kISRSegmentNotPresent( void );
void kISRStackSegmentFault( void );
void kISRGeneralProtection( void );
void kISRPageFault( void );
void kISR15( void );
void kISRFPUError( void );
void kISRAlignmentCheck( void );
void kISRMachineCheck( void );
void kISRSIMDError( void );


/* 
 * dummy handler 
 * handles rest of reserved exceptions from 20 to 31
 */

void kISRETCException( void );


/*
 * PIC interrupt handling functions
 * In MINT64OS, there are referenced from 32 to 47 gate descriptors
 */

void kISRTimer( void );
void kISRKeyboard( void );
void kISRSlavePIC( void );
void kISRSerial2( void );
void kISRSerial1( void );
void kISRParallel2( void );
void kISRFloppy( void );
void kISRParallel1( void );
void kISRRTC( void );
void kISRReserved( void );
void kISRNotUsed1( void );
void kISRNotUsed2( void );
void kISRMouse( void );
void kISRCoprocessor( void );
void kISRHDD1( void );
void kISRHDD2( void );


/* 
 * dummy handler 
 * handles rest gate descriptors from 48
 */

void kISRETCInterrupt( void );

#endif /* __ISR_H__ */