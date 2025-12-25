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
            uint16_t reg_value = read_reg16(inst.reg_1);
            uint16_t result = reg_value + 1;
            write_reg16(inst.reg_1, result);
            break;
        }
        case addr_mode::REG8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t result = reg_value + 1;
            write_reg8(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f &= ~FLAG_N;

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_half_carry_add(reg_value, 1)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t result = value + 1;
            bus_write(addr, result);

            uint8_t f = read_reg8(reg_type::F);
            f |= FLAG_N;
            
            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_half_carry_add(value, 1)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
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
            uint16_t reg_value = read_reg16(inst.reg_1);
            uint16_t result = reg_value - 1;
            write_reg16(inst.reg_1, result);
            break;
        }
        case addr_mode::REG8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t result = reg_value - 1;

            uint8_t f = read_reg8(reg_type::F);

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_half_carry_sub(reg_value, 1)) {
                f |= FLAG_H;
            }

            f |= FLAG_N;

            write_reg8(reg_type::F, f);
            write_reg8(inst.reg_1, result);
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t result = value - 1;
            bus_write(addr, result);

            uint8_t f = read_reg8(reg_type::F);
            f |= FLAG_N;

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_half_carry_sub(value, 1)) {
                f |= FLAG_H;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
            }
        }
    }
}

