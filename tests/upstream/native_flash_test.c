#if defined(FRLG_NATIVE_INTEGRATION_TEST)
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "frlg_native_flash.h"
#include "gba/flash_internal.h"
#include "load_save.h"
#include "save.h"

bool32 gFlashMemoryPresent;
u16 gSaveFileStatus;
void (*gGameContinueCallback)(void);
struct SaveSector gSaveDataBuffer;
struct SaveSector *gSaveDataBufferPtr = &gSaveDataBuffer;
struct SaveSectorLocation gRamSaveSectorLocations[NUM_SECTORS_PER_SLOT];
u8 gDecompressionBuffer[0x4000];
u32 gSaveCounter;
u16 gLastWrittenSector;
static u8 ram[NUM_SECTORS_PER_SLOT][SECTOR_DATA_SIZE];
static u16 io[0x400 / 2];
static void (*flash_timer_callback)(void);
static unsigned load_serialized_calls;

uintptr_t frlg_native_io_base(void)
{
    return (uintptr_t)io;
}

void InitFlashTimer(void)
{
    if (SetFlashTimerIntr(2, &flash_timer_callback) != 0)
        __builtin_trap();
}

static void UpdateSaveAddresses(void)
{
    for (u16 id = 0; id < NUM_SECTORS_PER_SLOT; id++) {
        gRamSaveSectorLocations[id].data = ram[id];
        gRamSaveSectorLocations[id].size = SECTOR_DATA_SIZE;
    }
}

void LoadSerializedGame(void)
{
    load_serialized_calls++;
}

#include "flash_check.inc"
#include "integration_flash_io.inc"
#include "integration_save_functions.inc"

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    memset(ram, 0xa5, sizeof(ram));
    if (frlg_native_flash_start_erased(argv[1], 0x09c2) != FRLG_NATIVE_FLASH_OK
        || !gFlashMemoryPresent || flash_timer_callback == NULL)
        return 1;
    if (LoadGameSave(SAVE_NORMAL) != SAVE_STATUS_EMPTY
        || gSaveFileStatus != SAVE_STATUS_EMPTY || gSaveCounter != 0
        || load_serialized_calls != 1 || gGameContinueCallback != NULL)
        return 1;
    for (size_t i = 0; i < sizeof(ram); i++)
        if (((u8 *)ram)[i] != 0xa5) return 1;
    puts("original flash-to-save empty path: ok");
    return 0;
}
#elif defined(FRLG_NATIVE_SERVICE_TEST)
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "global.h"
#include "frlg_native_flash.h"
#include "gba/flash_internal.h"
#include "load_save.h"

bool32 gFlashMemoryPresent;
static u16 io[0x400 / 2];
static void (*flash_timer_callback)(void);

uintptr_t frlg_native_io_base(void)
{
    return (uintptr_t)io;
}

void InitFlashTimer(void)
{
    if (SetFlashTimerIntr(2, &flash_timer_callback) != 0)
        __builtin_trap();
}

void CheckForFlashMemory(void)
{
    if (!IdentifyFlash()) {
        gFlashMemoryPresent = TRUE;
        InitFlashTimer();
    } else {
        gFlashMemoryPresent = FALSE;
    }
}

static int expect(FrlgNativeFlashResult actual, FrlgNativeFlashResult wanted, const char *name)
{
    if (actual == wanted) return 0;
    fprintf(stderr, "%s: got %d, wanted %d\n", name, actual, wanted);
    return 1;
}

