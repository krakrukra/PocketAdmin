/*-----------------------------------------------------------------------------/
/ Copyright (C) 2018, krakrukra, all rights reserved.
/
/ This is an open source software. Redistribution and use of it in
/ source and binary forms, with or without modification, are permitted
/ provided that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright holder or contributors SHALL NOT BE LIABLE for any damages
/ caused by use of this software.
/-----------------------------------------------------------------------------*/

#include "../cmsis/stm32f0xx.h"

extern int main();
extern void usb_handler() __attribute__((interrupt));

static void startup();
static inline void initialize_data(unsigned int* from, unsigned int* data_start, unsigned int* data_end) __attribute__((always_inline));
static inline void initialize_bss(unsigned int* bss_start, unsigned int* bss_end) __attribute__((always_inline));

static void default_handler() __attribute__((interrupt));

//these symbols are declared in the linker script
extern unsigned int __data_start__;//start of .data section in RAM
extern unsigned int __data_end__;//end of .data section in RAM
extern unsigned int __bss_end__;//end of .bss section in RAM
extern unsigned int __text_end__;//end of .text section in ROM

void* vectorTable[48] __attribute__(( section(".vectab,\"a\",%progbits@") )) =
  {
    (void*)0x20003FFF,//initial main SP value (16kB RAM size)
    (void*)&startup,//reset vector
    (void*)&default_handler,//NMI
    (void*)&default_handler,//HardFault
    (void*)0x00000000,//reserved
    (void*)0x00000000,//reserved
    (void*)0x00000000,//reserved
    (void*)0x00000000,//reserved
    (void*)0x00000000,//reserved
    (void*)0x00000000,//reserved
    (void*)0x00000000,//reserved
    (void*)&default_handler,//SVCall
    (void*)0x00000000,//reserved
    (void*)0x00000000,//reserved
    (void*)&default_handler,//PendSV
    (void*)&default_handler,//SysTick
    (void*)0x00000000,//IRQ0
    (void*)0x00000000,//IRQ1
    (void*)0x00000000,//IRQ2
    (void*)0x00000000,//IRQ3
    (void*)0x00000000,//IRQ4
    (void*)0x00000000,//IRQ5
    (void*)0x00000000,//IRQ6
    (void*)0x00000000,//IRQ7
    (void*)0x00000000,//IRQ8
    (void*)0x00000000,//IRQ9
    (void*)0x00000000,//IRQ10
    (void*)0x00000000,//IRQ11
    (void*)0x00000000,//IRQ12
    (void*)0x00000000,//IRQ13
    (void*)0x00000000,//IRQ14
    (void*)0x00000000,//IRQ15
    (void*)0x00000000,//IRQ16
    (void*)0x00000000,//IRQ17
    (void*)0x00000000,//IRQ18
    (void*)0x00000000,//IRQ19
    (void*)0x00000000,//IRQ20
    (void*)0x00000000,//IRQ21
    (void*)0x00000000,//IRQ22
    (void*)0x00000000,//IRQ23
    (void*)0x00000000,//IRQ24
    (void*)0x00000000,//IRQ25
    (void*)0x00000000,//IRQ26
    (void*)0x00000000,//IRQ27
    (void*)0x00000000,//IRQ28
    (void*)0x00000000,//IRQ29
    (void*)0x00000000,//IRQ30
    (void*)&usb_handler//IRQ31
  };

//copies initialized static variable values from ROM to RAM
static inline void initialize_data(unsigned int* from, unsigned int* data_start, unsigned int* data_end)
{
  while(data_start < data_end)
    {
      *data_start = *from;
      data_start++;
      from++;
    }
}

//writes to zero uninitialized static variable values in RAM
static inline void initialize_bss(unsigned int* bss_start, unsigned int* bss_end)
{
  while(bss_start < bss_end)
    {
      *bss_start = 0x00000000U;
      bss_start++;
    }
}

//the very first function that the CPU will run
static void startup()
{
  RCC->AHBENR |= (1<<17);//enable GPIOA clock
  RCC->APB1ENR |= (1<<0);//enable TIM2 clock
  RCC->APB2ENR |= (1<<12);//enable SPI1 clock
  
  GPIOA->MODER |= (1<<30)|(1<<15)|(1<<13)|(1<<11);//PA15 is output; PA7, PA6, PA5 are in alternate function mode (SPI1)
  GPIOA->BSRR = (1<<15);//pull PA15 high (SPI1 CS output)
  GPIOA->PUPDR |= (1<<4);//enable pullup at PA2
  
  FLASH->ACR = (1<<0);//insert 1 wait state for flash read access (needed because SYSCLK will be 48MHz)
  
  RCC->CR |= (1<<19)|(1<<16);//enable HSE clock, clock security system
  while( !(RCC->CR & (1<<17)) );//wait until HSE is ready
  RCC->CFGR |= (1<<20)|(1<<16);//derive PLL clock from HSE, multiply HSE by 6 (8MHz * 6 = 48MHz)
  RCC->CR |= (1<<24);//enable PLL

  initialize_data(&__text_end__, &__data_start__, &__data_end__);
  initialize_bss(&__data_end__, &__bss_end__);
  
  while( !(RCC->CR & (1<<25)) );//wait until PLL is ready
  RCC->CFGR |= (1<<1);//set PLL as system clock  
  while( !((RCC->CFGR & 0x0F) == 0b1010) );//wait until PLL is used as system clock
  RCC->CR &= ~(1<<0);//disable HSI clock
  
  main();

  NVIC_SystemReset();//if main() ever returns reset MCU
  return;
}

//default IRQ handler initiates system reset
static void default_handler()
{
  NVIC_SystemReset();
  return;
}
