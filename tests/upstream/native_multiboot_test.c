#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "global.h"
#include "libgcnmultiboot.h"
#include "link.h"
#include "main.h"
#include "frlg_native_multiboot.h"

static _Alignas(8) FrlgGbaMemory memory;
struct Main gMain;

static uint16_t read_reg(uint32_t offset)
{
    uint16_t value;
    assert(frlg_gba_memory_read16(&memory, FRLG_GBA_IO_BASE + offset, &value));
    return value;
}

static void write_reg(uint32_t offset, uint16_t value)
{
    assert(frlg_gba_memory_write16(&memory, FRLG_GBA_IO_BASE + offset, value));
}

static void set_word(struct GcmbStruct *mb, size_t offset, uint32_t value)
{
    memcpy((uint8_t *)mb + offset, &value, sizeof(value));
}

static uint32_t get_word(const struct GcmbStruct *mb, size_t offset)
{
    uint32_t value;
    memcpy(&value, (const uint8_t *)mb + offset, sizeof(value));
    return value;
}

static void test_normal(void)
{
    struct GcmbStruct mb = {0};
    uint8_t *bytes = (uint8_t *)&mb;
    assert(sizeof(mb) == 0x2c);
    bytes[1] = 0x55;
    bytes[2] = 2;
    bytes[3] = 0x27;
    memset(bytes + 4, 0xa5, 0x1c);
    set_word(&mb, 0x20, 0x02000100);
    set_word(&mb, 0x24, 0x02000100);
    write_reg(0x208, 1);
    write_reg(0x200, 0x0040);
    write_reg(0x202, 0x00c1);
    write_reg(0x140, 0x0007);
    write_reg(0x134, 0x1234);
    write_reg(0x158, 0x0030);
    GameCubeMultiBoot_Init(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);
    assert(bytes[0] == 0 && bytes[1] == 0x27 && bytes[2] == 0 && bytes[3] == 0x2a);
    for (size_t i = 4; i < 0x20; ++i) assert(bytes[i] == 0);
    assert(get_word(&mb, 0x20) == 0x02000100);
    assert(get_word(&mb, 0x24) == 0x02000100);
    assert(get_word(&mb, 0x28) != 0);
    assert(read_reg(0x208) == 1 && read_reg(0x200) == 0x00c0);
    assert(read_reg(0x202) == 0x0041 && read_reg(0x140) == 0x0040);
    assert(read_reg(0x134) == 0xc000 && read_reg(0x158) == 0);

    memset(&mb, 0, sizeof(mb));
    set_word(&mb, 0x20, 0x02000100);
    set_word(&mb, 0x24, 0x020001a0);
    GameCubeMultiBoot_Main(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);
    assert(get_word(&mb, 0x28) != 0);

    memset(&mb, 0, sizeof(mb));
    GameCubeMultiBoot_Init(&mb);
    for (unsigned i = 1; i <= 11; ++i) {
        GameCubeMultiBoot_Main(&mb);
        assert(bytes[0] == i && bytes[1] == i && bytes[2] == 0);
    }
    set_word(&mb, 0x20, 0x02000100);
    set_word(&mb, 0x24, 0x020001a0);
    GameCubeMultiBoot_Main(&mb);
    assert(bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0 && bytes[3] == 6);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);

    bytes[0] = 5;
    bytes[1] = 0xff;
    bytes[2] = 2;
    write_reg(0x208, 0);
    GameCubeMultiBoot_Main(&mb);
    assert(bytes[0] == 5 && bytes[1] == 0 && bytes[2] == 2 && read_reg(0x208) == 0);
    GameCubeMultiBoot_ExecuteProgram(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_EXECUTE_UNSUPPORTED);
    GameCubeMultiBoot_Init(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);
    assert(read_reg(0x208) == 0);

    uint8_t before[sizeof(mb)];
    memcpy(before, &mb, sizeof(mb));
    write_reg(0x208, 1);
    write_reg(0x200, 0x00c0);
    write_reg(0x202, 0x00c1);
    write_reg(0x140, 0x0047);
    GameCubeMultiBoot_Quit();
    assert(memcmp(before, &mb, sizeof(mb)) == 0);
    assert(read_reg(0x208) == 1 && read_reg(0x200) == 0x0040);
    assert(read_reg(0x202) == 0x0041 && read_reg(0x140) == 0);
    assert(read_reg(0x134) == 0x8000);
}

