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
#include <string>

cpu_state cpu;

// Namespace to manage log state that can be reset
namespace cpu_log_state {
    bool should_reset = true;
}

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
    cpu.ime_pending = false;   // IME pending enable flag
    cpu.instr_count = 0;
    cpu.cycle_count = 0;
    
    timer_init();
    
    // Reset log file state so it's cleared on each run
    cpu_log_state::should_reset = true;
}

uint8_t cpu_step() {
    // Check serial output for "Passed" message
    static std::string serial_buffer;
    uint8_t sc = bus_read(0xFF02);
    uint8_t sb = bus_read(0xFF01);
    
    // If serial transfer is active (SC bit 7 set), track the character
    if (sc & 0x80) {
        char c = static_cast<char>(sb);
        serial_buffer += c;
        
        // Keep buffer size reasonable (last 10 characters)
        if (serial_buffer.length() > 10) {
            serial_buffer = serial_buffer.substr(serial_buffer.length() - 10);
        }
        
        // Check if "Passed" appears in the buffer
        if (serial_buffer.find("Passed") != std::string::npos) {
            std::printf("\n*** PASSED ***\n");
            fflush(stdout);
        }
    }
    
    uint8_t cycles = 0;
    
    if (cpu.ime_pending) {
        cpu.ime_pending = false;
        cpu.ime = true;
    }
    
    if (cpu.ime) {
        uint8_t interrupt_cycles = cpu_handle_interrupts(&cpu);
        if (interrupt_cycles > 0) {
            cpu.enabling_ime = false;  // Clear EI delay flag
            return interrupt_cycles;
        }
    }
    
    cpu.enabling_ime = false;
    
    if (!cpu.halt) {
        cpu.instr_count++;

        uint16_t pc_before = cpu.PC;
        cpu.last_opcode_pc = pc_before;  // Capture PC for accurate bus write logging

        static std::ofstream log_file;
        static uint64_t log_count = 0;
        
        if (cpu_log_state::should_reset) {
            log_file.close();
            log_file.open("cpu_log.txt", std::ios::trunc);
            log_count = 0;
            cpu_log_state::should_reset = false;
        }
        if (log_count < 100000 && log_file.is_open()) {
            log_file << std::hex << std::uppercase << std::setfill('0');
            log_file << "A:" << std::setw(2) << static_cast<int>(cpu.A)
                     << " F:" << std::setw(2) << static_cast<int>(cpu.F)
                     << " B:" << std::setw(2) << static_cast<int>(cpu.B)
                     << " C:" << std::setw(2) << static_cast<int>(cpu.C)
                     << " D:" << std::setw(2) << static_cast<int>(cpu.D)
                     << " E:" << std::setw(2) << static_cast<int>(cpu.E)
                     << " H:" << std::setw(2) << static_cast<int>(cpu.H)
                     << " L:" << std::setw(2) << static_cast<int>(cpu.L)
                     << " SP:" << std::setw(4) << cpu.SP
                     << " PC:" << std::setw(4) << pc_before
                     << " PCMEM:";
            
            for (int i = 0; i < 4; i++) {
                if (i > 0) log_file << ",";
                uint8_t mem_val = bus_read(static_cast<uint16_t>(pc_before + i));
                log_file << std::setw(2) << static_cast<int>(mem_val);
            }
            log_file << "\n";
            // Flush every 10k lines instead of every line (massive perf improvement)
            if ((log_count & 0x2FFF) == 0) log_file.flush();
            ++log_count;
        }

    uint8_t op = bus_read(cpu.PC);
    cpu.PC = static_cast<uint16_t>(cpu.PC + 1);

    bool is_cb = false;

    if (op == 0xCB) {
        is_cb = true;
        op = bus_read(cpu.PC);
        cpu.PC = static_cast<uint16_t>(cpu.PC + 1);
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
        if (bus_read(0xFF0F) & bus_read(0xFFFF)) {
            cpu.halt = false;
        }
        cycles += 4;
    }

    if (cpu.enabling_ime) {
        cpu.enabling_ime = false;
        cpu.ime_pending = true; 
    }

    return cycles;
}