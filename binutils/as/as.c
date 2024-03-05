/*
 * as.c -- RISCV32 assembler
 */

#include <ctype.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../include/a.out.h"
#include "../include/dbg.h"
#include "dwarf.h"
#include "stab.h"

/**************************************************************/

#define NUM_REGS 32
#define AUX_REG 1

#define LINE_SIZE 600

#define TOK_EOL 0
#define TOK_LABEL 1
#define TOK_IDENT 2
#define TOK_STRING 3
#define TOK_NUMBER 4
#define TOK_IREGISTER 5

#define TOK_PLUS 6
#define TOK_MINUS 7
#define TOK_STAR 8
#define TOK_SLASH 9
#define TOK_PERCENT 10
#define TOK_LSHIFT 11
#define TOK_RSHIFT 12
#define TOK_LPAREN 13
#define TOK_RPAREN 14
#define TOK_COMMA 15
#define TOK_TILDE 16
#define TOK_AMPER 17
#define TOK_BAR 18
#define TOK_CARET 19
#define TOK_DOTRELADR 20
#define TOK_FREGISTER 21
#define TOK_ADD 22

#define STATUS_UNKNOWN 0 /* symbol is not yet defined */
#define STATUS_DEFINED 1 /* symbol is defined */
#define STATUS_GLOBREF 2 /* local entry refers to a global one */

#define GLOBAL_TABLE 0 /* global symbol table identifier */
#define LOCAL_TABLE 1  /* local symbol table identifier */

#define MSB ((unsigned int)1 << (sizeof (unsigned int) * 8 - 1))

/**************************************************************/

/* Arithmetic ops */
#define OP_ADD 0x60
#define OP_SUB 0x61
#define OP_SLL 0x6D
#define OP_SRL 0x6C
#define OP_SRA 0x6B
#define OP_AND 0x65
#define OP_OR 0x64
#define OP_XOR 0x66
#define OP_MUL 0x63
#define OP_MULIH 0x51
#define OP_MULI 0x50
/*#define OP_MULHU        0x*/
#define OP_DIV 0x62
#define OP_DIVI 0x52
/*
#define OP_DIVU         0x
#define OP_REM          0x
#define OP_REMU         0x
*/

#define OP_ADDI 0x53
#define OP_SLLI 0x5a
#define OP_NOP 0x60
#define OP_ANDI 0x56
#define OP_ORI 0x55
#define OP_XORI 0x57
#define OP_SLTI 0x5b
#define OP_SLTIU 0x5c
#define OP_SRLI 0x59
#define OP_SRAI 0x58

#define OP_NOT 0x67
#define OP_CTZ 0x68
#define OP_CLZ 0x69
#define OP_PCNT 0x6a
#define OP_ROR 0x6e
#define OP_ROL 0x6f

/* Block ops */
#define OP_COPY 0x10
#define OP_COPYMD 0x110
#define OP_COPYDM 0x210
#define OP_COPYDD 0x310
#define OP_SWAP 0x11
#define OP_SWAPMD 0x111
#define OP_SWAPDM 0x211
#define OP_SWAPDD 0x311
#define OP_FILL 0x12
#define OP_FILLH 0x112
#define OP_FILLSQ 0x212
#define OP_FILLW 0x312
#define OP_THRO 0x13
#define OP_FROM 0x14

/* Stack ops */
#define OP_POPB 0x15
#define OP_POPH 0x16
#define OP_POP 0x17
#define OP_PUSHB 0x18
#define OP_PUSHH 0x19
#define OP_PUSH 0x1a

#define OP_SAVE 0x1b
#define OP_RESTORE 0x1c
#define OP_EXCH 0x1d

/* Branches */
#define OP_BEQ 0x30
#define OP_BNE 0x31
#define OP_SLT 0x1e
#define OP_SLTU 0x1f
#define OP_BLT 0x32
#define OP_BGT 0x132
#define OP_BGE 0x33
#define OP_BLE 0x133
#define OP_BLTU 0x34
#define OP_BGTU 0x134
#define OP_BGEU 0x35
#define OP_BLEU 0x135

/* Loads */
#define OP_LB 0x42
#define OP_LH 0x46
#define OP_LW 0x4A /* The docs are wrong */
#define OP_LBU 0x43
#define OP_LHU 0x47
#define OP_LBD 0x44
#define OP_LBUD 0x45
#define OP_LHD 0x48
#define OP_LHUD 0x49
#define OP_LWD 0x4B /* The docs are wrong */

#define OP_LA 0x122
#define OP_LUI 0x21
#define OP_AUIPC 0x22

/* Stores */
#define OP_SB 0x70
#define OP_SH 0x72
#define OP_SW 0x74
#define OP_SBD 0x71
#define OP_SHD 0x73
#define OP_SWD 0x75

/* Jumps */
#define OP_JAL 0x20
#define OP_JALR 0x41
#define OP_CALL 0x00

/* System */
#define OP_CLL 0x02
#define OP_MRET 0x06
#define OP_GSR 0x03
#define OP_SSR 0x04
#define OP_TRACE 0x05
#define OP_CLI 0x106
#define OP_STI 0x206

/* MMU */
#define OP_MMUTOG 0x07
#define OP_MMUMAP 0x08
#define OP_MMUUNMAP 0x09
#define OP_MMUSTAT 0x0a
#define OP_MMUSPT 0x0b
#define OP_MMUUPDATE 0x0c
#define OP_MMUSW 0x0e
#define OP_MMUGPT 0x0f
#define OP_UMODETOG 0x0d

/**************************************************************/

int debugToken = 0;
int debugCode = 0;
int debugFixup = 0;

int debug = 0;
int rvc = 0;

char codeName[L_tmpnam];
char dataName[L_tmpnam];
char srcfileName[L_tmpnam];
char *outName = NULL;
char *inName = NULL;
char outpath[80];
char dbgfile[80];

FILE *codeFile = NULL;
FILE *dataFile = NULL;
FILE *outFile = NULL;
FILE *inFile = NULL;

char line[LINE_SIZE];
char debuglabel[LINE_SIZE];
char funcname[LINE_SIZE];
char varname[LINE_SIZE];
char currfuncnameend[LINE_SIZE];
char tmpstring[LINE_SIZE];
char *lineptr;
int lineno;
fpos_t lastline;

int token;
int tokenvalNumber;
char tokenvalString[LINE_SIZE];

int allowSyn = 1;
int currSeg = SEGMENT_CODE;
unsigned int segPtr[4] = { 0, 0, 0, 0 };
char *segName[4] = { "ABS", "CODE", "DATA", "BSS" };
char *methodName[6] = { "W32", "R12", "RL12", "RH20", "RS12", "J20" };

typedef struct fixup
{
  int segment;         /* in which segment */
  unsigned int offset; /* at which offset */
  int method;          /* what kind of coding method is to be used */
  int value;           /* known part of value */
  int base;            /* segment which this ref is relative to */
                       /* valid only when used for relocation */
  struct fixup *next;  /* next fixup */
} Fixup;

#define symbolmaxnamelen 256

typedef struct symbol
{
  char *name;             /* name of symbol */
  int status;             /* status of symbol */
  int segment;            /* the symbol's segment */
  int value;              /* the symbol's value */
  Fixup *fixups;          /* list of locations to fix */
  struct symbol *globref; /* set if this local refers to a global */
  struct symbol *left;    /* left son in binary search tree */
  struct symbol *right;   /* right son in binary search tree */
  int skip;               /* this symbol is not defined here nor is */
                          /* it used here: don't write to object file */
  int debug;
  int debugtype;
  int debugvalue;
} Symbol;

loc_t loc;
func_t func;
string_t funckey;
global_t glob;
typdef_t typdef;
typdefarr_t typdefarr;
funcvar_t funcvar;
root_t root;
m_serial_read_t in;
m_serial_write_t out;
m_serial_return_code_t ret;
FILE *f;

enum
{
  O32_LITTLE_ENDIAN = 0x03020100ul,
  O32_BIG_ENDIAN = 0x00010203ul,
  O32_PDP_ENDIAN = 0x01000302ul,      /* DEC PDP-11 (aka ENDIAN_LITTLE_WORD) */
  O32_HONEYWELL_ENDIAN = 0x02030001ul /* Honeywell 316 (aka ENDIAN_BIG_WORD) */
};

static const union
{
  unsigned char bytes[4];
  unsigned int value;
} o32_host_order = { { 0, 1, 2, 3 } };

#define O32_HOST_ORDER (o32_host_order.value)

unsigned int
toBE (unsigned int num)
{
  if (O32_HOST_ORDER == O32_LITTLE_ENDIAN)
    {
      return ((num & 0xff000000) >> 24) | ((num & 0x00ff0000) >> 8)
             | ((num & 0x0000ff00) << 8) | (num << 24);
    }
  else
    return num;
}

void
mlibinit ()
{
  loc_init (loc);
  funcvar_init (funcvar);
  func_init (func);
  string_init (funckey);
  global_init (glob);
  typdef_init (typdef);
  typdefarr_init (typdefarr);
  root_init (root);
}

