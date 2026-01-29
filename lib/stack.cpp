#include "cpu.h"
#include "bus.h"
#include "stack.h"

void stack_push(uint8_t value) {
    cpu.SP = static_cast<uint16_t>(cpu.SP - 1);
    bus_write8(cpu.SP, value);
}

void stack_push16(uint16_t value) {
    stack_push(static_cast<uint8_t>(value >> 8));
    stack_push(static_cast<uint8_t>(value & 0xFF));
}

uint8_t stack_pop() {
    uint8_t value = bus_read8(cpu.SP);
    cpu.SP = static_cast<uint16_t>(cpu.SP + 1);
    return value;
}

uint16_t stack_pop16() {
    uint8_t lo = stack_pop();
    uint8_t hi = stack_pop();
    return static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
}