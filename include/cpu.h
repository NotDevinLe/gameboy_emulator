#pragma once
#include <cstdint>

struct cpu_state {
    uint8_t A, F, B, C, D, E, H, L;
    uint16_t PC, SP;
    bool ime, halt, stop;
    // Monotonic instruction counter (increments once per executed instruction).
    // Used for lightweight timing approximations (e.g., LY).
    uint64_t instr_count;
};

extern cpu_state cpu;

constexpr uint8_t FLAG_Z = 0x80; // Zero
constexpr uint8_t FLAG_N = 0x40; // Subtract
constexpr uint8_t FLAG_H = 0x20; // Half-carry
constexpr uint8_t FLAG_C = 0x10; // Carry

void cpu_init();
bool cpu_step();