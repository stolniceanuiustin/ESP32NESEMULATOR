#include "cpu.h"

// Here are defined all the cpu instructions. i know this file looks long but each function is O(1) and quite simple actually.
// full documentation of all instructions can be found here:
// https://llx.com/Neil/a2/opcodes.html
// https://www.nesdev.org/obelisk-6502-guide/
//
inline void set_ZN(byte value)
{
    Z = (value == 0) ? 1 : 0;
    N = (value & 0x80) ? 1 : 0; // Checks bit 7 if it's set or not
}

// ADDRESSED INSTRUCTIONS - THE HARDEST
// GROUP 1 INSTRUCTIOS

void IRAM_ATTR ORA(uint16_t address)
{
    // OR between accumulator and the contents at the given address
    A = A | read(address);
    set_ZN(A);
}

void IRAM_ATTR AND(uint16_t address)
{
    byte operand = read(address);
    A = A & operand;
    set_ZN(A);
}

void IRAM_ATTR EOR(uint16_t address)
{
    A = A ^ read(address);
    set_ZN(A);
}

void IRAM_ATTR ADC(uint16_t address)
{
    byte operand = read(address);
    uint16_t result = (uint16_t)A + (uint16_t)(operand) + (uint16_t)(C);
    C = result > 0x00FF ? 1 : 0;
    bool overflow = ~(A ^ operand) & (A ^ result) & 0x80;
    O = overflow ? 1 : 0;
    A = (byte)(result & 0x00FF);
    set_ZN(A);
}

void IRAM_ATTR STA(uint16_t address)
{
    // Can't use IMEDIATE ADDRESSING
    // Stores the contents of the accumulator in memory
    // Doesnt change flags
    cpu_write(address, A);
}

void LDA(uint16_t address)
{
    // Loads A from memory
    A = read(address);
    set_ZN(A);
}

void CMP(uint16_t address)
{
    uint16_t result = A - read(address);
    C = !(result & 0xFF00) ? 1 : 0;
    set_ZN(result);
}

void SBC(uint16_t address)
{
    byte operand = read(address);
    operand = ~operand;
    uint16_t result = (uint16_t)A + (uint16_t)(operand) + (uint16_t)(C); // Accumulator + address + carry

    C = result > 0x00FF ? 1 : 0; // if more than 1 byte => carry
    bool overflow = ~(A ^ operand) & (A ^ result) & 0x80;
    O = overflow ? 1 : 0;
    A = (byte)(result & 0x00FF);
    set_ZN(A);
}

void IRAM_ATTR ASL_acc()
{
    uint16_t carry_flag = A & (1 << 7);
    C = !!carry_flag;
    A = A << 1;
    set_ZN(A);
}

void ASL_b(uint16_t address, bool accumulator)
{
    // Arithmetic shift left
    // Carry = old bit 7
    // bit 0 = 0
    if (accumulator)
    {
        uint16_t carry_flag = A & (1 << 7);
        C = !!carry_flag;

        A = A << 1;
        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        uint16_t carry_flag = operand & (1 << 7);
        C = !!carry_flag;
        cpu_write(address, operand << 1);
        set_ZN(read(address));
    }
}

void IRAM_ATTR ASL(uint16_t address)
{
    byte operand = read(address);
    uint16_t carry_flag = operand & (1 << 7);
    C = !!carry_flag;
    cpu_write(address, operand << 1);
    set_ZN(read(address));
}

void IRAM_ATTR ROL_acc()
{
    uint16_t carry_flag = A & (1 << 7);
    A = A << 1;
    A &= ~1;   // Clear last bit;
    A = A | C; // Set last bit to carry (the old carry, the documentation says so)

    C = !!carry_flag;
    set_ZN(A);
}

void IRAM_ATTR ROL_b(uint16_t address, bool accumulator)
{
    // rotate left
    if (accumulator)
    {
        uint16_t carry_flag = A & (1 << 7);
        A = A << 1;
        A &= ~1;   // Clear last bit;
        A = A | C; // Set last bit to carry (the old carry, the documentation says so)

        C = !!carry_flag;

        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        uint16_t carry_flag = operand & (1 << 7);
        operand = operand << 1;
        operand &= ~1;
        operand |= C;
        cpu_write(address, operand);
        C = !!carry_flag;
        set_ZN(operand);
    }
}

