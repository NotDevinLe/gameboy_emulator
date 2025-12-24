#include "cpu_instructions.h"
#include <cstdint>
#include <array>

static constexpr Instruction I(
    in_type t,
    addr_mode m = addr_mode::IMPLIED,
    reg_type r1 = reg_type::NONE,
    reg_type r2 = reg_type::NONE,
    cond_type c = cond_type::CT_NONE,
    uint8_t p = 0
) {
    return Instruction{t, m, r1, r2, c, p};
}

static constexpr reg_type kReg8[8] = {
    reg_type::B, reg_type::C, reg_type::D, reg_type::E,
    reg_type::H, reg_type::L, reg_type::HL, reg_type::A
};

static constexpr reg_type kReg16[4] = {
    reg_type::BC, reg_type::DE, reg_type::HL, reg_type::SP
};

const std::array<Instruction, 256> kInstrTable = [] {
    std::array<Instruction, 256> t{};

    t[0x00] = I(in_type::IN_NOP, addr_mode::IMPLIED);
    t[0x01] = I(in_type::IN_LD, addr_mode::REG16_IMM16, reg_type::BC, reg_type::IMM16);
    t[0x02] = I(in_type::IN_LD, addr_mode::MEM_REG16_REG8, reg_type::BC, reg_type::A);
    t[0x03] = I(in_type::IN_INC, addr_mode::REG16, reg_type::BC);
    t[0x04] = I(in_type::IN_INC, addr_mode::REG8, reg_type::B);
    t[0x05] = I(in_type::IN_DEC, addr_mode::REG8, reg_type::B);
    t[0x06] = I(in_type::IN_LD, addr_mode::REG8_IMM8, reg_type::B, reg_type::IMM8);
    t[0x07] = I(in_type::IN_RLCA, addr_mode::IMPLIED);
    t[0x08] = I(in_type::IN_LD, addr_mode::MEM_IMM16_REG16, reg_type::IMM16, reg_type::SP);
    t[0x09] = I(in_type::IN_ADD, addr_mode::REG16_REG16, reg_type::HL, reg_type::BC);
    t[0x0A] = I(in_type::IN_LD, addr_mode::REG8_MEM_REG16, reg_type::A, reg_type::BC);
    t[0x0B] = I(in_type::IN_DEC, addr_mode::REG16, reg_type::BC);
    t[0x0C] = I(in_type::IN_INC, addr_mode::REG8, reg_type::C);
    t[0x0D] = I(in_type::IN_DEC, addr_mode::REG8, reg_type::C);
    t[0x0E] = I(in_type::IN_LD, addr_mode::REG8_IMM8, reg_type::C, reg_type::IMM8);
    t[0x0F] = I(in_type::IN_RRCA, addr_mode::IMPLIED);

    t[0x10] = I(in_type::IN_STOP, addr_mode::IMPLIED);
    t[0x11] = I(in_type::IN_LD, addr_mode::REG16_IMM16, reg_type::DE, reg_type::IMM16);
    t[0x12] = I(in_type::IN_LD, addr_mode::MEM_REG16_REG8, reg_type::DE, reg_type::A);
    t[0x13] = I(in_type::IN_INC, addr_mode::REG16, reg_type::DE);
    t[0x14] = I(in_type::IN_INC, addr_mode::REG8, reg_type::D);
    t[0x15] = I(in_type::IN_DEC, addr_mode::REG8, reg_type::D);
    t[0x16] = I(in_type::IN_LD, addr_mode::REG8_IMM8, reg_type::D, reg_type::IMM8);
    t[0x17] = I(in_type::IN_RLA, addr_mode::IMPLIED);
    t[0x18] = I(in_type::IN_JR, addr_mode::REL8);
    t[0x19] = I(in_type::IN_ADD, addr_mode::REG16_REG16, reg_type::HL, reg_type::DE);
    t[0x1A] = I(in_type::IN_LD, addr_mode::REG8_MEM_REG16, reg_type::A, reg_type::DE);
    t[0x1B] = I(in_type::IN_DEC, addr_mode::REG16, reg_type::DE);
    t[0x1C] = I(in_type::IN_INC, addr_mode::REG8, reg_type::E);
    t[0x1D] = I(in_type::IN_DEC, addr_mode::REG8, reg_type::E);
    t[0x1E] = I(in_type::IN_LD, addr_mode::REG8_IMM8, reg_type::E, reg_type::IMM8);
    t[0x1F] = I(in_type::IN_RRA, addr_mode::IMPLIED);

    t[0x20] = I(in_type::IN_JR, addr_mode::REL8, reg_type::NONE, reg_type::NONE, cond_type::CT_NZ);
    t[0x21] = I(in_type::IN_LD, addr_mode::REG16_IMM16, reg_type::HL, reg_type::IMM16);
    t[0x22] = I(in_type::IN_LD, addr_mode::MEM_HLI_REG8, reg_type::HL, reg_type::A);
    t[0x23] = I(in_type::IN_INC, addr_mode::REG16, reg_type::HL);
    t[0x24] = I(in_type::IN_INC, addr_mode::REG8, reg_type::H);
    t[0x25] = I(in_type::IN_DEC, addr_mode::REG8, reg_type::H);
    t[0x26] = I(in_type::IN_LD, addr_mode::REG8_IMM8, reg_type::H, reg_type::IMM8);
    t[0x27] = I(in_type::IN_DAA, addr_mode::IMPLIED);
    t[0x28] = I(in_type::IN_JR, addr_mode::REL8, reg_type::NONE, reg_type::NONE, cond_type::CT_Z);
    t[0x29] = I(in_type::IN_ADD, addr_mode::REG16_REG16, reg_type::HL, reg_type::HL);
    t[0x2A] = I(in_type::IN_LD, addr_mode::REG8_MEM_HLI, reg_type::A, reg_type::HL);
    t[0x2B] = I(in_type::IN_DEC, addr_mode::REG16, reg_type::HL);
    t[0x2C] = I(in_type::IN_INC, addr_mode::REG8, reg_type::L);
    t[0x2D] = I(in_type::IN_DEC, addr_mode::REG8, reg_type::L);
    t[0x2E] = I(in_type::IN_LD, addr_mode::REG8_IMM8, reg_type::L, reg_type::IMM8);
    t[0x2F] = I(in_type::IN_CPL, addr_mode::IMPLIED);

    t[0x30] = I(in_type::IN_JR, addr_mode::REL8, reg_type::NONE, reg_type::NONE, cond_type::CT_NC);
    t[0x31] = I(in_type::IN_LD, addr_mode::REG16_IMM16, reg_type::SP, reg_type::IMM16);
    t[0x32] = I(in_type::IN_LD, addr_mode::MEM_HLD_REG8, reg_type::NONE, reg_type::A);
    t[0x33] = I(in_type::IN_INC, addr_mode::REG16, reg_type::SP);
    t[0x34] = I(in_type::IN_INC, addr_mode::MEM_REG16, reg_type::HL);
    t[0x35] = I(in_type::IN_DEC, addr_mode::MEM_REG16, reg_type::HL);
    t[0x36] = I(in_type::IN_LD, addr_mode::MEM_REG16_IMM8, reg_type::HL, reg_type::IMM8);
    t[0x37] = I(in_type::IN_SCF, addr_mode::IMPLIED);
    t[0x38] = I(in_type::IN_JR, addr_mode::REL8, reg_type::NONE, reg_type::NONE, cond_type::CT_C);
    t[0x39] = I(in_type::IN_ADD, addr_mode::REG16_REG16, reg_type::HL, reg_type::SP);
    t[0x3A] = I(in_type::IN_LD, addr_mode::REG8_MEM_HLD, reg_type::A, reg_type::NONE);
    t[0x3B] = I(in_type::IN_DEC, addr_mode::REG16, reg_type::SP);
    t[0x3C] = I(in_type::IN_INC, addr_mode::REG8, reg_type::A);
    t[0x3D] = I(in_type::IN_DEC, addr_mode::REG8, reg_type::A);
    t[0x3E] = I(in_type::IN_LD, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0x3F] = I(in_type::IN_CCF, addr_mode::IMPLIED);

    t[0xC0] = I(in_type::IN_RET, addr_mode::IMPLIED, reg_type::NONE, reg_type::NONE, cond_type::CT_NZ);
    t[0xC1] = I(in_type::IN_POP, addr_mode::REG16, reg_type::BC);
    t[0xC2] = I(in_type::IN_JP, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_NZ);
    t[0xC3] = I(in_type::IN_JP, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16);
    t[0xC4] = I(in_type::IN_CALL, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_NZ);
    t[0xC5] = I(in_type::IN_PUSH, addr_mode::REG16, reg_type::BC);
    t[0xC6] = I(in_type::IN_ADD, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xC7] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x00);
    t[0xC8] = I(in_type::IN_RET, addr_mode::IMPLIED, reg_type::NONE, reg_type::NONE, cond_type::CT_Z);
    t[0xC9] = I(in_type::IN_RET, addr_mode::IMPLIED);
    t[0xCA] = I(in_type::IN_JP, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_Z);
    t[0xCC] = I(in_type::IN_CALL, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_Z);
    t[0xCD] = I(in_type::IN_CALL, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16);
    t[0xCE] = I(in_type::IN_ADC, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xCF] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x08);

    t[0xD0] = I(in_type::IN_RET, addr_mode::IMPLIED, reg_type::NONE, reg_type::NONE, cond_type::CT_NC);
    t[0xD1] = I(in_type::IN_POP, addr_mode::REG16, reg_type::DE);
    t[0xD2] = I(in_type::IN_JP, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_NC);
    t[0xD4] = I(in_type::IN_CALL, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_NC);
    t[0xD5] = I(in_type::IN_PUSH, addr_mode::REG16, reg_type::DE);
    t[0xD6] = I(in_type::IN_SUB, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xD7] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x10);
    t[0xD8] = I(in_type::IN_RET, addr_mode::IMPLIED, reg_type::NONE, reg_type::NONE, cond_type::CT_C);
    t[0xD9] = I(in_type::IN_RETI, addr_mode::IMPLIED, reg_type::NONE, reg_type::NONE);
    t[0xDA] = I(in_type::IN_JP, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_C);
    t[0xDC] = I(in_type::IN_CALL, addr_mode::ABS16, reg_type::NONE, reg_type::IMM16, cond_type::CT_C);
    t[0xDE] = I(in_type::IN_SBC, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xDF] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x18);

    t[0xE0] = I(in_type::IN_LD, addr_mode::MEM_FF00_IMM8_REG8, reg_type::IMM8, reg_type::A);
    t[0xE1] = I(in_type::IN_POP, addr_mode::REG16, reg_type::HL);
    t[0xE2] = I(in_type::IN_LD, addr_mode::MEM_FF00_C_REG8, reg_type::C, reg_type::A);
    t[0xE5] = I(in_type::IN_PUSH, addr_mode::REG16, reg_type::HL);
    t[0xE6] = I(in_type::IN_AND, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xE7] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x20);
    t[0xE8] = I(in_type::IN_ADD, addr_mode::REG16_IMM8, reg_type::SP, reg_type::IMM8);
    t[0xE9] = I(in_type::IN_JP, addr_mode::REG16, reg_type::HL);
    t[0xEA] = I(in_type::IN_LD, addr_mode::MEM_IMM16_REG8, reg_type::IMM16, reg_type::A);
    t[0xEE] = I(in_type::IN_XOR, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xEF] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x28);

    t[0xF0] = I(in_type::IN_LD, addr_mode::REG8_MEM_FF00_IMM8, reg_type::A, reg_type::IMM8);
    t[0xF1] = I(in_type::IN_POP, addr_mode::REG16, reg_type::AF);
    t[0xF2] = I(in_type::IN_LD, addr_mode::REG8_MEM_FF00_C, reg_type::A, reg_type::C);
    t[0xF3] = I(in_type::IN_DI, addr_mode::IMPLIED);
    t[0xF5] = I(in_type::IN_PUSH, addr_mode::REG16, reg_type::AF);
    t[0xF6] = I(in_type::IN_OR, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xF7] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x30);
    t[0xF8] = I(in_type::IN_LD, addr_mode::REG16_SP_IMM8, reg_type::HL, reg_type::SP);
    t[0xF9] = I(in_type::IN_LD, addr_mode::REG16_REG16, reg_type::SP, reg_type::HL);
    t[0xFA] = I(in_type::IN_LD, addr_mode::REG8_MEM_IMM16, reg_type::A, reg_type::IMM16);
    t[0xFB] = I(in_type::IN_EI, addr_mode::IMPLIED);
    t[0xFE] = I(in_type::IN_CP, addr_mode::REG8_IMM8, reg_type::A, reg_type::IMM8);
    t[0xFF] = I(in_type::IN_RST, addr_mode::RST_VEC, reg_type::NONE, reg_type::NONE, cond_type::CT_NONE, 0x38);

    return t;
}();

Instruction decode_ld_r8_r8(uint8_t op) {
    reg_type dst = kReg8[(op >> 3) & 0x07];
    reg_type src = kReg8[op & 0x07];

    if (dst == reg_type::HL) {
        return I(in_type::IN_LD, addr_mode::MEM_REG16_REG8, reg_type::HL, src);
    }
    if (src == reg_type::HL) {
        return I(in_type::IN_LD, addr_mode::REG8_MEM_REG16, dst, reg_type::HL);
    }

    return I(in_type::IN_LD, addr_mode::REG8_REG8, dst, src);
}

static constexpr reg_type kReg8OrHLmem[8] = {
    reg_type::B,
    reg_type::C,
    reg_type::D,
    reg_type::E,
    reg_type::H,
    reg_type::L,
    reg_type::HL,
    reg_type::A
};

static constexpr in_type kAluOps[8] = {
    in_type::IN_ADD,
    in_type::IN_ADC,
    in_type::IN_SUB,
    in_type::IN_SBC,
    in_type::IN_AND,
    in_type::IN_XOR,
    in_type::IN_OR,
    in_type::IN_CP
};

static constexpr in_type kCbRot[8] = {
    in_type::IN_RLC,
    in_type::IN_RRC,
    in_type::IN_RL,
    in_type::IN_RR,
    in_type::IN_SLA,
    in_type::IN_SRA,
    in_type::IN_SWAP,
    in_type::IN_SRL
};

