/* Compile instead of upstream main.c, not alongside it. Keep the original
 * private reader and game state in their translation unit without editing
 * the locked source. AgbMain remains GBA-only and must not be executed. */
#include "../../../external/pokefirered/src/main.c"
#include "frlg_native_audio.h"
#include "frlg_native_flash.h"
#include "frlg_native_main.h"
#include "frlg_native_multiboot.h"
#include "frlg_native_quest_state.h"
#include "frlg_native_scanline.h"

_Static_assert(FRLG_KEY_ALL == KEYS_MASK, "native key mask differs from GBA");

/* The M1 route has no Help or save-failure renderer. Preserve the original
 * callback gate and stop explicitly if either overlay is requested. */
bool8 gHelpSystemEnabled = FALSE;
bool8 gHelpSystemToggleWithRButtonDisabled = FALSE;
u8 gQuestLogPlaybackState = QL_PLAYBACK_STATE_STOPPED;
static bool sNativeSaveFailedScreen;
static bool sNativeUnsupportedOperation;
static FrlgNativeTitleExitStatus sNativeTitleExitStatus;

FrlgNativeTitleExitStatus frlg_native_title_exit_status(void)
{
    return sNativeTitleExitStatus;
}

void CB2_InitMainMenu(void)
{
    if (sNativeTitleExitStatus == FRLG_NATIVE_TITLE_EXIT_OK)
        sNativeTitleExitStatus = FRLG_NATIVE_TITLE_EXIT_MAIN_MENU;
}

void CB2_SaveClearScreen_Init(void)
{
    if (sNativeTitleExitStatus == FRLG_NATIVE_TITLE_EXIT_OK)
        sNativeTitleExitStatus = FRLG_NATIVE_TITLE_EXIT_SAVE_CLEAR;
}

void CB2_InitBerryFixProgram(void)
{
    if (sNativeTitleExitStatus == FRLG_NATIVE_TITLE_EXIT_OK)
        sNativeTitleExitStatus = FRLG_NATIVE_TITLE_EXIT_BERRY_FIX;
}

void SetHelpContext(u8 contextId)
{
    if (contextId != HELPCONTEXT_TITLE_SCREEN)
        sNativeUnsupportedOperation = true;
}

void HelpSystem_Enable(void)
{
    gHelpSystemEnabled = TRUE;
    gHelpSystemToggleWithRButtonDisabled = FALSE;
}

void HelpSystem_Disable(void)
{
    gHelpSystemEnabled = FALSE;
}

bool8 RunHelpSystemCallback(void)
{
    if (gSaveBlock2Ptr->optionsButtonMode != OPTIONS_BUTTON_MODE_HELP ||
        !(gMain.newKeys & (L_BUTTON | R_BUTTON)) ||
        ((gMain.newKeys & R_BUTTON) && gHelpSystemToggleWithRButtonDisabled) ||
        !gHelpSystemEnabled)
        return FALSE;
    sNativeUnsupportedOperation = true;
    return TRUE;
}

void SetNotInSaveFailedScreen(void)
{
    sNativeSaveFailedScreen = false;
}

void DoSaveFailedScreen(u8 saveType)
{
    (void)saveType;
    sNativeSaveFailedScreen = true;
}

bool32 RunSaveFailedScreen(void)
{
    if (!sNativeSaveFailedScreen)
        return FALSE;
    sNativeUnsupportedOperation = true;
    return TRUE;
}

static bool frame_services_ok(void)
{
    return !sNativeUnsupportedOperation &&
        sNativeTitleExitStatus == FRLG_NATIVE_TITLE_EXIT_OK &&
        frlg_native_flash_last_result() == FRLG_NATIVE_FLASH_OK &&
        frlg_native_multiboot_status() == FRLG_NATIVE_MULTIBOOT_OK &&
        frlg_native_quest_status() == FRLG_NATIVE_QUEST_OK &&
        frlg_native_scanline_status() == FRLG_NATIVE_SCANLINE_OK &&
        frlg_native_audio_state()->error == FRLG_NATIVE_AUDIO_ERROR_NONE;
}

void frlg_native_main_read_keys(FrlgKeys held)
{
    REG_KEYINPUT = (held & KEYS_MASK) ^ KEYS_MASK;
    ReadKeys();
}

bool frlg_native_main_init(const char *flash_path)
{
    frlg_native_memory();
    frlg_native_quest_init();
    sNativeUnsupportedOperation = false;
    sNativeTitleExitStatus = FRLG_NATIVE_TITLE_EXIT_OK;
    *(vu16 *)BG_PLTT = RGB_WHITE;
    InitGpuRegManager();
    REG_WAITCNT = WAITCNT_PREFETCH_ENABLE | WAITCNT_WS0_S_1 | WAITCNT_WS0_N_3;
    InitKeys();
    m4aSoundInit();
    EnableVCountIntrAtLine150();
    if (frlg_native_flash_start_erased(flash_path, 0x09C2) != FRLG_NATIVE_FLASH_OK)
        return false;
    InitMainCallbacks();
    InitMapMusic();
    ClearDma3Requests();
    ResetBgs();
    InitHeap(gHeap, HEAP_SIZE);
    SetDefaultFontsPointer();
    gSoftResetDisabled = FALSE;
    gHelpSystemEnabled = FALSE;
    SetNotInSaveFailedScreen();
    gLinkTransferringData = FALSE;
    return gFlashMemoryPresent == TRUE &&
        gMain.callback2 == CB2_InitCopyrightScreenAfterBootup;
}

bool frlg_native_main_step(FrlgKeys held)
{
    if (!frame_services_ok())
        return false;
    frlg_native_scanline_begin_frame();
    frlg_native_main_read_keys(held);
    if (gSoftResetDisabled == FALSE && (gMain.heldKeysRaw & A_BUTTON) &&
        (gMain.heldKeysRaw & B_START_SELECT) == B_START_SELECT) {
        sNativeUnsupportedOperation = true;
        return false;
    }
    CallCallbacks();
    if (!frame_services_ok())
        return false;
    PlayTimeCounter_Update();
    MapMusicMain();
    if (!frame_services_ok())
        return false;
    /* Native frame scheduling supplies the VCount/VBlank work without RFU or
     * cable hardware. Keep the original visible callback and register order. */
    m4aSoundVSync();
    if (!frame_services_ok())
        return false;
    gMain.intrCheck |= INTR_FLAG_VCOUNT;
    if (gMain.vblankCounter1)
        (*gMain.vblankCounter1)++;
    if (gMain.vblankCallback)
        gMain.vblankCallback();
    if (!frame_services_ok())
        return false;
    gMain.vblankCounter2++;
    CopyBufferedValuesToGpuRegs();
    ProcessDma3Requests();
    gPcmDmaCounter = gSoundInfo.pcmDmaCounter;
    m4aSoundMain();
    if (!frame_services_ok())
        return false;
    Random();
    gMain.intrCheck |= INTR_FLAG_VBLANK;
    return true;
}
