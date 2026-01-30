#pragma once

#include "cpu_instructions.h"

// Core
uint8_t execute_nop(Instruction);
uint8_t execute_ld(Instruction);
uint8_t execute_ldh(Instruction);

// Arithmetic / logic
uint8_t execute_add(Instruction);
uint8_t execute_adc(Instruction);
uint8_t execute_sub(Instruction);
uint8_t execute_sbc(Instruction);
uint8_t execute_and(Instruction);
uint8_t execute_xor(Instruction);
uint8_t execute_or(Instruction);
uint8_t execute_cp(Instruction);

// Inc / Dec
uint8_t execute_inc(Instruction);
uint8_t execute_dec(Instruction);

// Rotates (A only)
uint8_t execute_rlca(Instruction);
uint8_t execute_rrca(Instruction);
uint8_t execute_rla(Instruction);
uint8_t execute_rra(Instruction);

// Flags
uint8_t execute_daa(Instruction);
uint8_t execute_cpl(Instruction);
uint8_t execute_scf(Instruction);
uint8_t execute_ccf(Instruction);

// Control flow
uint8_t execute_jr(Instruction);
uint8_t execute_jp(Instruction);
uint8_t execute_call(Instruction);
uint8_t execute_ret(Instruction);
uint8_t execute_reti(Instruction);
uint8_t execute_rst(Instruction);

// Stack
uint8_t execute_push(Instruction);
uint8_t execute_pop(Instruction);

// CPU state
uint8_t execute_halt(Instruction);
uint8_t execute_stop(Instruction);
uint8_t execute_di(Instruction);
uint8_t execute_ei(Instruction);

// CB-prefixed
uint8_t execute_rlc(Instruction);
uint8_t execute_rrc(Instruction);
uint8_t execute_rl(Instruction);
uint8_t execute_rr(Instruction);
uint8_t execute_sla(Instruction);
uint8_t execute_sra(Instruction);
uint8_t execute_swap(Instruction);
uint8_t execute_srl(Instruction);
uint8_t execute_bit(const Instruction& inst, uint8_t bit_to_check);
uint8_t execute_res(const Instruction& inst, uint8_t bit);
uint8_t execute_set(const Instruction& inst, uint8_t bit);