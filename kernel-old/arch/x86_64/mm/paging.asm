[bits 64]

load_pagedir:
    mov cr3, rdi
    retq
GLOBAL load_pagedir

invalidate_tlb:
    mov rax, cr3
    mov cr3, rax
    retq
GLOBAL invalidate_tlb
