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

# The brain of the emulator (CPU)

Jan 29 - All functionality for the Blargg's CPU test should be implemented now. However, there are some weird bugs that are causing it to loop infinitely. Created automated logging of opcode and register values and compare against SameBoy implementation to see where instructions diverges. Re-Implementing interrupts as that could be the source of issue as well.

Jan 30 - Rewrote timer logic. Added 100% accurate T-Cycle tracking through returning it via cpu_step(). Was originally running into a timer divergence error. Interrupts are most likely still wrong. Need to fix IO handling.

Blargg's CPU tests: it seems that there are some expectations of a false PPU. Similar to timer divergences, I suspect threre are also PPU timer divergences. The PPU has it's own timer called the LY register. However, for these tests, LY shouldn't depend on any of the PPU just yet, it will only be affected by the LCDC bit. We also need a minimum V-Blank logic implementation.

Feb 1 - Wrote my own debugging pipeline that checks registers, flags, and memory writes for discrepancies. Found several in the code. It's impossible to figure out where my gameboy implementation diverges from the SameBoy, so I'm going to use another gameboy cpu testing github **Game Boy CPU (SM83) Tests**. 

Feb 21 - Finally figured out the issue in the code was for my LD function where I was loading in the memory address of IMM16 into itself instead of from a register. All tests work now and I can work on the PPU after setting up UI.

# Getting Graphics to Display

There are 4 main components for this part.

Firstly, the Object Attribute Memory (OAM) is a 160 byte memory block that can store up to 40 sprites. It stores the (x,y) position of the sprite and flags such as the priority (should it be drawn first or last?) and flip (which direction it faces). 

Secondly, the Direct Memory Access (DMA) is a part of the gameboy that moves data into your OAM. This is necessary because the CPU is really busy and having this concurrent process allows for less workload on it.

Thirdly, the Pixel Processing Unit (PPU) is what turns memory into actual pixels. Before getting into what the PPU is, there are a few useful definitions here. 

Gameboy screen is 160 pixels wide by 144 pixels tall.
Scanline - Horizontal row of pixels.
T-cycles/dots - A count for how long each operation takes. Different operations can take different number of cycles (and can even depend on whether it suceeds at the actions such as a jump with a condition)

A full frame consists of 154 scanlines. 144 of them are the actual scanlines where the remainder 10 are basically a rest period for the graphics so that the CPU can update the VRAM which stores graphics information.

4 modes of the PPU:
    Mode 2 (OAM Scan dots 0-79): Checks which [0,10] sprites in OAM belong on the current scanline.
    Mode 3 (Draw Pixels dots 80-251): Fetch data from vram and oam to figure out which colors go on screen
    Mode 0 (H-Blank dots 252-455): PPU finished drawing a row. Move pointer back to left side for next row. 
    Mode 1 (V-Blank): Lets the PPU rest for 10 scanlines.

Finally, we have the renderer. This is going to be done with SDL2. 

March 2 - I've implemented most of the PPU. I just need to add window rendering (HUD for games). I also need to fix the interrupts and add proper timing.