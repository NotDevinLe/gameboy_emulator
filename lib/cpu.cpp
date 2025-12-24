#include "cpu.h"
#include "cpu_instructions.cpp"
#include "bus.h"
#include <cstdio>
#include <cstdint>

struct cpu_state {
    uint8_t A, F, B, C, D, E, H, L;
    uint16_t PC, SP;
    bool ime, halt;
};

static cpu_state cpu;

constexpr uint8_t FLAG_Z = 0x80; // Zero
constexpr uint8_t FLAG_N = 0x40; // Subtract
constexpr uint8_t FLAG_H = 0x20; // Half-carry
constexpr uint8_t FLAG_C = 0x10; // Carry

void cpu_init() {
    cpu.A = 0;
    cpu.F = 0;
    cpu.B = 0;
    cpu.C = 0;
    cpu.D = 0;
    cpu.E = 0;
    cpu.H = 0;
    cpu.L = 0;

    cpu.SP = 0xFFFE;
    cpu.PC = 0x0100;

    cpu.ime = false;
    cpu.halt = false;
}

bool cpu_step() {
    uint16_t pc_before = cpu.PC;
    uint8_t op = bus_read(cpu.PC);
    cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
    std::printf("PC=%04X OP=%02X\n", pc_before, op);

    bool is_cb = false;

    if (op == 0xCB) {
        is_cb = true;
        op = bus_read(cpu.PC);
        cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
    }

    if (op < 0x40) {
        Instruction inst = decode(op, is_cb);
    } else if (op < 0x80) {
        Instruction inst = decode(op, is_cb);
    } else if (op < 0xB0) {
        Instruction inst = decode(op, is_cb);
    } else {
        Instruction inst = decode(op, is_cb);
    }

    return true;
}