int main(int argc, char **argv)
{
    u8 sector[4096];
    u8 before[4096];
    if (argc != 7) return 2;
    if (expect(frlg_native_flash_start_erased(NULL, 0x09c2),
               FRLG_NATIVE_FLASH_NO_MEDIA, "null")) return 1;
    if (expect(frlg_native_flash_start_erased(argv[2], 0x09c2),
               FRLG_NATIVE_FLASH_NO_MEDIA, "missing")) return 1;
    if (expect(frlg_native_flash_start_erased(argv[3], 0x09c2),
               FRLG_NATIVE_FLASH_BAD_SIZE, "short")) return 1;
    if (expect(frlg_native_flash_start_erased(argv[4], 0x09c2),
               FRLG_NATIVE_FLASH_BAD_SIZE, "long")) return 1;
    if (expect(frlg_native_flash_start_erased(argv[5], 0x09c2),
               FRLG_NATIVE_FLASH_BAD_FORMAT, "format")) return 1;
    if (expect(frlg_native_flash_start_erased(argv[6], 0x09c2),
               FRLG_NATIVE_FLASH_IO, "io")) return 1;
    if (gFlashMemoryPresent) return 1;
    if (expect(frlg_native_flash_start_erased(argv[1], 0),
               FRLG_NATIVE_FLASH_BAD_ID, "id")) return 1;
    if (gFlashMemoryPresent || gFlash == NULL) return 1;
    if (expect(frlg_native_flash_start_erased(argv[1], 0x09c2),
               FRLG_NATIVE_FLASH_OK, "erased")) return 1;
    if (!gFlashMemoryPresent || flash_timer_callback == NULL || ReadFlashId() != 0x09c2)
        return 1;
    if ((io[0x204 / 2] & 3) != WAITCNT_SRAM_8) return 1;
    ReadFlash(31, 0, sector, sizeof(sector));
    for (size_t i = 0; i < sizeof(sector); i++)
        if (sector[i] != 0xff) return 1;
    memcpy(before, sector, sizeof(before));
    if (ProgramFlashByte(0, 0, 0) == 0 || ProgramFlashSector(0, sector) == 0
        || EraseFlashChip() == 0 || EraseFlashSector(0) == 0
        || WaitForFlashWrite(0, sector, 0) == 0
        || ProgramFlashSectorAndVerify(0, sector) == 0) return 1;
    if (expect(frlg_native_flash_last_result(),
               FRLG_NATIVE_FLASH_WRITE_UNSUPPORTED, "write")) return 1;
    ReadFlash(31, 0, sector, sizeof(sector));
    if (memcmp(before, sector, sizeof(before)) != 0) return 1;
    puts("native flash service: ok");
    return 0;
}
#else
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef u8 bool8;
#define TRUE 1
#define FALSE 0
#define NUM_SECTORS_PER_SLOT 14
#define NUM_SAVE_SLOTS 2
#define SECTOR_SIZE 4096
#define SECTOR_DATA_SIZE 3968
#define SECTOR_SIGNATURE 0x08012025
#define SAVE_STATUS_EMPTY 0
#define SAVE_STATUS_OK 1
#define SAVE_STATUS_INVALID 2
#define SAVE_STATUS_ERROR 0xff
#define SAVE_STATUS_NO_FLASH 4
#define FULL_SAVE_SLOT 0xffff
#define SAVE_NORMAL 0
#define SAVE_HALL_OF_FAME 1
#define SECTOR_ID_HOF_1 28
#define SECTOR_ID_HOF_2 29

struct SaveSector {
    u8 data[SECTOR_DATA_SIZE];
    u8 unused[116];
    u16 id;
    u16 checksum;
    u32 signature;
    u32 counter;
};
struct SaveSectorLocation { u8 *data; u16 size; };

static u8 image[32][SECTOR_SIZE];
static u8 ram[NUM_SECTORS_PER_SLOT][SECTOR_DATA_SIZE];
static struct SaveSector gSaveDataBuffer;
static struct SaveSector *gSaveDataBufferPtr = &gSaveDataBuffer;
static struct SaveSectorLocation gRamSaveSectorLocations[NUM_SECTORS_PER_SLOT];
static u8 gDecompressionBuffer[2 * SECTOR_DATA_SIZE];
static u8 gSaveFileStatus;
static u8 gFlashMemoryPresent = TRUE;
static void (*gGameContinueCallback)(void);
static unsigned load_serialized_calls;
static u32 gSaveCounter;
static u16 gLastWrittenSector;

