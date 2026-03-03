#include "cart.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

typedef struct {
    char filename[1024];
    uint32_t rom_size;
    uint8_t *rom_data;
    rom_header *header;

    // Shared MBC state
    bool ram_enabled;
    uint8_t ram_bank_reg;   // RAM bank select (2-bit MBC1/MBC3, 4-bit MBC5)

    // MBC1 state
    uint8_t rom_bank_reg;   // 5-bit register (0x01–0x1F)
    uint8_t banking_mode;   // 0 = ROM mode, 1 = RAM mode

    // MBC3 / MBC5 state
    uint16_t rom_bank;      // 7-bit for MBC3, 9-bit for MBC5

    // MBC3 RTC
    bool rtc_mapped;        // true when an RTC register is selected instead of RAM
    uint8_t rtc_select;     // which RTC register (0x08–0x0C)
    uint8_t rtc_regs[5];    // S, M, H, DL, DH
    uint8_t rtc_latched[5]; // latched copies
    uint8_t rtc_latch_prev; // previous write for 0x00→0x01 edge detection

    // External RAM
    uint8_t *ram_data;
    uint32_t ram_size_bytes;
    uint8_t num_ram_banks;
    uint16_t num_rom_banks;
} cart_context;

static cart_context ctx;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

static bool cart_is_mbc1() {
    return ctx.header->type >= 0x01 && ctx.header->type <= 0x03;
}

static bool cart_is_mbc3() {
    return ctx.header->type >= 0x0F && ctx.header->type <= 0x13;
}

static bool cart_is_mbc5() {
    return ctx.header->type >= 0x19 && ctx.header->type <= 0x1E;
}

static bool cart_has_rtc() {
    return ctx.header->type == 0x0F || ctx.header->type == 0x10;
}

static uint32_t get_ram_size_bytes(uint8_t ram_size_code) {
    switch (ram_size_code) {
        case 0x00: return 0;
        case 0x01: return 0;       // listed as unused
        case 0x02: return 8192;    // 8 KB (1 bank)
        case 0x03: return 32768;   // 32 KB (4 banks)
        case 0x04: return 131072;  // 128 KB (16 banks)
        case 0x05: return 65536;   // 64 KB (8 banks)
        default:   return 0;
    }
}

// -----------------------------------------------------------------------------
// Lookup Tables
// -----------------------------------------------------------------------------

static const char *ROM_TYPES[] = {
    "ROM ONLY", "MBC1", "MBC1+RAM", "MBC1+RAM+BATTERY",
    "0x04 ???", "MBC2", "MBC2+BATTERY", "0x07 ???",
    "ROM+RAM 1", "ROM+RAM+BATTERY 1", "0x0A ???", "MMM01",
    "MMM01+RAM", "MMM01+RAM+BATTERY", "0x0E ???", "MBC3+TIMER+BATTERY",
    "MBC3+TIMER+RAM+BATTERY 2", "MBC3", "MBC3+RAM 2", "MBC3+RAM+BATTERY 2",
    "0x14 ???", "0x15 ???", "0x16 ???", "0x17 ???",
    "0x18 ???", "MBC5", "MBC5+RAM", "MBC5+RAM+BATTERY",
    "MBC5+RUMBLE", "MBC5+RUMBLE+RAM", "MBC5+RUMBLE+RAM+BATTERY", "0x1F ???",
    "MBC6", "0x21 ???", "MBC7+SENSOR+RUMBLE+RAM+BATTERY",
};

static const char *LIC_CODE[0xA5] = {
    [0x00] = "None", [0x01] = "Nintendo R&D1", [0x08] = "Capcom", [0x13] = "Electronic Arts",
    [0x18] = "Hudson Soft", [0x19] = "b-ai", [0x20] = "kss", [0x22] = "pow",
    [0x24] = "PCM Complete", [0x25] = "san-x", [0x28] = "Kemco Japan", [0x29] = "seta",
    [0x30] = "Viacom", [0x31] = "Nintendo", [0x32] = "Bandai", [0x33] = "Ocean/Acclaim",
    [0x34] = "Konami", [0x35] = "Hector", [0x37] = "Taito", [0x38] = "Hudson",
    [0x39] = "Banpresto", [0x41] = "Ubi Soft", [0x42] = "Atlus", [0x44] = "Malibu",
    [0x46] = "angel", [0x47] = "Bullet-Proof", [0x49] = "irem", [0x50] = "Absolute",
    [0x51] = "Acclaim", [0x52] = "Activision", [0x53] = "American sammy", [0x54] = "Konami",
    [0x55] = "Hi tech entertainment", [0x56] = "LJN", [0x57] = "Matchbox", [0x58] = "Mattel",
    [0x59] = "Milton Bradley", [0x60] = "Titus", [0x61] = "Virgin", [0x64] = "LucasArts",
    [0x67] = "Ocean", [0x69] = "Electronic Arts", [0x70] = "Infogrames", [0x71] = "Interplay",
    [0x72] = "Broderbund", [0x73] = "sculptured", [0x75] = "sci", [0x78] = "THQ",
    [0x79] = "Accolade", [0x80] = "misawa", [0x83] = "lozc", [0x86] = "Tokuma Shoten Intermedia",
    [0x87] = "Tsukuda Original", [0x91] = "Chunsoft", [0x92] = "Video system", [0x93] = "Ocean/Acclaim",
    [0x95] = "Varie", [0x96] = "Yonezawa/s'pal", [0x97] = "Kaneko", [0x99] = "Pack in soft",
    [0xA4] = "Konami (Yu-Gi-Oh!)"
};

