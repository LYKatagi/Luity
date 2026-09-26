
bits 64

section .text

global _start
extern kmain

_start:
    cli

    ; Keep Limine's valid boot stack and align it for the SysV ABI.
    and rsp, -16

    call kmain

.hang:
    cli
    hlt
    jmp .hang
