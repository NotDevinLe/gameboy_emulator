#include "cpu.h"
#include "cpu_instructions.h"
#include "cpu_proc.h"
#include "bus.h"
#include <cstdio>
#include <cstdint>

cpu_state cpu;

enum class cb_target : uint8_t {
    B = 0,
    C = 1,
    D = 2,
    E = 3,
    H = 4,
    L = 5,
    HL = 6,
    A = 7
};

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
    cpu.stop = false;
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

    Instruction inst = decode(op, is_cb);

    switch (inst.type) {
        case in_type::IN_NOP: {
            execute_nop(inst);
            break;
        }
        case in_type::IN_LD: {
            execute_ld(inst);
            break;
        }
        case in_type::IN_INC: {
            execute_inc(inst);
            break;
        }
        case in_type::IN_DEC: {
            execute_dec(inst);
            break;
        }
        case in_type::IN_ADD: {
            execute_add(inst);
            break;
        }
        case in_type::IN_SUB: {
            execute_sub(inst);
            break;
        }
        case in_type::IN_RLCA: {
            execute_rlca(inst);
            break;
        }
        case in_type::IN_RRCA: {
            execute_rrca(inst);
            break;
        }
        case in_type::IN_RLA: {
            execute_rla(inst);
            break;
        }
        case in_type::IN_RRA: {
            execute_rra(inst);
            break;
        }
        case in_type::IN_DAA: {
            execute_daa(inst);
            break;
        }
        case in_type::IN_CPL: {
            execute_cpl(inst);
            break;
        }
        case in_type::IN_SCF: {
            execute_scf(inst);
            break;
        }
        case in_type::IN_CCF: {
            execute_ccf(inst);
            break;
        }
        case in_type::IN_JR: {
            execute_jr(inst);
            break;
        }
        case in_type::IN_STOP: {
            execute_stop(inst);
            break;
        }
        case in_type::IN_HALT: {
            execute_halt(inst);
            break;
        }
        case in_type::IN_ADC: {
            execute_adc(inst);
            break;
        }
        case in_type::IN_SBC: {
            execute_sbc(inst);
            break;
        }
        case in_type::IN_AND: {
            execute_and(inst);
            break;
        }
        case in_type::IN_XOR: {
            execute_xor(inst);
            break;
        }
        case in_type::IN_OR: {
            execute_or(inst);
            break;
        }
        case in_type::IN_CP: {
            execute_cp(inst);
            break;
        }
        case in_type::IN_RET: {
            execute_ret(inst);
            break;
        }
        case in_type::IN_RETI: {
            execute_reti(inst);
            break;
        }
        case in_type::IN_JP: {
            execute_jp(inst);
            break;
        }
        case in_type::IN_CALL: {
            execute_call(inst);
            break;
        }
        case in_type::IN_RST: {
            execute_rst(inst);
            break;
        }
        case in_type::IN_POP: {
            execute_pop(inst);
            break;
        }
        case in_type::IN_PUSH: {
            execute_push(inst);
            break;
        }
        case in_type::IN_LDH: {
            execute_ldh(inst);
            break;
        }
        case in_type::IN_DI: {
            execute_di(inst);
            break;
        }
        case in_type::IN_EI: {
            execute_ei(inst);
            break;
        }
        case in_type::IN_RLC: {
            execute_rlc(inst);
            break;
        }
        case in_type::IN_RRC: {
            execute_rrc(inst);
            break;
        }
        case in_type::IN_RL: {
            execute_rl(inst);
            break;
        }
        case in_type::IN_RR: {
            execute_rr(inst);
            break;
        }
        case in_type::IN_SLA: {
            execute_sla(inst);
            break;
        }
        case in_type::IN_SRA: {
            execute_sra(inst);
            break;
        }
        case in_type::IN_SWAP: {
            execute_swap(inst);
            break;
        }
        case in_type::IN_SRL: {
            execute_srl(inst);
            break;
        }
        case in_type::IN_BIT: {
            uint8_t bit_to_check = (op >> 3) & 0x07;
            execute_bit(inst, bit_to_check);
            break;
        }
        case in_type::IN_RES: {
            uint8_t bit_to_clear = (op >> 3) & 0x07;
            execute_res(inst, bit_to_clear);
            break;
        }
        case in_type::IN_SET: {
            uint8_t bit_to_set = (op >> 3) & 0x07;
            execute_set(inst, bit_to_set);
            break;
        }
        default: {
            std::printf("Unknown instruction: %d\n", static_cast<int>(inst.type));
            break;
        }
    }

    return true;
}