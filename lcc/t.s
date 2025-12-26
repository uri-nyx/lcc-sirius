	.align 4
	.text
	.globl f
	.align	4
f:
	addi x2,x2,-48
	sw  x8,44(x2)
	addi  x8,x2,32
	sw x1,24(x2)
	sw x12,16(x8)
	la x12,L.2
	call puts
	lw x30,0+16(x8)
	li x29,5
	idiv x10,x0,x30,x29
L.1:
	lw x1,24(x2)
	lw  x8,44(x2)
	addi  x2,x2,48
	jalr x0,0(x1)

	.globl main
	.align	4
main:
	addi x2,x2,-48
	sw  x8,44(x2)
	addi  x8,x2,32
	sw x1,24(x2)
	li x30,5
	sw x30,-8+16(x8)
	li x30,100
	sh x30,-10+16(x8)
	la x30,L.4
	lw x30,0(x30)
	sw x30,-16+16(x8)
	lw x30,-8+16(x8)
	lh x29,-10+16(x8)
	add x12,x30,x29
	call f
	mv x30,x10 ; LOADI4
L.3:
	lw x1,24(x2)
	lw  x8,44(x2)
	addi  x2,x2,48
	jalr x0,0(x1)

	.align 4
	.data
	.align	4
L.4:
	.word	0x42cc0000
	.align	1
L.2:
	.byte	0x52
	.byte	0x75
	.byte	0x6e
	.byte	0x6e
	.byte	0x69
	.byte	0x6e
	.byte	0x67
	.byte	0x20
	.byte	0x66
	.byte	0x28
	.byte	0x29
	.byte	0xa
	.byte	0x0
	.align 4
