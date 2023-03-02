GLOBAL loadIDT
loadIDT:
	lidt [rdi]
	ret

GLOBAL read_cr0
read_cr0:
	mov rax, cr0
	ret

GLOBAL read_cr2
read_cr2:
	mov rax, cr2
	ret

GLOBAL read_cr3
read_cr3:
	mov rax, cr3
	ret

GLOBAL read_cr4
read_cr4:
	mov rax, cr4
	ret
