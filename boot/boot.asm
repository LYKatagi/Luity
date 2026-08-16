
bits 64

section .text

global _start
extern kmain

_start:
    cli

    ; stack_top = 0xffffffff80005000
    mov rsp, 0xffffffff80005000

    ; 16-byte alignment
    and rsp, -16

    call kmain

.hang:
    cli
    hlt
    jmp .hang