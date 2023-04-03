#include <stdio.h>
#include <stdint.h>

const char* logo = "########::'##::::'##:'##::::'##::'#######::\n##.... ##: ###::'###:. ##::'##::'##.... ##:\n##:::: ##: ####'####::. ##'##::: ##:::: ##:\n##:::: ##: ## ### ##:::. ###::::: #######::\n##:::: ##: ##. #: ##::: ## ##:::'##.... ##:\n##:::: ##: ##:.:: ##:: ##:. ##:: ##:::: ##:\n########:: ##:::: ##: ##:::. ##:. #######::\n........:::..:::::..::..:::::..:::.......:::\n";

typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;
typedef uint64_t  LONG;

static WORD instruction_counter;


/*
-- PROGRAM
  256 bytes
-- PROGRAM
-- STACK
  16 bytes
-- STACK
-- DATA
  239 bytes
-- DATA
-- SYSTEM
  512 bytes
-- SYSTEM
*/

// I am not going to lie, sections are a mess right now

struct s_Section {
  WORD start;
  BYTE startHighByte;
  BYTE startLowByte;

  WORD end;
  BYTE endHighByte;
  BYTE endLowByte;
};

struct s_Disk {
  struct s_Section PROGRAM_SECT; 
  struct s_Section STACK_SECT;   
  struct s_Section DATA_SECT;   
  struct s_Section SYSTEM_SECT; 
};

struct s_Disk DISK_FORMAT = {
  (struct s_Section){0x0000, 0x00, 0x00, 0x00FF, 0x00, 0xFF},
  (struct s_Section){0x0100, 0x01, 0x00, 0x010F, 0x01, 0x0F},
  (struct s_Section){0x0110, 0x01, 0x10, 0x01FF, 0x01, 0xFF},
  (struct s_Section){0x0200, 0x02, 0x00, 0x0400, 0x04, 0x00},
};

static WORD stack_pointer;
static BYTE register_a;
static BYTE register_b;
static BYTE register_c;
static BYTE register_d;

enum {
  ic = 0x9,
  a = 0xA,
  b = 0xB,
  c = 0xC,
  d = 0xD,
  e = 0xE,
  f = 0xF,

  mvi = 0x10,
  mvd = 0x11,
  mva = 0x12,

  add = 0x1A,
  sub = 0x1B,

  jmp = 0xC0,
  jnq = 0xC1,
  cmp = 0xCB,
  call = 0xC8,
  ret = 0xC9,
  proc = 0xCA,

  and = 0xD0,
  shl = 0xD1,
  shr = 0xD2,

  push = 0xE0,
  pop = 0xE1,
};

WORD makeword(BYTE high, BYTE low)
{
    return ((high << 8) | low);
}

static inline void show_memory(BYTE* mem, DWORD row_length, DWORD hideamount)
{
  for (DWORD i = 0; i < DISK_FORMAT.SYSTEM_SECT.end-1-hideamount; i += row_length)
  {
    for (DWORD j = 0; j < row_length; j++)
    {
      printf("%02X ", mem[i+j]);
    }
    printf("\n");
  }
  printf("HIDDEN BYTES: %d", hideamount);
}

static inline void show_registers(BYTE* mem)
{
  printf(
    "-----------------------------------------\na: %02X|b: %02X|c: %02X|d: %02X|ic: %04X|sp: %04X\n-----------------------------------------\n",
   register_a, register_b, register_c, register_d, instruction_counter, stack_pointer
  );
}

static inline BYTE* get_register_pointer(BYTE rgr)
{
  BYTE* p_rgr;

  switch (rgr)
  {
    case a:
      p_rgr = &register_a;
      break;
    case b:
      p_rgr = &register_b;
      break;
    case c:
      p_rgr = &register_c;
      break;
    case d:
      p_rgr = &register_d;
      break;
    default:
      p_rgr = 0;
  }
  return p_rgr;
}

