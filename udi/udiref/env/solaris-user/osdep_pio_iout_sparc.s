!
! Assembly language implementation for inb, inw, inl, outb, outw, outl for
! Sparc architecture
! These routines are purely no-ops
!

.file "osdep_pio_iout_sparc.s"

.globl inb, outb, inw, outw, inl, outl

.align 4
inb:
	mov	0, %o0

.align 4
outb:
	mov	0, %o0

.align 4
inw:
	mov	0, %o0

.align 4
outw:
	mov	0, %o0

.align 4
inl:
	mov	0, %o0

.align 4
outl:
	mov	0, %o0
