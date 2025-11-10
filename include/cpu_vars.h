#ifndef CPU_VARS_HEADERGUARD
#define CPU_VARS_HEADERGUARD
#include <stdint.h>
typedef uint8_t byte;
struct CPU_VARS
{
	uint16_t PC;
	byte flags;
	byte A;
	byte X;
	byte Y;
	byte SP;
	uint64_t cylces;
};

#endif