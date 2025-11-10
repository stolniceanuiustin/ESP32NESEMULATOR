#include "cpu.h"


//Here are defined all the cpu instructions. i know this file looks long but each function is O(1) and quite simple actually.
//full documentation of all instructions can be found here:
//https://llx.com/Neil/a2/opcodes.html
//https://www.nesdev.org/obelisk-6502-guide/
//

/*
ADDRESSED INSTRUCTIONS - THE HARDEST 
*/
void CPU::set_ZN(byte value)
{
    Z = (value == 0) ? 1 : 0;
    N = (value & 0x80) ? 1 : 0; //Checks bit 7 if it's set or not
}

// GROUP 1 INSTRUCTIOS
void CPU::ORA(uint16_t address, bool page_cross)
{
    // OR between accumulator and the contents at the given address
    const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};
    A = A | read(address);
    set_ZN(A);

    cycles += lookup[inst.bbb] + (int)page_cross;
}

void CPU::AND(uint16_t address, bool page_cross)
{
    const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};
    byte operand = read(address);
    A = A & operand;
    set_ZN(A);

    cycles += lookup[inst.bbb] + (int)page_cross;
}

void CPU::EOR(uint16_t address, bool page_cross)
{
    const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};
    A = A ^ read(address);
    set_ZN(A);

    cycles += lookup[inst.bbb] + (int)page_cross;
}

void CPU::ADC(uint16_t address, bool page_cross)
{
    // TODO: TEST THIS
    const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};
    byte operand = read(address);
    uint16_t result = (uint16_t)A + (uint16_t)(operand) + (uint16_t)(C);
    C = result > 0x00FF ? 1 : 0; 
    bool overflow = ~(A ^ operand) & (A ^ result) & 0x80;
    O = overflow ? 1 : 0;
    A = (byte)(result & 0x00FF);
    set_ZN(A);

    cycles += lookup[inst.bbb] + (int)page_cross;
}

void CPU::STA(uint16_t address)
{
    // Can't use IMEDIATE ADDRESSING
    // Stores the contents of the accumulator in memory
    // Doesnt change flags
    const int lookup[] = {6, 3, -1, 4, 6, 4, 5, 5};
    write(address, A);
    cycles += lookup[inst.bbb];
}

void CPU::LDA(uint16_t address, bool page_cross)
{
    //Loads A from memory
    const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};
    A = read(address);
    set_ZN(A);
    cycles += lookup[inst.bbb] + (int)page_cross;
}

void CPU::CMP(uint16_t address, bool page_cross)
{
    //TODO not sure if the flags are set correctly but i think they are, will find out in unit testing
    const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};

    uint16_t result = A - read(address);
    //cpu->SR &= ~(CARRY | NEGATIVE | ZERO);
    C = !(result & 0xFF00) ? 1 : 0;
    set_ZN(result);
    cycles += lookup[inst.bbb] + (int)page_cross;
}

void CPU::SBC(uint16_t address, bool page_cross)
{
    // TODO: TEST THIS
    const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};
    byte operand = read(address);
    operand = ~operand; 
    uint16_t result = (uint16_t)A + (uint16_t)(operand) + (uint16_t)(C);     //Accumulator + address + carry
    
    C = result > 0x00FF ? 1 : 0;    //if more than 1 byte => carry
    bool overflow = ~(A ^ operand) & (A ^ result) & 0x80;
    O = overflow ? 1 : 0;
    A = (byte)(result & 0x00FF);
    set_ZN(A);

    cycles += lookup[inst.bbb] + (int)page_cross;
}


