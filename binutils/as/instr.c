void format3(unsigned int code)
{
        int dst, src1, src2;

        /* opcode with three register operands */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src2 = tokenvalNumber;
        getToken();
        emitWord((code & 0x7F) << 25 | dst << 20 | src1 << 15 | src2 << 10);
}

void format4(unsigned int code)
{
        int          dst, src1, src2, src3;
        unsigned int c;

        /* opcode with three register operands */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src2 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src3 = tokenvalNumber;
        getToken();
        emitWord((code & 0x7F) << 25 | dst << 20 | src1 << 15 | src2 << 10 |
                 src3 << 5 |
                 ((code & 0x100) ? 1 : 0 /* handle udiv and umul */));
}

void format3CSF(unsigned int code)
{
        int dst, src1, src2;
        int direction;

        /* opcode with three register operands */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src2 = tokenvalNumber;
        getToken();
        direction = (code & 0xf00) >> 8;
        emitWord((code & 0x7F) << 25 | dst << 20 | src1 << 15 | src2 << 10 |
                 direction & 0x3);
}

void format2B(unsigned int code)
{
        /* branch instructions with a 17 bit signed relative address */
        int          src1, src2, tmp;
        Value        v;
        unsigned int immed;

        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src2 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        if (token == TOK_DOTRELADR)
                getToken();
        v = parseExpression();
        if (v.sym == NULL) {
                immed = v.con / 2;
        } else {
                addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_R15, v.con);
                immed = 0;
        }
        if (code & 0x100) {
                tmp  = src2;
                src2 = src1;
                src1 = tmp;
                code &= 0xFF;
        }
        emitWord((code & 0xFF) << 25 | src1 << 20 | src2 << 15 |
                 (immed >> 2) & 0x7FFF);
}

void format2S(unsigned int code)
{
        /* Store instructions with a 15 bit signed operand */

        int          src1, src2, vcon;
        Value        v;
        unsigned int immed;
        int          rvccond;

        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v = parseExpression();
        if (v.sym == NULL) {
                immed = v.con;
                vcon  = v.con;
                expect(TOK_LPAREN);
                getToken();
                expect(TOK_IREGISTER);
                src2 = tokenvalNumber;
                getToken();
                expect(TOK_RPAREN);
                getToken();
        } else {
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                src2 = tokenvalNumber;
                getToken();
                addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
                immed = 0;
                emitWord(OP_AUIPC << 25 | src2 << 20 | immed);
                immed = v.con;
                addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RS12, v.con);
                immed = 0;
        }
        emitWord(code << 25 | src1 << 20 | src2 << 15 | ((immed) & 0x7FFF));
}

void format2(unsigned int code)
{
        int          dst, src1;
        Value        v;
        unsigned int immed;

        /* opcode with two register operands and immediate */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v     = parseExpression();
        immed = v.con;
        immed &= 0x7FFF;

        emitWord((code << 25) | dst << 20 | src1 << 15 | immed);
}

void format2NOIM(unsigned int code)
{
        int          dst, src1;
        Value        v;
        unsigned int immed;

        /* opcode with two register operands and immediate */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        immed = 0;

        emitWord((code << 25) | dst << 20 | src1 << 15 | immed);
}

void formatMv(unsigned int code)
{
        int dst, src1;

        /* opcode with two registers */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();

        emitWord(code << 25 | dst << 20 | src1 << 15);
}

void format2L(unsigned int code)
{
        int          dst, src1, vcon, rvccond;
        Value        v;
        unsigned int immed;

        /* load opcode with two register operands and immediate */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v = parseExpression();
        if (v.sym == NULL) {
                immed = v.con;
                vcon  = v.con;
                expect(TOK_LPAREN);
                getToken();
                expect(TOK_IREGISTER);
                src1 = tokenvalNumber;
                getToken();
                expect(TOK_RPAREN);
                getToken();
        } else {
                addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
                immed = 0;
                emitWord(OP_AUIPC << 25 | dst << 20 | immed);
                immed = v.con;
                addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RL12, v.con);
                immed = 0;
                src1  = dst;
        }

        emitWord(code << 25 | dst << 20 | src1 << 15 | ((immed) & 0x7FFF));
}

