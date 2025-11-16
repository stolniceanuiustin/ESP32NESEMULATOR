#ifndef HEADERGAURD_CPU_HEADER
#define HEADERGAURD_CPU_HEADER
#include <stdint.h>
#include "memory.h"
#include "emulator_config.h"
#include "bus.h"
#include "cpu_vars.h"

typedef uint8_t byte;

typedef enum
{
    FETCH_INSTRUCTION,
    WAIT_FOR_N_CYCLES,
    EXECUTE
} CPU_STATE;

struct Mem
{
    int x;
};

struct Instruction
{
    byte aaa;
    byte bbb;
    byte cc;
    byte opcode;
    byte xx;
    byte y;
};

extern struct Instruction inst;
extern byte A;
extern byte X;
extern byte Y;
extern uint16_t PC;
extern byte SP; // Stack pointer
extern uint64_t cycles;
extern int64_t estimated_cycles;
extern uint64_t elapsed_cycles;


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


byte ram_at(uint16_t address);
byte pack_flags();
void unpack_flags(byte flags);
CPU_VARS pack_vars();
byte read_pc();
inline byte read(uint16_t address) { return cpu_read(address); }
inline void write(uint16_t address, byte data) { cpu_write(address, data); }

uint16_t read_address_from_pc();
uint16_t read_address(byte offset);

extern CPU_STATE state;
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
inline void set_ZN(byte value)
{
    Z = (value == 0) ? 1 : 0;
    N = (value & 0x80) ? 1 : 0; //Checks bit 7 if it's set or not
}
uint16_t compute_addr_mode_g1(bool &page_cross);
void run_instruction_group1(uint16_t address, bool page_cross);
void ORA(uint16_t address, bool page_cross);
void AND(uint16_t address, bool page_cross);
void EOR(uint16_t address, bool page_cross);
void ADC(uint16_t address, bool page_cross);
void STA(uint16_t address);
void LDA(uint16_t address, bool page_cross);
void CMP(uint16_t address, bool page_cross);
void SBC(uint16_t address, bool page_cross);

// Second group of instructions
bool compute_addr_mode_g23(bool &page_cross, uint16_t &address_to_return);
void run_instruction_group2(uint16_t address, bool page_cross, bool accumulator);
void ASL(uint16_t address, bool accumulator);
void ROL(uint16_t address, bool accumulator);
void LSR(uint16_t address, bool accumulator);
void ROR(uint16_t address, bool accumulator);
void STX(uint16_t address);
void LDX(uint16_t address, bool page_cross);
void DECC(uint16_t address); // dec is defined as 10 somewhere?
void INC(uint16_t address);

// Third group of instructions
void run_instruction_group3(uint16_t address, bool page_cross);
void JMP_abs(uint16_t jump_address);
void JMP_indirect(uint16_t jump_address);
void BITT(uint16_t address);
void STY(uint16_t address);
void CPY(uint16_t address);
void CPX(uint16_t address);
void LDY(uint16_t address, bool page_cross);

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



#endif