# gameboy_emulator

This is an attempt to create a gameboy emulator. Took inspiration of project layout from LowLevelDevel.


Dec 23rd: I'm currently working on the CPU. Finished decoding opcodes. Given some opcode, I can figure out how the memory is being accessed and which registers are used as well as the command that should be executed. I now have to implement some helper functions that will help me execute the instructions.

cpu_instructions.cpp - File for opcode -> instruction object.

cpu.cpp - main cpu file that runs everything and takes cpu steps.

cpu_utils.cpp - helper functions for executing instructions.

cpu_proc.cpp - functions for executing instructions.

main.cpp - takes in a ROM and runs the cpu.

bus.cpp - handles the memory read and writes throughout the emulator.

cart.cpp - loads in a cartridge and gets the ROM data.
