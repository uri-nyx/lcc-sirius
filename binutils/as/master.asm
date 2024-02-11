; This is the basic assembler implementation for the Taleä Computer System
#once

SYS    = 0b000
MEM    = 0b001
BRANCH_  = 0b011
LOAD   = 0b100
ALUI   = 0b101
ALUR   = 0b110
STOR_E_  = 0b111

BLANK5 = 0b00000
BLANK10 = 0b00000_00000
BLANK15 = 0b00000_00000_00000

LUI     = 0b010_0001
AUIPC   = 0b010_0010
JAL     = 0b010_0000
JALR    = 0b100_0001

BEQ     = BRANCH_  @ 0x0
BNE     = BRANCH_  @ 0x1
BLT     = BRANCH_  @ 0x2
BGE     = BRANCH_  @ 0x3
BLTU    = BRANCH_  @ 0x4
BGEU    = BRANCH_  @ 0x5

LB      = LOAD @ 0x2
LBU     = LOAD @ 0x3
LBD     = LOAD @ 0x4
LBUD    = LOAD @ 0x5
LH      = LOAD @ 0x6
LHU     = LOAD @ 0x7
LHD     = LOAD @ 0x8
LHUD    = LOAD @ 0x9
LW      = LOAD @ 0xa
LWD     = LOAD @ 0xb

MULI    = ALUI @ 0x0
MULIH   = ALUI @ 0x1
IDIVI   = ALUI @ 0x2
ADDI    = ALUI @ 0x3
SUBI    = ALUI @ 0x4

OR_I     = ALUI @ 0x5
AND_I    = ALUI @ 0x6
XOR_I    = ALUI @ 0x7
SHIRA   = ALUI @ 0x8
SHIRL   = ALUI @ 0x9
SHILL   = ALUI @ 0xa

SLTI    = ALUI @ 0xb
SLTIU   = ALUI @ 0xc

ADD_    = ALUR @ 0x0 
SUB_    = ALUR @ 0x1 
IDIV    = ALUR @ 0x2
MUL     = ALUR @ 0x3

OR_      = ALUR @ 0x4 
AND_     = ALUR @ 0x5
XOR_     = ALUR @ 0x6 

NOT     = ALUR @ 0x7
CTZ     = ALUR @ 0x8
CLZ     = ALUR @ 0x9
PCOUNT  = ALUR @ 0xa

SHRA    = ALUR @ 0xb 
SHRL    = ALUR @ 0xc 
SHLL    = ALUR @ 0xd 
ROR_     = ALUR @ 0xe
ROL     = ALUR @ 0xf

SB      = STOR_E_ @ 0x0
SBD     = STOR_E_ @ 0x1
SH      = STOR_E_ @ 0x2
SHD     = STOR_E_ @ 0x3
SW      = STOR_E_ @ 0x4
SWD     = STOR_E_ @ 0x5

COPY    = MEM @ 0x0
SWAP_    = MEM @ 0x1
FILL_    = MEM @ 0x2
THRO    = MEM @ 0x3
FROM    = MEM @ 0x4

POPB    = MEM @ 0x5
POPH    = MEM @ 0x6
POP     = MEM @ 0x7
PUSHB   = MEM @ 0x8
PUSHH   = MEM @ 0x9
PUSH    = MEM @ 0xa

SAVE    = MEM @ 0xb
RESTOR_E_ = MEM @ 0xc
EXCH    = MEM @ 0xd
SLT     = MEM @ 0xe 
SLTU   = MEM @ 0xf 

SYSCALL  = SYS @ 0x2
GSREG    = SYS @ 0x3
SSREG    = SYS @ 0x4
TRACE   = SYS @ 0x5
SYSRET   = SYS @ 0x6
MMUTOGGLE = SYS @ 0x7
MMUMAP = SYS @ 0x8
MMUUNMAP = SYS @ 0x9
MMUSTAT = SYS @ 0xa
MMUSETPT = SYS @ 0xb
MMUUPDATE = SYS @ 0xc
UMODETOGGLE = SYS @ 0xd
MMUSW = SYS @ 0xe
MMUGPT = SYS @ 0xf