static inline void push_to_stack(BYTE* val, BYTE* mem)
{
  if (stack_pointer > DISK_FORMAT.STACK_SECT.end)
  {
    printf("STACK OVERFLOW!\n");
    return;
  }
  mem[stack_pointer + DISK_FORMAT.STACK_SECT.start] = *val;
  stack_pointer++; 
}

static inline void pop_stack(BYTE* rgr, BYTE* mem)
{
  if (stack_pointer < 0)
  {
    printf("STACK UNDERFLOW!\n");
    return;
  }
  *rgr = mem[stack_pointer - 1 + DISK_FORMAT.STACK_SECT.start];
  mem[stack_pointer - 1 + DISK_FORMAT.STACK_SECT.start] = 0x00;
  stack_pointer--;
}

/*
Instruction modes

IMMEDIATE - load value into register (mvi a 0x9)
DELEGATE - load register into another register (mvd a b)
ACCUMULATE - load memory data into register (mva c 0x01 0x22)
PROSPECTIVE - load memory address into two registers (MVP c d 0x02 0x73) // Might be discarded
*/


static inline void instr_mvi(BYTE* mem, WORD* ic)
{
  BYTE rgr = mem[*ic+1];
  BYTE val = mem[*ic+2];

  printf("mvi %X %02X\n", rgr, val);

  BYTE* actual_rgr = get_register_pointer(rgr);
  *actual_rgr = val;

  *ic += 3;
}

static inline void instr_mvd(BYTE* mem, WORD* ic)
{
  BYTE rgr1 = mem[*ic+1];
  BYTE rgr2 = mem[*ic+2];

  printf("mvd %X %X\n", rgr1, rgr2);

  BYTE* actual_rgr1 = get_register_pointer(rgr1);
  BYTE* actual_rgr2 = get_register_pointer(rgr2);
  *actual_rgr1 = * actual_rgr2;

  *ic += 3;
}

static inline void instr_mva(BYTE* mem, WORD* ic)
{
  BYTE rgr = mem[*ic+1];
  WORD addr = makeword(mem[*ic+2], mem[*ic+3]);

  printf("mva %X %04X\n", rgr, addr);

  BYTE* actual_rgr = get_register_pointer(rgr);
  *actual_rgr = mem[addr];

  *ic += 4;
}

static inline void instr_add(BYTE* mem, WORD* ic)
{
  BYTE* rgr1 = get_register_pointer(mem[*ic+1]);
  BYTE* rgr2 = get_register_pointer(mem[*ic+2]);

  *rgr1 = *rgr1 + *rgr2;

  printf("add %X %X\n", mem[*ic+1], mem[*ic+2]);

  *ic += 3;
}

static inline void instr_sub(BYTE* mem, WORD* ic)
{
  BYTE* rgr1 = get_register_pointer(mem[*ic+1]);
  BYTE* rgr2 = get_register_pointer(mem[*ic+2]);

  if (*rgr2 > *rgr1)
    *rgr1 = 0;
  else
    *rgr1 = *rgr1 - *rgr2;

  printf("sub %X %X\n", mem[*ic+1], mem[*ic+2]);

  *ic += 3;
}

static inline void instr_jmp(BYTE* mem, WORD* ic)
{
  WORD addr = makeword(mem[*ic+1], mem[*ic+2]);

  printf("jmp %04X\n", addr);

  *ic = addr;
}

static inline void instr_jnq(BYTE* mem, WORD* ic)
{
  WORD addr = makeword(mem[*ic+1], mem[*ic+2]);

  printf("jnq %04X\n", addr);

  // Check for cmp opcode
  if (mem[(*ic) - 3] == cmp)
  {
    if (*get_register_pointer(mem[(*ic) - 2]) != *get_register_pointer(mem[(*ic) - 1]))
    {
      *ic = addr;
    }
    else 
    {
      *ic += 3;
    }
  }
  else 
  {
    *ic += 3;
  }
}

static inline void instr_cmp(BYTE* mem, WORD* ic)
{
  printf("cmp %02X %02X\n", mem[*ic+1], mem[*ic+2]);

  *ic += 3;
}