void CPU::ASL(uint16_t address, bool accumulator)
{
    // Arithmetic shift left
    // Carry = old bit 7
    // bit 0 = 0
    int lookup[] = {-1, 5, 2, 6, -1, 6, -1, 7};
    if (accumulator)
    {
        uint16_t carry_flag = A & (1 << 7);
        if (carry_flag)
            C = 1;
        else
            C = 0;

        A = A << 1;
        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        uint16_t carry_flag = operand & (1 << 7);
        if (carry_flag)
            C = 1;
        else
            C = 0;
        write(address, operand << 1);
        set_ZN(read(address));
    }
    cycles += lookup[inst.bbb];
}
void CPU::ROL(uint16_t address, bool accumulator)
{
    // rotate left
    int lookup[] = {-1, 5, 2, 6, -1, 6, -1, 7};
    if (accumulator)
    {
        uint16_t carry_flag = A & (1 << 7);
        A = A << 1;
        A &= ~1;   // Clear last bit;
        A = A | C; // Set last bit to carry (the old carry, the documentation says so)

        if (carry_flag)
            C = 1;
        else
            C = 0;

        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        uint16_t carry_flag = operand & (1 << 7);
        operand = operand << 1;
        operand &= ~1;
        operand |= C;
        write(address, operand);
        if (carry_flag)
            C = 1;
        else
            C = 0;
        set_ZN(operand);
    }
    cycles += lookup[inst.bbb];
}
void CPU::LSR(uint16_t address, bool accumulator)
{
    // Logical shift right
    int lookup[] = {-1, 5, 2, 6, -1, 6, -1, 7};
    if (accumulator)
    {
        uint16_t carry_flag = A & 1;
        A = A >> 1;
        A &= ~(1 << 7); // clear first bit(should be clear already);

        if (carry_flag)
            C = 1;
        else
            C = 0;
        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        uint16_t carry_flag = operand & 1;
        operand = operand >> 1;
        operand &= ~(1 << 7);
        write(address, operand);
        if (carry_flag)
            C = 1;
        else
            C = 0;
        set_ZN(operand);
    }
    cycles += lookup[inst.bbb];
}
void CPU::ROR(uint16_t address, bool accumulator)
{
    // Rotate right
    int lookup[] = {-1, 5, 2, 6, -1, 6, -1, 7};
    if (accumulator)
    {
        int carry_flag = A & 1;
        A = A >> 1;
        A &= ~(1 << 7); // clear first bit(should be clear already);
        A |= C << 7;    // set first bit to carry flag

        if (carry_flag)
            C = 1;
        else
            C = 0;
        set_ZN(A);
    }
    else
    {
        byte operand = read(address);
        int carry_flag = operand & 1;

        operand = operand >> 1;
        operand &= ~(1 << 7);
        operand |= C << 7;
        write(address, operand);

        if (carry_flag)
            C = 1;
        else
            C = 0;
        set_ZN(operand);
    }

    cycles += lookup[inst.bbb];
}
void CPU::STX(uint16_t address)
{
    int lookup[] = {-1, 3, -1, 4, -1, 4, -1, -1};
    write(address, X);
    cycles += lookup[inst.bbb];
}
void CPU::LDX(uint16_t address, bool page_cross)
{
    int lookup[] = {2, 3, -1, 4, -1, 4, -1, 4};
    X = read(address);
    set_ZN(X);
    cycles += lookup[inst.bbb] + (int)page_cross;
}
void CPU::DECC(uint16_t address)
{
    int lookup[] = {-1, 5, -1, 6, -1, 6, -1, 7};
    byte operand = read(address);
    operand -= 1;
    write(address, operand);
    set_ZN(operand);
    cycles += lookup[inst.bbb];
}
void CPU::INC(uint16_t address)
{
    int lookup[] = {-1, 5, -1, 6, -1, 6, -1, 7};
    byte operand = read(address);
    operand += 1;
    write(address, operand);
    set_ZN(operand);
    cycles += lookup[inst.bbb];
}


void CPU::BITT(uint16_t address)
{
    // bit test, test if one or more bits are in a target memory location
    // mask patern in A is & with memory to keep zero, overflow, negative etc.
    int lookup[] = {-1, 3, -1, 4, -1, -1, -1, -1};
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

    cycles += lookup[inst.bbb];
}

void CPU::JMP_abs(uint16_t jump_address)
{
    PC = jump_address;
    cycles += 3;
}

void CPU::JMP_indirect(uint16_t jump_address)
{
    // TODO: ADDRESS BUG FROM ORIGINAL 6502(not a bug in my code)
    //PC = read_abs_address(jump_address);
    uint16_t aux_address = 0;
    byte low_byte = read(jump_address);
    uint16_t high_byte_of_addr = jump_address & 0xFF00;
    byte high_byte = read(((jump_address + 1) & 0x00FF) | high_byte_of_addr);
    aux_address = (high_byte << 8) | low_byte;
    PC = aux_address;
    cycles += 5;
}

void CPU::STY(uint16_t address)
{
    int lookup[] = {-1, 3, -1, 4, -1, 4, -1, -1};
    write(address, Y);
    cycles += lookup[inst.bbb];
}

void CPU::LDY(uint16_t address, bool page_cross)
{ // immediate, zeropage, nothig, absolut,nothing, zero page x, nothing, absolut x
    int lookup[] = {2, 3, -1, 4, -1, 4, -1, 4};
    Y = read(address);
    cycles += lookup[inst.bbb] + (int)page_cross;
    set_ZN(Y);
}

void CPU::CPY(uint16_t address)
{
    int lookup[] = {2, 3, -1, 4, -1, -1, -1, -1};
    byte operand = read(address);  
    byte result_byte = Y - operand;
    C = Y >= operand ? 1 : 0;
    set_ZN(result_byte);
    

    cycles += lookup[inst.bbb];
}

