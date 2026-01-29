#pragma once

#include "cpu_instructions.h"
#include <cstdint>

uint8_t read_reg8(reg_type r);
void    write_reg8(reg_type r, uint8_t v);

uint16_t read_reg16(reg_type r);
void     write_reg16(reg_type r, uint16_t v);

uint8_t  bus_read8(uint16_t addr);
void     bus_write8(uint16_t addr, uint8_t v);
uint16_t bus_read16(uint16_t addr);
void     bus_write16(uint16_t addr, uint16_t v);

uint8_t  fetch8();
uint16_t fetch16();

bool is_carry_add(uint8_t a, uint8_t b);
bool is_half_carry_add(uint8_t a, uint8_t b);
bool is_carry_sub(uint8_t a, uint8_t b);
bool is_half_carry_sub(uint8_t a, uint8_t b);

// 16-bit variants used for operations like ADD HL,rr.
bool is_carry_add16(uint16_t a, uint16_t b);
bool is_half_carry_add16_12(uint16_t a, uint16_t b);