void
mlibclear ()
{
  loc_clear (loc);
  funcvar_clear (funcvar);
  func_clear (func);
  string_clear (funckey);
  global_clear (glob);
  typdef_clear (typdef);
  typdefarr_clear (typdefarr);
  root_clear (root);
}

int
str2int (char *p, unsigned int *num)
{
  *num = 0;
  while (isdigit (*p))
    {
      if (*p > '9' || *p < '0')
        return -1;
      *num = *num * 10 + *p - '0';
      p++;
    }
  return 0;
}

int
insrange (int bits, int val)
{
  int msb = 1 << (bits - 1);
  int ll = -msb;
  return ((val <= (msb - 1) && val >= ll) ? 1 : 0);
}

int
inurange (int bits, int val)
{
  int msb = 1 << bits;
  int ll = 0;
  return ((val <= (msb - 1) && val >= ll) ? 1 : 0);
}

unsigned int
countbits (int n)
{
  unsigned int count = 0;
  while (n)
    {
      n &= (n - 1);
      count++;
    }
  return count;
}

void error (char *fmt, ...);

/**************************************************************/

void
error (char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  fprintf (stderr, "Error: ");
  vfprintf (stderr, fmt, ap);
  fprintf (stderr, "\n");
  va_end (ap);
  if (codeFile != NULL)
    {
      fclose (codeFile);
      codeFile = NULL;
    }
  if (dataFile != NULL)
    {
      fclose (dataFile);
      dataFile = NULL;
    }
  if (outFile != NULL)
    {
      fclose (outFile);
      outFile = NULL;
    }
  if (inFile != NULL)
    {
      fclose (inFile);
      inFile = NULL;
    }
  if (codeName != NULL)
    {
      unlink (codeName);
    }
  if (dataName != NULL)
    {
      unlink (dataName);
    }
  if (outName != NULL)
    {
      unlink (outName);
    }
  exit (1);
}

void *
allocateMemory (unsigned int size)
{
  void *p;

  p = malloc (size);
  if (p == NULL)
    {
      error ("out of memory");
    }
  return p;
}

void
freeMemory (void *p)
{
  free (p);
}

/**************************************************************/

int
getNextToken (void)
{
  char *p;
  int base;
  long long int digit;

  while (*lineptr == ' ' || *lineptr == '\t')
    {
      lineptr++;
    }
  if (*lineptr == '\n' || *lineptr == '\0' || *lineptr == ';'
      || *lineptr == '#' || (*lineptr == '/' && *(lineptr + 1) == '*'))
    {
      return TOK_EOL;
    }
  if (*lineptr == 'x')
    {
      lineptr++;
      if (!isdigit ((int)*lineptr))
        {
          lineptr--;
        }
      else
        {
          tokenvalNumber = 0;
          while (isdigit ((int)*lineptr))
            {
              digit = *lineptr++ - '0';
              tokenvalNumber *= 10;
              tokenvalNumber += digit;
            }
          if (tokenvalNumber < 0 || tokenvalNumber >= NUM_REGS)
            {
              error ("illegal register number %d in line %d", tokenvalNumber,
                     lineno);
            }
          return TOK_IREGISTER;
        }
    }
  if (*lineptr == '.' && *(lineptr + 1) == '+')
    {
      lineptr += 2;
      return TOK_DOTRELADR;
    }
  if (isalpha ((int)*lineptr) || *lineptr == '_' || *lineptr == '.')
    {
      p = tokenvalString;
      while (isalnum ((int)*lineptr) || *lineptr == '_' || *lineptr == '.')
        {
          *p++ = *lineptr++;
        }
      *p = '\0';
      if (*lineptr == ':')
        {
          lineptr++;
          return TOK_LABEL;
        }
      else
        {
          return TOK_IDENT;
        }
    }
  if (isdigit ((int)*lineptr))
    {
      base = 10;
      tokenvalNumber = 0;
      if (*lineptr == '0')
        {
          lineptr++;
          if (*lineptr == 'x' || *lineptr == 'X')
            {
              base = 16;
              lineptr++;
            }
          else if (isdigit ((int)*lineptr))
            {
              base = 8;
            }
          else
            {
              return TOK_NUMBER;
            }
        }
      while (isxdigit ((int)*lineptr))
        {
          digit = *lineptr++ - '0';
          if (digit >= 'A' - '0')
            {
              if (digit >= 'a' - '0')
                {
                  digit += '0' - 'a' + 10;
                }
              else
                {
                  digit += '0' - 'A' + 10;
                }
            }
          if (digit >= base)
            {
              error ("illegal digit value %d in line %d", digit, lineno);
            }
          tokenvalNumber *= base;
          tokenvalNumber += digit;
        }
      return TOK_NUMBER;
    }
  if (*lineptr == '\'')
    {
      lineptr++;
      if (!isprint ((int)*lineptr))
        {
          error ("cannot quote character 0x%02X in line %d", *lineptr, lineno);
        }
      tokenvalNumber = *lineptr;
      lineptr++;
      if (*lineptr != '\'')
        {
          error ("unbalanced quote in line %d", lineno);
        }
      lineptr++;
      return TOK_NUMBER;
    }
  if (*lineptr == '\"')
    {
      lineptr++;
      p = tokenvalString;
      while (1)
        {
          if (*lineptr == '\n' || *lineptr == '\0')
            {
              error ("unterminated string constant in line %d", lineno);
            }
          if (!isprint ((int)*lineptr))
            {
              error ("string contains illegal character 0x%02X in line %d",
                     *lineptr, lineno);
            }
          if (*lineptr == '\"')
            {
              break;
            }
          *p++ = *lineptr++;
        }
      lineptr++;
      *p = '\0';
      return TOK_STRING;
    }
  if (*lineptr == '+')
    {
      lineptr++;
      return TOK_PLUS;
    }
  if (*lineptr == '-')
    {
      lineptr++;
      return TOK_MINUS;
    }
  if (*lineptr == '*')
    {
      lineptr++;
      return TOK_STAR;
    }
  if (*lineptr == '/')
    {
      lineptr++;
      return TOK_SLASH;
    }
  if (*lineptr == '%')
    {
      lineptr++;
      return TOK_PERCENT;
    }
  if (*lineptr == '<' && *(lineptr + 1) == '<')
    {
      lineptr += 2;
      return TOK_LSHIFT;
    }
  if (*lineptr == '>' && *(lineptr + 1) == '>')
    {
      lineptr += 2;
      return TOK_RSHIFT;
    }
  if (*lineptr == '(')
    {
      lineptr++;
      return TOK_LPAREN;
    }
  if (*lineptr == ')')
    {
      lineptr++;
      return TOK_RPAREN;
    }
  if (*lineptr == ',')
    {
      lineptr++;
      return TOK_COMMA;
    }
  if (*lineptr == '~')
    {
      lineptr++;
      return TOK_TILDE;
    }
  if (*lineptr == '&')
    {
      lineptr++;
      return TOK_AMPER;
    }
  if (*lineptr == '|')
    {
      lineptr++;
      return TOK_BAR;
    }
  if (*lineptr == '^')
    {
      lineptr++;
      return TOK_CARET;
    }
  if (*lineptr == '@')
    {
      lineptr++;
      return TOK_ADD;
    }
  error ("illegal character 0x%02X in line %d", *lineptr, lineno);
  return 0;
}

void
showToken (void)
{
  printf ("DEBUG: ");
  switch (token)
    {
    case TOK_EOL:
      printf ("token = TOK_EOL\n");
      break;
    case TOK_LABEL:
      printf ("token = TOK_LABEL, value = %s\n", tokenvalString);
      break;
    case TOK_IDENT:
      printf ("token = TOK_IDENT, value = %s\n", tokenvalString);
      break;
    case TOK_STRING:
      printf ("token = TOK_STRING, value = %s\n", tokenvalString);
      break;
    case TOK_NUMBER:
      printf ("token = TOK_NUMBER, value = 0x%x\n", tokenvalNumber);
      break;
    case TOK_IREGISTER:
      printf ("token = TOK_IREGISTER, value = %d\n", tokenvalNumber);
      break;
    case TOK_FREGISTER:
      printf ("token = TOK_FREGISTER, value = %d\n", tokenvalNumber);
      break;
    case TOK_PLUS:
      printf ("token = TOK_PLUS\n");
      break;
    case TOK_MINUS:
      printf ("token = TOK_MINUS\n");
      break;
    case TOK_STAR:
      printf ("token = TOK_STAR\n");
      break;
    case TOK_SLASH:
      printf ("token = TOK_SLASH\n");
      break;
    case TOK_PERCENT:
      printf ("token = TOK_PERCENT\n");
      break;
    case TOK_LSHIFT:
      printf ("token = TOK_LSHIFT\n");
      break;
    case TOK_RSHIFT:
      printf ("token = TOK_RSHIFT\n");
      break;
    case TOK_LPAREN:
      printf ("token = TOK_LPAREN\n");
      break;
    case TOK_RPAREN:
      printf ("token = TOK_RPAREN\n");
      break;
    case TOK_COMMA:
      printf ("token = TOK_COMMA\n");
      break;
    case TOK_TILDE:
      printf ("token = TOK_TILDE\n");
      break;
    case TOK_AMPER:
      printf ("token = TOK_AMPER\n");
      break;
    case TOK_BAR:
      printf ("token = TOK_BAR\n");
      break;
    case TOK_CARET:
      printf ("token = TOK_CARET\n");
      break;
    default:
      error ("illegal token %d in showToken()", token);
    }
}