const char *cart_lic_name() {
    if (ctx.header->new_lic_code <= 0xA4) {
        return LIC_CODE[ctx.header->lic_code];
    }
    return "UNKNOWN";
}

const char *cart_type_name() {
    if (ctx.header->type <= 0x22) {
        return ROM_TYPES[ctx.header->type];
    }
    return "UNKNOWN";
}

// -----------------------------------------------------------------------------
// Cartridge Loading
// -----------------------------------------------------------------------------

bool cart_load(const char *cart) {
    snprintf(ctx.filename, sizeof(ctx.filename), "%s", cart);

    FILE *fp = fopen(cart, "rb");
    if (!fp) {
        printf("Failed to open: %s\n", cart);
        return false;
    }

    printf("Opened: %s\n", ctx.filename);

    fseek(fp, 0, SEEK_END);
    ctx.rom_size = ftell(fp);
    rewind(fp);

    ctx.rom_data = (uint8_t *)malloc(ctx.rom_size);
    fread(ctx.rom_data, ctx.rom_size, 1, fp);
    fclose(fp);

    ctx.header = (rom_header *)(ctx.rom_data + 0x100);
    ctx.header->title[15] = 0;

    printf("Cartridge Loaded:\n");
    printf("\t Title    : %s\n", ctx.header->title);
    printf("\t Type     : %2.2X (%s)\n", ctx.header->type, cart_type_name());
    printf("\t ROM Size : %d KB\n", 32 << ctx.header->rom_size);
    printf("\t RAM Size : %2.2X\n", ctx.header->ram_size);
    printf("\t LIC Code : %2.2X (%s)\n", ctx.header->lic_code, cart_lic_name());
    printf("\t ROM Vers : %2.2X\n", ctx.header->version);

    ctx.num_rom_banks = 2 << ctx.header->rom_size;

    // Common defaults
    ctx.ram_enabled = false;
    ctx.ram_bank_reg = 0;

    // MBC1
    ctx.rom_bank_reg = 1;
    ctx.banking_mode = 0;

    // MBC3 / MBC5
    ctx.rom_bank = 1;
    ctx.rtc_mapped = false;
    ctx.rtc_select = 0;
    memset(ctx.rtc_regs, 0, sizeof(ctx.rtc_regs));
    memset(ctx.rtc_latched, 0, sizeof(ctx.rtc_latched));
    ctx.rtc_latch_prev = 0xFF;

    // Allocate external RAM
    ctx.ram_size_bytes = get_ram_size_bytes(ctx.header->ram_size);
    ctx.num_ram_banks = ctx.ram_size_bytes > 0 ? (ctx.ram_size_bytes / 0x2000) : 0;
    if (ctx.ram_size_bytes > 0) {
        ctx.ram_data = (uint8_t *)calloc(ctx.ram_size_bytes, 1);
        printf("\t RAM      : %u bytes (%u banks) allocated\n",
               ctx.ram_size_bytes, ctx.num_ram_banks);
    } else {
        ctx.ram_data = nullptr;
    }

    uint16_t x = 0;
    for (uint16_t i = 0x0134; i <= 0x014C; i++) {
        x = x - ctx.rom_data[i] - 1;
    }

    printf("\t Checksum : %2.2X (%s)\n", ctx.header->checksum, (x & 0xFF) ? "PASSED" : "FAILED");

    return true;
}

// -----------------------------------------------------------------------------
// Read / Write — ROM ONLY
// -----------------------------------------------------------------------------

static uint8_t rom_only_read(uint16_t address) {
    return ctx.rom_data[address];
}

static void rom_only_write(uint16_t address, uint8_t value) {
    (void)address;
    (void)value;
}

// -----------------------------------------------------------------------------
// Read / Write — MBC1
// -----------------------------------------------------------------------------

