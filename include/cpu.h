#ifndef HEADERGAURD_CPU_HEADER
#define HEADERGAURD_CPU_HEADER
#include <stdint.h>
#include "memory.h"
#include "emulator_config.h"
#include "bus.h"
#include "cpu_vars.h"
#include "tracer.h"
// #include "tracer.h"
#define TEST

class BUS;
//class TRACER;
typedef uint8_t byte;

typedef enum
{
	FETCH_INSTRUCTION,
	WAIT_FOR_N_CYCLES,
	EXECUTE
}CPU_STATE;

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



class CPU
{
public:
	struct Instruction inst;
public:
	//STACK IS at page 1 0100 - 01FF, sp goes from 1ff to 100
	byte A; 
	byte X;
	byte Y;
	uint16_t PC;
	byte SP; // Stack pointer
	uint64_t cycles;
	int64_t estimated_cycles = 0;
	uint64_t elapsed_cycles = 0;
public:
	// Cpu Flags
	byte C; // carry
	byte Z; // zero
	byte I; // interrupt
	byte D; // decimal
	byte B; // break
	byte O; // overflow //iN DOCUMENTATION IT'S V BUT O is more intuitive
	byte N; // negative
	BUS* bus;
	CPU() //No longer need a tracer attached to the CPU(the esp is too weak to trace ever y instruction). tracer(*this)
	{
	};

	void init();
	bool reset();

	void connect_bus(BUS* new_bus)
	{
		bus = new_bus;
	}
	byte ram_at(uint16_t address)
	{	
		return read(address);
	}
	byte get_A()
	{
		return A;
	}
	byte get_X()
	{
		return X;
	}
	byte get_Y()
	{
		return Y;
	}
	uint64_t get_cycles()
	{
		return cycles;
	}
	void set_cycles(uint64_t new_cycles)
	{
		cycles = new_cycles;
	}
	uint16_t get_pc()
	{
		return PC;
	}
	uint16_t get_inst_opcode()
	{
		return inst.opcode;
	}
	byte get_SP()
	{
		return SP;
	}
	byte pack_flags();
	void unpack_flags(byte flags);
	CPU_VARS pack_vars();
	byte read_pc();
	byte read(uint16_t address);
	void write(uint16_t address, byte data);

	uint16_t read_address_from_pc();
	uint16_t read_address(byte offset);

	CPU_STATE state = FETCH_INSTRUCTION;
	std::string execute(bool debug);
	void fetch_instruction();
	void clock();
	void execute();
	std::string clock(bool debug);
	std::string execute_debug();
	uint16_t read_abs_address(uint16_t offset);
	void push(byte x);
	void push_address(uint16_t address);
	byte pop();
	uint16_t pop_address();
	

	//CYCLE ESTIMATION
	int estimate_cycles();
	int estimate_cycles_group_sb1();
	int estimate_cycles_group_sb2();
	bool estimate_page_cross_g1();
	int estimate_cycles_group_1();
	int estimate_cycles_group_2();
	int estimate_cycles_group_3();
	bool estimate_page_cross_g23();
	// First group of instructions
	void set_ZN(byte value);
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
	void DECC(uint16_t address); //dec is defined as 10 somewhere?
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

	//SINGLE BYTE INSTRUCTIONS
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
	
	//INTERRUPTS 
	const uint16_t NMI_vector = 0xFFFA;
	const uint16_t RESET_VECTOR = 0xFFFC;
	const uint16_t IRQ_vector = 0xFFFE;
	const uint16_t BRK_vector = 0xFFFE;

	bool pending_nmi;
	bool pending_irq;
	void trigger_irq();
	void trigger_nmi();

	void JSR_abs(uint16_t address);
	void RTS();	
	void RTI();

	void enqueue_nmi()
	{
		pending_nmi = true;
	}

	void hexdump();


};


#endif