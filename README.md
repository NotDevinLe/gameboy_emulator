# gameboy_emulator

This is an attempt to create a gameboy emulator. Took inspiration of project layout from LowLevelDevel.

cpu_instructions.cpp - File to decode opcode -> instruction object. This allows us to reduce redundant code.

cpu.cpp - main cpu file that runs everything and takes cpu steps.

cpu_utils.cpp - helper functions for executing instructions. This lets us read from the cpu/memory and check flag conditions.

cpu_proc.cpp - functions for executing instructions.

main.cpp - takes in a ROM and runs the cpu.

bus.cpp - handles the memory read and writes throughout the emulator.

cart.cpp - loads in a cartridge and gets the ROM data.

For the memory bank controller I only implement mbc1 and rom only since most games only utilize that. Eventually will implement mbc5 for support of pokemon.

Jan 29 - All functionality for the Blargg's CPU test should be implemented now. However, there are some weird bugs that are causing it to loop infinitely. Created automated logging of opcode and register values and compare against SameBoy implementation to see where instructions diverges. Re-Implementing interrupts as that could be the source of issue as well.