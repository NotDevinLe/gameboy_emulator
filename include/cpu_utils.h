static uint8_t read_reg8(reg_type r);
static void    write_reg8(reg_type r, uint8_t v);

static uint16_t read_reg16(reg_type r);
static void     write_reg16(reg_type r, uint16_t v);

static uint8_t  bus_read8(uint16_t addr);
static void     bus_write8(uint16_t addr, uint8_t v);
static uint16_t bus_read16(uint16_t addr);
static void     bus_write16(uint16_t addr, uint16_t v);

static uint8_t  fetch8();
static uint16_t fetch16();