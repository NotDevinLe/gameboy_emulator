#pragma once

#include <cstdint>

void stack_push(uint8_t value);
void stack_push16(uint16_t value);
uint8_t stack_pop();
uint16_t stack_pop16();