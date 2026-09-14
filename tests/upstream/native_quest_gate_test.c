#include <assert.h>

#include "global.h"
#include "berry_fix_program.h"
#include "clear_save_data_screen.h"
#include "frlg_native_audio.h"
#include "frlg_native_flash.h"
#include "frlg_native_main.h"
#include "frlg_native_multiboot.h"
#include "frlg_native_quest_state.h"
#include "frlg_native_scanline.h"
#include "gpu_regs.h"
#include "load_save.h"
#include "main_menu.h"
#include "m4a.h"
#include "quest_log.h"

struct SaveBlock1 gSaveBlock1;
struct SaveBlock2 gSaveBlock2;
struct SaveBlock1 *gSaveBlock1Ptr;
struct SaveBlock2 *gSaveBlock2Ptr;
struct SoundInfo gSoundInfo;
bool32 gFlashMemoryPresent = TRUE;

static FrlgGbaMemory sMemory;
static FrlgNativeAudioState sAudioState;
static int sCallbackCount;
static int sLaterPhaseCount;
static void (*sTitleExitCallback)(void);

FrlgGbaMemory *frlg_native_memory(void) { return &sMemory; }
uintptr_t frlg_native_io_base(void) { return (uintptr_t)sMemory.io; }
const FrlgNativeAudioState *frlg_native_audio_state(void) { return &sAudioState; }
FrlgNativeFlashResult frlg_native_flash_last_result(void) { return FRLG_NATIVE_FLASH_OK; }
FrlgNativeFlashResult frlg_native_flash_start_erased(const char *path, uint16_t device_id)
{
    (void)path;
    (void)device_id;
    return FRLG_NATIVE_FLASH_OK;
}
FrlgNativeMultibootStatus frlg_native_multiboot_status(void) { return FRLG_NATIVE_MULTIBOOT_OK; }
FrlgNativeScanlineStatus frlg_native_scanline_status(void) { return FRLG_NATIVE_SCANLINE_OK; }
void frlg_native_scanline_begin_frame(void) {}

void InitGpuRegManager(void) {}
u16 GetGpuReg(u8 offset) { (void)offset; return 0; }
void SetGpuReg(u8 offset, u16 value) { (void)offset; (void)value; }
void EnableInterrupts(u16 mask) { (void)mask; }
void m4aSoundInit(void) {}
void m4aSoundVSync(void) {}
void m4aSoundMain(void) {}
void InitMapMusic(void) {}
void ClearDma3Requests(void) {}
void ResetBgs(void) {}
void InitHeap(void *pointer, u32 size) { (void)pointer; (void)size; }
void SetDefaultFontsPointer(void) {}
void CB2_InitCopyrightScreenAfterBootup(void) {}
void PlayTimeCounter_Update(void) { sLaterPhaseCount++; }
void MapMusicMain(void) { sLaterPhaseCount++; }
void CopyBufferedValuesToGpuRegs(void) { sLaterPhaseCount++; }
void ProcessDma3Requests(void) { sLaterPhaseCount++; }
u16 Random(void) { sLaterPhaseCount++; return 0; }

static void set_unsupported_quest_state(void)
{
    sCallbackCount++;
    gQuestLogState = 1;
    QL_AddASLROffset(&gSaveBlock1);
}

static void schedule_title_exit(void)
{
    sCallbackCount++;
    SetMainCallback2(sTitleExitCallback);
}

static void test_title_exit(void (*callback)(void), FrlgNativeTitleExitStatus expected)
{
    int callbackCount = sCallbackCount;
    int laterPhaseCount = sLaterPhaseCount;

    assert(frlg_native_main_init("erased-test.sav"));
    assert(frlg_native_title_exit_status() == FRLG_NATIVE_TITLE_EXIT_OK);
    sTitleExitCallback = callback;
    SetMainCallback2(schedule_title_exit);
    assert(frlg_native_main_step(0));
    assert(sCallbackCount == callbackCount + 1);
    assert(sLaterPhaseCount > laterPhaseCount);
    laterPhaseCount = sLaterPhaseCount;

    assert(!frlg_native_main_step(0));
    assert(frlg_native_title_exit_status() == expected);
    assert(sLaterPhaseCount == laterPhaseCount);
    assert(!frlg_native_main_step(0));
    assert(frlg_native_title_exit_status() == expected);
    assert(sLaterPhaseCount == laterPhaseCount);
    assert(frlg_native_main_init("erased-test.sav"));
    assert(frlg_native_title_exit_status() == FRLG_NATIVE_TITLE_EXIT_OK);
}

int main(void)
{
    gQuestLogState = 1;
    QL_AddASLROffset(&gSaveBlock1);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);

    assert(frlg_native_main_init("erased-test.sav"));
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);
    SetMainCallback2(set_unsupported_quest_state);
    assert(!frlg_native_main_step(0));
    assert(sCallbackCount == 1);
    assert(sLaterPhaseCount == 0);
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_UNSUPPORTED_STATE);
    gQuestLogState = 0;
    assert(!frlg_native_main_step(0));
    assert(sCallbackCount == 1 && sLaterPhaseCount == 0);
    assert(frlg_native_main_init("erased-test.sav"));
    assert(frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK);
    test_title_exit(CB2_InitMainMenu, FRLG_NATIVE_TITLE_EXIT_MAIN_MENU);
    test_title_exit(CB2_SaveClearScreen_Init, FRLG_NATIVE_TITLE_EXIT_SAVE_CLEAR);
    test_title_exit(CB2_InitBerryFixProgram, FRLG_NATIVE_TITLE_EXIT_BERRY_FIX);
    return 0;
}
