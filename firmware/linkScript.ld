OUTPUT_FORMAT("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(startup)
SEARCH_DIR("./stdlib");

MEMORY
{
  ROM  (rx) : ORIGIN = 0x08000000, LENGTH = 64k
  SRAM (rw) : ORIGIN = 0x20000000, LENGTH = 16k
}

SECTIONS
{
  .vectab : ALIGN(4) { support.o(.vectab) }>ROM AT>ROM
  .text : ALIGN(4) { *(.text*) }>ROM AT>ROM
  .rodata : ALIGN(4) { *(.rodata*) }>ROM AT>ROM
  __text_end__ = ALIGN(4);

  .data : ALIGN(4)
  {
  __data_start__ = .;
  *(.data)
  __data_end__ = ALIGN(4);
  }>SRAM AT>ROM
  
  .bss : ALIGN(4)
  {
  *(.bss)
  __bss_end__ = ALIGN(4);
  }>SRAM AT>SRAM
  
  /DISCARD/ : { *(*) }
  
  end = .;
}