static uint8_t mbc1_read(uint16_t address) {
    if (address < 0x4000) {
        uint32_t bank = 0;
        if (ctx.banking_mode == 1) {
            bank = (ctx.ram_bank_reg << 5) & (ctx.num_rom_banks - 1);
        }
        uint32_t rom_addr = bank * 0x4000 + address;
        return ctx.rom_data[rom_addr % ctx.rom_size];
    }

    if (address < 0x8000) {
        uint32_t bank = (ctx.ram_bank_reg << 5) | ctx.rom_bank_reg;
        bank &= (ctx.num_rom_banks - 1);
        uint32_t rom_addr = bank * 0x4000 + (address - 0x4000);
        return ctx.rom_data[rom_addr % ctx.rom_size];
    }

    if (address >= 0xA000 && address < 0xC000) {
        if (!ctx.ram_enabled || !ctx.ram_data) {
            return 0xFF;
        }
        uint32_t ram_bank = 0;
        if (ctx.banking_mode == 1 && ctx.num_ram_banks > 1) {
            ram_bank = ctx.ram_bank_reg & (ctx.num_ram_banks - 1);
        }
        uint32_t ram_addr = ram_bank * 0x2000 + (address - 0xA000);
        return ctx.ram_data[ram_addr % ctx.ram_size_bytes];
    }

    return 0xFF;
}

static void mbc1_write(uint16_t address, uint8_t value) {
    if (address < 0x2000) {
        ctx.ram_enabled = ((value & 0x0F) == 0x0A);
        return;
    }

    if (address < 0x4000) {
        ctx.rom_bank_reg = value & 0x1F;
        if (ctx.rom_bank_reg == 0) {
            ctx.rom_bank_reg = 1;
        }
        return;
    }

    if (address < 0x6000) {
        ctx.ram_bank_reg = value & 0x03;
        return;
    }

    if (address < 0x8000) {
        ctx.banking_mode = value & 0x01;
        return;
    }

    if (address >= 0xA000 && address < 0xC000) {
        if (!ctx.ram_enabled || !ctx.ram_data) {
            return;
        }
        uint32_t ram_bank = 0;
        if (ctx.banking_mode == 1 && ctx.num_ram_banks > 1) {
            ram_bank = ctx.ram_bank_reg & (ctx.num_ram_banks - 1);
        }
        uint32_t ram_addr = ram_bank * 0x2000 + (address - 0xA000);
        ctx.ram_data[ram_addr % ctx.ram_size_bytes] = value;
        return;
    }
}

// -----------------------------------------------------------------------------
// Read / Write — MBC3
// -----------------------------------------------------------------------------

static uint8_t mbc3_read(uint16_t address) {
    // 0x0000-0x3FFF: ROM bank 0 (fixed)
    if (address < 0x4000) {
        return ctx.rom_data[address];
    }

    // 0x4000-0x7FFF: switchable ROM bank 1-127
    if (address < 0x8000) {
        uint32_t bank = ctx.rom_bank & (ctx.num_rom_banks - 1);
        uint32_t rom_addr = bank * 0x4000 + (address - 0x4000);
        return ctx.rom_data[rom_addr % ctx.rom_size];
    }

    // 0xA000-0xBFFF: external RAM or RTC register
    if (address >= 0xA000 && address < 0xC000) {
        if (!ctx.ram_enabled) {
            return 0xFF;
        }

        if (ctx.rtc_mapped && cart_has_rtc()) {
            uint8_t idx = ctx.rtc_select - 0x08;
            if (idx < 5) {
                return ctx.rtc_latched[idx];
            }
            return 0xFF;
        }

        if (!ctx.ram_data) {
            return 0xFF;
        }
        uint32_t ram_bank = ctx.ram_bank_reg;
        if (ctx.num_ram_banks > 0) {
            ram_bank &= (ctx.num_ram_banks - 1);
        }
        uint32_t ram_addr = ram_bank * 0x2000 + (address - 0xA000);
        return ctx.ram_data[ram_addr % ctx.ram_size_bytes];
    }

    return 0xFF;
}