void execute_add(Instruction inst) {
    switch (inst.addr_mode) {
        case addr_mode::REG16_REG16: {
            uint16_t reg_value_1 = read_reg16(inst.reg_1);
            uint16_t reg_value_2 = read_reg16(inst.reg_2);
            uint16_t result = reg_value_1 + reg_value_2;
            write_reg16(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f &= ~FLAG_N;

            if (is_carry_add(reg_value_1, reg_value_2)) {
                f |= FLAG_C;
            }
            if (is_half_carry_add(reg_value_1, reg_value_2)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::REG8_REG8: {
            uint8_t reg_value_1 = read_reg8(inst.reg_1);
            uint8_t reg_value_2 = read_reg8(inst.reg_2);
            uint8_t result = reg_value_1 + reg_value_2;
            write_reg8(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f &= ~FLAG_N;

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_carry_add(reg_value_1, reg_value_2)) {
                f |= FLAG_C;
            }
            if (is_half_carry_add(reg_value_1, reg_value_2)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::REG8_MEM_REG16: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint16_t addr = read_reg16(inst.reg_2);
            uint8_t value = bus_read(addr);
            uint8_t result = reg_value + value;
            write_reg8(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f &= ~FLAG_N;

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_carry_add(reg_value, value)) {
                f |= FLAG_C;
            }
            if (is_half_carry_add(reg_value, value)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::REG8_IMM8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t imm = fetch8();
            uint8_t result = reg_value + imm;
            write_reg8(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f &= ~FLAG_N;

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_carry_add(reg_value, imm)) {
                f |= FLAG_C;
            }
            if (is_half_carry_add(reg_value, imm)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
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
            uint8_t reg_value_1 = read_reg8(inst.reg_1);
            uint8_t reg_value_2 = read_reg8(inst.reg_2);
            uint8_t result = reg_value_1 - reg_value_2;
            write_reg8(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f |= FLAG_N;

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_carry_sub(reg_value_1, reg_value_2)) {
                f |= FLAG_C;
            }
            if (is_half_carry_sub(reg_value_1, reg_value_2)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::REG8_IMM8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t imm = fetch8();
            uint8_t result = reg_value - imm;
            write_reg8(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f &= ~FLAG_N;

            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_carry_sub(reg_value, imm)) {
                f |= FLAG_C;
            }
            if (is_half_carry_sub(reg_value, imm)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::REG8_MEM_REG16: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint16_t addr = read_reg16(inst.reg_2);
            uint8_t value = bus_read(addr);
            uint8_t result = reg_value - value;
            write_reg8(inst.reg_1, result);

            uint8_t f = read_reg8(reg_type::F);
            f &= ~FLAG_N;
            
            if (result == 0) {
                f |= FLAG_Z;
            }

            if (is_carry_sub(reg_value, value)) {
                f |= FLAG_C;
            }
            if (is_half_carry_sub(reg_value, value)) {
                f |= FLAG_H;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}

void execute_rlca(Instruction) {
    uint8_t a = cpu.A;
    uint8_t c = (a >> 7) & 1;

    cpu.A = (a << 1) | c;

    cpu.F = 0;

    if (c) {
        cpu.F |= FLAG_C;
    }
}

void execute_rrca(Instruction) {
    uint8_t a = cpu.A;
    uint8_t c = a & 1;

    cpu.A = (a >> 1) | (c << 7);

    cpu.F = 0;

    if (c) {
        cpu.F |= FLAG_C;
    }
}

void execute_rla(Instruction) {
    uint8_t a = cpu.A;
    uint8_t old_c = cpu.F & FLAG_C;
    uint8_t new_c = (a >> 7) & 1;

    cpu.A = (a << 1) | old_c;

    cpu.F = 0;

    if (new_c) {
        cpu.F |= FLAG_C;
    }
}

void execute_rra(Instruction) {
    uint8_t a = cpu.A;
    uint8_t old_c = (cpu.F & FLAG_C) ? 1 : 0;
    uint8_t new_c = a & 1;

    cpu.A = (a >> 1) | (old_c << 7);

    cpu.F = 0;

    if (new_c) {
        cpu.F |= FLAG_C;
    }
}

void execute_daa(Instruction) {
    uint8_t a = read_reg8(reg_type::A);
    uint8_t f = read_reg8(reg_type::F);

    uint8_t correction = 0;
    bool carry_out = false;

    const bool n = (f & FLAG_N) != 0;
    const bool h = (f & FLAG_H) != 0;
    const bool c = (f & FLAG_C) != 0;

    if (!n) {
        if (h || (a & 0x0F) > 9) {
            correction |= 0x06;
        }
        if (c || a > 0x99) {
            correction |= 0x60;
            carry_out = true;
        }
        a = static_cast<uint8_t>(a + correction);
    } else {
        if (h) correction |= 0x06;
        if (c) correction |= 0x60;
        a = static_cast<uint8_t>(a - correction);
        carry_out = c;
    }

    write_reg8(reg_type::A, a);

    f &= FLAG_N;
    if (a == 0) f |= FLAG_Z;
    if (carry_out) f |= FLAG_C;

    write_reg8(reg_type::F, f);
}

void execute_cpl(Instruction) {
    uint8_t a = read_reg8(reg_type::A);
    a = ~a;
    write_reg8(reg_type::A, a);

    uint8_t f = read_reg8(reg_type::F);
    f |= FLAG_N;
    f |= FLAG_H;

    write_reg8(reg_type::F, f);
}

void execute_scf(Instruction) {
    uint8_t f = read_reg8(reg_type::F);
    f &= FLAG_Z;
    f |= FLAG_C;
    write_reg8(reg_type::F, f);
}

void execute_ccf(Instruction) {
    uint8_t f = read_reg8(reg_type::F);

    uint8_t z = f & FLAG_Z;
    uint8_t c = (~f) & FLAG_C;

    write_reg8(reg_type::F, z | c);
}

void execute_jr(Instruction) {
    int8_t offset = fetch8();
    switch (inst.cond) {
        case cond_type::CT_NONE: {
            cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            break;
        }
        case cond_type::CT_Z: {
            if (read_reg8(reg_type::F) & FLAG_Z) {
                cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            }
            break;
        }
        case cond_type::CT_NZ: {
            if (!(read_reg8(reg_type::F) & FLAG_Z)) {
                cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            }
            break;
        }
        case cond_type::CT_C: {
            if (read_reg8(reg_type::F) & FLAG_C) {
                cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            }
            break;
        }
        case cond_type::CT_NC: {
            if (!(read_reg8(reg_type::F) & FLAG_C)) {
                cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            }
            break;
        }
        default: {
            std::printf("Unknown condition: %d\n", inst.cond);
            break;
        }
    }
}

void execute_jp(Instruction) {
    uint16_t addr = fetch16();
    switch (inst.cond) {
        case cond_type::CT_NONE: {
            cpu.PC = addr;
            break;
        }
        case cond_type::CT_Z: {
            if (read_reg8(reg_type::F) & FLAG_Z) {
                cpu.PC = addr;
            }
            break;
        }
        case cond_type::CT_NZ: {
            if (!(read_reg8(reg_type::F) & FLAG_Z)) {
                cpu.PC = addr;
            }
            break;
        }
        case cond_type::CT_C: {
            if (read_reg8(reg_type::F) & FLAG_C) {
                cpu.PC = addr;
            }
            break;
        }
        case cond_type::CT_NC: {
            if (!(read_reg8(reg_type::F) & FLAG_C)) {
                cpu.PC = addr;
            }
            break;
        }
        default: {
            std::printf("Unknown condition: %d\n", inst.cond);
            break;
        }
    }
}
void execute_call(Instruction) {
    uint16_t addr = fetch16();
    switch (inst.cond) {
        case cond_type::CT_NONE: {
            cpu.PC = addr;
            break;
        }
        case cond_type::CT_Z: {
            if (read_reg8(reg_type::F) & FLAG_Z) {
                uint16_t ret_addr = cpu.PC;
                bus_write16(cpu.SP - 2, ret_addr);
                cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
                cpu.PC = addr;
            }
            break;
        }
        case cond_type::CT_NZ: {
            if (!(read_reg8(reg_type::F) & FLAG_Z)) {
                uint16_t ret_addr = cpu.PC;
                bus_write16(cpu.SP - 2, ret_addr);
                cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
                cpu.PC = addr;
            }
            break;
        }
        case cond_type::CT_C: {
            if (read_reg8(reg_type::F) & FLAG_C) {
                uint16_t ret_addr = cpu.PC;
                bus_write16(cpu.SP - 2, ret_addr);
                cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
                cpu.PC = addr;
            }
            break;
        }
        case cond_type::CT_NC: {
            if (!(read_reg8(reg_type::F) & FLAG_C)) {
                uint16_t ret_addr = cpu.PC;
                bus_write16(cpu.SP - 2, ret_addr);
                cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
                cpu.PC = addr;
            }
            break;
        }
        default: {
            std::printf("Unknown condition: %d\n", inst.cond);
            break;
        }
    }
}

void execute_ret(Instruction) {
    uint16_t ret_addr = bus_read16(cpu.SP);
    cpu.SP = static_cast<uint16_t>(cpu.SP + 2);
    cpu.PC = ret_addr;
}

void execute_reti(Instruction) {
    uint16_t ret_addr = bus_read16(cpu.SP);
    cpu.SP = static_cast<uint16_t>(cpu.SP + 2);
    cpu.PC = ret_addr;
    cpu.ime = true;
}

void execute_rst(Instruction) {
    uint8_t rst_vec = inst.param;
    uint16_t ret_addr = cpu.PC;
    bus_write16(cpu.SP - 2, ret_addr);
    cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
    cpu.PC = rst_vec;
}

void execute_push(Instruction) {
    uint16_t reg_value = read_reg16(inst.reg_1);
    bus_write16(cpu.SP - 2, reg_value);
    cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
}

void execute_pop(Instruction) {
    uint16_t reg_value = bus_read16(cpu.SP);
    cpu.SP = static_cast<uint16_t>(cpu.SP + 2);
    write_reg16(inst.reg_1, reg_value);
}

void execute_halt(Instruction) {
    cpu.halt = true;
}
void execute_stop(Instruction) {
    cpu.stop = true;
}
void execute_di(Instruction) {
    cpu.ime = false;
}
void execute_ei(Instruction) {
    cpu.ime = true;
}

void execute_rlc(Instruction) {
    switch (inst.addr_mode) {
        case addr_mode::REG8: {
            uint8_t a = read_reg8(inst.reg_1);
            uint8_t c = (a >> 7) & 1;
            uint8_t result = (a << 1) | c;
            write_reg8(inst.reg_1, result);
            uint8_t f = 0;

            if (c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t c = (value >> 7) & 1;
            uint8_t result = (value << 1) | c;
            bus_write(addr, result);
            uint8_t f = 0;

            if (c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}

void execute_rrc(Instruction) {
    switch (inst.addr_mode) {
        case addr_mode::REG8: {
            uint8_t a = read_reg8(inst.reg_1);
            uint8_t c = a & 1;
            uint8_t result = (a >> 1) | (c << 7);
            write_reg8(inst.reg_1, result);
            uint8_t f = 0;

            if (c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t c = value & 1;
            uint8_t result = (value >> 1) | (c << 7);
            bus_write(addr, result);
            uint8_t f = 0;

            if (c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            write_reg8(reg_type::F, f);
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", inst.addr_mode);
            break;
        }
    }
}