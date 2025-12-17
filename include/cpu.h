#ifndef HEADERGAURD_CPU_HEADER
#define HEADERGAURD_CPU_HEADER
#include <stdint.h>
#include "memory.h"
#include "emulator_config.h"
#include "bus.h"
#include "cpu_vars.h"

typedef uint8_t byte;
typedef void (*InstructionHandler)(void);
void init_instruction_handler_lut();
void build_decode_table();
extern uint32_t opcode;
struct Instruction
{
    byte aaa;
    byte bbb;
    byte cc;
    byte opcode;
    byte xx;
    byte y;
};
extern volatile bool cpu_debug_print;
// CPU internal registers
extern byte A;
extern byte X;
extern byte Y;
extern uint16_t PC;
extern byte SP; // Stack pointer - Stack is in RAM
// Cpu Flags
extern byte C; // carry
extern byte Z; // zero
extern byte I; // interrupt
extern byte D; // decimal
extern byte B; // break
extern byte O; // overflow //iN DOCUMENTATION IT'S V BUT O is more intuitive
extern byte N; // negative

void cpu_init();
void cpu_reset();

byte inline pack_flags()
{
    byte to_return = 0;
    to_return |= N << 7;
    to_return |= O << 6;
    to_return |= 1 << 5;
    to_return |= B << 4;
    to_return |= D << 3;
    to_return |= I << 2;
    to_return |= Z << 1;
    to_return |= C;

    return to_return;
};
void inline unpack_flags(byte flags)
{
    N = ((1 << 7) & flags) >> 7;
    O = ((1 << 6) & flags) >> 6;
    B = 0; // break flag shouldnt change when loded with PLP
    D = ((1 << 3) & flags) >> 3;
    I = ((1 << 2) & flags) >> 2;
    Z = ((1 << 1) & flags) >> 1;
    C = 1 & flags;
    return;
}
CPU_VARS pack_vars();
// byte read_pc();
inline byte read(uint16_t address) { return cpu_read(address); }
inline void write(uint16_t address, byte data) { cpu_write(address, data); }
byte inline read_pc()
{
    byte val = read(PC++);
    // PC++;
    return val;
}
uint16_t read_address_from_pc();
uint16_t read_address(byte offset);

// extern CPU_STATE state;
void fetch_instruction();
void cpu_clock();
void cpu_execute();
uint16_t read_abs_address(uint16_t offset);
void push(byte x);
void push_address(uint16_t address);
byte pop();
uint16_t pop_address();

int estimate_cycles();
int estimate_cycles_group_sb1();
int estimate_cycles_group_sb2();
bool estimate_page_cross_g1();
int estimate_cycles_group_1();
int estimate_cycles_group_2();
int estimate_cycles_group_3();
bool estimate_page_cross_g23();

// First group of instructions
void branch();
uint16_t compute_addr_mode_g1();
void run_instruction_group1(uint16_t address);
void ORA(uint16_t address);
void AND(uint16_t address);
void EOR(uint16_t address);
void ADC(uint16_t address);
void STA(uint16_t address);
void LDA(uint16_t address);
void CMP(uint16_t address);
void SBC(uint16_t address);

// Second group of instructions
bool compute_addr_mode_g23(uint16_t &address_to_return);
void run_instruction_group2(uint16_t address, bool accumulator);
void ASL_acc();
void ASL(uint16_t address);
void ROL_acc();
void ROL(uint16_t address);
void LSR_acc();
void LSR(uint16_t address);
void ROR_acc();
void ROR(uint16_t address);
void ASL_b(uint16_t address, bool accumulator);
void ROL_b(uint16_t address, bool accumulator);
void LSR_b(uint16_t address, bool accumulator);
void ROR_b(uint16_t address, bool accumulator);
void STX(uint16_t address);
void LDX(uint16_t address);
void DECC(uint16_t address); // dec is defined as 10 somewhere?
void INC(uint16_t address);

// Third group of instructions
void run_instruction_group3(uint16_t addresss);
void JMP_abs(uint16_t jump_address);
void JMP_indirect(uint16_t jump_address);
void BITT(uint16_t address);
void STY(uint16_t address);
void CPY(uint16_t address);
void CPX(uint16_t address);
void LDY(uint16_t address);

// SINGLE BYTE INSTRUCTIONS
void run_instruction_group_sb1();
void PHP();
void PLP();
void PHA();
void PLA();
void DEY();
void TAY();
void INY();
void INX();
void CLC();
void SEC();
void CLI();
void SEI();
void TYA();
void CLV();
void CLD();
void SED();
void BRK();

void run_instruction_group_sb2();
void TXA();
void TXS();
void TAX();
void TSX();
void DEX();
void NOPP();

// INTERRUPTS
void RTS();
void RTI();
void JSR_abs(uint16_t address);

// BRANCHES
void OP_BPL();
void OP_BMI();
void OP_BVC();
void OP_BVS();
void OP_BCC();
void OP_BCS();
void OP_BNE();
void OP_BEQ();

const uint16_t NMI_vector = 0xFFFA;
const uint16_t RESET_VECTOR = 0xFFFC;
const uint16_t IRQ_vector = 0xFFFE;
const uint16_t BRK_vector = 0xFFFE;

extern bool pending_nmi;
extern bool pending_irq;
void trigger_irq();
void trigger_nmi();

inline void enqueue_nmi()
{
    pending_nmi = true;
}

// addressing modes
uint16_t addr_IMM(void);
uint16_t addr_zero_page_x_p(void);
uint16_t addr_zero_page_x(void);
uint16_t addr_zero_page_y(void);
uint16_t addr_zero_page(void);
uint16_t addr_abs(void);
uint16_t addr_pzero_page_y(void);
uint16_t addr_abs_y(void);
uint16_t addr_abs_x(void);

#endif