static void mbc3_write(uint16_t address, uint8_t value) {
    // 0x0000-0x1FFF: RAM & Timer enable
    if (address < 0x2000) {
        ctx.ram_enabled = ((value & 0x0F) == 0x0A);
        return;
    }

    // 0x2000-0x3FFF: ROM bank number (7 bits, 0 maps to 1)
    if (address < 0x4000) {
        ctx.rom_bank = value & 0x7F;
        if (ctx.rom_bank == 0) {
            ctx.rom_bank = 1;
        }
        return;
    }

    // 0x4000-0x5FFF: RAM bank or RTC register select
    if (address < 0x6000) {
        if (value <= 0x03) {
            ctx.ram_bank_reg = value;
            ctx.rtc_mapped = false;
        } else if (value >= 0x08 && value <= 0x0C) {
            ctx.rtc_select = value;
            ctx.rtc_mapped = true;
        }
        return;
    }

    // 0x6000-0x7FFF: latch clock data (write 0x00 then 0x01)
    if (address < 0x8000) {
        if (ctx.rtc_latch_prev == 0x00 && value == 0x01) {
            memcpy(ctx.rtc_latched, ctx.rtc_regs, sizeof(ctx.rtc_regs));
        }
        ctx.rtc_latch_prev = value;
        return;
    }

    // 0xA000-0xBFFF: external RAM or RTC register write
    if (address >= 0xA000 && address < 0xC000) {
        if (!ctx.ram_enabled) {
            return;
        }

        if (ctx.rtc_mapped && cart_has_rtc()) {
            uint8_t idx = ctx.rtc_select - 0x08;
            if (idx < 5) {
                ctx.rtc_regs[idx] = value;
            }
            return;
        }

        if (!ctx.ram_data) {
            return;
        }
        uint32_t ram_bank = ctx.ram_bank_reg;
        if (ctx.num_ram_banks > 0) {
            ram_bank &= (ctx.num_ram_banks - 1);
        }
        uint32_t ram_addr = ram_bank * 0x2000 + (address - 0xA000);
        ctx.ram_data[ram_addr % ctx.ram_size_bytes] = value;
        return;
    }
}

// -----------------------------------------------------------------------------
// Read / Write — MBC5
// -----------------------------------------------------------------------------

static uint8_t mbc5_read(uint16_t address) {
    // 0x0000-0x3FFF: ROM bank 0 (fixed)
    if (address < 0x4000) {
        return ctx.rom_data[address];
    }

    // 0x4000-0x7FFF: switchable ROM bank 0-511
    if (address < 0x8000) {
        uint32_t bank = ctx.rom_bank;
        if (ctx.num_rom_banks > 0) {
            bank &= (ctx.num_rom_banks - 1);
        }
        uint32_t rom_addr = bank * 0x4000 + (address - 0x4000);
        return ctx.rom_data[rom_addr % ctx.rom_size];
    }

    // 0xA000-0xBFFF: external RAM
    if (address >= 0xA000 && address < 0xC000) {
        if (!ctx.ram_enabled || !ctx.ram_data) {
            return 0xFF;
        }
        uint32_t ram_bank = ctx.ram_bank_reg;
        if (ctx.num_ram_banks > 0) {
            ram_bank &= (ctx.num_ram_banks - 1);
        }
        uint32_t ram_addr = ram_bank * 0x2000 + (address - 0xA000);
        return ctx.ram_data[ram_addr % ctx.ram_size_bytes];
    }

    return 0xFF;
}

static void mbc5_write(uint16_t address, uint8_t value) {
    // 0x0000-0x1FFF: RAM enable
    if (address < 0x2000) {
        ctx.ram_enabled = ((value & 0x0F) == 0x0A);
        return;
    }

    // 0x2000-0x2FFF: low 8 bits of ROM bank
    if (address < 0x3000) {
        ctx.rom_bank = (ctx.rom_bank & 0x100) | value;
        return;
    }

    // 0x3000-0x3FFF: bit 8 of ROM bank
    if (address < 0x4000) {
        ctx.rom_bank = (ctx.rom_bank & 0xFF) | ((value & 0x01) << 8);
        return;
    }

    // 0x4000-0x5FFF: RAM bank (0x00-0x0F)
    if (address < 0x6000) {
        ctx.ram_bank_reg = value & 0x0F;
        return;
    }

    // 0xA000-0xBFFF: external RAM write
    if (address >= 0xA000 && address < 0xC000) {
        if (!ctx.ram_enabled || !ctx.ram_data) {
            return;
        }
        uint32_t ram_bank = ctx.ram_bank_reg;
        if (ctx.num_ram_banks > 0) {
            ram_bank &= (ctx.num_ram_banks - 1);
        }
        uint32_t ram_addr = ram_bank * 0x2000 + (address - 0xA000);
        ctx.ram_data[ram_addr % ctx.ram_size_bytes] = value;
        return;
    }
}

// -----------------------------------------------------------------------------
// Public API (dispatches based on cart type)
// -----------------------------------------------------------------------------

uint8_t cart_read(uint16_t address) {
    if (cart_is_mbc1()) return mbc1_read(address);
    if (cart_is_mbc3()) return mbc3_read(address);
    if (cart_is_mbc5()) return mbc5_read(address);
    return rom_only_read(address);
}

void cart_write(uint16_t address, uint8_t value) {
    if (cart_is_mbc1()) { mbc1_write(address, value); return; }
    if (cart_is_mbc3()) { mbc3_write(address, value); return; }
    if (cart_is_mbc5()) { mbc5_write(address, value); return; }
    rom_only_write(address, value);
}