void
getToken (void)
{
  token = getNextToken ();
  if (debugToken)
    {
      showToken ();
    }
}

static char *tok2str[]
    = { "end-of-line", "label", "identifier", "string", "number",
        "register",    "+",     "-",          "*",      "/",
        "%",           "<<",    ">>",         "(",      ")",
        ",",           "~",     "&",          "|",      "^" };

void
expect (int expected)
{
  if (token != expected)
    {
      error ("'%s' expected, got '%s' in line %d", tok2str[expected],
             tok2str[token], lineno);
    }
}

/**************************************************************/

Fixup *fixupList = NULL;

Fixup *
newFixup (int segment, unsigned int offset, int method, int value)
{
  Fixup *f;

  f = allocateMemory (sizeof (Fixup));
  f->segment = segment;
  f->offset = offset;
  f->method = method;
  f->value = value;
  f->base = 0;
  f->next = NULL;
  return f;
}

void
addFixup (Symbol *s, int segment, unsigned int offset, int method, int value)
{
  Fixup *f;

  if (debugFixup)
    {
      printf ("DEBUG: fixup (s:%s, o:%08X, m:%s, v:%08X) added to '%s'\n",
              segName[segment], offset, methodName[method], value, s->name);
    }
  f = newFixup (segment, offset, method, value);
  f->next = s->fixups;
  s->fixups = f;
}

/**************************************************************/

Symbol *globalTable = NULL;
Symbol *localTable = NULL;

Symbol *
deref (Symbol *s)
{
  if (s->status == STATUS_GLOBREF)
    {
      return s->globref;
    }
  else
    {
      return s;
    }
}

Symbol *
newSymbol (char *name, int debug)
{
  Symbol *p;

  p = allocateMemory (sizeof (Symbol));
  if (debug)
    p->name = allocateMemory (strlen (name) + symbolmaxnamelen);
  else
    p->name = allocateMemory (strlen (name) + 1);
  strcpy (p->name, name);
  p->status = STATUS_UNKNOWN;
  p->segment = 0;
  p->value = 0;
  p->fixups = NULL;
  p->globref = NULL;
  p->left = NULL;
  p->right = NULL;
  p->debug = debug;
  p->debugtype = 0;
  p->debugvalue = 0;
  return p;
}

Symbol *
lookupEnter (char *name, int whichTable, int debug)
{
  Symbol *p, *q, *r;
  int cmp;

  if (whichTable == GLOBAL_TABLE)
    {
      p = globalTable;
    }
  else
    {
      p = localTable;
    }
  if (p == NULL)
    {
      r = newSymbol (name, debug);
      if (whichTable == GLOBAL_TABLE)
        {
          globalTable = r;
        }
      else
        {
          localTable = r;
        }
      return r;
    }
  while (1)
    {
      q = p;
      cmp = strcmp (name, q->name);
      if (cmp == 0)
        {
          return q;
        }
      if (cmp < 0)
        {
          p = q->left;
        }
      else
        {
          p = q->right;
        }
      if (p == NULL)
        {
          r = newSymbol (name, debug);
          if (cmp < 0)
            {
              q->left = r;
            }
          else
            {
              q->right = r;
            }
          return r;
        }
    }
}

static void
linkSymbol (Symbol *s)
{
  Fixup *f;
  Symbol *global;

  if (s->status == STATUS_UNKNOWN)
    {
      global = lookupEnter (s->name, GLOBAL_TABLE, 0);
      while (s->fixups != NULL)
        {
          f = s->fixups;
          s->fixups = f->next;
          f->next = global->fixups;
          global->fixups = f;
        }
      s->status = STATUS_GLOBREF;
      s->globref = global;
    }
  if (s->status == STATUS_GLOBREF)
    {
      if (s->fixups != NULL)
        {
          error ("local fixups detected with global symbol '%s'", s->name);
        }
    }
  else
    {
      if (debugFixup)
        {
          printf ("DEBUG: link '%s' (s:%s, v:%08X)\n", s->name,
                  segName[s->segment], s->value);
        }
      while (s->fixups != NULL)
        {
          /* get next fixup record */
          f = s->fixups;
          s->fixups = f->next;
          /* add the symbol's value to the value in the record */
          /* and remember the symbol's segment */
          if (debugFixup)
            {
              printf ("       (s:%s, o:%08X, m:%s, v:%08X --> %08X, b:%s)\n",
                      segName[f->segment], f->offset, methodName[f->method],
                      f->value, f->value + s->value, segName[s->segment]);
            }
          f->value += s->value;
          f->base = s->segment;
          /* transfer the record to the fixup list */
          f->next = fixupList;
          fixupList = f;
        }
    }
}

static void
linkTree (Symbol *s)
{
  if (s == NULL)
    {
      return;
    }
  linkTree (s->left);
  linkSymbol (s);
  linkTree (s->right);
  freeMemory (s->name);
  freeMemory (s);
}

void
linkLocals (void)
{
  linkTree (localTable);
  localTable = NULL;
  fseek (codeFile, 0, SEEK_END);
  fseek (dataFile, 0, SEEK_END);
}

/**************************************************************/

void
emitByte (unsigned int byte)
{
  byte &= 0x000000FF;
  if (debugCode)
    {
      printf ("DEBUG: byte @ segment = %s, offset = %08X", segName[currSeg],
              segPtr[currSeg]);
      printf (", value = %02X\n", byte);
    }
  switch (currSeg)
    {
    case SEGMENT_ABS:
      error ("illegal segment in emitByte()");
      break;
    case SEGMENT_CODE:
      fputc (byte, codeFile);
      break;
    case SEGMENT_DATA:
      fputc (byte, dataFile);
      break;
    case SEGMENT_BSS:
      break;
    }
  segPtr[currSeg] += 1;
}

void
emitHalf (unsigned int half)
{
  half &= 0x0000FFFF;
  if (debugCode)
    {
      printf ("DEBUG: half @ segment = %s, offset = %08X", segName[currSeg],
              segPtr[currSeg]);
      printf (", value = %02X%02X\n", (half >> 8) & 0xFF, half & 0xFF);
    }
  switch (currSeg)
    {
    case SEGMENT_ABS:
      error ("illegal segment in emitHalf()");
      break;
    case SEGMENT_CODE:
      fputc ((half >> 8) & 0xFF, codeFile);
      fputc (half & 0xFF, codeFile);
      break;
    case SEGMENT_DATA:
      fputc ((half >> 8) & 0xFF, dataFile);
      fputc (half & 0xFF, dataFile);
      break;
    case SEGMENT_BSS:
      break;
    }
  segPtr[currSeg] += 2;
}

void
emitWord (unsigned int word)
{
  if (debugCode)
    {
      printf ("DEBUG: word @ segment = %s, offset = %08X", segName[currSeg],
              segPtr[currSeg]);
      printf (", value = %02X%02X%02X%02X\n", (word >> 24) & 0xFF,
              (word >> 16) & 0xFF, (word >> 8) & 0xFF, word & 0xFF);
    }
  switch (currSeg)
    {
    case SEGMENT_ABS:
      error ("illegal segment in emitWord()");
      break;
    case SEGMENT_CODE:
      fputc ((word >> 24) & 0xFF, codeFile);
      fputc ((word >> 16) & 0xFF, codeFile);
      fputc ((word >> 8) & 0xFF, codeFile);
      fputc (word & 0xFF, codeFile);
      break;
    case SEGMENT_DATA:
      fputc ((word >> 24) & 0xFF, dataFile);
      fputc ((word >> 16) & 0xFF, dataFile);
      fputc ((word >> 8) & 0xFF, dataFile);
      fputc (word & 0xFF, dataFile);
      break;
    case SEGMENT_BSS:
      break;
    }
  segPtr[currSeg] += 4;
}

/**************************************************************/

typedef struct
{
  int con;
  long long int lcon;
  Symbol *sym;
} Value;

Value parseExpression (void);

Value
parsePrimaryExpression (void)
{
  Value v;
  Symbol *s;

  if (token == TOK_NUMBER)
    {
      v.con = tokenvalNumber & 0xFFFFFFFF;
      v.lcon = tokenvalNumber;
      v.sym = NULL;
      getToken ();
    }
  else if (token == TOK_IDENT)
    {
      s = deref (lookupEnter (tokenvalString, LOCAL_TABLE, 0));
      if (s->status == STATUS_DEFINED && s->segment == SEGMENT_ABS)
        {
          v.con = s->value;
          v.sym = NULL;
        }
      else
        {
          v.con = 0;
          v.sym = s;
        }
      getToken ();
    }
  else if (token == TOK_LPAREN)
    {
      getToken ();
      v = parseExpression ();
      expect (TOK_RPAREN);
      getToken ();
    }
  else
    {
      error ("illegal primary expression, line %d", lineno);
    }
  return v;
}

