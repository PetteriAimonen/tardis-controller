MEMORY
{
  ram (rwx) : ORIGIN = 0x20000000, LENGTH = 4K
  rom (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
}
SECTIONS
{
    .  = 0x08000000;         /* From 0x08000000 */
    .text : 
    {
        *(vectors)    /* Vector table */
        *(.text)      /* Program code */
        *(.text*)
        *(.rodata)    /* Read only data */
        *(.rodata*)
        *sound.o(.data)
        *exterminate.o(.data)
        _sidata = .;
    } >rom

    .  = 0x20000000;  /* From 0x20000000 */      
    .data : 
    {
        _sdata = .;
        *(.data)      /* Data memory */
        _edata = .;
    } >ram AT > rom

    .bss :
    {
        _sbss = .;
        *(.bss)       /* Zero-filled run time allocate data memory */
        _ebss = .;
    } >ram AT > rom
    
    _estack = 0x20000000 + 4096;
} 
