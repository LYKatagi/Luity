bits 64

section .text

global irq12_stub
global irq0_stub
global interrupt_fatal_stub
extern irq0_dispatch
extern irq12_dispatch

; Unexpected vectors terminate safely until dedicated exception handlers exist.
interrupt_fatal_stub:
    cli
.fatal_halt:
    hlt
    jmp .fatal_halt

%macro IRQ_STUB 2
%1:
    cld
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    ; Preserve the interrupted stack, then align the temporary call stack for C.
    mov rbx, rsp
    and rsp, -16
    call %2
    mov rsp, rbx

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    iretq
%endmacro

IRQ_STUB irq0_stub, irq0_dispatch
IRQ_STUB irq12_stub, irq12_dispatch
