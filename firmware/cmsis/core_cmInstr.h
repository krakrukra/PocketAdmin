/**************************************************************************//**
 * @file     core_cmInstr.h
 * @brief    CMSIS Cortex-M Core Instruction Access Header File
 * @version  V3.20
 * @date     05. March 2013
 *
 * @note
 *
 ******************************************************************************/
/* Copyright (c) 2009 - 2013 ARM LIMITED

   All rights reserved.
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   - Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
   - Neither the name of ARM nor the names of its contributors may be used
     to endorse or promote products derived from this software without
     specific prior written permission.
   *
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
   ---------------------------------------------------------------------------*/

#ifndef CORE_CMINSTR_H
#define CORE_CMINSTR_H

/* ##########################  Core Instruction Access  ######################### */

/*------------------ GNU Compiler ---------------------*/
/* GNU gcc specific functions */

/** \brief  No Operation
    No Operation does nothing. This instruction can be used for code alignment purposes.
 */
__attribute__( ( always_inline ) ) static inline void __NOP(void)
{
  __asm volatile ("nop");
}


/** \brief  Wait For Interrupt
    Wait For Interrupt is a hint instruction that suspends execution
    until one of a number of events occurs.
 */
__attribute__( ( always_inline ) ) static inline void __WFI(void)
{
  __asm volatile ("wfi");
}


/** \brief  Wait For Event
    Wait For Event is a hint instruction that permits the processor to enter
    a low-power state until one of a number of events occurs.
 */
__attribute__( ( always_inline ) ) static inline void __WFE(void)
{
  __asm volatile ("wfe");
}


/** \brief  Send Event
    Send Event is a hint instruction. It causes an event to be signaled to the CPU.
 */
__attribute__( ( always_inline ) ) static inline void __SEV(void)
{
  __asm volatile ("sev");
}


/** \brief  Instruction Synchronization Barrier
    Instruction Synchronization Barrier flushes the pipeline in the processor,
    so that all instructions following the ISB are fetched from cache or
    memory, after the instruction has been completed.
 */
__attribute__( ( always_inline ) ) static inline void __ISB(void)
{
  __asm volatile ("isb");
}


/** \brief  Data Synchronization Barrier
    This function acts as a special kind of Data Memory Barrier.
    It completes when all explicit memory accesses before this instruction complete.
 */
__attribute__( ( always_inline ) ) static inline void __DSB(void)
{
  __asm volatile ("dsb");
}


/** \brief  Data Memory Barrier
    This function ensures the apparent order of the explicit memory operations before
    and after the instruction, without ensuring their completion.
 */
__attribute__( ( always_inline ) ) static inline void __DMB(void)
{
  __asm volatile ("dmb");
}


/** \brief  Reverse byte order (32 bit)
    This function reverses the byte order in integer value.
    \param [in]    value  Value to reverse
    \return               Reversed value
 */
__attribute__( ( always_inline ) ) static inline uint32_t __REV(uint32_t value)
{
  uint32_t result;

  __asm volatile ("rev %0, %1" : "=r" (result) : "r" (value) );
  return(result);
}


/** \brief  Reverse byte order (16 bit)
    This function reverses the byte order in two unsigned short values.
    \param [in]    value  Value to reverse
    \return               Reversed value
 */
__attribute__( ( always_inline ) ) static inline uint32_t __REV16(uint32_t value)
{
  uint32_t result;

  __asm volatile ("rev16 %0, %1" : "=r" (result) : "r" (value) );
  return(result);
}


/** \brief  Reverse byte order in signed short value
    This function reverses the byte order in a signed short value with sign extension to integer.
    \param [in]    value  Value to reverse
    \return               Reversed value
 */
__attribute__( ( always_inline ) ) static inline int32_t __REVSH(int32_t value)
{
  uint32_t result;

  __asm volatile ("revsh %0, %1" : "=r" (result) : "r" (value) );
  return(result);
}


/** \brief  Rotate Right in unsigned value (32 bit)
    This function Rotate Right (immediate) provides the value of the contents of a register rotated by a variable number of bits.
    \param [in]    value  Value to rotate
    \param [in]    value  Number of Bits to rotate
    \return               Rotated value
 */
__attribute__( ( always_inline ) ) static inline uint32_t __ROR(uint32_t op1, uint32_t op2)
{
  return (op1 >> op2) | (op1 << (32 - op2)); 
}


/** \brief  Breakpoint
    This function causes the processor to enter Debug state.
    Debug tools can use this to investigate system state when the instruction at a particular address is reached.
    \param [in]    value  is ignored by the processor.
    If required, a debugger can use it to store additional information about the breakpoint.
 */
#define __BKPT(value) __asm volatile ("bkpt "#value)

#endif //CORE_CMINSTR_H