void IRAM_ATTR ROL(uint16_t address)
{
    byte operand = read(address);
    uint16_t carry_flag = operand & (1 << 7);
    operand = operand << 1;
    operand &= ~1;
    operand |= C;
    cpu_write(address, operand);
    C = !!carry_flag;
    set_ZN(operand);
}

void IRAM_ATTR LSR_acc()
{
    uint16_t carry_flag = A & 1;
    A = A >> 1;
    A &= ~(1 << 7); // clear first bit(should be clear already);
    C = !!carry_flag;
    set_ZN(A);
}

void LSR_b(uint16_t address, bool accumulator)
{
    // Logical shift right
    if (accumulator)
    {
        uint16_t carry_flag = A & 1;
        A = A >> 1;
        A &= ~(1 << 7); // clear first bit(should be clear already);

        C = !!carry_flag;
        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        uint16_t carry_flag = operand & 1;
        operand = operand >> 1;
        operand &= ~(1 << 7);
        cpu_write(address, operand);
        C = !!carry_flag;
        set_ZN(operand);
    }
}

void IRAM_ATTR LSR(uint16_t address)
{
    byte operand = read(address);
    uint16_t carry_flag = operand & 1;
    operand = operand >> 1;
    operand &= ~(1 << 7);
    cpu_write(address, operand);
    C = !!carry_flag;
    set_ZN(operand);
}

// todo : split this up into 2 functions

void IRAM_ATTR ROR_acc()
{
    int carry_flag = A & 1;
    A = A >> 1;
    A &= ~(1 << 7); // clear first bit(should be clear already);
    A |= C << 7;    // set first bit to carry flag

    C = !!carry_flag;
    set_ZN(A);
}
void IRAM_ATTR ROR_b(uint16_t address, bool accumulator)
{
    // Rotate right
    if (accumulator)
    {
        int carry_flag = A & 1;
        A = A >> 1;
        A &= ~(1 << 7); // clear first bit(should be clear already);
        A |= C << 7;    // set first bit to carry flag

        C = !!carry_flag;
        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        int carry_flag = operand & 1;

        operand = operand >> 1;
        operand &= ~(1 << 7);
        operand |= C << 7;
        cpu_write(address, operand);

        C = !!carry_flag;
        set_ZN(operand);
    }
}
void IRAM_ATTR ROR(uint16_t address)
{
    // Rotate right

    byte operand = read(address);
    int carry_flag = operand & 1;

    operand = operand >> 1;
    operand &= ~(1 << 7);
    operand |= C << 7;
    cpu_write(address, operand);

    C = !!carry_flag;
    set_ZN(operand);
}
void IRAM_ATTR STX(uint16_t address)
{
    cpu_write(address, X);
}
void IRAM_ATTR LDX(uint16_t address)
{
    X = read(address);
    set_ZN(X);
}
void IRAM_ATTR DECC(uint16_t address)
{
    byte operand = read(address);
    operand -= 1;
    cpu_write(address, operand);
    set_ZN(operand);
}
void IRAM_ATTR INC(uint16_t address)
{
    byte operand = read(address);
    operand += 1;
    cpu_write(address, operand);
    set_ZN(operand);
}

void IRAM_ATTR BITT(uint16_t address)
{
    // bit test, test if one or more bits are in a target memory location
    // mask patern in A is & with memory to keep zero, overflow, negative etc.
    uint8_t result = A & read(address);
    if (result == 0)
        Z = 1;
    else
        Z = 0;
    uint8_t negative = read(address) & (1 << 7);
    if (negative)
        N = 1;
    else
        N = 0;
    uint8_t overflow = read(address) & (1 << 6);
    if (overflow)
        O = 1;
    else
        O = 0;
}

void IRAM_ATTR JMP_abs(uint16_t jump_address)
{
    PC = jump_address;
}

void IRAM_ATTR JMP_indirect(uint16_t jump_address)
{
    // TODO: ADDRESS BUG FROM ORIGINAL 6502(not a bug in my code)
    // PC = read_abs_address(jump_address);
    uint16_t aux_address = 0;
    byte low_byte = read(jump_address);
    uint16_t high_byte_of_addr = jump_address & 0xFF00;
    byte high_byte = read(((jump_address + 1) & 0x00FF) | high_byte_of_addr);
    aux_address = (high_byte << 8) | low_byte;
    PC = aux_address;
}

void IRAM_ATTR STY(uint16_t address)
{
    cpu_write(address, Y);
}

