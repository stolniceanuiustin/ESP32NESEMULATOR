#include "cpu.h"


struct Instruction inst;
CPU_STATE state = FETCH_INSTRUCTION;

byte A = 0x00;
byte X = 0x00;
byte Y = 0x00;
uint16_t PC = 0x0000;
byte SP = 0xFD; 


uint64_t cycles = 0;
int64_t estimated_cycles = 0;
uint64_t elapsed_cycles = 0;


byte C = 0; // carry
byte Z = 0; // zero
byte I = 1; // interrupt 
byte D = 0; // decimal
byte B = 0; // break
byte O = 0; // overflow
byte N = 0; // negative


bool pending_nmi = false;
bool pending_irq = false;