static void UpdateSaveAddresses(void)
{
    for (u16 id = 0; id < NUM_SECTORS_PER_SLOT; id++) {
        gRamSaveSectorLocations[id].data = ram[id];
        gRamSaveSectorLocations[id].size = SECTOR_DATA_SIZE;
    }
}

static void LoadSerializedGame(void)
{
    load_serialized_calls++;
}

static u8 ReadFlashSector(u8 sectorId, struct SaveSector *sector)
{
    memcpy(sector, image[sectorId], SECTOR_SIZE);
    return 1;
}

static u16 CalculateChecksum(void *data, u16 size)
{
    u32 sum = 0;
    u16 i;
    for (i = 0; i < size / 4; i++) {
        u32 word;
        memcpy(&word, (u8 *)data + i * 4, 4);
        sum += word;
    }
    return (u16)((sum >> 16) + sum);
}

#include "save_functions.inc"

int main(int argc, char **argv)
{
    u16 id;
    if (argc != 2 && argc != 3) return 2;
    memset(image, 0xff, sizeof(image));
    memset(ram, 0xa5, sizeof(ram));
    if (strcmp(argv[1], "bad-id") == 0) {
        struct SaveSector *sector = (struct SaveSector *)image[0];
        sector->id = 14;
        sector->signature = SECTOR_SIGNATURE;
    } else if (strcmp(argv[1], "valid") == 0 || strcmp(argv[1], "bad-checksum") == 0) {
        for (id = 0; id < NUM_SECTORS_PER_SLOT; id++) {
            struct SaveSector *sector = (struct SaveSector *)image[id];
            memset(sector, 0, sizeof(*sector));
            sector->id = id;
            sector->signature = SECTOR_SIGNATURE;
        }
        for (id = 0; id < NUM_SECTORS_PER_SLOT; id++) {
            struct SaveSector *sector = (struct SaveSector *)image[id];
            for (u16 j = 0; j < SECTOR_DATA_SIZE; j++)
                sector->data[j] = (u8)(id * 17 + j);
            sector->checksum = CalculateChecksum(sector->data, SECTOR_DATA_SIZE);
        }
        if (strcmp(argv[1], "bad-checksum") == 0)
            ((struct SaveSector *)image[1])->checksum ^= 1;
    } else if (strcmp(argv[1], "empty") != 0) return 2;

    u8 status = LoadGameSave(SAVE_NORMAL);
    if (status != gSaveFileStatus || load_serialized_calls != 1 || gGameContinueCallback != NULL)
        return 1;
    if (strcmp(argv[1], "valid") == 0 && status != SAVE_STATUS_OK)
        return 1;
    if (strcmp(argv[1], "bad-checksum") == 0 && status != SAVE_STATUS_INVALID)
        return 1;
    if (strcmp(argv[1], "empty") == 0 && status != SAVE_STATUS_EMPTY)
        return 1;
    if (strcmp(argv[1], "valid") == 0 || strcmp(argv[1], "bad-checksum") == 0) {
        for (id = 0; id < NUM_SECTORS_PER_SLOT; id++) {
            for (u16 j = 0; j < SECTOR_DATA_SIZE; j++) {
                u8 wanted = strcmp(argv[1], "bad-checksum") == 0 && id == 1
                    ? 0xa5 : (u8)(id * 17 + j);
                if (ram[id][j] != wanted) return 1;
            }
        }
    }
    if (argc == 3) {
        FILE *output = fopen(argv[2], "wb");
        if (output == NULL) return 1;
        size_t written = fwrite(ram, 1, sizeof(ram), output);
        int closed = fclose(output);
        if (written != sizeof(ram) || closed != 0) return 1;
    }
    printf("status=%u counter=%u first=%u\n", status, gSaveCounter, ram[0][0]);
    return 0;
}
#endif
