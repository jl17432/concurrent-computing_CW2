/* Copyright (C) 2017 Daniel Page <csdsp@bristol.ac.uk>
 *
 * Use of this source code is restricted per the CC BY-NC-ND license, a copy of 
 * which can be found via http://creativecommons.org (and should be included as 
 * LICENSE.txt within the associated archive or repository).
 */

#include "hilevel.h"

/* We assume there will be two user processes, stemming from execution of the 
 * two user programs P1 and P2, and can therefore
 * 
 * - allocate a fixed-size process table (of PCBs), and then maintain an index 
 *   into it to keep track of the currently executing process, and
 * - employ a fixed-case of round-robin scheduling: no more processes can be 
 *   created, and neither can be terminated, so assume both are always ready
 *   to execute.
 */

pcb_t procTab[ MAX_PROCS ]; pcb_t* executing = NULL;

void dispatch( ctx_t* ctx, pcb_t* prev, pcb_t* next ) {
  char prev_pid = '?', next_pid = '?';

  if( NULL != prev ) {
    memcpy( &prev->ctx, ctx, sizeof( ctx_t ) ); // preserve execution context of P_{prev}
    prev_pid = '0' + prev->pid;
  }
  if( NULL != next ) {
    memcpy( ctx, &next->ctx, sizeof( ctx_t ) ); // restore  execution context of P_{next}
    next_pid = '0' + next->pid;
  }

    PL011_putc( UART0, '[',      true );
    PL011_putc( UART0, prev_pid, true );
    PL011_putc( UART0, '-',      true );
    PL011_putc( UART0, '>',      true );
    PL011_putc( UART0, next_pid, true );
    PL011_putc( UART0, ']',      true );

    executing = next;                           // update   executing process to P_{next}

  return;
}

void schedule( ctx_t* ctx ) {
  if     ( executing->pid == procTab[ 0 ].pid ) {
    dispatch( ctx, &procTab[ 0 ], &procTab[ 1 ] );  // context switch P_1 -> P_2

    procTab[ 0 ].status = STATUS_READY;             // update   execution status  of P_1 
    procTab[ 1 ].status = STATUS_EXECUTING;         // update   execution status  of P_2
  }
  else if( executing->pid == procTab[ 1 ].pid ) {
    dispatch( ctx, &procTab[ 1 ], &procTab[ 0 ] );  // context switch P_2 -> P_1

    procTab[ 1 ].status = STATUS_READY;             // update   execution status  of P_2
    procTab[ 0 ].status = STATUS_EXECUTING;         // update   execution status  of P_1
  }

  return;
}

extern void     main_P1(); 
extern uint32_t tos_P1;
extern void     main_P2(); 
extern uint32_t tos_P2;

void hilevel_handler_rst( ctx_t* ctx              ) { 
  /* Invalidate all entries in the process table, so it's clear they are not
   * representing valid (i.e., active) processes.
   */

  for( int i = 0; i < MAX_PROCS; i++ ) {
    procTab[ i ].status = STATUS_INVALID;
  }

  /* Automatically execute the user programs P1 and P2 by setting the fields
   * in two associated PCBs.  Note in each case that
   *    
   * - the CPSR value of 0x50 means the processor is switched into USR mode, 
   *   with IRQ interrupts enabled, and
   * - the PC and SP values match the entry point and top of stack. 
   */

  memset( &procTab[ 0 ], 0, sizeof( pcb_t ) ); // initialise 0-th PCB = P_1
  procTab[ 0 ].pid      = 1;
  procTab[ 0 ].status   = STATUS_READY;
  procTab[ 0 ].tos      = ( uint32_t )( &tos_P1  );
  procTab[ 0 ].ctx.cpsr = 0x50;
  procTab[ 0 ].ctx.pc   = ( uint32_t )( &main_P1 );
  procTab[ 0 ].ctx.sp   = procTab[ 0 ].tos;

  memset( &procTab[ 1 ], 0, sizeof( pcb_t ) ); // initialise 1-st PCB = P_2
  procTab[ 1 ].pid      = 2;
  procTab[ 1 ].status   = STATUS_READY;
  procTab[ 1 ].tos      = ( uint32_t )( &tos_P2  );
  procTab[ 1 ].ctx.cpsr = 0x50;
  procTab[ 1 ].ctx.pc   = ( uint32_t )( &main_P2 );
  procTab[ 1 ].ctx.sp   = procTab[ 1 ].tos;

  /* Once the PCBs are initialised, we arbitrarily select the 0-th PCB to be 
   * executed: there is no need to preserve the execution context, since it 
   * is invalid on reset (i.e., no process was previously executing).
   */

  dispatch( ctx, NULL, &procTab[ 0 ] );

  return;
}

void hilevel_handler_svc( ctx_t* ctx, uint32_t id ) { 
  /* Based on the identifier (i.e., the immediate operand) extracted from the
   * svc instruction, 
   *
   * - read  the arguments from preserved usr mode registers,
   * - perform whatever is appropriate for this system call, then
   * - write any return value back to preserved usr mode registers.
   */

  switch( id ) {
    case 0x00 : { // 0x00 => yield()
      schedule( ctx );

      break;
    }

    case 0x01 : { // 0x01 => write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );  
      char*  x = ( char* )( ctx->gpr[ 1 ] );  
      int    n = ( int   )( ctx->gpr[ 2 ] ); 

      for( int i = 0; i < n; i++ ) {
        PL011_putc( UART0, *x++, true );
      }
      
      ctx->gpr[ 0 ] = n;

      break;
    }

    default   : { // 0x?? => unknown/unsupported
      break;
    }
  }

  return;
}
