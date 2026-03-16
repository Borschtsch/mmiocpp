ENTRY(Reset_Handler)

MEMORY
{
  RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 32M
}

PHDRS
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .text :
  {
    *(.text*)
    *(.rodata*)
    . = ALIGN(4);
    _etext = .;
  } > RAM :text

  .data : ALIGN(4)
  {
    _sdata = .;
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > RAM :data

  _sidata = LOADADDR(.data);

  .bss (NOLOAD) : ALIGN(4)
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM :data
}
