#include <Arduino.h>
#include <stdint.h>
// Include your necessary header files
#include "cpu.h"
#include "tracer.h"
#include "cpu_vars.h" // Needed for CPU_VARS definition


// Define byte type if not in your headers
typedef uint8_t byte;

// Forward declaration for the temporary Instruction struct used internally

// --- Helper Functions (Adapted to use const char* instead of std::string) ---

// retruns length of operand!
byte compute_operand_length_g1(byte bbb)
{
    switch (bbb)
    {
    case 0x0: // (ZERO PAGE, X)
    case 0x01: // ZERO PAGE
    case 0x02: // Immediate
    case 0x04: // (zero page), Y
    case 0x05: // zero page, X
        return 1;
    case 0x03: // Absolute
    case 0x06: // absolute, Y
    case 0x07: // Absolute, X
        return 2;
    }
    return 0;
}

const char* compute_instruction_name_group1(byte aaa)
{
    switch (aaa)
    {
    case 0x0: return "ORA";
    case 0x1: return "AND";
    case 0x2: return "EOR";
    case 0x3: return "ADC";
    case 0x4: return "STA";
    case 0x5: return "LDA";
    case 0x6: return "CMP";
    case 0x7: return "SBC";
    }
    return "ILL_G1";
}

byte compute_operand_length_g23(byte bbb, byte opcode)
{
    if (opcode == 0x004C || opcode == 0x006C) // JMP (absolute and indirect)
        return 2;

    switch (bbb)
    {
    case 0x0: // Immediate
    case 0x1: // Zero Page
    case 0x5: // Zero Page, X (or Y for LDX/STX)
        return 1;
    case 0x3: // Absolute
    case 0x7: // Absolute, X (or Y for LDX/STX)
        return 2;
    case 0x2: // Accumulator (A) - no extra bytes needed
        return 0; 
    }
    return 0;
}

const char* compute_instruction_name_group2(byte aaa)
{
    switch (aaa)
    {
    case 0x0: return "ASL";
    case 0x1: return "ROL";
    case 0x2: return "LSR";
    case 0x3: return "ROR";
    case 0x4: return "STX";
    case 0x5: return "LDX";
    case 0x6: return "DEC";
    case 0x7: return "INC";
    }
    return "ILL_G2";
}

// Simplified from original: removed unused 'string &observations' parameter
const char* compute_instruction_name_group3(byte aaa)
{
    switch (aaa)
    {
    case 0x1: return "BIT";
    case 0x2: return "JMP"; // JMPabs
    case 0x3: return "JMP"; // JMPind
    case 0x4: return "STY";
    case 0x5: return "LDY";
    case 0x6: return "CPY";
    case 0x7: return "CPX";
    }
    return "ILL_G3";
}


/**
 * @brief Generates a trace line for the current CPU state and prints it to the Serial monitor.
 * @param cpu_state The current state of the CPU registers.
 * @return String (Arduino String) containing the formatted trace line.
 */
