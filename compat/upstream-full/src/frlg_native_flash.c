#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "gba/flash_internal.h"
#include "load_save.h"
#include "frlg_native_flash.h"

static u8 sBytes[FLASH_ROM_SIZE_1M];
static u16 sDeviceId;
static bool8 sReady;
static FrlgNativeFlashResult sResult = FRLG_NATIVE_FLASH_NO_MEDIA;

static u16 RejectByte(u16 sector, u32 offset, u8 value)
{
    (void)sector; (void)offset; (void)value;
    sResult = FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED;
    return 1;
}

static u16 RejectSector(u16 sector, void *source)
{
    (void)sector; (void)source;
    sResult = FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED;
    return 1;
}

static u16 RejectChip(void)
{
    sResult = FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED;
    return 1;
}

static u16 RejectEraseSector(u16 sector)
{
    (void)sector;
    sResult = FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED;
    return 1;
}

static u16 RejectWait(u8 phase, u8 *address, u8 value)
{
    (void)phase; (void)address; (void)value;
    sResult = FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED;
    return 1;
}

static const u16 sMaxTime[12] = {
    10, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
    10, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
    2000, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
    2000, 65469, TIMER_ENABLE | TIMER_INTR_ENABLE | TIMER_256CLK,
};

const struct FlashSetupInfo MX29L010 = {
    RejectByte, RejectSector, RejectChip, RejectEraseSector, RejectWait, sMaxTime,
    {131072, {4096, 12, 32, 0}, {3, 1}, {{0xC2, 0x09}}}
};
const struct FlashSetupInfo LE26FV10N1TS = {
    RejectByte, RejectSector, RejectChip, RejectEraseSector, RejectWait, sMaxTime,
    {131072, {4096, 12, 32, 0}, {3, 1}, {{0x62, 0x13}}}
};
const struct FlashSetupInfo DefaultFlash = {
    RejectByte, RejectSector, RejectChip, RejectEraseSector, RejectWait, sMaxTime,
    {131072, {4096, 12, 32, 0}, {3, 1}, {{0x00, 0x00}}}
};

u8 gFlashTimeoutFlag;
u8 (*PollFlashStatus)(u8 *) = NULL;
u16 (*WaitForFlashWrite)(u8, u8 *, u8) = NULL;
u16 (*ProgramFlashSector)(u16, void *) = NULL;
const struct FlashType *gFlash = NULL;
u16 (*ProgramFlashByte)(u16, u32, u8) = NULL;
u16 gFlashNumRemainingBytes;
u16 (*EraseFlashChip)() = NULL;
u16 (*EraseFlashSector)(u16) = NULL;
const u16 *gFlashMaxTime = NULL;

static void UnexpectedFlashTimer(void)
{
    __builtin_trap();
}

u16 SetFlashTimerIntr(u8 timer_num, void (**intr_func)(void))
{
    if (timer_num >= 4 || intr_func == NULL)
        return 1;
    *intr_func = UnexpectedFlashTimer;
    return 0;
}

void StartFlashTimer(u8 phase)
{
    (void)phase;
    __builtin_trap();
}

void StopFlashTimer(void)
{
    __builtin_trap();
}

u32 ProgramFlashSectorAndVerify(u16 sector, u8 *source)
{
    (void)sector; (void)source;
    sResult = FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED;
    return 1;
}

u16 ReadFlashId(void)
{
    if (!sReady)
        __builtin_trap();
    return sDeviceId;
}

void ReadFlash(u16 sector, u32 offset, void *destination, u32 size)
{
    if (!sReady || sector >= 32 || offset > 4096 || size > 4096 - offset
        || (size != 0 && destination == NULL))
        __builtin_trap();
    if (size != 0)
        memcpy(destination, sBytes + (u32)sector * 4096 + offset, size);
}

FrlgNativeFlashResult frlg_native_flash_last_result(void)
{
    return sResult;
}

FrlgNativeFlashResult frlg_native_flash_start_erased(const char *path, uint16_t device_id)
{
    FILE *file;
    size_t count;
    int extra;
    size_t index;

    sReady = FALSE;
    gFlashMemoryPresent = FALSE;
    sResult = FRLG_NATIVE_FLASH_NO_MEDIA;
    if (path == NULL || path[0] == '\0')
        return sResult;
    file = fopen(path, "rb");
    if (file == NULL)
    {
        sResult = errno == ENOENT ? FRLG_NATIVE_FLASH_NO_MEDIA : FRLG_NATIVE_FLASH_IO;
        return sResult;
    }
    count = fread(sBytes, 1, sizeof(sBytes), file);
    if (ferror(file))
        sResult = FRLG_NATIVE_FLASH_IO;
    else if (count != sizeof(sBytes))
        sResult = FRLG_NATIVE_FLASH_BAD_SIZE;
    else
    {
        extra = fgetc(file);
        if (ferror(file))
            sResult = FRLG_NATIVE_FLASH_IO;
        else if (extra != EOF)
            sResult = FRLG_NATIVE_FLASH_BAD_SIZE;
        else
            sResult = FRLG_NATIVE_FLASH_OK;
    }
    if (fclose(file) != 0)
        sResult = FRLG_NATIVE_FLASH_IO;
    if (sResult != FRLG_NATIVE_FLASH_OK)
        return sResult;
    for (index = 0; index < sizeof(sBytes); index++)
        if (sBytes[index] != 0xff)
        {
            sResult = FRLG_NATIVE_FLASH_BAD_FORMAT;
            return sResult;
        }

    sDeviceId = device_id;
    sReady = TRUE;
    CheckForFlashMemory();
    if (!gFlashMemoryPresent)
    {
        sReady = FALSE;
        sResult = FRLG_NATIVE_FLASH_BAD_ID;
        return sResult;
    }
    return sResult;
}
