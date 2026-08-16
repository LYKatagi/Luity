
; Luity Kernel - Boot Entry

bits 64

section .text
global _start
extern kmain

_start:
    cli

    ; Set up the kernel stack.
    mov rsp, stack_top

    ; Align stack to 16 bytes as required by the x86-64 ABI.
    and rsp, -16

    ; Enter the kernel.
    call kmain

.hang:
    cli
    hlt
    jmp .hang