void formatLA(unsigned int code)
{
        int          dst, src1, vcon;
        Value        v;
        unsigned int immed;

        /* load opcode with two register operands and immediate */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v = parseExpression();
        addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
        immed = 0;
        emitWord(OP_AUIPC << 25 | dst << 20);
        immed = v.con;
        addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RL12, v.con);
        immed = 0;
        src1  = dst;
        emitWord(OP_ADDI << 25 | dst << 20 | src1 << 15 | immed);
}

void formatCALL(unsigned int code)
{
        int          dst, src1, vcon;
        Value        v;
        unsigned int immed;

        dst = 1;

        /* load opcode with two register operands and immediate */
        v = parseExpression();
        addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
        immed = 0;
        emitWord(OP_AUIPC << 25 | dst << 20);
        immed = v.con;
        addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_JALR12, v.con);
        immed = 0;
        src1  = dst;
        emitWord(OP_JALR << 25 | dst << 20 | src1 << 15 | immed);
}

void format1JAL(unsigned int code)
{
        int   dst;
        Value v;

        /* jal opcode with one register operands and label */
        // expect(TOK_IREGISTER);
        if (token == TOK_IREGISTER) {
                dst = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
        } else
                dst = 1;
        v = parseExpression();
        addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_J20, v.con);
        emitWord((code << 25) | (dst << 20));
}

void format1J(unsigned int code)
{
        int   dst;
        Value v;

        /* jal opcode with label */
        dst = 0;
        v   = parseExpression();
        addFixup(v.sym, currSeg, segPtr[currSeg], METHOD_J20, v.con);
        emitWord((code << 25));
}

void formatJR(unsigned int code)
{
        int          dst, src1;
        Value        v;
        unsigned int immed;

        /* opcode with two register operands and immediate */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v = parseExpression();
        if (v.sym)
                error("jalr instruction does not support labels (yet)");
        immed = v.con;
        immed &= 0x3FFFF;
        expect(TOK_LPAREN);
        getToken();
        expect(TOK_IREGISTER);
        src1 = tokenvalNumber;
        getToken();
        expect(TOK_RPAREN);
        getToken();

        emitWord((code << 25) | dst << 20 | src1 << 15 | immed >> 2);
}

void formatJ0R(unsigned int code)
{
        int          dst, src1;
        unsigned int immed;

        /* opcode with one register operands and immediate */
        dst = 0;
        expect(TOK_IREGISTER);
        src1  = tokenvalNumber;
        immed = 0;
        getToken();
        emitWord(code << 25 | src1 << 15);
}

void formatRET(unsigned int code)
{
        int          dst, src1;
        unsigned int immed;

        /* opcode with two register operands and immediate */
        dst = 0;
        getToken();
        src1  = 1;
        immed = 0;

        emitWord(code << 25 | src1 << 15);
}

void formatLI(unsigned int code)
{
        int          dst, src;
        Value        v;
        unsigned int immed;

        /* opcode with one register operands and immediate */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        src = 0;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v     = parseExpression();
        immed = v.con;
        if (insrange(15, v.con)) {
                emitWord(OP_ADDI << 25 | dst << 20 | immed & 0x7fff);
                return;
        } else {
                emitWord(OP_LUI << 25 | dst << 20 | (immed >> 12) & 0xfffff);
                src = dst;
        }
        if (dst != 0) {
                emitWord(OP_ADDI << 25 | dst << 20 | src << 15 | immed & 0xfff);
        }
}