Value
parseUnaryExpression (void)
{
  Value v;

  if (token == TOK_PLUS)
    {
      getToken ();
      v = parseUnaryExpression ();
    }
  else if (token == TOK_MINUS)
    {
      getToken ();
      v = parseUnaryExpression ();
      if (v.sym != NULL)
        {
          error ("cannot negate symbol '%s' in line %d", v.sym->name, lineno);
        }
      v.con = -v.con;
    }
  else if (token == TOK_TILDE)
    {
      getToken ();
      v = parseUnaryExpression ();
      if (v.sym != NULL)
        {
          error ("cannot complement symbol '%s' in line %d", v.sym->name,
                 lineno);
        }
      v.con = ~v.con;
    }
  else
    {
      v = parsePrimaryExpression ();
    }
  return v;
}

Value
parseMultiplicativeExpression (void)
{
  Value v1, v2;

  v1 = parseUnaryExpression ();
  while (token == TOK_STAR || token == TOK_SLASH || token == TOK_PERCENT)
    {
      if (token == TOK_STAR)
        {
          getToken ();
          v2 = parseUnaryExpression ();
          if (v1.sym != NULL || v2.sym != NULL)
            {
              error ("multiplication of symbols not supported, line %d",
                     lineno);
            }
          v1.con *= v2.con;
        }
      else if (token == TOK_SLASH)
        {
          getToken ();
          v2 = parseUnaryExpression ();
          if (v1.sym != NULL || v2.sym != NULL)
            {
              error ("division of symbols not supported, line %d", lineno);
            }
          if (v2.con == 0)
            {
              error ("division by zero, line %d", lineno);
            }
          v1.con /= v2.con;
        }
      else if (token == TOK_PERCENT)
        {
          getToken ();
          v2 = parseUnaryExpression ();
          if (v1.sym != NULL || v2.sym != NULL)
            {
              error ("division of symbols not supported, line %d", lineno);
            }
          if (v2.con == 0)
            {
              error ("division by zero, line %d", lineno);
            }
          v1.con %= v2.con;
        }
    }
  return v1;
}

Value
parseAdditiveExpression (void)
{
  Value v1, v2;

  v1 = parseMultiplicativeExpression ();
  while (token == TOK_PLUS || token == TOK_MINUS)
    {
      if (token == TOK_PLUS)
        {
          getToken ();
          v2 = parseMultiplicativeExpression ();
          if (v1.sym != NULL && v2.sym != NULL)
            {
              error ("addition of symbols not supported, line %d", lineno);
            }
          if (v2.sym != NULL)
            {
              v1.sym = v2.sym;
            }
          v1.con += v2.con;
        }
      else if (token == TOK_MINUS)
        {
          getToken ();
          v2 = parseMultiplicativeExpression ();
          if (v2.sym != NULL)
            {
              printf ("subtraction of symbols not supported, line %d\r\n",
                      lineno);
              v2.con = 0;
            }
          v1.con -= v2.con;
        }
    }
  return v1;
}

Value
parseShiftExpression (void)
{
  Value v1, v2;

  v1 = parseAdditiveExpression ();
  while (token == TOK_LSHIFT || token == TOK_RSHIFT)
    {
      if (token == TOK_LSHIFT)
        {
          getToken ();
          v2 = parseAdditiveExpression ();
          if (v1.sym != NULL || v2.sym != NULL)
            {
              error ("shifting of symbols not supported, line %d", lineno);
            }
          v1.con <<= v2.con;
        }
      else if (token == TOK_RSHIFT)
        {
          getToken ();
          v2 = parseAdditiveExpression ();
          if (v1.sym != NULL || v2.sym != NULL)
            {
              error ("shifting of symbols not supported, line %d", lineno);
            }
          v1.con >>= v2.con;
        }
    }
  return v1;
}

Value
parseAndExpression (void)
{
  Value v1, v2;

  v1 = parseShiftExpression ();
  while (token == TOK_AMPER)
    {
      getToken ();
      v2 = parseShiftExpression ();
      if (v2.sym != NULL)
        {
          error ("bitwise 'and' of symbols not supported, line %d", lineno);
        }
      v1.con &= v2.con;
    }
  return v1;
}

Value
parseExclusiveOrExpression (void)
{
  Value v1, v2;

  v1 = parseAndExpression ();
  while (token == TOK_CARET)
    {
      getToken ();
      v2 = parseAndExpression ();
      if (v2.sym != NULL)
        {
          error ("bitwise 'xor' of symbols not supported, line %d", lineno);
        }
      v1.con ^= v2.con;
    }
  return v1;
}

Value
parseInclusiveOrExpression (void)
{
  Value v1, v2;

  v1 = parseExclusiveOrExpression ();
  while (token == TOK_BAR)
    {
      getToken ();
      v2 = parseExclusiveOrExpression ();
      if (v2.sym != NULL)
        {
          error ("bitwise 'or' of symbols not supported, line %d", lineno);
        }
      v1.con |= v2.con;
    }
  return v1;
}

Value
parseExpression (void)
{
  Value v;

  v = parseInclusiveOrExpression ();
  return v;
}

/**************************************************************/

void
dotSyn (unsigned int code)
{
  allowSyn = 1;
}

void
dotNosyn (unsigned int code)
{
  allowSyn = 0;
}

void
dotCode (unsigned int code)
{
  currSeg = SEGMENT_CODE;
}

void
dotData (unsigned int code)
{
  currSeg = SEGMENT_DATA;
  while (token != TOK_EOL)
    getToken ();
}
int
newsection ()
{
  if ((strncmp (tokenvalString, ".section", strlen (".section")) == 0)
      || (strcmp (tokenvalString, ".text") == 0)
      || (strcmp (tokenvalString, ".bss") == 0)
      || (strcmp (tokenvalString, ".data") == 0))
    {
      return 1;
    }
  else
    return 0;
}

void
asm_dbgabrev ()
{
  fgetpos (inFile, &lastline);
  while (fgets (line, LINE_SIZE, inFile) != NULL)
    {
      lineno++;
      lineptr = line;
      getToken ();
      if (newsection ())
        {
          fsetpos (inFile, &lastline);
          lineno--;
          break;
        }
      fgetpos (inFile, &lastline);
    }
}

void
parse_type ()
{
  int no;
  str2int (&tokenvalString[1], &no);
  fgets (line, LINE_SIZE, inFile);
  lineno++;
  fgets (line, LINE_SIZE, inFile);
  lineno++;
  lineptr = line;
  getToken ();
  getToken ();
  string_set_str (typdef->name, tokenvalString);
  typdef->number = no;
  typdefarr_push_back (typdefarr, typdef);
}

void
parse_symbol ()
{
  int no;
  Symbol *label;
  fgets (line, LINE_SIZE, inFile);
  lineno++;
  lineptr = line;
  getToken ();
  getToken ();
  switch (tokenvalNumber)
    {
    case DW_TAG_subprogram:
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      string_set_str (funckey, tokenvalString);
      string_set_str (func->filename, srcfileName);
      sprintf (debuglabel, "%s beg", tokenvalString);
      label = deref (lookupEnter (debuglabel, GLOBAL_TABLE, 1));
      label->status = STATUS_DEFINED;
      label->debug = DBG_FUNCBEG;
      strcpy (currfuncnameend, tokenvalString);
      strcat (currfuncnameend, "_end");
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      str2int (&tokenvalString[1], &no);
      func->rettype = no;
      break;
    case DW_TAG_formal_parameter:
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      string_set_str (funcvar->name, tokenvalString);
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      str2int (&tokenvalString[1], &no);
      funcvar->type = no;
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      if (tokenvalNumber == 1)
        {
          fgets (line, LINE_SIZE, inFile);
          lineno++;
          lineptr = line;
          getToken ();
          getToken ();
          funcvar->pos = tokenvalNumber - DW_OP_reg0;
          funcvararr_push_back (func->regargs, funcvar);
        }
      else
        {
          fgets (line, LINE_SIZE, inFile);
          fgets (line, LINE_SIZE, inFile);
          lineno += 2;
          lineptr = line;
          getToken ();
          getToken ();
          funcvar->pos = tokenvalNumber;
          funcvararr_push_back (func->stackargs, funcvar);
        }
      break;
    case DW_TAG_variable:
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      string_set_str (funcvar->name, tokenvalString);
      strcpy (varname, tokenvalString);
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      str2int (&tokenvalString[1], &no);
      funcvar->type = no;
      fgets (line, LINE_SIZE, inFile);
      lineno++;
      lineptr = line;
      getToken ();
      getToken ();
      // global
      if (tokenvalNumber == 1)
        {
          sprintf (debuglabel, "%s ", varname);
          label = deref (lookupEnter (debuglabel, GLOBAL_TABLE, 1));
          deref (lookupEnter (varname, LOCAL_TABLE, 0));
          label->status = STATUS_DEFINED;
          label->debug = DBG_VARGLO;
          label->debugtype = no;
          sprintf (label->name + strlen (label->name), "%d", no);
        }
      else
        {
          fgets (line, LINE_SIZE, inFile);
          lineno++;
          lineptr = line;
          getToken ();
          getToken ();
          // reglocal
          if (tokenvalNumber == 1)
            {
              fgets (line, LINE_SIZE, inFile);
              lineno++;
              lineptr = line;
              getToken ();
              getToken ();
              funcvar->pos = tokenvalNumber - DW_OP_reg0;
              funcvararr_push_back (func->reglocals, funcvar);
              // stacklocal
            }
          else
            {
              fgets (line, LINE_SIZE, inFile);
              fgets (line, LINE_SIZE, inFile);
              lineno += 2;
              lineptr = line;
              getToken ();
              getToken ();
              funcvar->pos = tokenvalNumber;
              funcvararr_push_back (func->stacklocals, funcvar);
            }
        }
    }
}

