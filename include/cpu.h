#pragma once
#include <cstdint>

struct cpu_state {
    uint8_t A, F, B, C, D, E, H, L;
    uint16_t PC, SP;
    bool ime, halt, stop;
    bool enabling_ime;  // EI delays IME enabling by one instruction
    uint8_t IF, IE;  // Interrupt flags: IF (0xFF0F) and IE (0xFFFF) are 8-bit registers
    // Monotonic instruction counter (increments once per executed instruction).
    // Used for lightweight timing approximations (e.g., LY).
    uint64_t instr_count;
    // Cycle counter (increments by instruction cycle count).
    // More accurate than instruction count for timing (LY, DIV, etc.)
    // Average instruction takes ~4 cycles, so we approximate with 4 cycles per instruction.
    uint64_t cycle_count;
};

extern cpu_state cpu;

constexpr uint8_t FLAG_Z = 0x80; // Zero
constexpr uint8_t FLAG_N = 0x40; // Subtract
constexpr uint8_t FLAG_H = 0x20; // Half-carry
constexpr uint8_t FLAG_C = 0x10; // Carry

void cpu_init();
uint8_t cpu_step();