std::string TRACER::tracer(CPU_VARS cpu_state)
{
    // Use a fixed buffer for efficient, formatted output (80 characters should be plenty)
    char output_buffer[80]; 
    const char* instruction_name = "UNKNOWN";
    byte instruction_length = 1;

    Instruction inst;
    // Access the CPU reference held by the TRACER object
    inst.opcode = cpu.ram_at(cpu_state.PC); 

    inst.aaa = (0xE0 & inst.opcode) >> 5;
    inst.bbb = (0x1C & inst.opcode) >> 2;
    inst.cc = (0x03 & inst.opcode); 
    inst.xx = (0b11000000 & inst.opcode) >> 6; 
    inst.y = (0b00100000 & inst.opcode) >> 5; 
    byte low_nibble = inst.opcode & 0x0F;
    byte high_nibble = inst.opcode >> 4;

    // --- Single-Byte Instructions (Non-Grouped) & NOPs ---

    // Handle NOPs (Using the local 'cpu' reference)
    if (inst.opcode == 0xEA) {
        instruction_name = "NOP"; instruction_length = 1;
    } else if (inst.opcode == 0x04 || inst.opcode == 0x44 || inst.opcode == 0x64) {
        instruction_name = "NOP"; instruction_length = 2;
    } else if (inst.opcode == 0xC0) {
        instruction_name = "NOP"; instruction_length = 3;
    } else if (inst.opcode == 0x14 || inst.opcode == 0x34 || inst.opcode == 0x54 || inst.opcode == 0x74 || inst.opcode == 0xD4 || inst.opcode == 0xF4) {
        instruction_name = "NOP"; instruction_length = 2;
    }
    // BRK, JSR, RTI, RTS
    else if (inst.opcode == 0x00) { instruction_name = "BRK"; instruction_length = 1; }
    else if (inst.opcode == 0x20) { instruction_name = "JSR"; instruction_length = 3; } 
    else if (inst.opcode == 0x40) { instruction_name = "RTI"; instruction_length = 1; }
    else if (inst.opcode == 0x60) { instruction_name = "RTS"; instruction_length = 1; }
    
    // Low nibble 0x08 (Stack/Flag operations)
    else if (low_nibble == 0x08) {
        instruction_length = 1;
        switch (inst.opcode) {
        case 0x08: instruction_name = "PHP"; break; case 0x28: instruction_name = "PLP"; break;
        case 0x48: instruction_name = "PHA"; break; case 0x68: instruction_name = "PLA"; break;
        case 0x88: instruction_name = "DEY"; break; case 0xA8: instruction_name = "TAY"; break;
        case 0xC8: instruction_name = "INY"; break; case 0xE8: instruction_name = "INX"; break;
        case 0x18: instruction_name = "CLC"; break; case 0x38: instruction_name = "SEC"; break;
        case 0x58: instruction_name = "CLI"; break; case 0x78: instruction_name = "SEI"; break;
        case 0x98: instruction_name = "TYA"; break; case 0xB8: instruction_name = "CLV"; break;
        case 0xD8: instruction_name = "CLD"; break; case 0xF8: instruction_name = "SED"; break;
        default: break;
        }
    }
    // Low nibble 0x0A (Transfer/Shift/Dec)
    else if (low_nibble == 0x0A && high_nibble >= 0x08) {
        instruction_length = 1;
        switch (inst.opcode) {
        case 0x8A: instruction_name = "TXA"; break; case 0x9A: instruction_name = "TXS"; break;
        case 0xAA: instruction_name = "TAX"; break; case 0xBA: instruction_name = "TSX"; break;
        case 0xCA: instruction_name = "DEX"; break;
        // The original code has 0xEA here, which is NOP and handled above.
        default: break;
        }
    }
    
    // --- Branch Instructions (Low Nibble 0x00 and not BRK) ---
    else if (low_nibble == 0x00 && (inst.opcode & 0x1F) == 0b00010000)
    {
        instruction_length = 2;

        if (inst.xx == 0b00) {
            instruction_name = (cpu.N == inst.y) ? "BPL" : "BMI"; // Uses CPU flag 'N'
        } else if (inst.xx == 0b01) {
            instruction_name = (cpu.O == inst.y) ? "BVC" : "BVS"; // Uses CPU flag 'O'
        } else if (inst.xx == 0b10) {
            instruction_name = (cpu.C == inst.y) ? "BCC" : "BCS"; // Uses CPU flag 'C'
        } else if (inst.xx == 0b11) {
            instruction_name = (cpu.Z == inst.y) ? "BNE" : "BEQ"; // Uses CPU flag 'Z'
        }
    }
    
    // --- Grouped Instructions (cc=01, cc=10, cc=00) ---
    else {
        switch (inst.cc)
        {
        case 0x01: // cc = 01 (Group 1: ORA, AND, EOR, ADC, STA, LDA, CMP, SBC)
            instruction_length += compute_operand_length_g1(inst.bbb);
            instruction_name = compute_instruction_name_group1(inst.aaa);
            break;
        case 0x02: // cc = 10 (Group 2: ASL, ROL, LSR, ROR, STX, LDX, DEC, INC)
            instruction_length += compute_operand_length_g23(inst.bbb, inst.opcode);
            instruction_name = compute_instruction_name_group2(inst.aaa);
            break;
        case 0x0: // cc = 00 (Group 3: BIT, JMP, STY, LDY, CPY, CPX, etc.)
            instruction_length += compute_operand_length_g23(inst.bbb, inst.opcode);
            instruction_name = compute_instruction_name_group3(inst.aaa);
            break;
        }
    }

    // --- FORMATTING LOGIC USING SNPRINTF AND SERIAL OUTPUT ---

    // 1. Prepare the Instruction Bytes string (to ensure correct padding/spacing)
    char instruction_bytes_str[10] = "";
    if (instruction_length == 1) {
        snprintf(instruction_bytes_str, sizeof(instruction_bytes_str), "%02X      ", cpu.ram_at(cpu_state.PC));
    } else if (instruction_length == 2) {
        snprintf(instruction_bytes_str, sizeof(instruction_bytes_str), 
                 "%02X %02X   ", 
                 cpu.ram_at(cpu_state.PC), cpu.ram_at(cpu_state.PC + 1));
    } else if (instruction_length == 3) {
        snprintf(instruction_bytes_str, sizeof(instruction_bytes_str), 
                 "%02X %02X %02X", 
                 cpu.ram_at(cpu_state.PC), cpu.ram_at(cpu_state.PC + 1), cpu.ram_at(cpu_state.PC + 2));
    }

    // 2. Combine all parts into the final output buffer using snprintf
    // Format: ADDR OP CD CD CD INST... A:XX X:XX Y:XX P:XX SP:XX CYC:NUM
    snprintf(output_buffer, sizeof(output_buffer),
             // PC   | Instr Bytes | Instr Name | A   | X   | Y   | P   | SP  | CYC
             "%04X %s %-4s A:%02X X:%02X Y:%02X P:%02X SP:%02X CYC:%u",
             cpu_state.PC,
             instruction_bytes_str,
             instruction_name,
             cpu_state.A,
             cpu_state.X,
             cpu_state.Y,
             cpu_state.flags,
             cpu.get_SP(), // Use CPU function for current SP
             (unsigned int)cpu_state.cylces // Ensure cycles is printed as unsigned int
    );

    // 3. Print the result to the Arduino Serial Monitor
    Serial.println(output_buffer);

    // The function signature still requires std::string, so we return the buffer wrapped in String (Arduino String)
    return std::string(output_buffer); 
}