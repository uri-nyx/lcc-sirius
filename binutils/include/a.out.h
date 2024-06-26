/*
 * a.out.h -- structure of linkable object and executable files
 */


#ifndef _A_OUT_H_
#define _A_OUT_H_


#define EXEC_MAGIC	0314


#define METHOD_W32	0	/* write full 32 bit word with value */
#define METHOD_R15	1	/* write 15 bit relative to PC */
#define METHOD_RL12	2	/* write low 12 bit relative to PC*/
#define METHOD_RH20	3	/* write low 20 bit relative to PC */
#define METHOD_RS12	4	/* write low 12 bit relative to PC*/
#define METHOD_J20	5	/* write branch 20 bit relative to PC*/
#define METHOD_JALR12	6	/* write branch 20 bit relative to PC*/

#define SEGMENT_ABS	0	/* absolute values */
#define SEGMENT_CODE	1	/* code segment */
#define SEGMENT_DATA	2	/* initialized data segment */
#define SEGMENT_BSS	3	/* uninitialized data segment */

#define DBG_LINE 1
#define DBG_FUNCBEG 2
#define DBG_FUNCEND 3
#define DBG_FUNCRET 4
#define DBG_FUNCARGREG 5
#define DBG_FUNCARGSTACK 6
#define DBG_VARGLO 7
#define DBG_TYPEDEF 8
#define DBG_VARLOCSTACK 9
#define DBG_VARLOCREG 10

typedef struct {
  unsigned int magic;		/* must be EXEC_MAGIC */
  unsigned int csize;		/* size of code in bytes */
  unsigned int dsize;		/* size of initialized data in bytes */
  unsigned int bsize;		/* size of uninitialized data in bytes */
  unsigned int crsize;		/* size of code relocation info in bytes */
  unsigned int drsize;		/* size of data relocation info in bytes */
  unsigned int symsize;		/* size of symbol table in bytes */
  unsigned int strsize;		/* size of string space in bytes */
} ExecHeader;

typedef struct {
  unsigned int offset;		/* where to relocate */
  int method;			/* how to relocate */
  int value;			/* additive part of value */
  int base;			/* if MSB = 0: segment number */
				/* if MSB = 1: symbol table index */
} RelocRecord;

typedef struct {
  unsigned int name;		/* offset in string space */
  int type;			/* if MSB = 0: the symbol's segment */
				/* if MSB = 1: the symbol is undefined */
  int value;			/* if symbol defined: the symbol's value */
				/* if symbol not defined: meaningless */
  int debug;
  int debugtype;
  int debugvalue;
} SymbolRecord;


#endif /* _A_OUT_H_ */
