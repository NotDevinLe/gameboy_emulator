#include "cpu_utils.h"
#include "cpu_instructions.h"
#include "cpu.h"
#include "bus.h"
#include <cstdint>

uint8_t read_reg8(reg_type r) {
    switch (r) {
        case reg_type::A: return cpu.A;
        case reg_type::B: return cpu.B;
        case reg_type::C: return cpu.C;
        case reg_type::D: return cpu.D;
        case reg_type::E: return cpu.E;
        case reg_type::H: return cpu.H;
        case reg_type::L: return cpu.L;
        default:
            return 0; // Invalid for 8-bit registers
    }
}

void write_reg8(reg_type r, uint8_t v) {
    switch (r) {
        case reg_type::A: cpu.A = v; break;
        case reg_type::B: cpu.B = v; break;
        case reg_type::C: cpu.C = v; break;
        case reg_type::D: cpu.D = v; break;
        case reg_type::E: cpu.E = v; break;
        case reg_type::H: cpu.H = v; break;
        case reg_type::L: cpu.L = v; break;
        default:
            break; // Invalid for 8-bit registers
    }
}

uint16_t read_reg16(reg_type r) {
    switch (r) {
        case reg_type::BC: return (static_cast<uint16_t>(cpu.B) << 8) | static_cast<uint16_t>(cpu.C);
        case reg_type::DE: return (static_cast<uint16_t>(cpu.D) << 8) | static_cast<uint16_t>(cpu.E);
        case reg_type::HL: return (static_cast<uint16_t>(cpu.H) << 8) | static_cast<uint16_t>(cpu.L);
        case reg_type::SP: return static_cast<uint16_t>(cpu.SP);
        case reg_type::AF: return (static_cast<uint16_t>(cpu.A) << 8) | static_cast<uint16_t>(cpu.F & 0xF0);
        default:
            return 0; // Invalid for 16-bit registers
    }
}
void write_reg16(reg_type r, uint16_t v) {
    switch (r) {
        case reg_type::BC: cpu.B = static_cast<uint8_t>(v >> 8); cpu.C = static_cast<uint8_t>(v & 0xFF); break;
        case reg_type::DE: cpu.D = static_cast<uint8_t>(v >> 8); cpu.E = static_cast<uint8_t>(v & 0xFF); break;
        case reg_type::HL: cpu.H = static_cast<uint8_t>(v >> 8); cpu.L = static_cast<uint8_t>(v & 0xFF); break;
        case reg_type::SP: cpu.SP = static_cast<uint16_t>(v); break;
        case reg_type::AF:
            cpu.A = static_cast<uint8_t>(v >> 8);
            cpu.F = static_cast<uint8_t>(v & 0xF0); // Lower nibble always 0
            break;
        default:
            break; // Invalid for 16-bit registers
    }
}

uint8_t fetch8() {
    uint8_t value = bus_read(cpu.PC);
    cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
    return value;
}

uint16_t fetch16() {
    uint8_t lo = bus_read(cpu.PC);
    cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
    uint8_t hi = bus_read(cpu.PC);
    cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
    return (static_cast<uint16_t>(hi) << 8) | static_cast<uint16_t>(lo);
}

bool is_carry_add(uint8_t a, uint8_t b) {
    return (uint16_t)a + (uint16_t)b > 0xFF;
}

bool is_half_carry_add(uint8_t a, uint8_t b) {
    return ((a & 0x0F) + (b & 0x0F)) > 0x0F;
}

bool is_carry_sub(uint8_t a, uint8_t b) {
    return a < b;
}

bool is_half_carry_sub(uint8_t a, uint8_t b) {
    return (a & 0x0F) < (b & 0x0F);
}