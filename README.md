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
The code is written in a BYTE list that is sent to the interpreter.
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