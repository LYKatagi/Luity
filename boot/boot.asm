; Luity Kernel
; boot/boot.asm

bits 64

section .text
global _start
extern kmain

_start:
    cli

    ; Limine já colocou a CPU em long mode.
    ; Usamos uma stack própria antes de chamar o kernel.
    lea rsp, stack_top

    call kmain

.hang:
    cli
    hlt
    jmp .hang


section .bss
align 16

stack_bottom:
    resb 16384                ; 16 KiB stack
stack_top: