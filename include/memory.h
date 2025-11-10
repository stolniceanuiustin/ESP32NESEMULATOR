#ifndef MEMORY_HEADERGUARD
#define MEMORY_HEADERGUARD

#include <Arduino.h>
#include <stdint.h>
#include <cstring>
#include <stdio.h>

typedef uint8_t byte;

class Memory
{
private:
    byte *memory;
    size_t size;

public:
    // Constructor
    Memory(size_t size) : size(size)
    {
        if (size == 0 || size > 65536)
        {
            Serial.println("Error: Invalid memory size!");
            memory = nullptr;
            return;
        }

        memory = new byte[size];
        if (!memory)
        {
            Serial.println("Error: Memory allocation failed!");
            return;
        }
         Serial.println(" Memory allocation Succsesful!");
        // Zero initialize memory
        memset(memory, 0, size);
    }

    // Delete copy assignment
    Memory &operator=(const Memory &) = delete;

    // Destructor
    ~Memory()
    {
        if (memory)
        {
            delete[] memory;
            memory = nullptr;
        }
    }

    // Access operator with bounds check
    byte &operator[](uint16_t address)
    {
        if (!memory)
        {
            Serial.println("Error: Memory not allocated!");
            while (1)
                ;
        }

        if (address < size)
        {
            return memory[address];
        }
        else
        {
            Serial.printf("Out of bounds memory access at %u (size=%u)\n", address, (unsigned)size);
            while (1)
                ;
        }
    }

    // Reset memory to zero
    void reset()
    {
        if (memory)
            memset(memory, 0, size);
    }

    // Hexdump to file on SD/LittleFS (optional)
    void hexdump(const char *filename, uint16_t dump_size)
    {
        if (!memory) return;

        FILE *file = fopen(filename, "wb");
        if (file)
        {
            fwrite(memory, sizeof(byte), dump_size, file);
            fclose(file);
        }
        else
        {
            Serial.println("Could not open file for hexdump");
        }
    }
};

#endif
