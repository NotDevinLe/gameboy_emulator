# gameboy_emulator

This is an attempt to create a gameboy emulator. Took inspiration of project layout from LowLevelDevel.

cpu_instructions.cpp - File to decode opcode -> instruction object. This allows us to reduce redundant code.

cpu.cpp - main cpu file that runs everything and takes cpu steps.

cpu_utils.cpp - helper functions for executing instructions. This lets us read from the cpu/memory and check flag conditions.

cpu_proc.cpp - functions for executing instructions.

main.cpp - takes in a ROM and runs the cpu.

bus.cpp - handles the memory read and writes throughout the emulator.

cart.cpp - loads in a cartridge and gets the ROM data.