void
asm_dbginfo ()
{
  fgetpos (inFile, &lastline);
  while (fgets (line, LINE_SIZE, inFile) != NULL)
    {
      lineno++;
      lineptr = line;
      getToken ();
      if ((token == TOK_LABEL) && (tokenvalString[0] == 'T'))
        parse_type ();
      if ((token == TOK_LABEL) && (tokenvalString[0] == 'E'))
        parse_symbol ();
      if (newsection ())
        {
          fsetpos (inFile, &lastline);
          lineno--;
          break;
        }
      fgetpos (inFile, &lastline);
    }
}

void
asm_dbgpubnames ()
{
  fgetpos (inFile, &lastline);
  while (fgets (line, LINE_SIZE, inFile) != NULL)
    {
      lineno++;
      lineptr = line;
      getToken ();
      if (newsection ())
        {
          fsetpos (inFile, &lastline);
          lineno--;
          break;
        }
      fgetpos (inFile, &lastline);
    }
}

void
dotSection (unsigned int code)
{
  // expect(TOK_STRING);
  if (strncmp (tokenvalString, ".debug_abbrev", strlen (".debug_abbrev")) == 0)
    asm_dbgabrev ();
  else if (strncmp (tokenvalString, ".debug_info", strlen (".debug_info"))
           == 0)
    asm_dbginfo ();
  else if (strncmp (tokenvalString, ".debug_pubnames",
                    strlen (".debug_pubnames"))
           == 0)
    asm_dbgpubnames ();
  else if (strncmp (tokenvalString, ".rodata", strlen (".rodata")) == 0)
    dotData (0);
  else
    error ("invalid section '%s'", tokenvalString);
  while (token != TOK_EOL)
    getToken ();
}

void
dotBss (unsigned int code)
{
  currSeg = SEGMENT_BSS;
}

void
Ignore (unsigned int code)
{
  while (token != TOK_EOL)
    getToken ();
}

void
dotStabs (unsigned int code)
{
  Value v;
  unsigned int i, typenum, semicol;
  char *ptr, *def, *name;
  char ch;
  Symbol *label, *local;
  expect (TOK_STRING);
  strcpy (tmpstring, tokenvalString);
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  semicol = strcspn (tmpstring, ":");
  name = strndup (tmpstring, semicol);
  def = strchr (tmpstring, '=');
  if (def)
    {
      str2int (tmpstring + semicol + 2, &typenum);
      string_set_str (typdef->name, name);
      typdef->number = typenum;
      string_set_str (typdef->desc, def);
      typdefarr_push_back (typdefarr, typdef);
    }
  else
    {
    }
  expect (TOK_COMMA);
  getToken ();
  parseExpression ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  expect (TOK_COMMA);
  getToken ();
  if (token == TOK_IDENT)
    {
      getToken ();
    }
  else
    {
      v = parseExpression ();
    }
  if (!def)
    {
      ptr = strchr (tmpstring, ':');
      ch = *(ptr + 1);
      switch (ch)
        {
        case 'G':
          sprintf (debuglabel, "%s ", name);
          label = deref (lookupEnter (debuglabel, GLOBAL_TABLE, 1));
          local = deref (lookupEnter (name, LOCAL_TABLE, 0));
          label->status = STATUS_DEFINED;
          label->segment = currSeg;
          label->value = local->value;
          label->debug = DBG_VARGLO;
          ptr += 2;
          label->debugvalue = 0;
          str2int (ptr, &i);
          sprintf (label->name + strlen (label->name), "%d", i);
          label->debugtype = i;
          break;
        case 'f':
          return;
          break;
        case 'F':
          sprintf (debuglabel, "%s beg", name);
          strcpy (funcname, name);
          label = deref (lookupEnter (debuglabel, GLOBAL_TABLE, 1));
          label->status = STATUS_DEFINED;
          label->segment = currSeg;
          label->value = segPtr[currSeg];
          label->debug = DBG_FUNCBEG;
          ptr += 2;
          str2int (ptr, &i);
          label->debugtype = i;
          string_set_str (funckey, name);
          func->rettype = i;
          string_set_str (func->filename, srcfileName);
          return;
          break;
        case 'E':
          funcdict_set_at (root->functions, funckey, func);
          return;
          break;
        case 'P':
          ptr += 2;
          str2int (ptr, &i);
          string_set_str (funcvar->name, name);
          funcvar->type = i;
          funcvar->pos = v.con;
          funcvararr_push_back (func->regargs, funcvar);
          break;
        case 'r':
          ptr += 2;
          str2int (ptr, &i);
          string_set_str (funcvar->name, name);
          funcvar->type = i;
          funcvar->pos = v.con;
          funcvararr_push_back (func->reglocals, funcvar);
          break;
        case 'p':
          ptr += 2;
          str2int (ptr, &i);
          string_set_str (funcvar->name, name);
          funcvar->type = i;
          funcvar->pos = v.con;
          funcvararr_push_back (func->stackargs, funcvar);
          break;
        default:
          ptr++;
          str2int (ptr, &i);
          string_set_str (funcvar->name, name);
          funcvar->type = i;
          funcvar->pos = v.con;
          funcvararr_push_back (func->stacklocals, funcvar);
          break;
        }
    }
}

void
dotStabn (unsigned int code)
{
  parseExpression ();
  expect (TOK_COMMA);
  getToken ();
  parseExpression ();
  expect (TOK_COMMA);
  getToken ();
  parseExpression ();
  expect (TOK_COMMA);
  getToken ();
  if (token == TOK_IDENT)
    {
      getToken ();
    }
  else
    {
      parseExpression ();
    }
}

void
dotFile (unsigned int code)
{
  Value v;
  v = parseExpression ();
  if (v.sym != NULL)
    {
      error ("absolute expression expected in line %d", lineno);
    }
  expect (TOK_STRING);
  strcpy (srcfileName, tokenvalString);
  getToken ();
}

void
dotLoc (unsigned int code)
{
  Value v, w;
  Symbol *label;
  v = parseExpression ();
  if (v.sym != NULL)
    {
      error ("absolute expression expected in line %d", lineno);
    }
  v = parseExpression ();
  if (v.sym != NULL)
    {
      error ("absolute expression expected in line %d", lineno);
    }
  if (token != TOK_EOL)
    {
      w = parseExpression ();
      if (w.sym != NULL)
        {
          error ("absolute expression expected in line %d", lineno);
        }
    }
  if (debug)
    {
      sprintf (debuglabel, "\"%s\":%d", srcfileName, v.con);
      label = deref (lookupEnter (debuglabel, GLOBAL_TABLE, 0));
      label->status = STATUS_DEFINED;
      label->segment = currSeg;
      label->value = segPtr[currSeg];
      label->debug = DBG_LINE;
    }
}

void
dotExport (unsigned int code)
{
  Symbol *global;
  Symbol *local;
  Fixup *f;

  while (1)
    {
      expect (TOK_IDENT);
      global = lookupEnter (tokenvalString, GLOBAL_TABLE, 0);
      if (global->status != STATUS_UNKNOWN)
        {
          error ("exported symbol '%s' multiply defined in line %d",
                 global->name, lineno);
        }
      local = lookupEnter (tokenvalString, LOCAL_TABLE, 0);
      if (local->status == STATUS_GLOBREF)
        {
          error ("exported symbol '%s' multiply exported in line %d",
                 local->name, lineno);
        }
      global->status = local->status;
      global->segment = local->segment;
      global->value = local->value;
      while (local->fixups != NULL)
        {
          f = local->fixups;
          local->fixups = f->next;
          f->next = global->fixups;
          global->fixups = f;
        }
      local->status = STATUS_GLOBREF;
      local->globref = global;
      getToken ();
      if (token != TOK_COMMA)
        {
          break;
        }
      getToken ();
    }
}

void
dotSet (unsigned int code)
{
  Symbol *global;
  Symbol *globalnew;

  expect (TOK_IDENT);
  globalnew = lookupEnter (tokenvalString, GLOBAL_TABLE, 0);
  if (globalnew->status == STATUS_DEFINED)
    {
      error ("'%s' already not defined, line %d", globalnew->name, lineno);
    }
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IDENT);
  global = lookupEnter (tokenvalString, GLOBAL_TABLE, 0);
  if (global->status != STATUS_DEFINED)
    {
      error (".set '%s' not defined, line %d", global->name, lineno);
    }
  globalnew->segment = global->segment;
  globalnew->value = global->value;
  getToken ();
}

