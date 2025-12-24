// Core
void execute_nop(Instruction);
void execute_ld(Instruction);
void execute_ldh(Instruction);

// Arithmetic / logic
void execute_add(Instruction);
void execute_adc(Instruction);
void execute_sub(Instruction);
void execute_sbc(Instruction);
void execute_and(Instruction);
void execute_xor(Instruction);
void execute_or(Instruction);
void execute_cp(Instruction);

// Inc / Dec
void execute_inc(Instruction);
void execute_dec(Instruction);

// Rotates (A only)
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