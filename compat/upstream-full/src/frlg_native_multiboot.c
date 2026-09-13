#include "global.h"
#include "libgcnmultiboot.h"
#include "frlg_native_multiboot.h"
#include <string.h>

enum {
    GCMB_BASE_DEST = 0x20,
    GCMB_CUR_DEST = 0x24,
    GCMB_HANDLER = 0x28,
    REG_RCNT_OFFSET = 0x134,
    REG_JOYCNT_OFFSET = 0x140,
    REG_JOYSTAT_OFFSET = 0x158,
    REG_IE_OFFSET = 0x200,
    REG_IF_OFFSET = 0x202,
    REG_IME_OFFSET = 0x208,
    SERIAL_IRQ = 0x80
};

_Static_assert(sizeof(struct GcmbStruct) == 0x2c, "GBA multiboot layout changed");
static FrlgNativeMultibootStatus status;

FrlgNativeMultibootStatus frlg_native_multiboot_status(void)
{
    return status;
}

static uint16_t read_reg(unsigned offset)
{
    uint16_t value;
    memcpy(&value, frlg_native_memory()->io + offset, sizeof(value));
    return value;
}

static void write_reg(unsigned offset, uint16_t value)
{
    memcpy(frlg_native_memory()->io + offset, &value, sizeof(value));
}

static uint32_t read_word(const uint8_t *bytes, unsigned offset)
{
    uint32_t value;
    memcpy(&value, bytes + offset, sizeof(value));
    return value;
}

static void write_word(uint8_t *bytes, unsigned offset, uint32_t value)
{
    memcpy(bytes + offset, &value, sizeof(value));
}

void GameCubeMultiBoot_Init(struct GcmbStruct *mb)
{
    uint8_t *bytes = (uint8_t *)mb;
    uint8_t old_counter2 = bytes[1];
    uint8_t old_vcount = bytes[3];
    uint16_t ime = read_reg(REG_IME_OFFSET);
    status = FRLG_NATIVE_MULTIBOOT_OK;
    write_reg(REG_IME_OFFSET, 0);
    memset(bytes, 0, GCMB_BASE_DEST);
    bytes[1] = old_vcount;
    bytes[3] = old_counter2 >> 1;
    /* The GBA callback address is a 32-bit marker, never a host pointer. */
    write_word(bytes, GCMB_HANDLER, 1);
    write_reg(REG_RCNT_OFFSET, 0x8000);
    write_reg(REG_RCNT_OFFSET, 0xc000);
    write_reg(REG_JOYCNT_OFFSET, (read_reg(REG_JOYCNT_OFFSET) & (uint16_t)~0x7) | 0x40);
    write_reg(REG_JOYSTAT_OFFSET, 0);
    write_reg(REG_IF_OFFSET, read_reg(REG_IF_OFFSET) & (uint16_t)~SERIAL_IRQ);
    write_reg(REG_IE_OFFSET, read_reg(REG_IE_OFFSET) | SERIAL_IRQ);
    write_reg(REG_IME_OFFSET, ime);
}

void GameCubeMultiBoot_Main(struct GcmbStruct *mb)
{
    uint8_t *bytes = (uint8_t *)mb;
    if (!read_word(bytes, GCMB_HANDLER)) {
        GameCubeMultiBoot_Init(mb);
        return;
    }
    bytes[1]++;
    if (mb->gcmb_field_2 == 2)
        return;
    uint16_t ime = read_reg(REG_IME_OFFSET);
    write_reg(REG_IME_OFFSET, 0);
    if (bytes[0] <= 10)
        bytes[0]++;
    else {
        write_reg(REG_IME_OFFSET, ime);
        GameCubeMultiBoot_Init(mb);
        return;
    }
    write_reg(REG_IME_OFFSET, ime);
    if (mb->gcmb_field_2 == 0) {
        uint32_t base = read_word(bytes, GCMB_BASE_DEST);
        uint32_t current = read_word(bytes, GCMB_CUR_DEST);
        if (current != base && current - base >= 0xa0 && status == FRLG_NATIVE_MULTIBOOT_OK)
            status = FRLG_NATIVE_MULTIBOOT_TRANSFER_UNSUPPORTED;
    } else if (mb->gcmb_field_2 != 2 && status == FRLG_NATIVE_MULTIBOOT_OK) {
        status = FRLG_NATIVE_MULTIBOOT_TRANSFER_UNSUPPORTED;
    }
}

void GameCubeMultiBoot_Quit(void)
{
    uint16_t ime = read_reg(REG_IME_OFFSET);
    write_reg(REG_IME_OFFSET, 0);
    write_reg(REG_JOYCNT_OFFSET, read_reg(REG_JOYCNT_OFFSET) & (uint16_t)~0x47);
    write_reg(REG_RCNT_OFFSET, 0x8000);
    write_reg(REG_IF_OFFSET, read_reg(REG_IF_OFFSET) & (uint16_t)~SERIAL_IRQ);
    write_reg(REG_IE_OFFSET, read_reg(REG_IE_OFFSET) & (uint16_t)~SERIAL_IRQ);
    write_reg(REG_IME_OFFSET, ime);
}

void GameCubeMultiBoot_HandleSerialInterrupt(struct GcmbStruct *mb)
{
    (void)mb;
    if (status == FRLG_NATIVE_MULTIBOOT_OK)
        status = FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED;
}

void GameCubeMultiBoot_ExecuteProgram(struct GcmbStruct *mb)
{
    if (mb->gcmb_field_2 == 2 && status == FRLG_NATIVE_MULTIBOOT_OK)
        status = FRLG_NATIVE_MULTIBOOT_EXECUTE_UNSUPPORTED;
}