void
dotImport (unsigned int code)
{
  Symbol *global;
  Symbol *local;
  Fixup *f;

  while (1)
    {
      expect (TOK_IDENT);
      global = lookupEnter (tokenvalString, GLOBAL_TABLE, 0);
      local = lookupEnter (tokenvalString, LOCAL_TABLE, 0);
      if (local->status != STATUS_UNKNOWN)
        {
          error ("imported symbol '%s' multiply defined in line %d",
                 local->name, lineno);
        }
      while (local->fixups != NULL)
        {
          f = local->fixups;
          local->fixups = f->next;
          f->next = global->fixups;
          global->fixups = f;
        }
      local->status = STATUS_GLOBREF;
      local->globref = global;
      getToken ();
      if (token != TOK_COMMA)
        {
          break;
        }
      getToken ();
    }
}

int
countBits (unsigned int x)
{
  int n;

  n = 0;
  while (x != 0)
    {
      x &= x - 1;
      n++;
    }
  return n;
}

void
dotAlign (unsigned int code)
{
  Value v;
  unsigned int padding_to_emit;

  v = parseExpression ();
  if (v.sym != NULL)
    {
      error ("absolute expression expected in line %d", lineno);
    }

  if (v.con != 0 && (v.con & (v.con - 1)) == 0) {
    padding_to_emit = ((segPtr[currSeg] + (v.con - 1)) & ~(v.con - 1)) - segPtr[currSeg];
    while (padding_to_emit--)
    {
      emitByte(0);
    }
    
  } else {
    error("epxression in alignment must be a power of 2, line %d", lineno);
  }

  while (token != TOK_EOL)
    getToken ();
}

void
dotSpace (unsigned int code)
{
  Value v;
  int i;

  v = parseExpression ();
  if (v.sym != NULL)
    {
      error ("absolute expression expected in line %d", lineno);
    }
  for (i = 0; i < v.con; i++)
    {
      emitByte (0);
    }
}

void
dotLocate (unsigned int code)
{
  Value v;

  v = parseExpression ();
  if (v.sym != NULL)
    {
      error ("absolute expression expected in line %d", lineno);
    }
  while (segPtr[currSeg] != v.con)
    {
      emitByte (0);
    }
}

void
dotByte (unsigned int code)
{
  Value v;
  char *p;

  while (1)
    {
      if (token == TOK_STRING)
        {
          p = tokenvalString;
          while (*p != '\0')
            {
              emitByte (*p);
              p++;
            }
          getToken ();
        }
      else
        {
          v = parseExpression ();
          if (v.sym != NULL)
            {
              error ("absolute expression expected in line %d", lineno);
            }
          emitByte (v.con);
        }
      if (token != TOK_COMMA)
        {
          break;
        }
      getToken ();
    }
}

void
dotHalf (unsigned int code)
{
  Value v;

  while (1)
    {
      v = parseExpression ();
      if (v.sym != NULL)
        {
          error ("absolute expression expected in line %d", lineno);
        }
      emitHalf (v.con);
      if (token != TOK_COMMA)
        {
          break;
        }
      getToken ();
    }
}

void
dotWord (unsigned int code)
{
  Value v;

  while (1)
    {
      v = parseExpression ();
      if (v.sym == NULL)
        {
          emitWord (v.con);
        }
      else
        {
          addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_W32, v.con);
          emitWord (0);
        }
      if (token != TOK_COMMA)
        {
          break;
        }
      getToken ();
    }
}

void
dotDword (unsigned int code)
{
  Value v;

  while (1)
    {
      v = parseExpression ();
      emitWord (v.lcon & 0xFFFFFFFF);
      emitWord ((v.lcon >> 32) & 0xFFFFFFFF);
      if (token != TOK_COMMA)
        {
          break;
        }
      getToken ();
    }
}

void
dotSet2 (unsigned int code)
{
  Value v;
  Symbol *symbol;

  expect (TOK_IDENT);
  symbol = deref (lookupEnter (tokenvalString, LOCAL_TABLE, 0));
  if (symbol->status != STATUS_UNKNOWN)
    {
      error ("symbol '%s' multiply defined in line %d", symbol->name, lineno);
    }
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  if (v.sym == NULL)
    {
      symbol->status = STATUS_DEFINED;
      symbol->segment = SEGMENT_ABS;
      symbol->value = v.con;
    }
  else
    {
      error ("illegal type of symbol '%s' in expression, line %d", v.sym->name,
             lineno);
    }
}

void
format3 (unsigned int code)
{
  int dst, src1, src2;

  /* opcode with three register operands */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken ();
  emitWord ((code & 0x7F) << 25 | dst << 20 | src1 << 15 | src2 << 10);
}

void
format4 (unsigned int code)
{
  int dst, src1, src2, src3;

  /* opcode with three register operands */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src3 = tokenvalNumber;
  getToken ();
  emitWord ((code & 0x7F) << 25 | dst << 20 | src1 << 15 | src2 << 10
            | src3 << 5);
}

void
format3CSF (unsigned int code)
{
  int dst, src1, src2;
  int direction;

  /* opcode with three register operands */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken ();
  direction = (code & 0xf00) >> 8;
  emitWord ((code & 0x7F) << 25 | dst << 20 | src1 << 15 | src2 << 10
            | direction & 0x3);
}

void
format2B (unsigned int code)
{
  /* branch instructions with a 17 bit signed relative address */
  int src1, src2, tmp;
  Value v;
  unsigned int immed;

  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src2 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  if (token == TOK_DOTRELADR)
    getToken ();
  v = parseExpression ();
  if (v.sym == NULL)
    {
      immed = v.con / 2;
    }
  else
    {
      addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_R15, v.con);
      immed = 0;
    }
  if (code & 0x100)
    {
      tmp = src2;
      src2 = src1;
      src1 = tmp;
      code &= 0xFF;
    }
  emitWord ((code & 0xFF) << 25 | src1 << 20 | src2 << 15
            | (immed >> 2) & 0x7FFF);
}

void
format2S (unsigned int code)
{
  /* Store instructions with a 15 bit signed operand */

  int src1, src2, vcon;
  Value v;
  unsigned int immed;
  int rvccond;

  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  if (v.sym == NULL)
    {
      immed = v.con;
      vcon = v.con;
      expect (TOK_LPAREN);
      getToken ();
      expect (TOK_IREGISTER);
      src2 = tokenvalNumber;
      getToken ();
      expect (TOK_RPAREN);
      getToken ();
    }
  else
    {
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      src2 = tokenvalNumber;
      getToken ();
      addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
      immed = 0;
      emitWord(OP_AUIPC << 25 | src2 << 20 | immed);
      immed = v.con;
      addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_RS12, v.con);
      immed = 0;
    }
  emitWord (code << 25 | src1 << 20 | src2 << 15 | ((immed) & 0x7FFF));
}

void
format2 (unsigned int code)
{
  int dst, src1;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  immed = v.con;
  immed &= 0x7FFF;

  emitWord ((code << 25) | dst << 20 | src1 << 15 | immed);
}

void
format2NOIM (unsigned int code)
{
  int dst, src1;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  immed = 0;

  emitWord ((code << 25) | dst << 20 | src1 << 15 | immed);
}

void
formatMv (unsigned int code)
{
  int dst, src1;

  /* opcode with two registers */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();

  emitWord (code << 25 | dst << 20 | src1 << 15);
}

void
format2L (unsigned int code)
{
  int dst, src1, vcon, rvccond;
  Value v;
  unsigned int immed;

  /* load opcode with two register operands and immediate */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  if (v.sym == NULL)
    {
      immed = v.con;
      vcon = v.con;
      expect (TOK_LPAREN);
      getToken ();
      expect (TOK_IREGISTER);
      src1 = tokenvalNumber;
      getToken ();
      expect (TOK_RPAREN);
      getToken ();
    }
  else
    {
      addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
      immed = 0;
      emitWord (OP_AUIPC << 25 | dst << 20 | immed);
      immed = v.con;
      addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_RL12, v.con);
      immed = 0;
      src1 = dst;
    }

  emitWord (code << 25 | dst << 20 | src1 << 15 | ((immed) & 0x7FFF));
}

void
formatLA (unsigned int code)
{
  int dst, src1, vcon;
  Value v;
  unsigned int immed;

  /* load opcode with two register operands and immediate */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
  immed = 0;
  emitWord (OP_AUIPC << 25 | dst << 20);
  immed = v.con;
  addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_RL12, v.con);
  immed = 0;
  src1 = dst;
  emitWord (OP_ADDI << 25 | dst << 20 | src1 << 15 | immed);
}

void
formatCALL (unsigned int code)
{
  int dst, src1, vcon;
  Value v;
  unsigned int immed;

  dst = 1;

  /* load opcode with two register operands and immediate */
  v = parseExpression ();
  addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_RH20, v.con);
  immed = 0;
  emitWord (OP_AUIPC << 25 | dst << 20);
  immed = v.con;
  addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_JALR12, v.con);
  immed = 0;
  src1 = dst;
  emitWord (OP_JALR << 25 | dst << 20 | src1 << 15 | immed);
}

void
format1JAL (unsigned int code)
{
  int dst;
  Value v;

  /* jal opcode with one register operands and label */
  // expect(TOK_IREGISTER);
  if (token == TOK_IREGISTER)
    {
      dst = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
    }
  else
    dst = 1;
  v = parseExpression ();
  addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_J20, v.con);
  emitWord ((code << 25) | (dst << 20));
}

