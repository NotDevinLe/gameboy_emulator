#include "cpu.h"
#include "cpu_instructions.h"
#include "cpu_proc.h"
#include "bus.h"
#include "interrupt.h"
#include <cstdio>
#include <cstdint>
#include <unordered_map>
#include <fstream>
#include <iomanip>

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
    // Standard "DMG" (Original Game Boy) Post-Boot State
    cpu.PC = 0x0100;
    cpu.SP = 0xFFFE;
    
    // AF = 0x01B0 (A=01, F=B0: Z=1, N=0, H=1, C=0)
    cpu.A = 0x01;
    cpu.F = 0xB0;
    
    // BC = 0x0013
    cpu.B = 0x00;
    cpu.C = 0x13;
    
    // DE = 0x00D8
    cpu.D = 0x00;
    cpu.E = 0xD8;
    
    // HL = 0x014D (Points to the Nintendo Logo checksum)
    cpu.H = 0x01;
    cpu.L = 0x4D;

    cpu.ime = false;
    cpu.halt = false;
    cpu.stop = false;
    cpu.instr_count = 0;
}

bool cpu_step() {
    cpu_handle_interrupts(&cpu);

    // Count instructions for lightweight timing approximations.
    cpu.instr_count++;

    uint16_t pc_before = cpu.PC;
    uint8_t op = bus_read(cpu.PC);
    cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
    
    // Log at most the first 10,000 instructions to out_ours.log
    static int log_count = 0;
    static std::ofstream log_file("out_ours.txt", std::ios::trunc);
    if (log_count < 100000 && log_file.is_open()) {
        log_file << std::hex << std::uppercase << std::setfill('0');
        log_file << "PC=" << std::setw(4) << pc_before << " OP=" << std::setw(2) << static_cast<int>(op);
        if (op == 0xCB) {
            uint8_t cb_op = bus_read(cpu.PC);
            log_file << " CB=" << std::setw(2) << static_cast<int>(cb_op);
        }
        log_file << " AF=" << std::setw(4) << ((static_cast<int>(cpu.A) << 8) | cpu.F)
                 << " BC=" << std::setw(4) << ((static_cast<int>(cpu.B) << 8) | cpu.C)
                 << " DE=" << std::setw(4) << ((static_cast<int>(cpu.D) << 8) | cpu.E)
                 << " HL=" << std::setw(4) << ((static_cast<int>(cpu.H) << 8) | cpu.L)
                 << " SP=" << std::setw(4) << cpu.SP;
        log_file << std::dec << "\n";
        log_file.flush();
        ++log_count;
    }
 
    bool is_cb = false;

    if (op == 0xCB) {
        is_cb = true;
        op = bus_read(cpu.PC);
        cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
    }

    // std::printf("OP: %02X\n", op);

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