void format1U(unsigned int code)
{
        int          dst;
        Value        v;
        unsigned int immed;

        /* opcode with one register operands and immediate */
        expect(TOK_IREGISTER);
        dst = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v     = parseExpression();
        immed = v.con;
        emitWord(code << 25 | dst << 20 | immed & 0xFFFFF);
}

void formatSCALL(unsigned int code)
{
        int          src;
        Value        v;
        unsigned int immed;

        /* Syscall format */
        expect(TOK_IREGISTER);
        src = tokenvalNumber;
        getToken();
        expect(TOK_COMMA);
        getToken();
        v     = parseExpression();
        immed = v.con;
        immed &= 0xFF;
        emitWord(code << 25 | src << 20 | immed);
}

void formatSRET(unsigned int code)
{
        int cli_sti = (code & 0xf00) >> 8;
        emitWord(code << 25 | cli_sti << 20);
}

void formatSREG(unsigned int code)
{
        int reg;

        expect(TOK_IREGISTER);
        reg = tokenvalNumber;
        getToken();

        emitWord(code << 25 | reg << 20);
}

void formatMMU(unsigned int code)
{
        int   r1, r2, r3, r4;
        int   w, x, pt, dirty, present, clone, immed;
        Value v;
        switch (code) {
        case OP_MMUTOG:
        case OP_MMUGPT:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                emitWord(code << 25 | r1 << 20);
                break;
        case OP_MMUMAP:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r2 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r3 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r4 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                v = parseExpression();
                w = v.con & 0x1;
                expect(TOK_COMMA);
                getToken();
                v = parseExpression();
                x = v.con & 0x1;
                //getToken ();
                emitWord(code << 25 | r1 << 20 | r2 << 15 | r3 << 10 | r4 << 5 |
                         w << 1 | x);
                break;
        case OP_MMUUNMAP:
        case OP_UMODETOG:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r2 = tokenvalNumber;
                getToken();
                emitWord(code << 25 | r1 << 20 | r2 << 15);
                break;
        case OP_MMUSTAT:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r2 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r3 = tokenvalNumber;
                getToken();
                emitWord(code << 25 | r1 << 20 | r2 << 15 | r3 << 10);
                break;
        case OP_MMUGETFRAME:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r2 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r3 = tokenvalNumber;
                getToken();
                emitWord(code & 0xf << 25 | r1 << 20 | r2 << 15 | r3 << 10 | 1);
                break;
        case OP_MMUSPT:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r2 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                v     = parseExpression();
                immed = v.con & 0xfff;
                //getToken();
                emitWord(code << 25 | r1 << 20 | r2 << 15 | immed);
                break;
        case OP_MMUUPDATE:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r2 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r3 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                v  = parseExpression();
                pt = v.con & 0xf;
                //getToken ();
                expect(TOK_COMMA);
                getToken();
                v     = parseExpression();
                dirty = v.con & 0x1;
                //getToken ();
                expect(TOK_COMMA);
                getToken();
                v       = parseExpression();
                present = v.con & 0x1;
                //getToken ();
                emitWord(code << 25 | r1 << 20 | r2 << 15 | r3 << 10 | pt << 2 |
                         dirty << 1 | present);
                break;
        case OP_MMUSW:
                expect(TOK_IREGISTER);
                r1 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                expect(TOK_IREGISTER);
                r2 = tokenvalNumber;
                getToken();
                expect(TOK_COMMA);
                getToken();
                v  = parseExpression();
                pt = v.con & 0xf;
                //getToken ();
                expect(TOK_COMMA);
                getToken();
                v     = parseExpression();
                clone = v.con & 0xf;
                //getToken ();
                emitWord(code << 25 | r1 << 20 | r2 << 15 | clone << 4 | pt);
                break;
        default:
                error("No such MMU opcode!");
        }
}

typedef struct instr {
        char *name;
        void (*func)(unsigned int code);
        unsigned int code;
} Instr;