void
format1J (unsigned int code)
{
  int dst;
  Value v;

  /* jal opcode with label */
  dst = 0;
  v = parseExpression ();
  addFixup (v.sym, currSeg, segPtr[currSeg], METHOD_J20, v.con);
  emitWord ((code << 25));
}

void
formatJR (unsigned int code)
{
  int dst, src1;
  Value v;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  if (v.sym)
    error ("jalr instruction does not support labels (yet)");
  immed = v.con;
  immed &= 0x3FFFF;
  expect (TOK_LPAREN);
  getToken ();
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  getToken ();
  expect (TOK_RPAREN);
  getToken ();

  emitWord ((code << 25) | dst << 20 | src1 << 15 | immed >> 2);
}

void
formatJ0R (unsigned int code)
{
  int dst, src1;
  unsigned int immed;

  /* opcode with one register operands and immediate */
  dst = 0;
  expect (TOK_IREGISTER);
  src1 = tokenvalNumber;
  immed = 0;
  getToken ();
  emitWord (code << 25 | src1 << 15);
}

void
formatRET (unsigned int code)
{
  int dst, src1;
  unsigned int immed;

  /* opcode with two register operands and immediate */
  dst = 0;
  getToken ();
  src1 = 1;
  immed = 0;

  emitWord (code << 25 | src1 << 15);
}

void
formatLI (unsigned int code)
{
  int dst, src;
  Value v;
  unsigned int immed;

  /* opcode with one register operands and immediate */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  src = 0;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  immed = v.con;
  if (insrange (15, v.con))
    {
      emitWord (OP_ADDI << 25 | dst << 20 | immed & 0x7fff);
      return;
    }
  else
    {
      emitWord (OP_LUI << 25 | dst << 20 | (immed >> 12) & 0xfffff);
      src = dst;
    }
  if (dst != 0)
    {
      emitWord (OP_ADDI << 25 | dst << 20 | src << 15 | immed & 0xfff);
    }
}

void
format1U (unsigned int code)
{
  int dst;
  Value v;
  unsigned int immed;

  /* opcode with one register operands and immediate */
  expect (TOK_IREGISTER);
  dst = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  immed = v.con;
  emitWord (code << 25 | dst << 20 | immed & 0xFFFFF);
}

void
formatSCALL (unsigned int code)
{
  int src;
  Value v;
  unsigned int immed;

  /* Syscall format */
  expect (TOK_IREGISTER);
  src = tokenvalNumber;
  getToken ();
  expect (TOK_COMMA);
  getToken ();
  v = parseExpression ();
  immed = v.con;
  immed &= 0xFF;
  emitWord (code << 25 | src << 20 | immed);
}

void
formatSRET (unsigned int code)
{
  int cli_sti = (code & 0xf00) >> 8;
  emitWord (code << 25 | cli_sti << 20);
}

void
formatSREG (unsigned int code)
{
  int reg;

  expect (TOK_IREGISTER);
  reg = tokenvalNumber;
  getToken ();

  emitWord (code << 25 | reg << 20);
}

void
formatMMU (unsigned int code)
{
  int r1, r2, r3, r4;
  int w, x, pt, dirty, present, clone, immed;
  Value v;
  switch (code)
    {
    case OP_MMUTOG:
    case OP_MMUGPT:
      expect (TOK_IREGISTER);
      r1 = tokenvalNumber;
      getToken ();
      emitWord (code << 25 | r1 << 20);
      break;
    case OP_MMUMAP:
      expect (TOK_IREGISTER);
      r1 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r2 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r3 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r4 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression ();
      w = v.con & 0x1;
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression ();
      x = v.con & 0x1;
      //getToken ();
      emitWord (code << 25 | r1 << 20 | r2 << 15 | r3 << 10 | r4 << 5 | w << 1 | x);
      break;
    case OP_MMUUNMAP:
    case OP_UMODETOG:
      expect (TOK_IREGISTER);
      r1 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r2 = tokenvalNumber;
      getToken ();
      emitWord(code << 25 | r1 << 20 | r2 << 15);
      break;
    case OP_MMUSTAT:
      expect (TOK_IREGISTER);
      r1 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r2 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r3 = tokenvalNumber;
      getToken ();
      emitWord (code << 25 | r1 << 20 | r2 << 15 | r3 << 10 );
      break;
    case OP_MMUSPT:
      expect (TOK_IREGISTER);
      r1 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r2 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression();
      immed = v.con & 0xfff;
      //getToken();
      emitWord (code << 25 | r1 << 20 | r2 << 15 | immed );
      break;
    case OP_MMUUPDATE:
      expect (TOK_IREGISTER);
      r1 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r2 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r3 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression ();
      pt = v.con & 0xf;
      //getToken ();
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression ();
      dirty = v.con & 0x1;
      //getToken ();
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression ();
      present = v.con & 0x1;
      //getToken ();
      emitWord (code << 25 | r1 << 20 | r2 << 15 | r3 << 10 | pt << 2 | dirty << 1 | present);
      break;
    case OP_MMUSW:
      expect (TOK_IREGISTER);
      r1 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      expect (TOK_IREGISTER);
      r2 = tokenvalNumber;
      getToken ();
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression ();
      pt = v.con & 0xf;
      //getToken ();
      expect (TOK_COMMA);
      getToken ();
      v = parseExpression ();
      clone = v.con & 0xf;
      //getToken ();
      emitWord (code << 25 | r1 << 20 | r2 << 15 | clone << 4 | pt );
      break;
    default:
      error ("No such MMU opcode!");
    }
}

typedef struct instr
{
  char *name;
  void (*func) (unsigned int code);
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
  { "imul", format4, OP_MUL },
  { "umul", format4, OP_MUL },
  { "mulih", format2, OP_MULIH },
  { "muli", format2, OP_MULI },
  /*{ "mulhu",   formatR, OP_MULHU  },*/
  { "idiv", format4, OP_DIV },
  { "udiv", format4, OP_DIV },
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
  { "sysret", formatSRET, OP_MRET },  //
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
  { "mmu.setpt", formatMMU, OP_MMUSPT },
  { "mmu.update", formatMMU, OP_MMUUPDATE },
  { "mmu.switch", formatMMU, OP_MMUSW },
  { "mmu.getpt", formatMMU, OP_MMUGPT },
  { "umode.toggle", formatMMU, OP_UMODETOG }
};

typedef struct dbginstr
{
  char *name;
  void (*func) (unsigned int code);
} DbgInstr;

DbgInstr DbgInstrTable[] = { { ".uleb128", Ignore } };

static int
cmpInstr (const void *instr1, const void *instr2)
{
  return strcmp (((Instr *)instr1)->name, ((Instr *)instr2)->name);
}

void
sortInstrTable (void)
{
  qsort (instrTable, sizeof (instrTable) / sizeof (instrTable[0]),
         sizeof (instrTable[0]), cmpInstr);
}

Instr *
lookupInstr (char *name)
{
  int lo, hi, tst;
  int res;

  lo = 0;
  hi = sizeof (instrTable) / sizeof (instrTable[0]) - 1;
  while (lo <= hi)
    {
      tst = (lo + hi) / 2;
      res = strcmp (instrTable[tst].name, name);
      if (res == 0)
        {
          return &instrTable[tst];
        }
      if (res < 0)
        {
          lo = tst + 1;
        }
      else
        {
          hi = tst - 1;
        }
    }
  return NULL;
}

/**************************************************************/

void
roundupSegments (void)
{
  int rounding = (rvc) ? 1 : 3;
  while (segPtr[SEGMENT_CODE] & rounding)
    {
      fputc (0, codeFile);
      segPtr[SEGMENT_CODE] += 1;
    }
  while (segPtr[SEGMENT_DATA] & rounding)
    {
      fputc (0, dataFile);
      segPtr[SEGMENT_DATA] += 1;
    }
  while (segPtr[SEGMENT_BSS] & rounding)
    {
      segPtr[SEGMENT_BSS] += 1;
    }
}

void
asmModule (void)
{
  Symbol *label;
  Instr *instr;

  allowSyn = 1;
  currSeg = SEGMENT_CODE;
  lineno = 0;
  while (fgets (line, LINE_SIZE, inFile) != NULL)
    {
      lineno++;
      lineptr = line;
      getToken ();
      while (token == TOK_LABEL)
        {
          if (debug && (strcmp (currfuncnameend, tokenvalString) == 0))
            funcdict_set_at (root->functions, funckey, func);
          label = deref (lookupEnter (tokenvalString, LOCAL_TABLE, 0));
          if (label->status != STATUS_UNKNOWN)
            {
              error ("label '%s' multiply defined in line %d", label->name,
                     lineno);
            }
          label->status = STATUS_DEFINED;
          label->segment = currSeg;
          label->value = segPtr[currSeg];
          getToken ();
        }
      if (token == TOK_IDENT)
        {
          instr = lookupInstr (tokenvalString);
          if (instr == NULL)
            {
              error ("unknown instruction '%s' in line %d", tokenvalString,
                     lineno);
            }
          getToken ();
          (*instr->func) (instr->code);
        }
      if (token != TOK_EOL)
        {
          error ("garbage in line %d", lineno);
        }
    }
  roundupSegments ();
}

/**************************************************************/

