ENTRY(Reset_Handler)

_estack = 0x20040000;    /* end of RAM */

_Min_Heap_Size = 0x200;      /* required amount of heap  */
_Min_Stack_Size = 0x400; /* required amount of stack */

__app_start = ORIGIN(AFLASH);
__app_end = ORIGIN(AFLASH) + LENGTH(AFLASH);

__ldr_start = ORIGIN(LFLASH);
__ldr_end = ORIGIN(LFLASH) + LENGTH(LFLASH);

__cfg_ldr_start = ORIGIN(CLFLASH);
__cfg_ldr_end = ORIGIN(CLFLASH) + LENGTH(CLFLASH);

__cfg_app_start = ORIGIN(CAFLASH);
__cfg_app_end = ORIGIN(CAFLASH) + LENGTH(CAFLASH);

MEMORY
{
RAM (xrw)         : ORIGIN = 0x20000000, LENGTH = 320K
LFLASH (rx)       : ORIGIN = 0x08000000, LENGTH = 64K
CLFLASH (rx)      : ORIGIN = 0x08010000, LENGTH = 32K
CAFLASH (rx)      : ORIGIN = 0x08018000, LENGTH = 32K
AFLASH (rx)       : ORIGIN = 0x08040000, LENGTH = 768K
}

