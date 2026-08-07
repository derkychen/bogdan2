/*
 * Linker script for the IND.I/O.
 *
 * The first 16 kilobytes of flash are reserved for the bootloader, so
 * application flash starts at 0x00004000.
 */
OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/* Memory regions available to the application. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00004000, LENGTH = 0x0003C000
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00008000
}

/* Program entry point. */
ENTRY(Reset_Handler)

/* Reserve the top 4 kilobytes of RAM for the descending stack. */
STACK_SIZE = DEFINED(STACK_SIZE) ? STACK_SIZE : 0x1000;

_estack = ORIGIN(RAM) + LENGTH(RAM);
_sstack = _estack - STACK_SIZE;

SECTIONS
{
    /* Put executable and read-only content in flash. */
    .text :
    {
        . = ALIGN(4);
        _sfixed = .;

        /* Keep the vector table even when --gc-sections is enabled. */
        KEEP(*(.vectors))
        KEEP(*(.isr_vector))

        *(.text.Reset_Handler)
        *(.text*)
        *(.rodata*)

        *(.glue_7t)
        *(.glue_7)
        *(.vfp11_veneer)
        *(.v4_bx)

        . = ALIGN(4);
        _efixed = .;
    } > FLASH

    /* ARM exception unwinding tables. */
    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
        __exidx_end = .;
    } > FLASH

    /*
     * Initialized RAM content is stored in flash and copied into RAM at
     * startup.
     */
    .relocate :
    {
        . = ALIGN(4);
        _srelocate = .;

        *(.ramfunc*)
        *(.data*)

        . = ALIGN(4);
        _erelocate = .;
    } > RAM AT > FLASH

    _etext = LOADADDR(.relocate);

    /* Zero-initialized global and static storage cleared at startup. */
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

    /* Ensure statically allocated RAM does not overlap with reserved stack. */
    ASSERT(_end <= _sstack, "RAM overflow: data/bss collide with stack")
    ASSERT(_sstack >= ORIGIN(RAM), "Stack does not fit in RAM")

    /*
     * Dynamic-linking sections should remain empty in this statically linked
     * firmware.
     */
    .iplt 0 :
    {
        *(.iplt)
    }

    .rel.iplt 0 :
    {
        *(.rel.iplt)
    }

    .igot.plt 0 :
    {
        *(.igot.plt)
    }

    ASSERT(SIZEOF(.iplt) == 0, "Unexpected non-empty .iplt section")
    ASSERT(SIZEOF(.rel.iplt) == 0, "Unexpected non-empty .rel.iplt section")
    ASSERT(SIZEOF(.igot.plt) == 0, "Unexpected non-empty .igot.plt section")

    /*
     * Debug sections do not occupy target memory. Their addresses are set to
     * zero, so they have a location in the ELF file without allocating target
     * memory. This matches the GNU linker default script.
     */
    .debug          0 : { *(.debug) }
    .line           0 : { *(.line) }
    .debug_srcinfo  0 : { *(.debug_srcinfo) }
    .debug_sfnames  0 : { *(.debug_sfnames) }
    .debug_aranges  0 : { *(.debug_aranges) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line .debug_line.* .debug_line_end) }
    .debug_frame    0 : { *(.debug_frame) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_macinfo  0 : { *(.debug_macinfo) }
    .debug_weaknames 0 : { *(.debug_weaknames) }
    .debug_funcnames 0 : { *(.debug_funcnames) }
    .debug_typenames 0 : { *(.debug_typenames) }
    .debug_varnames  0 : { *(.debug_varnames) }
    .debug_pubtypes 0 : { *(.debug_pubtypes) }
    .debug_ranges   0 : { *(.debug_ranges) }
    .debug_addr     0 : { *(.debug_addr) }
    .debug_line_str 0 : { *(.debug_line_str) }
    .debug_loclists 0 : { *(.debug_loclists) }
    .debug_macro    0 : { *(.debug_macro) }
    .debug_names    0 : { *(.debug_names) }
    .debug_rnglists 0 : { *(.debug_rnglists) }
    .debug_str_offsets 0 : { *(.debug_str_offsets) }
    .debug_sup      0 : { *(.debug_sup) }

    .ARM.attributes 0 :
    {
        KEEP(*(.ARM.attributes))
        KEEP(*(.gnu.attributes))
    }

    .note.gnu.arm.ident 0 :
    {
        KEEP(*(.note.gnu.arm.ident))
    }

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.comment*)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
        *(.gnu_object_only)
    }
}