#ruledef reg {
    zero => 0b00000
    ra  =>  0b00001
    sp  =>  0b00010
    gp  =>  0b00011
    tp  =>  0b00100
    t0  =>  0b00101
    t1  =>  0b00110
    t2  =>  0b00111
    s1  =>  0b01001
    fp  =>  0b01000
    s0  =>  0b01000
    a0  =>  0b01010
    a1  =>  0b01011
    a2  =>  0b01100
    a3  =>  0b01101
    a4  =>  0b01110
    a5  =>  0b01111
    a6  =>  0b10000
    a7  =>  0b10001
    s2  =>  0b10010
    s3  =>  0b10011
    s4  =>  0b10100
    s5  =>  0b10101
    s6  =>  0b10110
    s7  =>  0b10111
    s8  =>  0b11000
    s9  =>  0b11001
    s10 =>  0b11010
    s11 =>  0b11011
    t3  =>  0b11100
    t4  =>  0b11101
    t5  =>  0b11110
    t6  =>  0b11111
    x {n: u5} => n
}

#ruledef {
    lui   {rd: reg}, {imm: u20}  => LUI   @ rd @ imm ;done
    auipc {rd: reg}, {imm: u20}  => AUIPC @ rd @ imm ;done
    jal   {rd: reg}, {label}  => { ;done
        imm = label - $
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm20 = imm >> 2
        JAL @ rd @ imm20`20
    }

    jal   {rd: reg}, [{offset: i22}]  => {
        assert((offset % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm20 = offset >> 2
        JAL @ rd @ imm20`20
    }
    jalr {rd: reg}, {imm: s17}({rs1: reg}) => {  ;done
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        JALR @ rd @ rs1 @ (imm >> 2)`15
    }

    beq  {rs1: reg}, {rs2: reg}, {label} => { ;done
        imm = label - $
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = imm >> 2
        BEQ @ rs1 @ rs2 @ imm15`15
    }
    bne  {rs1: reg}, {rs2: reg}, {label} => { ;done
        imm = label - $
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = imm >> 2
        BNE @ rs1 @ rs2 @ imm15`15
    }
    blt  {rs1: reg}, {rs2: reg}, {label} => {;done
        imm = label - $
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = imm >> 2
        BLT @ rs1 @ rs2 @ imm15`15
    }
    bge  {rs1: reg}, {rs2: reg}, {label} => {;done
        imm = label - $
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = imm >> 2
        BGE @ rs1 @ rs2 @ imm15`15
    }
    bltu {rs1: reg}, {rs2: reg}, {label} => {;done
        imm = label - $
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = imm >> 2
        BLTU @ rs1 @ rs2 @ imm15`15
    }
    bgeu {rs1: reg}, {rs2: reg}, {label} => {;done
        imm = label - $
        assert((imm % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = imm >> 2
        BGEU @ rs1 @ rs2 @ imm15`15
    }

    beq  {rs1: reg}, {rs2: reg}, [{offset: i17}] => {
        assert((offset % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = offset >> 2
        BEQ @ rs1 @ rs2 @ imm15`15
    }
    bne  {rs1: reg}, {rs2: reg}, [{offset: i17}] => {
        assert((offset % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = offset >> 2
        BNE @ rs1 @ rs2 @ imm15`15
    }
    blt  {rs1: reg}, {rs2: reg}, [{offset: i17}] => {
        assert((offset % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = offset >> 2
        BLT @ rs1 @ rs2 @ imm15`15
    }
    bge  {rs1: reg}, {rs2: reg}, [{offset: i17}] => {
        assert((offset % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = offset >> 2
        BGE @ rs1 @ rs2 @ imm15`15
    }
    bltu {rs1: reg}, {rs2: reg}, [{offset: i17}] => {
        assert((offset % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = offset >> 2
        BLTU @ rs1 @ rs2 @ imm15`15
    }
    bgeu {rs1: reg}, {rs2: reg}, [{offset: i17}] => {
        assert((offset % 4) == 0)
        ; NOTE OFFSETS FOR_ JUMPS MUST BE 4 BYTE ALIGNED,
        ; OTHERWISE IT WILL RESULT IN AN EXCEPTION
        imm15 = offset >> 2
        BGEU @ rs1 @ rs2 @ imm15`15
    }

    lb   {rd: reg}, {imm: s15}({rs1: reg})  => LB @ rd @ rs1 @ imm ;done
    lbu  {rd: reg}, {imm: s15}({rs1: reg})  => LBU @ rd @ rs1 @ imm;done
    lbd  {rd: reg}, {imm: s15}({rs1: reg})  => LBD @ rd @ rs1 @ imm
    lbud {rd: reg}, {imm: s15}({rs1: reg})  => LBUD @ rd @ rs1 @ imm
    lh   {rd: reg}, {imm: s15}({rs1: reg})  => LH @ rd @ rs1 @ imm;done
    lhu  {rd: reg}, {imm: s15}({rs1: reg})  => LHU @ rd @ rs1 @ imm;done
    lhd  {rd: reg}, {imm: s15}({rs1: reg})  => LHD @ rd @ rs1 @ imm
    lhud {rd: reg}, {imm: s15}({rs1: reg})  => LHUD @ rd @ rs1 @ imm
    lw   {rd: reg}, {imm: s15}({rs1: reg})  => LW @ rd @ rs1 @ imm;done
    lwd  {rd: reg}, {imm: s15}({rs1: reg})  => LWD @ rd @ rs1 @ imm

    muli  {rd: reg}, {rs1: reg}, {imm: i15} => MULI @ rd @ rs1 @ imm;done
    mulih {rd: reg}, {rs1: reg}, {imm: i15} => MULIH @ rd @ rs1 @ imm;done
    idivi {rd: reg}, {rs1: reg}, {imm: i15} => {;done
        assert(imm != 0)
        IDIVI @ rd @ rs1 @ imm
    }
    addi  {rd: reg}, {rs1: reg}, {imm: i15} => ADDI @ rd @ rs1 @ imm;done
    subi  {rd: reg}, {rs1: reg}, {imm: i15} => SUBI @ rd @ rs1 @ imm;done

    ori   {rd: reg}, {rs1: reg}, {imm: i15} => OR_I @ rd @ rs1 @ imm;done
    andi  {rd: reg}, {rs1: reg}, {imm: i15} => AND_I @ rd @ rs1 @ imm;done
    xori  {rd: reg}, {rs1: reg}, {imm: i15} => XOR_I @ rd @ rs1 @ imm;done
    shrai {rd: reg}, {rs1: reg}, {imm: i15} => SHIRA @ rd @ rs1 @ imm;done
    shrli {rd: reg}, {rs1: reg}, {imm: i15} => SHIRL @ rd @ rs1 @ imm;done
    shlli {rd: reg}, {rs1: reg}, {imm: i15} => SHILL @ rd @ rs1 @ imm ;done;TODO: add change of i to documentation

    slti  {rd: reg}, {rs1: reg}, {imm: i15} => SLTI @ rd @ rs1 @ imm;done
    sltiu {rd: reg}, {rs1: reg}, {imm: u15} => SLTIU @ rd @ rs1 @ imm;done

    add    {rd: reg}, {rs1: reg}, {rs2: reg} => ADD_ @ rd @ rs1 @ rs2 @ BLANK10;done
    sub    {rd: reg}, {rs1: reg}, {rs2: reg} => SUB_ @ rd @ rs1 @ rs2 @ BLANK10;done
    idiv   {rd: reg}, {rd2: reg}, {rs1: reg}, {rs2: reg} => IDIV @ rd @ rd2 @ rs1 @ rs2 @ BLANK5;done
    mul    {rd: reg}, {rd2: reg}, {rs1: reg}, {rs2: reg} => MUL @ rd @ rd2 @ rs1 @ rs2  @ BLANK5;done

    or     {rd: reg}, {rs1: reg}, {rs2: reg} => OR_ @ rd @ rs1 @ rs2  @ BLANK10;done
    and    {rd: reg}, {rs1: reg}, {rs2: reg} => AND_ @ rd @ rs1 @ rs2 @ BLANK10;done
    xor    {rd: reg}, {rs1: reg}, {rs2: reg} => XOR_ @ rd @ rs1 @ rs2 @ BLANK10;done

    slt    {rd: reg}, {rs1: reg}, {rs2: reg} => SLT @ rd @ rs1 @ rs2 @ BLANK10;done
    sltu   {rd: reg}, {rs1: reg}, {rs2: reg} => SLTU @ rd @ rs1 @ rs2 @ BLANK10;done

    not    {rd: reg}, {rs1: reg} => NOT @ rd @ rs1 @ BLANK15
    ctz    {rd: reg}, {rs1: reg} => CTZ @ rd @ rs1 @ BLANK15
    clz    {rd: reg}, {rs1: reg} => CLZ @ rd @ rs1 @ BLANK15
    pcount {rd: reg}, {rs1: reg} => PCOUNT @ rd @ rs1 @ BLANK15

    shra   {rd: reg}, {rs1: reg}, {rs2: reg} => SHRA @ rd @ rs1 @ rs2 @ BLANK10;done
    shrl   {rd: reg}, {rs1: reg}, {rs2: reg} => SHRL @ rd @ rs1 @ rs2 @ BLANK10;done
    shll   {rd: reg}, {rs1: reg}, {rs2: reg} => SHLL @ rd @ rs1 @ rs2 @ BLANK10;done
    ror    {rd: reg}, {rs1: reg}, {rs2: reg} => ROR_ @ rd @ rs1 @ rs2  @ BLANK10
    rol    {rd: reg}, {rs1: reg}, {rs2: reg} => ROL @ rd @ rs1 @ rs2  @ BLANK10

    sb  {rs2: reg}, {imm: i15}({rs1: reg})   => SB @ rs2 @ rs1 @ imm;done
    sbd {rs2: reg}, {imm: i15}({rs1: reg})   => SBD @ rs2 @ rs1 @ imm
    sh  {rs2: reg}, {imm: i15}({rs1: reg})   => SH @ rs2 @ rs1 @ imm;done
    shd {rs2: reg}, {imm: i15}({rs1: reg})   => SHD @ rs2 @ rs1 @ imm
    sw  {rs2: reg}, {imm: i15}({rs1: reg})   => SW @ rs2 @ rs1 @ imm;done
    swd {rs2: reg}, {imm: i15}({rs1: reg})   => SWD @ rs2 @ rs1 @ imm

    copy    {rd: reg}, {rs1: reg}, {rs2: reg} => COPY @ rd @ rs1 @ rs2 @ BLANK10
    copymd    {rd: reg}, {rs1: reg}, {rs2: reg} => COPY @ rd @ rs1 @ rs2 @ BLANK5 @ 0b00001
    copydm    {rd: reg}, {rs1: reg}, {rs2: reg} => COPY @ rd @ rs1 @ rs2 @ BLANK5 @ 0b00010
    copydd    {rd: reg}, {rs1: reg}, {rs2: reg} => COPY @ rd @ rs1 @ rs2 @ BLANK5 @ 0b00011
    swap    {rd: reg}, {rs1: reg}, {rs2: reg} => SWAP_ @ rd @ rs1 @ rs2 @ BLANK10
    fill    {rd: reg}, {rs1: reg}, {rs2: reg} => FILL_ @ rd @ rs1 @ rs2 @ BLANK10
    fillh    {rd: reg}, {rs1: reg}, {rs2: reg} => FILL_ @ rd @ rs1 @ rs2 @ BLANK5 @ 0b00001
    fillsq    {rd: reg}, {rs1: reg}, {rs2: reg} => FILL_ @ rd @ rs1 @ rs2 @ BLANK5 @ 0b00010
    fillw    {rd: reg}, {rs1: reg}, {rs2: reg} => FILL_ @ rd @ rs1 @ rs2 @ BLANK5 @ 0b00011
    thro    {rd: reg}, {rs1: reg} => THRO @ rd @ rs1 @ BLANK15
    from    {rd: reg}, {rs1: reg} => FROM @ rd @ rs1 @ BLANK15

    popb    {rd: reg}, {rs1: reg} => POPB @ rd @ rs1 @ BLANK15
    poph    {rd: reg}, {rs1: reg} => POPH @ rd @ rs1 @ BLANK15
    pop     {rd: reg}, {rs1: reg} => POP @ rd @ rs1  @ BLANK15
    pushb   {rd: reg}, {rs1: reg} => PUSHB @ rd @ rs1 @ BLANK15
    pushh   {rd: reg}, {rs1: reg} => PUSHH @ rd @ rs1  @ BLANK15
    push    {rd: reg}, {rs1: reg} => PUSH @ rd @ rs1  @ BLANK15

    save    {rd: reg}, {rs1: reg}, {rs2: reg} => SAVE @ rd @ rs1 @ rs2 @ BLANK10
    restore {rd: reg}, {rs1: reg}, {rs2: reg} => RESTOR_E_ @ rd @ rs1 @ rs2 @ BLANK10
    exch    {rd: reg}, {rs1: reg}  => EXCH @ rd @ rs1 @ BLANK15
    
    syscall {rd: reg}, {vector: u8} => SYSCALL @ rd @ 0x000 @ vector 
    gsreg   {rd: reg} => GSREG @ rd @ BLANK10 @ BLANK10
    ssreg   {rs1: reg} => SSREG @ rs1 @ BLANK10 @ BLANK10
    trace   {r1: reg}, {r2: reg}, {r3: reg}, {r4: reg} => TRACE @ r1 @ r2 @ r3 @ r4 @ BLANK5
    sysret  => SYSRET @ BLANK15 @ BLANK10
    cli  => SYSRET @ 0b00001 @ BLANK10 @ BLANK10
    sti  => SYSRET @ 0b00010 @ BLANK10 @ BLANK10


    mmu.toggle  {r1: reg} => MMUTOGGLE @ r1 @ BLANK10 @ BLANK10
    mmu.map     {r1: reg}, {r2: reg}, {r3: reg}, {r4: reg}, {w: u1}, {x: u1} => MMUMAP @ r1 @ r2 @ r3 @ r4 @ 0b000 @ w @ x
    mmu.unmap   {r1: reg}, {r2: reg} => MMUUNMAP @ r1 @ r2 @ BLANK5 @ BLANK10
    mmu.stat    {rd: reg}, {rs1: reg}, {rs2: reg} => MMUSTAT @ rd @ rs1 @ rs2 @ BLANK10
    mmu.setpt   {r1: reg}, {r2: reg}, {imm: u12} => MMUSETPT @ r1 @ r2 @ 0b000 @ imm
    mmu.update  {r1: reg}, {r2: reg}, {r3: reg}, {pt: u4}, {dirty: u1}, {present: u1} => MMUUPDATE @ r1 @ r2 @ r3 @ 0b0000 @ pt @ dirty @ present
    mmu.switch  {r1: reg}, {r2: reg}, {pt: u4}, {clone: u4} => MMUSW @ r1 @ r2 @ BLANK5 @ 0b00 @ clone @ pt
    mmu.getpt   {rd1: reg} => MMUGPT @ rd1 @ BLANK10 @ BLANK10
    umode.toggle {entry: reg}, {usp: reg} => UMODETOGGLE @ entry @ usp @ BLANK15

}

#once

#ruledef {
    seqz {rd: reg}, {rs: reg} => asm {
        sltiu {rd}, {rs}, 1
    }
    snez {rd: reg}, {rs: reg} => asm {
        sltu {rd}, zero, {rs}
    }
    sgt {rd: reg}, {rs1: reg}, {rs2: reg} => asm {
        slt {rd}, {rs2}, {rs1}
    }
    sgtu {rd: reg}, {rs1: reg}, {rs2: reg} => asm {
        sltu {rd}, {rs2}, {rs1}
    }
    beqz {rd: reg}, {label} => asm {
        beq {rd}, zero, {label}
    }
    bnez {rd: reg}, {label} => asm {
        bne {rd}, zero, {label}
    }

    ble  {rs1: reg}, {rs2: reg}, {label} => asm {
        bge {rs2}, {rs1}, {label}
    };done

    bleu  {rs1: reg}, {rs2: reg}, {label} => asm {
        bgeu {rs2}, {rs1}, {label}
    };done

    bgt  {rs1: reg}, {rs2: reg}, {label} => asm {
        blt {rs2}, {rs1}, {label}
    };done

    bgtu  {rs1: reg}, {rs2: reg}, {label} => asm {
        bltu {rs2}, {rs1}, {label}
    };done

    li {rd: reg}, {const: i32} => {
        assert(const < 0x3fff)
        asm {
        addi {rd}, zero, {const}`15
        }
    };done
    li {rd: reg}, {const: i32}  => {
        cons = const`12
        asm {
        lui {rd}, ({const} >> 12)`20
        addi {rd}, {rd}, {cons}
        }
    };done
    la {rd: reg}, {label} => asm {
        auipc    {rd}, (({label}-$) >> 12)`20
        addi     {rd},{rd},(({label}-($-4))`12)
    };done
    llb {rd: reg}, {label} => asm {
        auipc {rd},(({label}-$) >> 12)`20
        lb {rd},(({label}-($-4))`12)({rd})

    }
    llh {rd: reg}, {label} => asm {
        auipc {rd},(({label}-$) >> 12)`20
        lh {rd},(({label}-($-4))`12)({rd})
    }
    llbu {rd: reg}, {label} => asm {
        auipc {rd},(({label}-$) >> 12)`20
        lbu {rd},(({label}-($-4))`12)({rd})

    }
    llhu {rd: reg}, {label} => asm {
        auipc {rd},(({label}-$) >> 12)`20
        lhu {rd},(({label}-($-4))`12)({rd})
    }
    llw {rd: reg}, {label} => asm {
        auipc {rd},(({label}-$) >> 12)`20
        lw {rd},(({label}-($-4))`12)({rd})
    }
    ssb {rd: reg}, {label}, {rt: reg} => asm {
        auipc    {rt},(({label}-$) >> 12)`20
        sb {rd},(({label}-($-4))`12)({rt})
    }
    ssh {rd: reg}, {label}, {rt: reg} => asm {
        auipc    {rt},(({label}-$) >> 12)`20
        sh {rd},(({label}-($-4))`12)({rt})
    }
    ssw {rd: reg}, {label}, {rt: reg} => asm {
        auipc    {rt},(({label}-$) >> 12)`20
        sw {rd},(({label}-($-4))`12)({rt})
    }
    call {label} => {
        offset = ({label}-$)`12
        assert((label % 4) == 0)
        ; CALLS SHOULD BE ALIGNED
        asm {
            auipc    ra, (({label}-$) >> 12)`20
            jalr     ra, ({offset})(ra)
        }
    }
    tail {label}, {rt: reg} => {
        offset = (label-$)`12
        assert((offset % 4) == 0)
        ; Tail Calls SHOULD BE ALIGNED
        asm {
            auipc    {rt}, (({label}-$) >> 12)`20
            jalr     zero, (offset) ({rt})
        }
    }

    call [{offset}] => {
        ; CALLS SHOULD BE ALIGNED
        asm {
            auipc    ra, ((offset) >> 12)`20
            jalr     ra, (offset`12) (ra)
        }
    }
    tail [{offset}], {rt: reg} => {
        ; Tail Calls SHOULD BE ALIGNED
        asm {
            auipc    {rt}, ((offset) >> 12)`20
            jalr     zero, (offset`12) ({rt})
        }
    }

    mv {rd: reg}, {rs: reg} => asm {addi {rd}, {rs}, 0}
    neg {rd: reg}, {rs: reg} => asm {sub {rd}, zero, {rs}}
    j {label} => asm {jal zero, {label}} ;done
    j [{offset}] => asm {jal zero, [{offset}]}
    jr {rs: reg} => asm {jalr zero, 0({rs})};done
    ret => asm {jalr zero, 0(ra)};done
}

#ruledef {
;     li  a0, 0b1_1_0_010_111110_11111111_000000000000
;                    0b10_000000_00000000_000000000000
;                         ; supervisor, intterupt enabled, mmu disabled
;                         ; priority 2, ivt at 0xf800, pdt at 0xff00

    priority {rs1: reg}, {rt: reg}, {rt2: reg} => asm {
        gsreg {rt}
        li {rt2},  0xe3ffffff ; mask
        and {rt}, {rt}, {rt2}
        shlli {rs1}, {rs1}, 26
        or {rt}, {rt}, {rs1}
        ssreg {rt}
    }
}