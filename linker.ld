ENTRY(_start)

PHDRS
{
    text PT_LOAD FLAGS(5);      /* Read and execute. */
    requests PT_LOAD FLAGS(6);  /* Limine fills request responses at boot. */
    rodata PT_LOAD FLAGS(4);    /* Read only. */
    data PT_LOAD FLAGS(6);      /* Read and write. */
}

SECTIONS
{
    . = 0xffffffff80000000;

    .text : ALIGN(4K)
    {
        KEEP(*(.text.boot))
        *(.text .text.*)
    } :text

    /* Different permission segments must not share a 4 KiB memory page. */
    .limine_requests_start : ALIGN(4K)
    {
        KEEP(*(.limine_requests_start))
    } :requests

    .limine_requests : ALIGN(8)
    {
        KEEP(*(.limine_requests))
    } :requests

    .limine_requests_end : ALIGN(8)
    {
        KEEP(*(.limine_requests_end))
    } :requests

    .rodata : ALIGN(4K)
    {
        *(.rodata .rodata.*)
    } :rodata

    .data : ALIGN(4K)
    {
        *(.data .data.*)
    } :data

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.note .note.*)
        *(.comment)
    }
}
