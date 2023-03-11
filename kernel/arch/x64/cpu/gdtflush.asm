[bits 64]

; Function to load the GDT
global FlushGDT
FlushGDT:
	lgdt [rdi]
	mov ax, 0x30
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	pop rdi
	mov rax, 0x28
	push rax
	push rdi
	retfq

; Function to load the TSS
global FlushTSS
FlushTSS:
	mov ax, 0x48
	ltr ax
	ret
