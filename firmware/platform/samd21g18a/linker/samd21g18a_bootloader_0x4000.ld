OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00004000, LENGTH = 0x0003C000
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00008000
}

ENTRY(Reset_Handler)

STACK_SIZE = DEFINED(STACK_SIZE) ? STACK_SIZE : 0x1000;

_estack = ORIGIN(RAM) + LENGTH(RAM);
_sstack = _estack - STACK_SIZE;

SECTIONS
{
  .text :
  {
    . = ALIGN(4);
    _sfixed = .;

    KEEP(*(.vectors))
    KEEP(*(.isr_vector))

    *(.text.Reset_Handler)
    *(.text*)
    *(.rodata*)

    . = ALIGN(4);
    _efixed = .;
  } > FLASH

  . = ALIGN(4);
  _etext = .;

  .relocate : AT (_etext)
  {
    . = ALIGN(4);
    _srelocate = .;

    *(.ramfunc*)
    *(.data*)

    . = ALIGN(4);
    _erelocate = .;
  } > RAM

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _szero = .;

    *(.bss*)
    *(COMMON)

    . = ALIGN(4);
    _ezero = .;
  } > RAM

  . = ALIGN(8);
  _end = .;
  PROVIDE(end = _end);

  ASSERT(_end <= _sstack, "RAM overflow: data/bss collide with stack")
  ASSERT(_sstack >= ORIGIN(RAM), "Stack does not fit in RAM")

  /DISCARD/ :
  {
    *(.note*)
    *(.comment*)
  }
}
