#include "cpu.h"

//This file contains the logic to count cycles for each instruciton 

bool estimate_page_cross_g1() {
	bool page_cross = false;
	uint16_t address = 0x0000;
	uint16_t aux_pc = 0x0000;
	switch (inst.bbb) {
		case 0x0:  //(ZERO PAGE, X). Next byte + X represents an adress in zero page
				   // that needs to be refferenced
		{
			return false;
			break;
		}
		case 0x01:	// ZERO PAGE.
		{
			return false;
			break;
		}
		case 0x02:	// Immediate. The opperand is in the next byte of memory
		{
			// FIXED THIS
			return false;
			break;
		}
		case 0x03:	// Absolute. The full 16-bit address
		{
			return false;
			break;
		}
		case 0x04:	//(zero page), Y; Takes an address from zero page as a pointer then
					// adds Y to that address
		{			// CAN CROSS PAGES
			aux_pc = PC;
			uint16_t zeropage_adr = read(aux_pc);
			byte low_byte = read(zeropage_adr);
			byte high_byte = read((zeropage_adr + 1) & 0x00FF);
			address = (high_byte << 8) | low_byte;
			uint16_t aux_addr = address;
			address += Y;
			page_cross = (aux_addr & 0xFF00) != (address & 0xFF00);
			return page_cross;
		}
		case 0x05:	// zero page, X; basically zeropage_addres + x
		{
			return false;
		}
		case 0x06:	// absolute, Y; The full 16-bit address is in memory and we have to
					// add Y
		{			// CAN CROSS PAGES
			aux_pc = PC;
			address = read_abs_address(aux_pc);
			uint16_t aux_addr = address;
			address += Y;
			page_cross = (aux_addr & 0xFF00) != (address & 0xFF00);
			return page_cross;
		}
		case 0x07:	// Absolute, X. the same as before
			// CAN CROSS PAGES
			{
				aux_pc = PC;
				address = read_abs_address(aux_pc);
				int high_byte = address >> 8;
				address += X;
				int high_byte_after_increment = address >> 8;
				if (high_byte < high_byte_after_increment) return true;
				return false;
			}
	}
	return false;
}

bool estimate_page_cross_g23() {
	uint16_t address = 0;
	uint16_t aux_pc = PC;
	bool page_cross = false;
	if (inst.opcode != 0x004C && inst.opcode != 0x006C)	 // EXCLUDE THE JUMPS;
	{
		switch (inst.bbb) {
			case 0x0:  // #immediate: literally the next byte in memory
			{
				return false;
			}
			case 0x1:  // zero page: an adress in range 0x0000 - 0x0FFF
			{
				return false;
				break;
			}
			case 0x2:  // accumulator. there is no address
				return false;

			case 0x3:  // absolute
			{
				return false;
				break;
			}
			case 0x4:  // No instruction here
			{
				return false;
				break;
			}
			case 0x5:  // zero page, x
			{		   // STX, LDX use zero page, y not x
				return false;
				break;
			}
			case 0x6: {
				return false;
				break;
			}
			case 0x7:  // absolute, x
			{		   // THIS could be more compact but i don't want to mess with the pagecross
				// logic
				address = read_abs_address(aux_pc);
				if (inst.opcode == 0xBE) {
					int high_byte = address >> 8;  // We get the first 4 bits -> one digit in Hexa
					address += Y;
					int high_byte_after_increment = address >> 8;
					if (high_byte < high_byte_after_increment) page_cross = true;
					break;
				} else {
					int high_byte = address >> 8;  // We get the first 4 bits -> one digit in Hexa
					address += X;
					int high_byte_after_increment = address >> 8;
					if (high_byte < high_byte_after_increment) page_cross = true;
				}
				return page_cross;
				break;
			}
		}
	}
	return false;
}

int estimate_cycles_group_sb1() {
	switch (inst.opcode) {
		case 0x08:
			return 3;
		case 0x28:
			return 4;
		case 0x48:
			return 3;
		case 0x68:
			return 4;
		case 0x88:
			return 2;
		case 0xA8:
			return 2;
		case 0xC8:
			return 2;
			break;
		case 0xE8:
			return 2;
			break;
		case 0x18:
			return 2;
			break;
		case 0x38:
			return 2;
			break;
		case 0x58:
			return 2;
			break;
		case 0x78:
			return 2;
			break;
		case 0x98:
			return 2;
			break;
		case 0xB8:
			return 2;
			break;
		case 0xD8:
			return 2;
		case 0xF8:
			return 2;
			break;
		default:
			//std::cerr << "ILLEGAL";
			return 2;
	}
}

int estimate_cycles_group_sb2() {
	return 2;
}