Instr instrTable[] = {

        /* directives */
        { ".syn", dotSyn, 0 },
        { ".nosyn", dotNosyn, 0 },
        { ".text", dotCode, 0 },
        { ".data", dotData, 0 },
        { ".section", dotSection, 0 },
        { ".bss", dotBss, 0 },
        { ".globl", dotExport, 0 },
        { ".import", dotImport, 0 },
        { ".extern", dotImport, 0 },
        { ".align", dotAlign, 0 },
        { ".p2align", dotAlign, 0 },
        { ".space", dotSpace, 0 },
        { ".locate", dotLocate, 0 },
        { ".byte", dotByte, 0 },
        { ".asciz", dotByte, 0 },
        { ".ascii", dotByte, 0 },
        { ".half", dotHalf, 0 },
        { ".short", dotHalf, 0 },
        { ".word", dotWord, 0 },
        { ".long", dotWord, 0 },
        { ".quad", dotDword, 0 },
        { ".set", dotSet, 0 },
        { ".file", dotFile, 0 },
        { ".loc", dotLoc, 0 },
        { ".stabs", dotStabs, 0 },
        { ".stabn", dotStabn, 0 },
        { ".type", Ignore, 0 },
        { ".size", Ignore, 0 },
        { ".ident", Ignore, 0 },
        { ".equ", Ignore, 0 },
        { ".weak", Ignore, 0 },
        { ".local", Ignore, 0 },
        { ".comm", Ignore, 0 },
        { ".uleb128", Ignore, 0 },
        { ".cfi_sections", Ignore, 0 },
        { ".cfi_sections", Ignore, 0 },
        { ".cfi_startproc", Ignore, 0 },
        { ".cfi_endproc", Ignore, 0 },
        /* HARDWARE INSTRUCTIONS */
        /* arithmetico-logical instructions */
        { "add", format3, OP_ADD },
        { "sub", format3, OP_SUB },
        { "shll", format3, OP_SLL },
        { "shrl", format3, OP_SRL },
        { "shra", format3, OP_SRA },
        { "and", format3, OP_AND },
        { "or", format3, OP_OR },
        { "xor", format3, OP_XOR },
        { "slt", format3, OP_SLT },
        { "sltu", format3, OP_SLTU },
        { "not", format2NOIM, OP_NOT },
        { "ctz", format2NOIM, OP_CTZ },
        { "clz", format2NOIM, OP_CLZ },
        { "pcount", format2NOIM, OP_PCNT },
        { "ror", format3, OP_ROR },
        { "rol", format3, OP_ROL },
        /* branch instructions */
        { "beq", format2B, OP_BEQ },
        { "bne", format2B, OP_BNE },
        { "ble", format2B, OP_BLE },
        { "bleu", format2B, OP_BLEU },
        { "blt", format2B, OP_BLT },
        { "bltu", format2B, OP_BLTU },
        { "bge", format2B, OP_BGE },
        { "bgeu", format2B, OP_BGEU },
        { "bgt", format2B, OP_BGT },
        { "bgtu", format2B, OP_BGTU },
        { "jal", format1JAL, OP_JAL },
        { "j", format1J, OP_JAL },
        { "jalr", formatJR, OP_JALR },
        { "ret", formatRET, OP_JALR },
        { "jr", formatJ0R, OP_JALR },
        { "call", formatCALL, OP_CALL },
        /* immediate instructions */
        { "addi", format2, OP_ADDI },
        { "mv", formatMv, OP_ADDI },
        { "andi", format2, OP_ANDI },
        { "ori", format2, OP_ORI },
        { "xori", format2, OP_XORI },
        { "slti", format2, OP_SLTI },
        { "sltiu", format2, OP_SLTIU },
        { "shlli", format2, OP_SLLI },
        { "shrli", format2, OP_SRLI },
        { "shrai", format2, OP_SRAI },
        { "imul", format4, OP_IMUL },
        { "umul", format4, OP_MUL },
        { "mulih", format2, OP_MULIH },
        { "muli", format2, OP_MULI },
        /*{ "mulhu",   formatR, OP_MULHU  },*/
        { "idiv", format4, OP_DIV },
        { "udiv", format4, OP_UDIV },
        { "idivi", format2, OP_DIVI },
        /*{ "divu",    formatR, OP_DIVU  },
  { "rem",     format4, OP_REM  },
  { "remu",    formatR, OP_REMU  },*/
        /* Load instructions (testing of relocations remains) */
        { "lb", format2L, OP_LB },
        { "lbd", format2L, OP_LBD },
        { "lh", format2L, OP_LH },
        { "lhd", format2L, OP_LHD },
        { "lw", format2L, OP_LW },
        { "lwd", format2L, OP_LWD },
        { "lbu", format2L, OP_LBU },
        { "lbud", format2L, OP_LBUD },
        { "lhu", format2L, OP_LHU },
        { "lhud", format2L, OP_LHUD },
        { "la", formatLA, OP_LA },
        { "li", formatLI, OP_LUI },
        { "lui", format1U, OP_LUI },
        { "auipc", format1U, OP_AUIPC },
        /* Store instructions */
        { "sb", format2S, OP_SB },
        { "sh", format2S, OP_SH },
        { "sw", format2S, OP_SW },
        { "sbd", format2S, OP_SBD },
        { "shd", format2S, OP_SHD },
        { "swd", format2S, OP_SWD },
        /* Block ops */
        { "copy", format3CSF, OP_COPY },
        { "copymd", format3CSF, OP_COPYMD },
        { "copydm", format3CSF, OP_COPYDM },
        { "copydd", format3CSF, OP_COPYDD },
        { "swap", format3CSF, OP_SWAP },
        { "swapmd", format3CSF, OP_SWAPMD },
        { "swapdm", format3CSF, OP_SWAPDM },
        { "swapdd", format3CSF, OP_SWAPDD },
        { "fill", format3CSF, OP_FILL },
        { "fillh", format3CSF, OP_FILLH },
        { "fillsq", format3CSF, OP_FILLSQ },
        { "fillw", format3CSF, OP_FILLW },
        { "thro", format2NOIM, OP_THRO },
        { "from", format2NOIM, OP_FROM },
        /* Stack ops */
        { "popb", format2NOIM, OP_POPB },
        { "poph", format2NOIM, OP_POPH },
        { "pop", format2NOIM, OP_POP },
        { "pushb", format2NOIM, OP_PUSHB },
        { "pushh", format2NOIM, OP_PUSHH },
        { "push", format2NOIM, OP_PUSH },

        { "save", format3, OP_SAVE },
        { "restore", format3, OP_RESTORE },
        { "exch", format2NOIM, OP_EXCH },
        /* System instructions */
        { "syscall", formatSCALL, OP_CLL }, //
        { "sysret", formatSRET, OP_MRET }, //
        { "gsreg", formatSREG, OP_GSR },
        { "ssreg", formatSREG, OP_SSR },
        { "trace", format4, OP_TRACE },
        { "cli", formatSRET, OP_CLI },
        { "sti", formatSRET, OP_STI },

        /* MMU */
        { "mmu.toggle", formatMMU, OP_MMUTOG },
        { "mmu.map", formatMMU, OP_MMUMAP },
        { "mmu.unmap", formatMMU, OP_MMUUNMAP },
        { "mmu.stat", formatMMU, OP_MMUSTAT },
        { "mmu.getframe", formatMMU, OP_MMUGETFRAME },
        { "mmu.setpt", formatMMU, OP_MMUSPT },
        { "mmu.update", formatMMU, OP_MMUUPDATE },
        { "mmu.switch", formatMMU, OP_MMUSW },
        { "mmu.getpt", formatMMU, OP_MMUGPT },
        { "umode.toggle", formatMMU, OP_UMODETOG }
};
