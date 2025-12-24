#include "cpu_proc.h"
#include "cpu_instructions.h"
#include "cpu_utils.h"
#include <cstdint>

void execute_ld(Instruction inst) {
    switch (inst.addr_mode) {
        case addr_mode::REG16_IMM16: {
            cpu.write_reg16(inst.reg_1, fetch16());
            break;
        }
        case addr_mode::MEM_REG16_REG8: {
            bus_write(read_reg16(inst.reg_1), read_reg8(inst.reg_2));
            break;
        }
        case addr_mode::REG8_MEM_REG16: {
            write_reg8(inst.reg_1, bus_read(read_reg16(inst.reg_2)));
            break;
        }
        case addr_mode::MEM_IMM16_REG16: {
            bus_write16(fetch16(), read_reg16(inst.reg_2));
            break;
        }
        case addr_mode::REG8_IMM8: {
            write_reg8(inst.reg_1, fetch8());
            break;
        }
        case addr_mode::REG8_REG8: {
            write_reg8(inst.reg_1, read_reg8(inst.reg_2));
            break;
        }
        case addr_mode::REG16_SP_IMM8: {
            uint16_t reg_value = read_reg16(inst.reg_2);
            uint8_t imm = fetch8();
            uint16_t result = reg_value + imm;
            cpu.F = 0;
            if (is_carry_add(reg_value, imm)) {
                cpu.F |= FLAG_C;
            }
            if (is_half_carry_add(reg_value, imm)) {
                cpu.F |= FLAG_H;
            }
            write_reg16(inst.reg_1, result);
            break;
        }
        case addr_mode::REG16_REG16: {
            write_reg16(inst.reg_1, read_reg16(inst.reg_2));
            break;
        }

        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}

void execute_inc(Instruction inst) {
    switch (inst.addr_mode) {
        case addr_mode::REG16: {
            break;
        }
        case addr_mode::REG8: {
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}

void execute_dec(Instruction inst) {
    switch (inst.addr_mode) {
        case addr_mode::REG16: {
            break;
        }
        case addr_mode::REG8: {
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}

void execute_add(Instruction inst) {
    switch (inst.addr_mode) {
        case addr_mode::REG16_REG16: {
            break;
        }
        case addr_mode::REG8_REG8: {
            break;
        }
        case addr_mode::REG8_IMM8: {
            break;
        }
        case addr_mode::REG16_IMM8: {
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}

void execute_sub(Instruction inst) {
    switch (inst.addr_mode) {
        case addr_mode::REG8_REG8: {
            break;
        }
        case addr_mode::REG8_IMM8: {
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}

void execute_rlca(Instruction);
void execute_rrca(Instruction);
void execute_rla(Instruction);
void execute_rra(Instruction);

// Flags
void execute_daa(Instruction);
void execute_cpl(Instruction);
void execute_scf(Instruction);
void execute_ccf(Instruction);

// Control flow
void execute_jr(Instruction);
void execute_jp(Instruction);
void execute_call(Instruction);
void execute_ret(Instruction);
void execute_reti(Instruction);
void execute_rst(Instruction);

// Stack
void execute_push(Instruction);
void execute_pop(Instruction);

// CPU state
void execute_halt(Instruction);
void execute_stop(Instruction);
void execute_di(Instruction);
void execute_ei(Instruction);

// CB-prefixed
void execute_cb(uint8_t cb_opcode);