void CPU::CPX(uint16_t address)
{
    //TODO FIX THIS THE SAME AS CPY
    int lookup[] = {2, 3, -1, 4, -1, -1, -1, -1};
    byte operand = read(address);
    byte result_byte = X - operand;
    C = X >= operand ? 1 : 0;
    set_ZN(result_byte);
    

    cycles += lookup[inst.bbb];
}

/*
Single Byte instructions
*/

byte CPU::pack_flags()
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

}


void CPU::unpack_flags(byte flags)
{
    N = ((1 << 7) & flags) >> 7;
    O = ((1 << 6) & flags) >> 6;
    B = 0;                      //break flag shouldnt change when loded with PLP
    D = ((1 << 3) & flags) >> 3;
    I = ((1 << 2) & flags) >> 2;
    Z = ((1 << 1) & flags) >> 1;
    C = 1 & flags;
    return;
}
void CPU::PHP()
{
    byte to_push = pack_flags();
    push(to_push);
    cycles += 3;
}

void CPU::PLP()
{
    byte flags = pop();
    unpack_flags(flags);

    cycles += 4;

}

void CPU::PHA()
{
    push(A);
    cycles += 3;
}

void CPU::PLA()
{
    A = pop();
    set_ZN(A);
    cycles += 4;
}

void CPU::DEY()
{
    Y = Y-1;
    set_ZN(Y);
    cycles += 2;
}

void CPU::TAY()
{
    Y = A;
    set_ZN(Y);
    cycles += 2;
}

void CPU::INY()
{
    Y = Y+1;
    set_ZN(Y);
    cycles += 2;
}

void CPU::INX()
{
    X = X+1;
    set_ZN(X);
    cycles += 2;
}

void CPU::CLC()
{
    C = 0;
    cycles += 2;
}

void CPU::SEC()
{
    C = 1;
    cycles += 2;
}

void CPU::CLI()
{
    I = 0;
    cycles += 2;
}

void CPU::SEI()
{
    I = 1;
    cycles += 2;   
}

void CPU::TYA()
{
    A = Y;
    set_ZN(A);
    cycles += 2;
}

void CPU::CLV()
{
    O = 0;
    cycles += 2;   
}

void CPU::CLD()
{
    //SHOULDNT USE IN NES EMU
    //std::cout << "CLD shouldn't be used\n";
    D = 0;
    cycles += 2; 
}

void CPU::SED()
{
    D = 1;
    cycles += 2;
}

void CPU::TXA()
{
    A = X;
    set_ZN(A);
    cycles += 2;
}

void CPU::TXS()
{
    SP = X;
    cycles += 2;
}

void CPU::TAX()
{
    X = A;
    set_ZN(X);
    cycles += 2;
}

void CPU::TSX()
{
    X = SP;
    set_ZN(X);
    cycles += 2;
}

void CPU::DEX()
{
    X = X-1;
    set_ZN(X);
    cycles += 2;
}

/*
interrupts
*/
void CPU::NOPP()
{
    cycles += 2;
}

void CPU::JSR_abs(uint16_t address)
{
    push_address(PC);
    PC = address;
    cycles += 6;
}

void CPU::RTS()
{
    uint16_t aux = pop_address();
    aux += 1;
    PC = aux;
    cycles += 6;
}

void CPU::BRK()
{
    int t = 0;
    if(I == 0)
    {
        trigger_irq();
        cycles += 7;
    }
    else cycles+=2;
    B = 1;
    //TODO CHECK THIS: shouldn't happen in NES
    // cycles += 7;
}


void CPU::trigger_irq()
{
    if(I == 0) //interrupts enabled
    {
        SP = 0xFD;
        push_address(PC);
        push((PC & 0xFF00) >> 8);
        push((PC & 0x00FF));
        push(pack_flags());
        PC = read_abs_address(IRQ_vector);
        I = 1;
        //std::cout << "IRQ TRIGGERED. CHECK FUNCTION FOR CYCLE COUNT!\n";
    }
}

void CPU::trigger_nmi()
{   //STACK STARTS AT 0xFD
    //std::cout << "===============NMI TRIGGERED===============\n";
    SP = 0xFD;
    push_address(PC);
    push(pack_flags());
    PC = read_abs_address(NMI_vector);
    I = 1;
    //TODO check if this was an issue
    cycles += 7;
    pending_nmi = false;
}

void CPU::RTI()
{
    byte flags = pop();
    unpack_flags(flags);
    PC = pop_address();
    cycles += 6;
}