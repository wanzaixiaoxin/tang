; x86_64 汇编实现上下文切换
; 使用System V AMD64调用约定

section .text
    global coro_swap_context

; void coro_swap_context(void** from, void* to)
; rdi = from
; rsi = to
coro_swap_context:
    ; 保存当前上下文
    ; 保存调用者保存的寄存器
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    
    ; 保存栈指针
    mov [rdi], rsp
    
    ; 恢复目标上下文的栈指针
    mov rsp, rsi
    
    ; 恢复目标上下文的寄存器
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    
    ; 返回
    ret
