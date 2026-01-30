#include "cpu.h"
#include "cpu_instructions.h"
#include "cpu_proc.h"
#include "bus.h"
#include "interrupt.h"
#include "timer.h"
#include "emu.h"
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
    cpu.enabling_ime = false;  // EI delay flag
    cpu.instr_count = 0;
    cpu.cycle_count = 0;
    
    timer_init();
}

uint8_t cpu_step() {
    uint8_t cycles = 0;
    if (!cpu.halt) {
        // Count instructions
        cpu.instr_count++;

        uint16_t pc_before = cpu.PC;

        // Fetch opcode
        uint8_t op = bus_read(cpu.PC);
        cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
     
        bool is_cb = false;
        uint8_t cb_op = 0;

        if (op == 0xCB) {
            is_cb = true;
            // Fetch CB opcode - takes 1 cycle
            op = bus_read(cpu.PC);
            cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
            cb_op = op;
        }

        // --- Logging (first 100000 instructions) ---
        static std::ofstream log_file("out_ours.txt", std::ios::trunc);
        static uint64_t log_count = 0;

        if (log_count < 100000 && log_file.is_open()) {
            log_file << std::hex << std::uppercase << std::setfill('0');

            log_file << "PC=" << std::setw(4) << pc_before
                     << " OP=" << std::setw(2) << static_cast<int>(op);

            if (is_cb) {
                log_file << " CB=" << std::setw(2) << static_cast<int>(cb_op);
            }

            // Core 16-bit register pairs
            log_file << " AF=" << std::setw(4) << ((static_cast<int>(cpu.A) << 8) | cpu.F)
                     << " BC=" << std::setw(4) << ((static_cast<int>(cpu.B) << 8) | cpu.C)
                     << " DE=" << std::setw(4) << ((static_cast<int>(cpu.D) << 8) | cpu.E)
                     << " HL=" << std::setw(4) << ((static_cast<int>(cpu.H) << 8) | cpu.L)
                     << " SP=" << std::setw(4) << cpu.SP;

            // Individual flag register and decoded flags
            uint8_t f = cpu.F;
            int z = (f & FLAG_Z) ? 1 : 0;
            int n = (f & FLAG_N) ? 1 : 0;
            int h = (f & FLAG_H) ? 1 : 0;
            int c = (f & FLAG_C) ? 1 : 0;

            log_file << " F=" << std::setw(2) << static_cast<int>(f)
                     << " Z=" << z
                     << " N=" << n
                     << " H=" << h
                     << " C=" << c;

            // CPU control flags
            log_file << " IME=" << (cpu.ime ? 1 : 0)
                     << " HALT=" << (cpu.halt ? 1 : 0)
                     << " STOP=" << (cpu.stop ? 1 : 0);

            // Timer registers (DIV high byte, TIMA, TMA, TAC)
            uint8_t div  = timer_read(0xFF04);
            uint8_t tima = timer_read(0xFF05);
            uint8_t tma  = timer_read(0xFF06);
            uint8_t tac  = timer_read(0xFF07);

            log_file << " DIV="  << std::setw(2) << static_cast<int>(div)
                     << " TIMA=" << std::setw(2) << static_cast<int>(tima)
                     << " TMA="  << std::setw(2) << static_cast<int>(tma)
                     << " TAC="  << std::setw(2) << static_cast<int>(tac)
                     << std::dec << "\n";

            log_file.flush();
            ++log_count;
        }

        Instruction inst = decode(op, is_cb);

        switch (inst.type) {
        case in_type::IN_NOP: {
            cycles = execute_nop(inst);
            break;
        }
        case in_type::IN_LD: {
            cycles = execute_ld(inst);
            break;
        }
        case in_type::IN_INC: {
            cycles = execute_inc(inst);
            break;
        }
        case in_type::IN_DEC: {
            cycles = execute_dec(inst);
            break;
        }
        case in_type::IN_ADD: {
            cycles = execute_add(inst);
            break;
        }
        case in_type::IN_SUB: {
            cycles = execute_sub(inst);
            break;
        }
        case in_type::IN_RLCA: {
            cycles = execute_rlca(inst);
            break;
        }
        case in_type::IN_RRCA: {
            cycles = execute_rrca(inst);
            break;
        }
        case in_type::IN_RLA: {
            cycles = execute_rla(inst);
            break;
        }
        case in_type::IN_RRA: {
            cycles = execute_rra(inst);
            break;
        }
        case in_type::IN_DAA: {
            cycles = execute_daa(inst);
            break;
        }
        case in_type::IN_CPL: {
            cycles = execute_cpl(inst);
            break;
        }
        case in_type::IN_SCF: {
            cycles = execute_scf(inst);
            break;
        }
        case in_type::IN_CCF: {
            cycles = execute_ccf(inst);
            break;
        }
        case in_type::IN_JR: {
            cycles = execute_jr(inst);
            break;
        }
        case in_type::IN_STOP: {
            cycles = execute_stop(inst);
            break;
        }
        case in_type::IN_HALT: {
            cycles = execute_halt(inst);
            break;
        }
        case in_type::IN_ADC: {
            cycles = execute_adc(inst);
            break;
        }
        case in_type::IN_SBC: {
            cycles = execute_sbc(inst);
            break;
        }
        case in_type::IN_AND: {
            cycles = execute_and(inst);
            break;
        }
        case in_type::IN_XOR: {
            cycles = execute_xor(inst);
            break;
        }
        case in_type::IN_OR: {
            cycles = execute_or(inst);
            break;
        }
        case in_type::IN_CP: {
            cycles = execute_cp(inst);
            break;
        }
        case in_type::IN_RET: {
            cycles = execute_ret(inst);
            break;
        }
        case in_type::IN_RETI: {
            cycles = execute_reti(inst);
            break;
        }
        case in_type::IN_JP: {
            cycles = execute_jp(inst);
            break;
        }
        case in_type::IN_CALL: {
            cycles = execute_call(inst);
            break;
        }
        case in_type::IN_RST: {
            cycles = execute_rst(inst);
            break;
        }
        case in_type::IN_POP: {
            cycles = execute_pop(inst);
            break;
        }
        case in_type::IN_PUSH: {
            cycles = execute_push(inst);
            break;
        }
        case in_type::IN_LDH: {
            cycles = execute_ldh(inst);
            break;
        }
        case in_type::IN_DI: {
            cycles = execute_di(inst);
            break;
        }
        case in_type::IN_EI: {
            cycles = execute_ei(inst);
            break;
        }
        case in_type::IN_RLC: {
            cycles = execute_rlc(inst);
            break;
        }
        case in_type::IN_RRC: {
            cycles = execute_rrc(inst);
            break;
        }
        case in_type::IN_RL: {
            cycles = execute_rl(inst);
            break;
        }
        case in_type::IN_RR: {
            cycles = execute_rr(inst);
            break;
        }
        case in_type::IN_SLA: {
            cycles = execute_sla(inst);
            break;
        }
        case in_type::IN_SRA: {
            cycles = execute_sra(inst);
            break;
        }
        case in_type::IN_SWAP: {
            cycles = execute_swap(inst);
            break;
        }
        case in_type::IN_SRL: {
            cycles = execute_srl(inst);
            break;
        }
        case in_type::IN_BIT: {
            uint8_t bit_to_check = (op >> 3) & 0x07;
            cycles = execute_bit(inst, bit_to_check);
            break;
        }
        case in_type::IN_RES: {
            uint8_t bit_to_clear = (op >> 3) & 0x07;
            cycles = execute_res(inst, bit_to_clear);
            break;
        }
        case in_type::IN_SET: {
            uint8_t bit_to_set = (op >> 3) & 0x07;
            cycles = execute_set(inst, bit_to_set);
            break;
        }
        default: {
            std::printf("Unknown instruction: %d\n", static_cast<int>(inst.type));
            break;
        }
        }
    } else {
        // If interrupts are pending, wake up (but don't service unless IME is enabled)
        if (bus_read(0xFF0F) & bus_read(0xFFFF)) {
            cpu.halt = false;
        }
        cycles += 4;
    }

    // Handle interrupts AFTER instruction execution (LLD's order)
    // Only check interrupts if IME is enabled
    if (cpu.ime) {
        cycles += cpu_handle_interrupts(&cpu);
        cpu.enabling_ime = false;  // Clear the delay flag after handling interrupts
    }

    // If EI was executed, enable IME on the next instruction
    if (cpu.enabling_ime) {
        cpu.ime = true;
    }

    return cycles;
}