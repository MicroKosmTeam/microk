[bits 64]

; Function to activate SSE
global ActivateSSE
ActivateSSE:
	mov rax, cr0
	and ax, 0xFFFB		; Clear coprocessor emulation CR0.EM
	or ax, 0x2		; Set coprocessor monitoring  CR0.MP
	mov cr0, rax
	mov rax, cr4
	or ax, 3 << 9		; Set CR4.OSFXSR and CR4.OSXMMEXCPT at the same time
	mov cr4, rax
	retq