int estimate_cycles_group_1() {
	const int lookup[] = {6, 3, 2, 4, 5, 4, 4, 4};
	switch (inst.aaa) {
		case 0x0:
			return lookup[inst.bbb];
		case 0x1:
			return lookup[inst.bbb];
		case 0x2:
			return lookup[inst.bbb];
		case 0x3:
			return lookup[inst.bbb];
		case 0x4: {
			const int lookup2[] = {6, 3, -1, 4, 6, 4, 5, 5};
			return lookup2[inst.bbb];
		}
		case 0x5:
			return lookup[inst.bbb];
		case 0x6:
			return lookup[inst.bbb];
		case 0x7:
			return lookup[inst.bbb];
	}
	return 2;
}

int estimate_cycles_group_2() {
	const int lookup[] = {-1, 5, 2, 6, -1, 6, -1, 7};
	switch (inst.aaa) {
		case 0x0:
			return lookup[inst.bbb];
		case 0x1:
			return lookup[inst.bbb];
		case 0x2:
			return lookup[inst.bbb];
		case 0x3:
			return lookup[inst.bbb];
		case 0x4: {
			const int lookup2[] = {-1, 3, -1, 4, -1, 4, -1, -1};
			return lookup2[inst.bbb];
		}
		case 0x5: {
			const int lookup2[] = {2, 3, -1, 4, -1, 4, -1, 4};
			return lookup2[inst.bbb];
		}
		case 0x6: {
			const int lookup2[] = {-1, 5, -1, 6, -1, 6, -1, 7};
			return lookup2[inst.bbb];
		}
		case 0x7:
			const int lookup2[] = {-1, 5, -1, 6, -1, 6, -1, 7};
			return lookup2[inst.bbb];
	}
	return 2;
}

int estimate_cycles_group_3() {
	uint16_t jump_address = 0;
	const int lookup[] = {2, 3, -1, 4, -1, 4, -1, 4};
	switch (inst.aaa) {
		case 0x0:
			// printf("INVALID OPCODE \n");
			break;
		case 0x1:
			return lookup[inst.bbb];
		case 0x2:
			return 3;
		case 0x3:
			return 5;
		case 0x4:
			return lookup[inst.bbb];
			break;
		case 0x5:
			return lookup[inst.bbb];
			break;
		case 0x6:
			return lookup[inst.bbb];
			break;
		case 0x7:
			return lookup[inst.bbb];
			break;
	}
	return 2;
}

int estimate_cycles() {
	byte last_5_bits = (0b00011111 & inst.opcode);
	byte low_nibble = inst.opcode & 0x0F;
	byte high_nibble = inst.opcode >> 4;
	bool page_cross = false;
	if (inst.opcode == 0x04 || inst.opcode == 0x44 || inst.opcode == 0x64) {
		return 3;
	} else if (inst.opcode == 0x0C) {
		return 4;
	} else if (inst.opcode == 0x14 || inst.opcode == 0x34 || inst.opcode == 0x54 ||
			   inst.opcode == 0x74 || inst.opcode == 0xD4 || inst.opcode == 0xF4) {
		return 4;
	}

	else if (low_nibble == 0x00 && last_5_bits == 0b00010000) {
		// Branching instructions
		// read but dont increment!
		uint16_t aux_pc2 = PC;
		int8_t branch_position = (int8_t)read(aux_pc2);
		aux_pc2++;
		bool branch_succeded = false;
		bool page_cross = false;
		uint16_t aux_pc = PC;
		if (inst.xx == 0b00 && N == inst.y)
			branch_succeded = true;
		else if (inst.xx == 0b01 && O == inst.y)
			branch_succeded = true;
		else if (inst.xx == 0b10 && C == inst.y)
			branch_succeded = true;
		else if (inst.xx == 0b11 && Z == inst.y)
			branch_succeded = true;

		if (branch_succeded) {
			aux_pc2 += branch_position;
			page_cross = (aux_pc & 0xFF00) != (aux_pc2 & 0xFF00);
		}
		return (2 + (int)branch_succeded + 2 * (int)page_cross);
	}

	else if (inst.opcode == 0x00) {
		if(I == 0) return 7;
		else return 2;
		// brk = forced interrupt.
	} else if (inst.opcode == 0x20) {
		return 6;
	} else if (inst.opcode == 0x40) {
		return 6;
	} else if (inst.opcode == 0x60) {
		return 6;
	}
	// for single byte instructions !
	else if (low_nibble == 0x08) {
		return estimate_cycles_group_sb1();
	} else if (low_nibble == 0x0A && high_nibble >= 0x08) {
		return estimate_cycles_group_sb2();
	} else {
		switch (inst.cc) {
			// compute_addr_mode DOES return an address via reffrence(&)
			case 0x01:	// cc = 1
				page_cross = estimate_page_cross_g1();
				return estimate_cycles_group_1() + (int)page_cross;
			case 0x02:	// cc = 10
				// Will return address via pointer, the function returns a boolean.
				page_cross = estimate_page_cross_g23();
				return estimate_cycles_group_2() + (int)page_cross;	 // Not accumulator, on address
				break;
			case 0x0:  // cc = 00
				page_cross = estimate_page_cross_g23();
				return estimate_cycles_group_3() + (int)page_cross;
				break;
		}
	}
	return 2;//assume next instruction will take 2 clocks in case o undefined opcode
}
