ENTRY(Reset_Handler)

__ram_end = 0x20020000;

MEMORY
{
	flash (rx)	: ORIGIN = 0x08000000, LENGTH = 256K
	ram (xrw)	: ORIGIN = 0x20000000, LENGTH = 128K
	stack (xrw)	: ORIGIN = 0x2001fffc, LENGTH = 0K
	ccmram (rw)	: ORIGIN = 0x10000000, LENGTH = 4K
}

SECTIONS
{
	.vectors :
	{
		. = ALIGN(4);
		KEEP(*(.vectors));
		. = ALIGN(4);
	} >flash

	.text :
	{
		. = ALIGN(4);
		*(.text);          /* .text sections (code) */
		*(.text*);         /* .text* sections (code) */
		*(.glue_7);        /* glue arm to thumb code */
		*(.glue_7t);       /* glue thumb to arm code */
		*(.eh_frame);
		. = ALIGN(4);
		__text_end = .;
	} >flash

	.rodata :
	{
		. = ALIGN(4);
		*(.rodata);        /* .rodata sections (constants, strings, etc.) */
		*(.rodata*);       /* .rodata* sections (constants, strings, etc.) */
		. = ALIGN(4);
	} >flash

	.ARM.extab :
	{
		*(.ARM.extab* .gnu.linkonce.armextab.*);
	} >flash
	.ARM : {
		__exidx_start = .;
		*(.ARM.exidx*);
		__exidx_end = .;
	} >flash

	/* used by the startup to initialize data */
	__data_init_start = LOADADDR(.data);

	/* Initialized data sections goes into RAM, load LMA copy after code */
	.data : 
	{
		. = ALIGN(4);
		__data_start = .;
		*(.data);          /* .data sections */
		*(.data*);         /* .data* sections */
		. = ALIGN(4);
		__data_end = .;
	} >ram AT >flash

	__ccmram_init_start = LOADADDR(.ccmram);

	/* CCM-RAM section 
	 * 
	 * IMPORTANT NOTE! 
	 * If initialized variables will be placed in this section,
	 * the startup code needs to be modified to copy the init-values.  
	 */
	.ccmram :
	{
		. = ALIGN(4);
		__ccmram_start = .;
		*(.ccmram);
		*(.ccmram*);
		. = ALIGN(4);
		__ccmram_end = .;
	} >ccmram AT >flash

	/* Uninitialized data section */
	. = ALIGN(4);
	.bss :
	{
		__bss_start = .;
		*(.bss);
		*(.bss*);
		*(COMMON);
		. = ALIGN(4);
		__bss_end = .;
	} >ram

	/* User_heap_stack section, used to check that there is enough RAM left */
	.heap :
	{
		. = ALIGN(4);
		__heap_end = .;
	} >ram

	.stack :
	{
		. = ALIGN(4);
		__stack_start = .;
		__stack_end = .;
	} >stack

	/* Remove information from the standard libraries */
	/DISCARD/ :
	{
		libc.a (*)
		libm.a (*)
		libgcc.a (*)
	}

	.ARM.attributes 0 :
	{
		*(.ARM.attributes)
	}
}
