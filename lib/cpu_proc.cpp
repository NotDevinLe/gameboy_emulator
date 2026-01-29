#include "cpu_proc.h"
#include "cpu_instructions.h"
#include "cpu_utils.h"
#include "cpu.h"
#include "bus.h"
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include "stack.h"

void execute_nop(Instruction) {
    // NOP does nothing
}

void execute_ld(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG16_IMM16: {
            write_reg16(inst.reg_1, fetch16());
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
            // LD HL,SP+e8 (opcode 0xF8):
            // e8 is a signed 8-bit immediate. Result is stored in HL.
            // Flags: Z=0, N=0, H/C from low-byte addition of SP and e8.
            uint16_t sp = read_reg16(inst.reg_2); // SP
            int8_t offset = static_cast<int8_t>(fetch8());
            uint16_t result = static_cast<uint16_t>(static_cast<int32_t>(sp) + static_cast<int32_t>(offset));

            cpu.F = 0;
            uint8_t sp_lo = static_cast<uint8_t>(sp & 0x00FF);
            uint8_t off_u = static_cast<uint8_t>(offset);
            if (((sp_lo & 0x0F) + (off_u & 0x0F)) > 0x0F) {
                cpu.F |= FLAG_H;
            }
            if (static_cast<uint16_t>(sp_lo) + static_cast<uint16_t>(off_u) > 0xFF) {
                cpu.F |= FLAG_C;
            }

            write_reg16(inst.reg_1, result);
            break;
        }
        case addr_mode::REG16_REG16: {
            write_reg16(inst.reg_1, read_reg16(inst.reg_2));
            break;
        }
        case addr_mode::MEM_HLI_REG8: {
            uint16_t addr = read_reg16(reg_type::HL);
            bus_write(addr, read_reg8(inst.reg_2));
            write_reg16(reg_type::HL, static_cast<uint16_t>(addr + 1));
            break;
        }
        case addr_mode::REG8_MEM_HLI: {
            uint16_t addr = read_reg16(reg_type::HL);
            write_reg8(inst.reg_1, bus_read(addr));
            write_reg16(reg_type::HL, static_cast<uint16_t>(addr + 1));
            break;
        }
        case addr_mode::MEM_HLD_REG8: {
            uint16_t addr = read_reg16(reg_type::HL);
            bus_write(addr, read_reg8(inst.reg_2));
            write_reg16(reg_type::HL, static_cast<uint16_t>(addr - 1));
            break;
        }
        case addr_mode::REG8_MEM_HLD: {
            uint16_t addr = read_reg16(reg_type::HL);
            write_reg8(inst.reg_1, bus_read(addr));
            write_reg16(reg_type::HL, static_cast<uint16_t>(addr - 1));
            break;
        }
        case addr_mode::MEM_REG16_IMM8: {
            uint16_t addr = read_reg16(inst.reg_1);
            bus_write(addr, fetch8());
            break;
        }
        case addr_mode::MEM_FF00_IMM8_REG8: {
            uint8_t imm = fetch8();
            uint16_t addr = static_cast<uint16_t>(0xFF00 + imm);
            bus_write(addr, read_reg8(inst.reg_2));
            break;
        }
        case addr_mode::REG8_MEM_FF00_IMM8: {
            uint8_t imm = fetch8();
            uint16_t addr = static_cast<uint16_t>(0xFF00 + imm);
            uint8_t value = bus_read(addr);
            write_reg8(inst.reg_1, value);
            break;
        }
        case addr_mode::MEM_FF00_C_REG8: {
            uint16_t addr = static_cast<uint16_t>(0xFF00 + cpu.C);
            bus_write(addr, read_reg8(inst.reg_2));
            break;
        }
        case addr_mode::REG8_MEM_FF00_C: {
            uint16_t addr = static_cast<uint16_t>(0xFF00 + cpu.C);
            write_reg8(inst.reg_1, bus_read(addr));
            break;
        }
        case addr_mode::REG8_MEM_IMM16: {
            uint16_t addr = fetch16();
            write_reg8(inst.reg_1, bus_read(addr));
            break;
        }
        case addr_mode::MEM_IMM16_REG8: {
            uint16_t addr = fetch16();
            bus_write(addr, read_reg8(inst.reg_1));
            break;
        }

        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_inc(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG16: {
            uint16_t reg_value = read_reg16(inst.reg_1);
            uint16_t result = reg_value + 1;
            write_reg16(inst.reg_1, result);
            break;
        }
        case addr_mode::REG8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t result = static_cast<uint8_t>(reg_value + 1);
            write_reg8(inst.reg_1, result);

            // INC r: Z set if result == 0, N reset, H from bit 3 carry, C preserved
            uint8_t f = cpu.F & FLAG_C; // preserve carry only
            if (result == 0) {
                f |= FLAG_Z;
            } else {
                // Explicitly clear Z if result != 0
                f &= ~FLAG_Z;
            }
            if (is_half_carry_add(reg_value, 1)) {
                f |= FLAG_H;
            } else {
                f &= ~FLAG_H;
            }
            // N is 0 for INC (already clear from f &= FLAG_C above)
            cpu.F = f;
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t result = value + 1;
            bus_write(addr, result);

            // INC (HL): same flags as INC r
            uint8_t f = cpu.F & FLAG_C; // preserve carry only
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_add(value, 1)) {
                f |= FLAG_H;
            }
            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_dec(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG16: {
            uint16_t reg_value = read_reg16(inst.reg_1);
            uint16_t result = reg_value - 1;
            write_reg16(inst.reg_1, result);
            break;
        }
        case addr_mode::REG8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t result = static_cast<uint8_t>(reg_value - 1);

            // DEC r: Z set if result == 0, N set, H from borrow on bit 4, C preserved
            uint8_t f = cpu.F & FLAG_C; // preserve carry only
            if (result == 0) {
                f |= FLAG_Z;
            } else {
                f &= ~FLAG_Z; // Explicitly clear Z if result != 0
            }
            if (is_half_carry_sub(reg_value, 1)) {
                f |= FLAG_H;
            } else {
                f &= ~FLAG_H; // Explicitly clear H if no half-carry
            }
            f |= FLAG_N;
            cpu.F = f;
            write_reg8(inst.reg_1, result);
            
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t result = value - 1;
            bus_write(addr, result);

            // DEC (HL): same flags as DEC r
            uint8_t f = cpu.F & FLAG_C; // preserve carry only
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_sub(value, 1)) {
                f |= FLAG_H;
            }
            f |= FLAG_N;
            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_add(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG16_REG16: {
            uint16_t reg_value_1 = read_reg16(inst.reg_1);
            uint16_t reg_value_2 = read_reg16(inst.reg_2);
            uint16_t result = reg_value_1 + reg_value_2;
            write_reg16(inst.reg_1, result);

            // ADD HL,rr: N reset, H from bit 11 carry, C from bit 15 carry, Z unaffected
            uint8_t f = cpu.F & FLAG_Z; // preserve Z only
            f &= ~FLAG_N;
            if (is_half_carry_add16_12(reg_value_1, reg_value_2)) {
                f |= FLAG_H;
            }
            if (is_carry_add16(reg_value_1, reg_value_2)) {
                f |= FLAG_C;
            }
            cpu.F = f;
            break;
        }
        case addr_mode::REG16_IMM8: {
            // This mode is used for ADD SP, r8 (opcode 0xE8).
            // r8 is a signed immediate; flags are computed from the low byte addition:
            // Z=0, N=0, H/C from carries out of bit 3/7 of the low byte.
            //
            // Note: LD HL,SP+e8 (opcode 0xF8) is handled separately in execute_ld (REG16_SP_IMM8).
            int8_t imm = static_cast<int8_t>(fetch8());
            uint16_t sp = read_reg16(inst.reg_1); // expected SP
            uint16_t result = static_cast<uint16_t>(static_cast<int32_t>(sp) + static_cast<int32_t>(imm));

            cpu.F = 0;
            uint16_t uimm = static_cast<uint16_t>(static_cast<int16_t>(imm)) & 0x00FF;
            if (((sp & 0x000F) + (uimm & 0x000F)) > 0x000F) {
                cpu.F |= FLAG_H;
            }
            if (((sp & 0x00FF) + (uimm & 0x00FF)) > 0x00FF) {
                cpu.F |= FLAG_C;
            }

            write_reg16(inst.reg_1, result);
            break;
        }
        case addr_mode::REG8_REG8: {
            uint8_t reg_value_1 = read_reg8(inst.reg_1);
            uint8_t reg_value_2 = read_reg8(inst.reg_2);
            uint8_t result = reg_value_1 + reg_value_2;
            write_reg8(inst.reg_1, result);

            // 8-bit ADD: Z from result, N reset, H/C from carry, other flags cleared
            uint8_t f = 0;
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_add(reg_value_1, reg_value_2)) {
                f |= FLAG_H;
            }
            if (is_carry_add(reg_value_1, reg_value_2)) {
                f |= FLAG_C;
            }
            cpu.F = f;
            break;
        }
        case addr_mode::REG8_MEM_REG16: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint16_t addr = read_reg16(inst.reg_2);
            uint8_t value = bus_read(addr);
            uint8_t result = reg_value + value;
            write_reg8(inst.reg_1, result);

            uint8_t f = 0;
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_add(reg_value, value)) {
                f |= FLAG_H;
            }
            if (is_carry_add(reg_value, value)) {
                f |= FLAG_C;
            }
            cpu.F = f;
            break;
        }
        case addr_mode::REG8_IMM8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t imm = fetch8();
            uint8_t result = reg_value + imm;
            write_reg8(inst.reg_1, result);

            uint8_t f = 0;
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_add(reg_value, imm)) {
                f |= FLAG_H;
            }
            if (is_carry_add(reg_value, imm)) {
                f |= FLAG_C;
            }
            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_sub(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG8_REG8: {
            uint8_t reg_value_1 = read_reg8(inst.reg_1);
            uint8_t reg_value_2 = read_reg8(inst.reg_2);
            uint8_t result = reg_value_1 - reg_value_2;
            write_reg8(inst.reg_1, result);

            // 8-bit SUB: Z from result, N set, H/C from borrow, all from scratch
            uint8_t f = FLAG_N;
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_sub(reg_value_1, reg_value_2)) {
                f |= FLAG_H;
            }
            if (is_carry_sub(reg_value_1, reg_value_2)) {
                f |= FLAG_C;
            }
            cpu.F = f;
            break;
        }
        case addr_mode::REG8_IMM8: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint8_t imm = fetch8();
            uint8_t result = reg_value - imm;
            write_reg8(inst.reg_1, result);

            uint8_t f = FLAG_N;
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_sub(reg_value, imm)) {
                f |= FLAG_H;
            }
            if (is_carry_sub(reg_value, imm)) {
                f |= FLAG_C;
            }
            cpu.F = f;
            break;
        }
        case addr_mode::REG8_MEM_REG16: {
            uint8_t reg_value = read_reg8(inst.reg_1);
            uint16_t addr = read_reg16(inst.reg_2);
            uint8_t value = bus_read(addr);
            uint8_t result = reg_value - value;
            write_reg8(inst.reg_1, result);

            uint8_t f = FLAG_N;
            if (result == 0) {
                f |= FLAG_Z;
            }
            if (is_half_carry_sub(reg_value, value)) {
                f |= FLAG_H;
            }
            if (is_carry_sub(reg_value, value)) {
                f |= FLAG_C;
            }
            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_adc(Instruction inst) {
    uint8_t src = 0;
    switch (inst.mode) {
        case addr_mode::REG8_REG8:
            src = read_reg8(inst.reg_2);
            break;
        case addr_mode::REG8_MEM_REG16:
            src = bus_read(read_reg16(inst.reg_2));
            break;
        case addr_mode::REG8_IMM8:
            src = fetch8();
            break;
        default:
            return;
    }
    
    uint8_t a = cpu.A;
    bool carry_in = (cpu.F & FLAG_C) != 0;
    uint16_t result = static_cast<uint16_t>(a) + static_cast<uint16_t>(src) + (carry_in ? 1 : 0);
    cpu.A = static_cast<uint8_t>(result & 0xFF);
    
    cpu.F = 0;
    if (cpu.A == 0) cpu.F |= FLAG_Z;
    if (((a & 0x0F) + (src & 0x0F) + (carry_in ? 1 : 0)) > 0x0F) cpu.F |= FLAG_H;
    if (result > 0xFF) cpu.F |= FLAG_C;
}

void execute_sbc(Instruction inst) {
    uint8_t src = 0;
    switch (inst.mode) {
        case addr_mode::REG8_REG8:
            src = read_reg8(inst.reg_2);
            break;
        case addr_mode::REG8_MEM_REG16:
            src = bus_read(read_reg16(inst.reg_2));
            break;
        case addr_mode::REG8_IMM8:
            src = fetch8();
            break;
        default:
            return;
    }
    
    uint8_t a = cpu.A;
    bool carry_in = (cpu.F & FLAG_C) != 0;
    int result = static_cast<int>(a) - static_cast<int>(src) - (carry_in ? 1 : 0);
    cpu.A = static_cast<uint8_t>(result & 0xFF);
    
    cpu.F = FLAG_N;
    if (cpu.A == 0) cpu.F |= FLAG_Z;
    if (((a & 0x0F) - (src & 0x0F) - (carry_in ? 1 : 0)) < 0) cpu.F |= FLAG_H;
    if (result < 0) cpu.F |= FLAG_C;
}

void execute_and(Instruction inst) {
    uint8_t src = 0;
    switch (inst.mode) {
        case addr_mode::REG8_REG8:
            src = read_reg8(inst.reg_2);
            break;
        case addr_mode::REG8_MEM_REG16:
            src = bus_read(read_reg16(inst.reg_2));
            break;
        case addr_mode::REG8_IMM8:
            src = fetch8();
            break;
        default:
            return;
    }
    
    cpu.A &= src;
    cpu.F = FLAG_H;
    if (cpu.A == 0) cpu.F |= FLAG_Z;
}

void execute_xor(Instruction inst) {
    uint8_t src = 0;
    switch (inst.mode) {
        case addr_mode::REG8_REG8:
            src = read_reg8(inst.reg_2);
            break;
        case addr_mode::REG8_MEM_REG16:
            src = bus_read(read_reg16(inst.reg_2));
            break;
        case addr_mode::REG8_IMM8:
            src = fetch8();
            break;
        default:
            return;
    }
    
    cpu.A ^= src;
    cpu.F = 0;
    if (cpu.A == 0) cpu.F |= FLAG_Z;
}

void execute_or(Instruction inst) {
    uint8_t src = 0;
    switch (inst.mode) {
        case addr_mode::REG8_REG8:
            src = read_reg8(inst.reg_2);
            break;
        case addr_mode::REG8_MEM_REG16:
            src = bus_read(read_reg16(inst.reg_2));
            break;
        case addr_mode::REG8_IMM8:
            src = fetch8();
            break;
        default:
            return;
    }
    
    cpu.A |= src;
    cpu.F = 0;
    if (cpu.A == 0) cpu.F |= FLAG_Z;
}

void execute_cp(Instruction inst) {
    uint8_t src = 0;
    switch (inst.mode) {
        case addr_mode::REG8_REG8:
            src = read_reg8(inst.reg_2);
            break;
        case addr_mode::REG8_MEM_REG16:
            src = bus_read(read_reg16(inst.reg_2));
            break;
        case addr_mode::REG8_IMM8:
            src = fetch8();
            break;
        default:
            return;
    }
    
    uint8_t a = cpu.A;
    uint8_t result = a - src;
    
    cpu.F = FLAG_N;
    if (result == 0) cpu.F |= FLAG_Z;
    if ((a & 0x0F) < (src & 0x0F)) cpu.F |= FLAG_H;
    if (a < src) cpu.F |= FLAG_C;
}

void execute_ldh(Instruction inst) {
    // LDH is handled as LD with MEM_FF00 modes
    execute_ld(inst);
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
    uint8_t a = cpu.A;
    uint8_t f = cpu.F;

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

    cpu.A = a;

    f &= FLAG_N;
    if (a == 0) f |= FLAG_Z;
    if (carry_out) f |= FLAG_C;

    cpu.F = f;
}

void execute_cpl(Instruction) {
    cpu.A = ~cpu.A;

    cpu.F |= FLAG_N;
    cpu.F |= FLAG_H;
}

void execute_scf(Instruction) {
    // SCF: Clear N and H, preserve Z, set C
    uint8_t z = cpu.F & FLAG_Z;
    cpu.F = z | FLAG_C;
}

void execute_ccf(Instruction) {
    // CCF: Clear N and H, preserve Z, toggle C
    uint8_t z = cpu.F & FLAG_Z;
    uint8_t c = (~cpu.F) & FLAG_C;
    cpu.F = z | c;
}

void execute_jr(Instruction inst) {
    int8_t offset = fetch8();
    switch (inst.cond) {
        case cond_type::CT_NONE: {
            cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            break;
        }
        case cond_type::CT_Z: {
            if (cpu.F & FLAG_Z) {
                cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            }
            break;
        }
        case cond_type::CT_NZ: {
            if (!(cpu.F & FLAG_Z)) {
                cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            }
            break;
        }
        case cond_type::CT_C: {
            if (cpu.F & FLAG_C) {
                cpu.PC = static_cast<uint16_t>(cpu.PC + offset);
            }
            break;
        }
        case cond_type::CT_NC: {
            if (!(cpu.F & FLAG_C)) {
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

void execute_jp(Instruction inst) {
    bool should_jump = false;
    
    if (inst.mode == addr_mode::REG16) {
        // JP (HL) - unconditional jump to address in HL
        uint16_t addr = read_reg16(inst.reg_1);
        // std::printf("JP (HL): jumping to %04X (HL=%04X)\n", addr, read_reg16(reg_type::HL));
        cpu.PC = addr;
        return;
    }
    
    uint16_t addr = fetch16();
    switch (inst.cond) {
        case cond_type::CT_NONE:
            should_jump = true;
            break;
        case cond_type::CT_Z:
            should_jump = (cpu.F & FLAG_Z) != 0;
            break;
        case cond_type::CT_NZ:
            should_jump = (cpu.F & FLAG_Z) == 0;
            break;
        case cond_type::CT_C:
            should_jump = (cpu.F & FLAG_C) != 0;
            break;
        case cond_type::CT_NC:
            should_jump = (cpu.F & FLAG_C) == 0;
            break;
        default:
            std::printf("Unknown condition: %d\n", static_cast<int>(inst.cond));
            return;
    }
    
    if (should_jump) {
        cpu.PC = addr;
    }
}
void execute_call(Instruction inst) {
    uint16_t addr = fetch16();
    bool should_call = false;
    
    switch (inst.cond) {
        case cond_type::CT_NONE:
            should_call = true;
            break;
        case cond_type::CT_Z:
            should_call = (cpu.F & FLAG_Z) != 0;
            break;
        case cond_type::CT_NZ:
            should_call = (cpu.F & FLAG_Z) == 0;
            break;
        case cond_type::CT_C:
            should_call = (cpu.F & FLAG_C) != 0;
            break;
        case cond_type::CT_NC:
            should_call = (cpu.F & FLAG_C) == 0;
            break;
        default:
            std::printf("Unknown condition: %d\n", static_cast<int>(inst.cond));
            return;
    }
    
    if (should_call) {
        uint16_t ret_addr = cpu.PC;
        cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
        bus_write16(cpu.SP, ret_addr);
        // std::printf("CALL: jumping to %04X, return addr %04X, SP=%04X\n", addr, ret_addr, cpu.SP);
        cpu.PC = addr;
    }
}

void execute_ret(Instruction inst) {
    bool should_ret = false;
    
    switch (inst.cond) {
        case cond_type::CT_NONE:
            should_ret = true;
            break;
        case cond_type::CT_Z:
            should_ret = (cpu.F & FLAG_Z) != 0;
            break;
        case cond_type::CT_NZ:
            should_ret = (cpu.F & FLAG_Z) == 0;
            break;
        case cond_type::CT_C:
            should_ret = (cpu.F & FLAG_C) != 0;
            break;
        case cond_type::CT_NC:
            should_ret = (cpu.F & FLAG_C) == 0;
            break;
        default:
            return;
    }
    
    if (should_ret) {
        uint16_t ret_addr = bus_read16(cpu.SP);
        // std::printf("RET: returning to %04X from SP=%04X\n", ret_addr, cpu.SP);
        cpu.SP = static_cast<uint16_t>(cpu.SP + 2);
        cpu.PC = ret_addr;
    }
}

void execute_reti(Instruction) {
    uint16_t ret_addr = bus_read16(cpu.SP);
    cpu.SP = static_cast<uint16_t>(cpu.SP + 2);
    cpu.PC = ret_addr;
    cpu.ime = true;
}

void execute_rst(Instruction inst) {
    uint8_t rst_vec = inst.param;
    uint16_t ret_addr = cpu.PC;
    cpu.SP = static_cast<uint16_t>(cpu.SP - 2);
    bus_write16(cpu.SP, ret_addr);
    cpu.PC = rst_vec;
}

void execute_push(Instruction inst) {
    uint16_t reg_value = read_reg16(inst.reg_1);
    stack_push(reg_value);
}

void execute_pop(Instruction inst) {
    uint16_t reg_value = stack_pop();
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

void execute_rlc(Instruction inst) {
    switch (inst.mode) {
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

            cpu.F = f;
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

            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_rrc(Instruction inst) {
    switch (inst.mode) {
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

            cpu.F = f;
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

            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_rl(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t a = read_reg8(inst.reg_1);
            uint8_t old_c = (cpu.F & FLAG_C) ? 1 : 0;
            uint8_t new_c = (a >> 7) & 1;
            uint8_t result = static_cast<uint8_t>((a << 1) | old_c);
            write_reg8(inst.reg_1, result);
            uint8_t f = 0;

            if (new_c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            cpu.F = f;
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t old_c = (cpu.F & FLAG_C) ? 1 : 0;
            uint8_t new_c = (value >> 7) & 1;
            uint8_t result = static_cast<uint8_t>((value << 1) | old_c);
            bus_write(addr, result);
            uint8_t f = 0;

            if (new_c) {
                f |= FLAG_C;
            }
            if (result == 0) {
                f |= FLAG_Z;
            }

            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_rr(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t a = read_reg8(inst.reg_1);
            uint8_t old_c = (cpu.F & FLAG_C) ? 1 : 0;
            uint8_t new_c = a & 1;
            uint8_t result = static_cast<uint8_t>((a >> 1) | (old_c << 7));
            write_reg8(inst.reg_1, result);
            uint8_t f = 0;

            if (new_c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            cpu.F = f;
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t old_c = (cpu.F & FLAG_C) ? 1 : 0;
            uint8_t new_c = value & 1;
            uint8_t result = static_cast<uint8_t>((value >> 1) | (old_c << 7));
            bus_write(addr, result);
            uint8_t f = 0;

            if (new_c) {
                f |= FLAG_C;
            }
            if (result == 0) {
                f |= FLAG_Z;
            }

            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_sla(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t a = read_reg8(inst.reg_1);
            uint8_t c = (a >> 7) & 1;
            uint8_t result = a << 1;
            write_reg8(inst.reg_1, result);
            uint8_t f = 0;

            if (c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            cpu.F = f;
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t c = (value >> 7) & 1;
            uint8_t result = value << 1;
            bus_write(addr, result);
            uint8_t f = 0;

            if (c) {
                f |= FLAG_C;
            }

            if (result == 0) {
                f |= FLAG_Z;
            }

            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_sra(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t v = read_reg8(inst.reg_1);
            uint8_t c = v & 1;
            uint8_t result = static_cast<uint8_t>((v >> 1) | (v & 0x80));
            write_reg8(inst.reg_1, result);

            uint8_t f = 0;
            if (result == 0) f |= FLAG_Z;
            if (c)          f |= FLAG_C;
            cpu.F = f;
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t v = bus_read(addr);
            uint8_t c = v & 1;
            uint8_t result = static_cast<uint8_t>((v >> 1) | (v & 0x80));
            bus_write(addr, result);

            uint8_t f = 0;
            if (result == 0) f |= FLAG_Z;
            if (c)          f |= FLAG_C;
            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_swap(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t a = read_reg8(inst.reg_1);
            uint8_t result = (a << 4) | (a >> 4);
            write_reg8(inst.reg_1, result);
            uint8_t f = 0;
            if (result == 0) f |= FLAG_Z;
            cpu.F = f;
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t result = (value << 4) | (value >> 4);
            bus_write(addr, result);
            uint8_t f = 0;
            if (result == 0) f |= FLAG_Z;
            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_srl(Instruction inst) {
    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t a = read_reg8(inst.reg_1);
            uint8_t c = a & 1;
            uint8_t result = a >> 1;
            write_reg8(inst.reg_1, result);
            uint8_t f = 0;
            if (result == 0) f |= FLAG_Z;
            if (c) f |= FLAG_C;
            cpu.F = f;
            break;
        }
        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t value = bus_read(addr);
            uint8_t c = value & 1;
            uint8_t result = value >> 1;
            bus_write(addr, result);
            uint8_t f = 0;
            if (result == 0) f |= FLAG_Z;
            if (c) f |= FLAG_C;
            cpu.F = f;
            break;
        }
        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_bit(const Instruction& inst, uint8_t bit_to_check) {
    uint8_t mask = static_cast<uint8_t>(1u << (bit_to_check & 7));

    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t v = read_reg8(inst.reg_1);
            uint8_t test = static_cast<uint8_t>(v & mask);

            uint8_t f = cpu.F & FLAG_C;
            f |= FLAG_H;
            if (test == 0) f |= FLAG_Z;
            cpu.F = f;
            break;
        }

        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t v = bus_read(addr);
            uint8_t test = static_cast<uint8_t>(v & mask);

            uint8_t f = cpu.F & FLAG_C;
            f |= FLAG_H;
            if (test == 0) f |= FLAG_Z;
            cpu.F = f;
            break;
        }

        default: {
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
        }
    }
}

void execute_res(const Instruction& inst, uint8_t bit) {
    uint8_t mask = static_cast<uint8_t>(~(1u << (bit & 7)));

    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t v = read_reg8(inst.reg_1);
            write_reg8(inst.reg_1, static_cast<uint8_t>(v & mask));
            break;
        }

        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t v = bus_read(addr);
            bus_write(addr, static_cast<uint8_t>(v & mask));
            break;
        }

        default:
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
    }
}

void execute_set(const Instruction& inst, uint8_t bit) {
    uint8_t mask = static_cast<uint8_t>(1u << (bit & 7));

    switch (inst.mode) {
        case addr_mode::REG8: {
            uint8_t v = read_reg8(inst.reg_1);
            write_reg8(inst.reg_1, static_cast<uint8_t>(v | mask));
            break;
        }

        case addr_mode::MEM_REG16: {
            uint16_t addr = read_reg16(inst.reg_1);
            uint8_t v = bus_read(addr);
            bus_write(addr, static_cast<uint8_t>(v | mask));
            break;
        }

        default:
            std::printf("Unknown address mode: %d\n", static_cast<int>(inst.mode));
            break;
    }
}