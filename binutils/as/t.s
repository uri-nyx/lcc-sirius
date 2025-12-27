; _exec_header:
; ;	MAGIC  TEXTSZ  DATASZ  BSS_SZ
; #d32	0o314, _end - _start, 0, 0x0000
; ;	SYMS   ENTRY   TRSIZE  DRSIZE
; #d32	0, 0,  0x0000, 0x0000

_start:
; beq x10, x0, hello
; hello:
;     add x10, x10, x10


; format III 
; add x10, x11, x12
; sub x10, x11, x12
; shll x10, x11, x12
; shrl x10, x11, x12
; shra x10, x11, x12
; and x10, x11, x12     
; or x10, x11, x12
; xor x10, x11, x12
; slt x10, x11, x12
; sltu x10, x11, x12
; not x10, x11
; ctz x10, x11
; clz x10, x11
; pcount x10, x11
; ror x10, x11, x12
; rol x10, x11, x12

; format II Branches
; RELOC RECORD (LITTLE ENDIAN)
; offset:   2800 0000 -> 40 +32
; method:   0100 0000 ->  1 (12 bits relative)
; value:    5000 0000 -> 80 
; base:     0100 0000 ->  1 (code segment)
; Relocation to ve done at offset 40 in the code section
; 'beq x10, x11, _end'
; the value will be 80 + the start of code segment (that is 0)
; and will be written in the immediate field
; group:3, opcode: 4, reg: 5, reg: 5, imm: 15                   │ ENCODING  II │
; IN A.OUT:
;    60 a5 80 00
;   group:  0b011
;   opcode: 0x0
;   reg1:   10
;   reg2:   11
;   imm:    0 (it has to relocate to the address 80)
; The linker shall perform this calculation to relocate:
; (address - pc) >> 2. The shift occurs because Sirius performs jumps
; only to 4 byte aligned addresses, so it wins two extra bits.
; IN CUSTOM.BIN (as should be loaded):
;    60 a5 80 0a

; beq x10, x11, _end
; bne x10, x11, _end
; ble x10, x11, _end
; bleu x10, x11, _end
; blt x10, x11, _end
; bltu x10, x11, _end
; bge x10, x11, _end
; bgeu x10, x11, _end
; bgt x10, x11, _end
; bgtu x10, x11, _end


; J instructions
;jal x1, _end
; RELOC RECORD (BIG ENDIAN)
;   offset:     0000 0004 -> 4 +32
;   method:     0000 0005 -> 5 (20 bit relative)
;   value:      0000 0014 -> 20
;   base:       0000 0001 -> 1 (code segment)
;
; INSTRUCTION:  4010 0000
;      offset:  ___0 0000
; computed address shall be base + value (0+20)
; the offset jump is thus (address - pc) >> 2.
; That is (20 - 0) >> 2 = 5.
; and the LOADED INSTRUCTION:
;   4010 0005


; j _end
; j _end
;jalr x1, 0x10(x10)
; ret
; jr x10

; addi x10, x11, 0xff
; addi x10, x11, -255
; andi x10, x11, 0xff
; ori x10, x11, 0xff
; xori x10, x11, 0xff
; slti x10, x11, 0xff
; sltiu x10, x11, 0xff
; shrli x10, x11, 0xf
; shrai x10, x11, 0xf
; shlli x10, x11, 0xf
; muli x10, x11, 0xf
; mulih x10, x11, 0xf
; idivi x10, x11, 0xf
; mv x10, x11

; lb x10, 0xF(x11)
; lbu x10, 0xF(x11)
; lh x10, 0xF(x11)
; lhu x10, 0xF(x11)

; lbd x10, 0xF(x11)
; lbud x10, 0xF(x11)
; lhd x10, 0xF(x11)
; lhud x10, 0xF(x11)
; lwd x10, 0xF(x11)

; la x10, _end
; li x10, 0xfffffff
; lui x10, 0xfff
; auipc x10, 0xfff

; sb x10, 0xf(x11)
; sh x10, 0xf(x11)
; sw x10, 0xf(x11)

; sbd x10, 0xf(x11)
; shd x10, 0xf(x11)
; swd x10, 0xf(x11)

; copy x10, x11, x12
; copymd x10, x11, x12
; copydm x10, x11, x12
; copydd x10, x11, x12
; swap x10, x11, x12
; ;swapmd x10, x11, x12
; ;swapdm x10, x11, x12
; ;swapdd x10, x11, x12
; fill x10, x11, x12
; fillh x10, x11, x12
; fillsq x10, x11, x12
; fillw x10, x11, x12
; thro x10, x11
; from x10, x11

; popb x10, x11
; poph x10, x11
; pop x10, x11
; pushb x10, x11
; pushh x10, x11
; push x10, x11
; save x10, x11, x12
; restore x10, x11, x12
; exch x10, x11

; syscall x10, 0xfa
; sysret
; gsreg x10
; ssreg x10
; trace x10, x11, x12, x13
; cli
; sti
; mmu.toggle x10
; mmu.map x10, x11, x12, x13, 1, 1
; add x10, x10, x10
; mmu.unmap x10, x11
; mmu.stat    x10, x11, x12
; mmu.setpt   x10, x11, 0x3ff
; mmu.update  x10, x11, x12, 0xf, 1, 1
; mmu.switch  x10, x11, 0x7, 0xf
; mmu.getpt   x10
; umode.toggle x10, x11
;.text
;.import _in_another_file
; 0000 0000 name
; 8000 0000 type UNDEFINED
; 0000 0000 value meaningless
la x10, _in_another_file
ret
;.space 0x10000
;#res 0x10000
_end: