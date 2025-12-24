#pragma once

#include <array>
#include <cstdint>

// Register identifiers used by the decoder
enum class reg_type {
    A, B, C, D, E, H, L,
    HL, BC, DE, SP, AF,
    IMM8, IMM16, NONE
};

// High-level instruction kind
enum class in_type {
    IN_NOP,
    IN_LD,
    IN_INC,
    IN_DEC,
    IN_ADD,
    IN_SUB,
    IN_RLCA,
    IN_RRCA,
    IN_RLA,
    IN_RRA,
    IN_DAA,
    IN_CPL,
    IN_SCF,
    IN_CCF,
    IN_JR,
    IN_STOP,
    IN_HALT,
    IN_ADC,
    IN_SBC,
    IN_AND,
    IN_XOR,
    IN_OR,
    IN_CP,
    IN_RET,
    IN_RETI,
    IN_JP,
    IN_CALL,
    IN_RST,
    IN_POP,
    IN_PUSH,
    IN_LDH,
    IN_DI,
    IN_EI,
    IN_RLC, 
    IN_RRC, 
    IN_RL, 
    IN_RR, 
    IN_SLA, 
    IN_SRA, 
    IN_SWAP, 
    IN_SRL,
    IN_BIT, 
    IN_RES, 
    IN_SET
};

// Addressing modes captured by the decoder
enum class addr_mode {
    IMPLIED,
    REG8,
    REG16,
    REG8_REG8,
    REG16_REG16,
    REG16_IMM16,
    REG8_IMM8,
    REG16_IMM8,
    MEM_REG16,
    MEM_REG16_REG8,
    MEM_REG16_IMM8,
    MEM_IMM16_REG16,
    REG8_MEM_REG16,
    MEM_HLI_REG8,
    REG8_MEM_HLI,
    MEM_HLD_REG8,
    REG8_MEM_HLD,
    MEM_IMM16_REG8,
    REG8_MEM_IMM16,
    REG16_SP_IMM8,
    MEM_FF00_IMM8_REG8,
    REG8_MEM_FF00_IMM8,
    MEM_FF00_C_REG8,
    REG8_MEM_FF00_C,
    REL8,
    ABS16,
    RST_VEC
};

// Conditional flags used by control flow instructions
enum class cond_type {
    CT_NONE,
    CT_Z,
    CT_NZ,
    CT_C,
    CT_NC,
};

struct Instruction {
    in_type type;
    addr_mode mode;
    reg_type reg_1;
    reg_type reg_2;
    cond_type cond;
    uint8_t param;
};

// Decode a single opcode into a high-level Instruction
Instruction decode(uint8_t opcode, bool is_cb = false);

