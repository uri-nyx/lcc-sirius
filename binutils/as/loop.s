;.text
txtmod_puts:
	addi x2,x2,-64
	sw  x8,60(x2)
	addi  x8,x2,48
	sw x1,24(x2)
	sw x27,28(x2)
	sw x12,16(x8)
	sw x13,20(x8)
	mv x27,x0 ; LOADI4
	mv x27,x0 ; LOADI4
L49:
	mv x12,x27 ; LOADI4
	call _trace
L50:
	addi x27,x27,1
	li x30,5
	blt x27,x30,L49
L48:
	lw x1,24(x2)
	lw x27,28(x2)
	lw  x8,60(x2)
	addi  x2,x2,64
	jalr x0,0(x1)


_trace:
    trace x12, x12, x12, x12