static ExecHeader execHeader;
static int numSymbols;
static int crelSize;
static int drelSize;
static int symtblSize;
static int stringSize;

static void
walkTree (Symbol *s, void (*fp) (Symbol *sp))
{
  if (s == NULL)
    {
      return;
    }
  walkTree (s->left, fp);
  (*fp) (s);
  walkTree (s->right, fp);
}

void
writeDummyHeader (void)
{
  fwrite (&execHeader, sizeof (ExecHeader), 1, outFile);
}

void
writeObjectBE (const void *object, size_t size, FILE *stream)
{
  int i;
  char *serialized = (char *)object;
  for (i = 0; i < size; i++)
    {
      fputc (serialized[i], stream);
    }
}

void
writeRealHeader (void)
{
  rewind (outFile);
  execHeader.magic = toBE (EXEC_MAGIC);
  execHeader.csize = toBE (segPtr[SEGMENT_CODE]);
  execHeader.dsize = toBE (segPtr[SEGMENT_DATA]);
  execHeader.bsize = toBE (segPtr[SEGMENT_BSS]);
  execHeader.crsize = toBE (crelSize);
  execHeader.drsize = toBE (drelSize);
  execHeader.symsize = toBE (symtblSize);
  execHeader.strsize = toBE (stringSize);
  writeObjectBE (&execHeader, sizeof (ExecHeader), outFile);
}

void
writeCode (void)
{
  int data;

  rewind (codeFile);
  while (1)
    {
      data = fgetc (codeFile);
      if (data == EOF)
        {
          break;
        }
      fputc (data, outFile);
    }
}

void
writeData (void)
{
  int data;

  rewind (dataFile);
  while (1)
    {
      data = fgetc (dataFile);
      if (data == EOF)
        {
          break;
        }
      fputc (data, outFile);
    }
}

void
transferFixupsForSymbol (Symbol *s)
{
  Fixup *f;

  if (s->status != STATUS_UNKNOWN && s->status != STATUS_DEFINED)
    {
      /* this should never happen */
      error ("global symbol is neither unknown nor defined");
    }
  if (s->status == STATUS_UNKNOWN && s->fixups == NULL)
    {
      /* this symbol is neither defined here nor referenced here: skip */
      s->skip = 1;
      return;
    }
  s->skip = 0;
  while (s->fixups != NULL)
    {
      /* get next fixup record */
      f = s->fixups;
      s->fixups = f->next;
      /* use the 'base' component to store the current symbol number */
      f->base = MSB | numSymbols;
      /* transfer the record to the fixup list */
      f->next = fixupList;
      fixupList = f;
    }
  numSymbols++;
}

void
transferFixups (void)
{
  numSymbols = 0;
  walkTree (globalTable, transferFixupsForSymbol);
}

void
writeCodeRelocs (void)
{
  Fixup *f;
  RelocRecord relRec;

  crelSize = 0;
  f = fixupList;
  while (f != NULL)
    {
      if (f->segment != SEGMENT_CODE && f->segment != SEGMENT_DATA)
        {
          /* this should never happan */
          error ("fixup found in a segment other than code or data");
        }
      if (f->segment == SEGMENT_CODE)
        {
          relRec.offset = toBE (f->offset);
          relRec.method = toBE (f->method);
          relRec.value = toBE (f->value);
          relRec.base = toBE (f->base);
          fwrite (&relRec, sizeof (RelocRecord), 1, outFile);
          crelSize += sizeof (RelocRecord);
        }
      f = f->next;
    }
}

void
writeDataRelocs (void)
{
  Fixup *f;
  RelocRecord relRec;

  drelSize = 0;
  f = fixupList;
  while (f != NULL)
    {
      if (f->segment != SEGMENT_CODE && f->segment != SEGMENT_DATA)
        {
          /* this should never happan */
          error ("fixup found in a segment other than code or data");
        }
      if (f->segment == SEGMENT_DATA)
        {
          relRec.offset = toBE(f->offset);
          relRec.method = toBE(f->method);
          relRec.value = toBE(f->value);
          relRec.base = toBE(f->base);
          fwrite (&relRec, sizeof (RelocRecord), 1, outFile);
          drelSize += sizeof (RelocRecord);
        }
      f = f->next;
    }
}

void
writeSymbol (Symbol *s)
{
  SymbolRecord symRec;
  if (s->skip)
    {
      /* this symbol is neither defined here nor referenced here: skip */
      return;
    }
  symRec.name = toBE(stringSize);
  if (s->status == STATUS_UNKNOWN)
    {
      symRec.type = toBE(MSB);
      symRec.value = toBE(0);
      symRec.debug = toBE(0);
      symRec.debugtype = toBE(0);
      symRec.debugvalue = toBE(0);
    }
  else
    {
      symRec.type = toBE(s->segment);
      symRec.value = toBE(s->value);
      symRec.debug = toBE(s->debug);
      symRec.debugtype = toBE(s->debugtype);
      symRec.debugvalue = toBE(s->debugvalue);
    }
  fwrite (&symRec, sizeof (SymbolRecord), 1, outFile);
  symtblSize += sizeof (SymbolRecord);
  stringSize += strlen (s->name) + 1;
}

void
writeSymbols (void)
{
  symtblSize = 0;
  stringSize = 0;
  walkTree (globalTable, writeSymbol);
}

void
writeString (Symbol *s)
{
  if (s->skip)
    {
      /* this symbol is neither defined here nor referenced here: skip */
      return;
    }
  fputs (s->name, outFile);
  fputc ('\0', outFile);
}

void
writeStrings (void)
{
  walkTree (globalTable, writeString);
}

/**************************************************************/

void
usage (char *myself)
{
  fprintf (stderr, "Usage: %s\n", myself);
  fprintf (stderr, "         [-o objfile]     set object file name\n");
  fprintf (stderr, "         file             source file name\n");
  fprintf (stderr, "         [files...]       additional source files\n");
  exit (1);
}

int
main (int argc, char *argv[])
{
  int i;
  char *argp;
  char *tmp;
  sortInstrTable ();
  tmpnam (codeName);
  tmpnam (dataName);
  outName = "a.out";
  for (i = 1; i < argc; i++)
    {
      argp = argv[i];
      if (*argp != '-')
        {
          break;
        }
      argp++;
      switch (*argp)
        {
        case 'o':
          if (i == argc - 1)
            {
              usage (argv[0]);
            }
          outName = argv[++i];
          break;
        case 'g':
          if (i == argc - 1)
            {
              usage (argv[0]);
            }
          debug = 1;
          break;
        case 'c':
          if (i == argc - 1)
            {
              usage (argv[0]);
            }
          rvc = 1;
          break;
        default:
          usage (argv[0]);
        }
    }
  if (i == argc)
    {
      usage (argv[0]);
    }
  codeFile = fopen (codeName, "w+b");
  if (codeFile == NULL)
    {
      error ("cannot create temporary code file '%s'", codeName);
    }
  dataFile = fopen (dataName, "w+b");
  if (dataFile == NULL)
    {
      error ("cannot create temporary data file '%s'", dataName);
    }
  outFile = fopen (outName, "wb");
  if (outFile == NULL)
    {
      error ("cannot open output file '%s'", outName);
    }
  tmp = dirname (strdup (outName));
  strcpy (outpath, tmp);
  strcat (outpath, "/");

  if (debug)
    {
      tmp = strdup (outName);
      tmp = basename (tmp);
      *strchr (tmp, '.') = '\0';
      strcat (tmp, ".dbg");
      strcpy (dbgfile, outpath);
      strcat (dbgfile, tmp);
      mlibinit ();
      f = m_core_fopen (dbgfile, "wt");
      if (!f)
        abort ();
      m_serial_json_write_init (out, f);
    }
  do
    {
      inName = argv[i];
      if (*inName == '-')
        {
          usage (argv[0]);
        }
      inFile = fopen (inName, "rt");
      if (inFile == NULL)
        {
          error ("cannot open input file '%s'", inName);
        }
      fprintf (stderr, "Assembling module '%s'...\n", inName);
      string_set_str (func->filename, inName);
      asmModule ();
      if (inFile != NULL)
        {
          fclose (inFile);
          inFile = NULL;
        }
      linkLocals ();
      if (debug)
        typdefdict_set_at (root->typdefs, func->filename, typdefarr);
    }
  while (++i < argc);
  writeDummyHeader ();
  writeCode ();
  writeData ();
  transferFixups ();
  writeCodeRelocs ();
  writeDataRelocs ();
  writeSymbols ();
  writeStrings ();
  writeRealHeader ();
  if (codeFile != NULL)
    {
      fclose (codeFile);
      codeFile = NULL;
    }
  if (dataFile != NULL)
    {
      fclose (dataFile);
      dataFile = NULL;
    }
  if (outFile != NULL)
    {
      fclose (outFile);
      outFile = NULL;
    }
  if (codeName != NULL)
    {
      unlink (codeName);
    }
  if (dataName != NULL)
    {
      unlink (dataName);
    }
  if (debug)
    {
      ret = root_out_serial (out, root);
      assert (ret == M_SERIAL_OK_DONE);
      m_serial_json_write_clear (out);
      mlibclear ();
      fclose (f);
    }
  return 0;
}
