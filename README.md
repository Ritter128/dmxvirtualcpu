# Description
A virtual CPU for learning low level programming and hardware emulation

# Running the virtual CPU
To compile

        gcc dmx.c -o DMX

To run

        ./DMX

# Programming in DMX Assembly
I do not have full documentation of the whole instruction set.
However, the set differs very little from most assembly languages (e.g. x86, ARM, 6502 etc.).
This will be a brief overiew to the language.

Macros, that represent opcodes, are used to form instructions.
The code is written in a byte list that is sent to the interpreter.
Each macro is one byte long.  

Example:

    BYTE memory[1024] = {
        mvi, a, 0x05,
        mvi, b, 0x05,
        push, a,
        mvi, a, 0x88,
        pop, d,

    // end command. Not crucial, but handy since it puts less stress on the interpeter to not have to read all 1024 bytes!
        0xff, 
    };

    read_instructions(memory, &instruction_counter);


# DMX architecture 
There are alphabetically named four registers: A, B, C, and D.
They are all one byte in size, can be manipulated by code.  

The processor has a 16 byte stack. "push" and "pop" commands can be used to save registers onto the stack.  
Example:

    mvi a, 0x44, 
    push, a,
    mvi a, 0x77,
    pop, b,

    0xff
    // A: 0x77, B: 0x44

The processor also has different addressing modes for the mov command:  
1. IMMEDIATE; move value into register (mvi, d, 0x23)
2. DELEGATE; move register value into another register (mvd, a, b)
3. ACCUMULATE move value from memory into register (mva, a, 0x01, 0x1A)


The memory is organized like a program, inspired by the structure x86 programs. It is a bit messy:

    /*
    -- PROGRAM
      256 bytes
    -- ENDPROGRAM
    -- STACK
      16 bytes
    -- ENDSTACK
    -- DATA
      239 bytes
    -- ENDDATA
    -- SYSTEM
      512 bytes
    -- ENDSYSTEM
    */

    // I am not going to lie, sections are a mess right now

    struct s_Section {
      WORD start;
      BYTE startHighByte;
      BYTE startLowByte;
    
      WORD end;
      BYTE endHighByte;
      BYTE endLowByte;
    };
    
    struct s_Disk {
      struct s_Section PROGRAM_SECT; 
      struct s_Section STACK_SECT;   
      struct s_Section DATA_SECT;   
      struct s_Section SYSTEM_SECT; 
    };
    
    struct s_Disk DISK_FORMAT = {
      (struct s_Section){0x0000, 0x00, 0x00, 0x00FF, 0x00, 0xFF},
      (struct s_Section){0x0100, 0x01, 0x00, 0x010F, 0x01, 0x0F},
      (struct s_Section){0x0110, 0x01, 0x10, 0x01FF, 0x01, 0xFF},
      (struct s_Section){0x0200, 0x02, 0x00, 0x0400, 0x04, 0x00},
    };