Instruction decode_alu_block(uint8_t op) {
    if (op < 0x80 || op > 0xBF) {
        return I(in_type::IN_NOP, addr_mode::IMPLIED);
    }

    uint8_t alu_id = (op >> 3) & 0x07;
    uint8_t r_id   = op & 0x07;

    in_type  t   = kAluOps[alu_id];
    reg_type src = kReg8OrHLmem[r_id];

    if (src == reg_type::HL) {
        return I(t, addr_mode::REG8_MEM_REG16, reg_type::A, reg_type::HL);
    }

    return I(t, addr_mode::REG8_REG8, reg_type::A, src);
}

static constexpr reg_type kCbTarget[8] = {
    reg_type::B, reg_type::C, reg_type::D, reg_type::E,
    reg_type::H, reg_type::L, reg_type::HL, reg_type::A
};

static inline Instruction decode_cb(uint8_t cb_op) {
    uint8_t x = cb_op >> 6;
    uint8_t y = (cb_op >> 3) & 0x07;
    uint8_t z = cb_op & 0x07;

    reg_type target = kCbTarget[z];

    addr_mode mode;
    reg_type  r1;
    if (target == reg_type::HL) {
        mode = addr_mode::MEM_REG16;
        r1   = reg_type::HL;
    } else {
        mode = addr_mode::REG8;
        r1   = target;
    }

    if (x == 0) {
        return I(kCbRot[y], mode, r1, reg_type::NONE, cond_type::CT_NONE, 0);
    }
    if (x == 1) {
        return I(in_type::IN_BIT, mode, r1, reg_type::NONE, cond_type::CT_NONE, y);
    }
    if (x == 2) {
        return I(in_type::IN_RES, mode, r1, reg_type::NONE, cond_type::CT_NONE, y);
    }
    return I(in_type::IN_SET, mode, r1, reg_type::NONE, cond_type::CT_NONE, y);
}

Instruction decode(uint8_t opcode, bool is_cb) {
    if (is_cb) {
        return decode_cb(opcode);
    }

    if (opcode >= 0x40 && opcode <= 0x7F) {
        if (opcode == 0x76) {
            return I(in_type::IN_HALT, addr_mode::IMPLIED);
        }
        return decode_ld_r8_r8(opcode);
    }

    if (opcode >= 0x80 && opcode <= 0xBF) {
        return decode_alu_block(opcode);
    }

    return kInstrTable[opcode];
}