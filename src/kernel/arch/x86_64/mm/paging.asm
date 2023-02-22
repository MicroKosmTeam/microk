[bits 64]

PushPageDir:
    mov cr3, rdi
    retq
GLOBAL PushPageDir

invalidate_tlb:
    mov rax, cr3
    mov cr3, rax
    retq
GLOBAL invalidate_tlb