static inline void instr_call(BYTE* mem, WORD* ic)
{
  WORD addr = makeword(mem[*ic+1], mem[*ic+2]);

  printf("call %04X\n", addr);

  BYTE retAddrHigh = (*ic) >> 8;
  BYTE retAddrLow = ((BYTE)*ic);

  push_to_stack(&retAddrLow, mem);
  push_to_stack(&retAddrHigh, mem);

  *ic = addr;
}

static inline void instr_ret(BYTE* mem, WORD* ic)
{
  BYTE addrHigh;
  BYTE addrLow;
  pop_stack(&addrHigh, mem);
  pop_stack(&addrLow, mem);

  WORD addr = makeword(addrHigh, addrLow);

  printf("ret\n");

  *ic = addr + 3;
}

static inline void instr_proc(BYTE* mem, WORD* ic)
{
  printf("[%04X] proc:\n", *ic);

  *ic += 1;
}

static inline void instr_shl(BYTE* mem, WORD* ic)
{
  BYTE* rgr = get_register_pointer(mem[*ic+1]);
  BYTE count = mem[*ic+2];

  *rgr = *rgr << count;

  printf("shl %X %02X\n", mem[*ic+1], count);

  *ic += 3;
}

static inline void instr_shr(BYTE* mem, WORD* ic)
{
  BYTE* rgr = get_register_pointer(mem[*ic+1]);
  BYTE count = mem[*ic+2];

  *rgr = *rgr >> count;

  printf("shr %X %02X\n", mem[*ic+1], count);

  *ic += 3;
}

static inline void instr_push(BYTE* mem, WORD* ic)
{
  BYTE* rgr = get_register_pointer(mem[*ic+1]);

  push_to_stack(rgr, mem);

  printf("push %X\n", mem[*ic+1]);

  *ic += 2;
}

static inline void instr_pop(BYTE* mem, WORD* ic)
{
  BYTE* rgr = get_register_pointer(mem[*ic+1]);

  pop_stack(rgr, mem);

  printf("pop %X\n", mem[*ic+1]);

  *ic += 2;
}

static inline void read_instructions(BYTE* mem, WORD* ic)
{
  for (DWORD i = 0; i < DISK_FORMAT.PROGRAM_SECT.endLowByte; i++)
  {
    switch (mem[*ic])
    {
      case mvi:
      {
        instr_mvi(mem, ic);
      } break;
      case mvd:
      {
        instr_mvd(mem, ic);
      } break;
      case mva:
      {
        instr_mva(mem, ic);
      } break;
      case add:
      {
        instr_add(mem, ic);
      } break;
      case sub:
      {
        instr_sub(mem, ic);
      } break;
      case jmp:
      {
        instr_jmp(mem, ic);
      } break;
      case jnq:
      {
        instr_jnq(mem, ic);
      } break;
      case cmp:
      {
        instr_cmp(mem, ic);
      } break;
      case call:
      {
        instr_call(mem, ic);
      } break;
      case ret:
      {
        instr_ret(mem, ic);
      } break;
      case proc:
      {
        instr_proc(mem, ic);
      } break;
      case shl:
      {
        instr_shl(mem, ic);
      } break;
      case shr:
      {
        instr_shr(mem, ic);
      } break;
      case push:
      {
        instr_push(mem, ic);
      } break;
      case pop:
      {
        instr_pop(mem, ic);
      } break;
      case 0xFF:
      {
        break;
      }
    }
  }
}

int main() 
{
  printf("%s", logo);

  BYTE memory[1024] = {
    mvi, a, 0x05,
    mvi, b, 0x05,
    push, a,
    mvi, a, 0x88,
    pop, d,

    // end command. Not crucial, but handy since it puts less stress on the interpeter to not have to read all 1024 bytes!
    0xff, 
  };
  
  read_instructions(memory, &instruction_counter);

  show_registers(memory);
  show_memory(memory, 26, 512);

  return 0;
}