static void test_serial(void)
{
    struct GcmbStruct mb = {0};
    GameCubeMultiBoot_Init(&mb);
    GameCubeMultiBoot_HandleSerialInterrupt(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    assert(mb.gcmb_field_2 == 0);
    set_word(&mb, 0x20, 0x02000100);
    set_word(&mb, 0x24, 0x020001a0);
    GameCubeMultiBoot_Main(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    GameCubeMultiBoot_Quit();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    GameCubeMultiBoot_Init(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);
}

static void test_execute(void)
{
    struct GcmbStruct mb = {0};
    GameCubeMultiBoot_Init(&mb);
    GameCubeMultiBoot_ExecuteProgram(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);
    mb.gcmb_field_2 = 2;
    write_reg(0x208, 1);
    GameCubeMultiBoot_ExecuteProgram(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_EXECUTE_UNSUPPORTED);
    assert(read_reg(0x208) == 1);
}

static void test_transfer(void)
{
    struct GcmbStruct mb = {0};
    GameCubeMultiBoot_Init(&mb);
    set_word(&mb, 0x20, 0x02000100);
    set_word(&mb, 0x24, 0x0200019f);
    GameCubeMultiBoot_Main(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);
    set_word(&mb, 0x24, 0x020001a0);
    GameCubeMultiBoot_Main(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_TRANSFER_UNSUPPORTED);
    assert(mb.gcmb_field_2 == 0);
    GameCubeMultiBoot_Init(&mb);
    set_word(&mb, 0x20, 0x02000100);
    set_word(&mb, 0x24, 0x020000ff);
    GameCubeMultiBoot_Main(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_TRANSFER_UNSUPPORTED);
}

static void test_serial_boundary(void)
{
    struct GcmbStruct mb = {0};
    GameCubeMultiBoot_Init(&mb);
    gMain.state = 142;
    gMain.serialCallback = SerialCB;
    write_reg(0x200, 0x0041);
    write_reg(0x134, 0x8000);
    write_reg(0x140, 0);
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    GameCubeMultiBoot_Init(&mb);
    write_reg(0x200, 0x00c1);
    GameCubeMultiBoot_Quit();
    assert((read_reg(0x200) & 0x80) == 0);
    write_reg(0x10e, 0x0040);
    write_reg(0x202, 0x00c1);
    write_reg(0x128, 0xa55a);
    write_reg(0x12a, 0x1234);
    write_reg(0x120, 0x5678);
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);
    assert(read_reg(0x200) == 0x0001 && read_reg(0x202) == 0x0001);
    assert(read_reg(0x10e) == 0 && gMain.serialCallback == SerialCB);
    assert(read_reg(0x128) == 0xa55a && read_reg(0x12a) == 0x1234);
    assert(read_reg(0x120) == 0x5678);
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);

    GameCubeMultiBoot_Init(&mb);
    GameCubeMultiBoot_Quit();
    SerialCB();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    GameCubeMultiBoot_Init(&mb);
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK);

    GameCubeMultiBoot_Quit();
    gMain.state = 141;
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    GameCubeMultiBoot_Init(&mb);
    GameCubeMultiBoot_Quit();
    gMain.state = 142;
    write_reg(0x200, read_reg(0x200) | 0x80);
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    assert(read_reg(0x200) & 0x80);
    GameCubeMultiBoot_Init(&mb);
    GameCubeMultiBoot_Quit();
    gMain.serialCallback = NULL;
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    gMain.serialCallback = SerialCB;
    GameCubeMultiBoot_Init(&mb);
    GameCubeMultiBoot_Quit();
    write_reg(0x10e, 0x0080);
    ResetSerial();
    assert(frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_SERIAL_UNSUPPORTED);
    assert(read_reg(0x10e) == 0x0080);
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    assert(frlg_native_io_bind(&memory));
    if (!strcmp(argv[1], "normal")) test_normal();
    else if (!strcmp(argv[1], "serial")) test_serial();
    else if (!strcmp(argv[1], "execute")) test_execute();
    else if (!strcmp(argv[1], "transfer")) test_transfer();
    else if (!strcmp(argv[1], "serial-boundary")) test_serial_boundary();
    else assert(0);
    return 0;
}