void IRAM_ATTR LDY(uint16_t address)
{ // immediate, zeropage, nothig, absolut,nothing, zero page x, nothing, absolut x
    Y = read(address);
    set_ZN(Y);
}

void IRAM_ATTR CPY(uint16_t address)
{
    byte operand = read(address);
    byte result_byte = Y - operand;
    C = Y >= operand ? 1 : 0;
    set_ZN(result_byte);
}

void IRAM_ATTR CPX(uint16_t address)
{

    byte operand = read(address);
    byte result_byte = X - operand;
    C = X >= operand ? 1 : 0;
    set_ZN(result_byte);
}

/*
Single Byte instructions
*/

void IRAM_ATTR PHP()
{
    byte to_push = pack_flags();
    push(to_push);
}

void IRAM_ATTR PLP()
{
    byte flags = pop();
    unpack_flags(flags);
}

void IRAM_ATTR PHA()
{
    push(A);
}

void IRAM_ATTR PLA()
{
    A = pop();
    set_ZN(A);
}

void IRAM_ATTR DEY()
{
    Y = Y - 1;
    set_ZN(Y);
}

void IRAM_ATTR TAY()
{
    Y = A;
    set_ZN(Y);
}

void IRAM_ATTR INY()
{
    Y = Y + 1;
    set_ZN(Y);
    //N = 1;
}

void IRAM_ATTR INX()
{
    X = X + 1;
    set_ZN(X);
}

void IRAM_ATTR CLC()
{
    C = 0;
}

void IRAM_ATTR SEC()
{
    C = 1;
}

void IRAM_ATTR CLI()
{
    I = 0;
}

void IRAM_ATTR SEI()
{
    I = 1;
}

void IRAM_ATTR TYA()
{
    A = Y;
    set_ZN(A);
}

void IRAM_ATTR CLV()
{
    O = 0;
}

void IRAM_ATTR CLD()
{
    // SHOULDNT USE IN NES EMU
    // std::cout << "CLD shouldn't be used\n";
    D = 0;
}

void IRAM_ATTR SED()
{
    D = 1;
}

void IRAM_ATTR TXA()
{
    A = X;
    set_ZN(A);
}

void IRAM_ATTR TXS()
{
    SP = X;
}

void IRAM_ATTR TAX()
{
    X = A;
    set_ZN(X);
}

void IRAM_ATTR TSX()
{
    X = SP;
    set_ZN(X);
}

void IRAM_ATTR DEX()
{
    X = X - 1;
    set_ZN(X);
}

/*
interrupts
*/
void IRAM_ATTR NOPP()
{
    // cycles += 2;
}

void IRAM_ATTR JSR_abs(uint16_t address)
{
    push_address(PC);
    PC = address;
}

void IRAM_ATTR RTS()
{
    uint16_t aux = pop_address();
    aux += 1;
    PC = aux;
}

void IRAM_ATTR BRK()
{
    int t = 0;
    if (I == 0)
    {
        trigger_irq();
    }
    // else
    //     cycles += 2;
    B = 1;
    // TODO CHECK THIS: shouldn't happen in NES
    //  cycles += 7;
}

void IRAM_ATTR trigger_irq()
{
    if (I == 0) // interrupts enabled
    {
        //SP = 0xFD;
        //push_address(PC);
        push((PC & 0xFF00) >> 8);
        push((PC & 0x00FF));
        push(pack_flags());
        PC = read_abs_address(IRQ_vector);
        I = 1;
        // std::cout << "IRQ TRIGGERED. CHECK FUNCTION FOR CYCLE COUNT!\n";
    }
}

void IRAM_ATTR trigger_nmi()
{ // STACK STARTS AT 0xFD
    // std::cout << "===============NMI TRIGGERED===============\n";
    //SP = 0xFD;
    push_address(PC);
    push(pack_flags());
    PC = read_abs_address(NMI_vector);
    //I = 1;
    // TODO check if this was an issue
    pending_nmi = false;
}

// void IRAM_ATTR trigger_nmi()
// {
//     // push((PC >> 8) & 0xFF);
//     // push(PC & 0xFF);
//     // uint8_t flags = pack_flags();
//     // push(flags);

//     // PC = read_abs_address(NMI_vector);

//     pending_nmi = false;
// }
void IRAM_ATTR RTI()
{
    byte flags = pop();
    unpack_flags(flags);
    PC = pop_address();
}