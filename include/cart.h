#pragma once
#include <cstdint>

typedef struct {
    uint8_t entry[4];
    uint8_t logo[0x30];

    char title[16];
    uint16_t new_lic_code;
    uint8_t sgb_flag;
    uint8_t type;
    uint8_t rom_size;
    uint8_t ram_size;
    uint8_t dest_code;
    uint8_t lic_code;
    uint8_t version;
    uint8_t checksum;
    uint8_t global_checksum;
} rom_header;

bool cart_load(const char *cart);

uint8_t cart_read(uint16_t address);
void cart_write(uint16_t address, uint8_t value);

uint8_t cart_ram_read(uint16_t address);
void cart_ram_write(uint16_t address, uint8_t value);