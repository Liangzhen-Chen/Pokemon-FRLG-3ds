/*
 * P2c3 Mode 0 probe written against libctru APIs. Lifecycle, input, and framebuffer patterns
 * were checked against devkitPro/3ds-examples commit
 * be2001fee08cf8ec7d3366e095e83f6f75da8942.
 */
#include <3ds.h>
#include <stdio.h>

#include "frlg_gba_display.h"
#include "frlg_gba_memory.h"
#include "frlg_gba_mode0.h"
#include "frlg_input_3ds.h"
#include "frlg_runtime.h"
#include "frlg_video_3ds.h"

#ifdef FRLG_DEV_INPUT
#include <sys/stat.h>
#include "frlg_test_input.h"
#define DEV_DIR "sdmc:/frlg-dev-control"
static FrlgTestInput dev_input;

static FrlgKeys dev_poll(uint64_t frame)
{
    uint8_t packet[25];
    FILE *file = frame % 6 == 0 ? fopen(DEV_DIR "/command.bin", "rb") : NULL;
    if (file) {
        size_t size = fread(packet, 1, sizeof(packet), file);
        if (!ferror(file)) frlg_test_input_accept(&dev_input, packet, size);
        fclose(file);
    }
    return frlg_test_input_tick(&dev_input);
}

static void dev_status(uint64_t frame, unsigned x, unsigned y, bool front, FrlgKeys held)
{
    FILE *file = fopen(DEV_DIR "/status.tmp", "w");
    if (!file) return;
    int result = fprintf(file,
        "{\"session\":\"%llu\",\"sequence\":%lu,\"remaining\":%u,"
        "\"frame\":%llu,\"x\":%u,\"y\":%u,\"front\":%u,\"held\":%u}\n",
        (unsigned long long)dev_input.session, (unsigned long)dev_input.sequence,
        dev_input.remaining, (unsigned long long)frame, x, y, front, held);
    int close_result = fclose(file);
    if (result > 0 && close_result == 0) rename(DEV_DIR "/status.tmp", DEV_DIR "/status.json");
}
#endif

static const char *button_state(const FrlgKeypad *keypad, FrlgKeys key)
{
    if (keypad->pressed & key)
        return "DOWN";
    if (keypad->released & key)
        return "UP";
    if (keypad->repeated & key)
        return "RPT";
    if (keypad->held & key)
        return "HELD";
    return "----";
}

static const char *touch_state(u32 down, u32 held, u32 up)
{
    if (down & KEY_TOUCH)
        return "PRESSED ";
    if (up & KEY_TOUCH)
        return "RELEASED";
    if (held & KEY_TOUCH)
        return "HELD    ";
    return "IDLE    ";
}

static void draw_bottom(PrintConsole *bottom, const FrlgKeypad *keypad,
                        const FrlgSnapshot *snapshot, u32 physical_down,
                        u32 physical_held, u32 physical_up,
                        const touchPosition *touch)
{
    consoleSelect(bottom);
    printf("\x1b[1;1HP2c3 Mode 0 + input monitor        ");
    printf("\x1b[3;1HTouch: %-8s X:%3u Y:%3u       ",
           touch_state(physical_down, physical_held, physical_up),
           (unsigned)touch->px, (unsigned)touch->py);
    printf("\x1b[5;1HA:     %-4s  B:      %-4s        ",
           button_state(keypad, FRLG_KEY_A),
           button_state(keypad, FRLG_KEY_B));
    printf("\x1b[6;1HSTART: %-4s  SELECT: %-4s        ",
           button_state(keypad, FRLG_KEY_START),
           button_state(keypad, FRLG_KEY_SELECT));
    printf("\x1b[8;1HUP:    %-4s  DOWN:   %-4s        ",
           button_state(keypad, FRLG_KEY_UP),
           button_state(keypad, FRLG_KEY_DOWN));
    printf("\x1b[9;1HLEFT:  %-4s  RIGHT:  %-4s        ",
           button_state(keypad, FRLG_KEY_LEFT),
           button_state(keypad, FRLG_KEY_RIGHT));
    printf("\x1b[10;1HL:     %-4s  R:      %-4s        ",
           button_state(keypad, FRLG_KEY_L),
           button_state(keypad, FRLG_KEY_R));
    printf("\x1b[12;1HPressed: 0x%04X  Held: 0x%04X    ",
           (unsigned)keypad->pressed, (unsigned)keypad->held);
    printf("\x1b[13;1HRepeat:  0x%04X  Up:   0x%04X    ",
           (unsigned)keypad->repeated, (unsigned)keypad->released);
    printf("\x1b[15;1HLast core press: 0x%04X           ",
           (unsigned)snapshot->last_pressed);
    printf("\x1b[17;1HPhase:%-5s Frame:%-10llu  ",
           frlg_probe_phase_name(snapshot->phase),
           (unsigned long long)snapshot->frame);
    printf("\x1b[19;1HD-pad: scroll BG1  A: priority  ");
    printf("\x1b[21;1HFRLG source: NOT LINKED          ");
    printf("\x1b[23;1HPress X + Y together to exit    ");
}

static bool build_mode0_test_image(FrlgGbaMemory *memory, FrlgRgb8 *pixels)
{
    FrlgGbaDisplaySnapshot display;
    frlg_gba_memory_reset(memory);
    /* Original asymmetric arrow tile, with transparent margins. Four palette
     * banks and flip combinations make decoding errors visible. */
    const uint16_t colors[4] = {0x001f, 0x03e0, 0x7c00, 0x7fff};
    frlg_gba_memory_write16(memory, FRLG_GBA_PALETTE_BASE, 0x1084);
    for (unsigned int bank = 0; bank < 4; bank++)
    {
        frlg_gba_memory_write16(memory, FRLG_GBA_PALETTE_BASE + (bank * 16 + 1) * 2, colors[bank]);
        frlg_gba_memory_write16(memory, FRLG_GBA_PALETTE_BASE + (bank * 16 + 2) * 2, 0x294a);
    }
    /* Separate 8bpp tiles in CBB1; colors 64..191 avoid the 4bpp banks. */
    for (unsigned int i = 0; i < 128; i++) {
        uint16_t gradient = (uint16_t)((i % 32) | (((i / 4) % 32) << 5) | ((31 - i % 32) << 10));
        frlg_gba_memory_write16(memory, FRLG_GBA_PALETTE_BASE + (64 + i) * 2, gradient);
        memory->vram[16384 + 64 + i] = (i % 8 == 0) ? 0 : (uint8_t)(64 + i);
    }
    for (unsigned int y = 0; y < 8; y++)
    for (unsigned int x = 0; x < 8; x++)
    {
        unsigned int index = ((x == 1 && y >= 1 && y <= 6) ||
            (y == 1 && x >= 1 && x <= 5) || (x == 4 && y == 2)) ? 1 : 0;
        memory->vram[32 + y * 4 + x / 2] |= (uint8_t)(index << ((x & 1) * 4));
        memory->vram[64 + y * 4 + x / 2] |= (uint8_t)(((x + y) & 1 ? 2 : 0) << ((x & 1) * 4));
    }
    /* BG0 stationary arrows. BG1 is a 512x512 checker layer, scrollable through
     * all four screenblocks. No ROM or upstream graphics are used. */
    for (unsigned int block = 0; block < 4; block++)
    for (unsigned int tile = 0; tile < 1024; tile++)
    {
        unsigned int bank = (tile % 32 / 4 + block) % 4;
        unsigned int flip = ((tile / 32 / 4) % 4) << 10;
        if (block == 0)
        {
            if (!frlg_gba_memory_write16(memory, FRLG_GBA_VRAM_BASE + 16 * 2048 + tile * 2,
                    (uint16_t)(1 | flip | (bank << 12))))
                return false;
        }
        if (!frlg_gba_memory_write16(memory, FRLG_GBA_VRAM_BASE + (20 + block) * 2048 + tile * 2,
                (uint16_t)(((tile / 32 + tile % 32) & 1 ? 1 : 2) | (bank << 12))))
            return false;
    }
    return frlg_gba_display_set_control(memory, FRLG_GBA_DISPCNT_BG0 | FRLG_GBA_DISPCNT_BG1) &&
        frlg_gba_display_set_background_control(memory, 0, 16 << 8) &&
        frlg_gba_display_set_background_control(memory, 1, (20 << 8) | 0xc000 | 1) &&
        frlg_gba_display_snapshot(memory, &display) &&
        frlg_gba_mode0_render(memory, &display, pixels, FRLG_GBA_SCREEN_PIXELS);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    PrintConsole bottom;
    static FrlgGbaMemory gba_memory;
    static FrlgRgb8 mode0_pixels[FRLG_GBA_SCREEN_PIXELS];
    touchPosition last_touch = {0};
    FrlgKeypad keypad;
    FrlgRuntime runtime;

    gfxInitDefault();
    gfxSetDoubleBuffering(GFX_TOP, false);
    consoleInit(GFX_BOTTOM, &bottom);
    frlg_keypad_reset(&keypad);
    frlg_runtime_init(&runtime);
#ifdef FRLG_DEV_INPUT
    mkdir(DEV_DIR, 0700);
    dev_input.session = ((uint64_t)osGetTime() ^ svcGetSystemTick()) | 1u;
    dev_status(0, 0, 0, false, 0);
#endif

    consoleSelect(&bottom);
    consoleClear();

    if (!build_mode0_test_image(&gba_memory, mode0_pixels))
    {
        printf("Mode 0 test image failed.\n");
        gfxExit();
        return 1;
    }
    frlg_video_3ds_blit_centered(mode0_pixels);

    unsigned int scroll_x = 0, scroll_y = 0;
    bool front = false;
    bool background_8bpp = false;
    while (aptMainLoop())
    {
        hidScanInput();

        const u32 physical_down = hidKeysDown();
        const u32 physical_held = hidKeysHeld();
        const u32 physical_up = hidKeysUp();

        if ((physical_down | physical_held) & KEY_TOUCH)
            hidTouchRead(&last_touch);

        FrlgKeys logical_held = frlg_input_3ds_map(physical_held);
#ifdef FRLG_DEV_INPUT
        logical_held |= dev_poll(runtime.frame);
#endif
        frlg_keypad_update(&keypad, logical_held);
        frlg_runtime_step(&runtime, &keypad);
        const FrlgSnapshot snapshot = frlg_runtime_snapshot(&runtime);

        scroll_x = (scroll_x + !!(keypad.held & FRLG_KEY_RIGHT) - !!(keypad.held & FRLG_KEY_LEFT)) & 511;
        scroll_y = (scroll_y + !!(keypad.held & FRLG_KEY_DOWN) - !!(keypad.held & FRLG_KEY_UP)) & 511;
        if (keypad.pressed & FRLG_KEY_A)
            front = !front;
        if (keypad.pressed & FRLG_KEY_B)
            background_8bpp = !background_8bpp;
        frlg_gba_memory_write16(&gba_memory, FRLG_GBA_REG_BG0HOFS + 4, (uint16_t)scroll_x);
        frlg_gba_memory_write16(&gba_memory, FRLG_GBA_REG_BG0VOFS + 4, (uint16_t)scroll_y);
        frlg_gba_display_set_background_control(&gba_memory, 0, (16 << 8) | (front ? 2 : 0));
        frlg_gba_display_set_background_control(&gba_memory, 1,
            (20 << 8) | 0xc000 | 1 | (background_8bpp ? 0x84 : 0));
        FrlgGbaDisplaySnapshot display;
        if (!frlg_gba_display_snapshot(&gba_memory, &display) ||
            !frlg_gba_mode0_render(&gba_memory, &display, mode0_pixels, FRLG_GBA_SCREEN_PIXELS))
        {
            gfxExit();
            return 1;
        }
        frlg_video_3ds_blit_centered(mode0_pixels);

        draw_bottom(&bottom, &keypad, &snapshot, physical_down, physical_held,
                    physical_up, &last_touch);
        printf("\x1b[20;1HB: BG1 color mode  %s      ", background_8bpp ? "8bpp" : "4bpp");
#ifdef FRLG_DEV_INPUT
        printf("\x1b[21;1HDEV INPUT seq:%-10lu           ", (unsigned long)dev_input.sequence);
        static uint32_t reported_sequence;
        static FrlgKeys reported_held;
        if (snapshot.frame % 60 == 0 || reported_sequence != dev_input.sequence ||
            reported_held != keypad.held) {
            dev_status(snapshot.frame, scroll_x, scroll_y, front, keypad.held);
            reported_sequence = dev_input.sequence;
            reported_held = keypad.held;
        }
#endif

        gfxFlushBuffers();
        gfxSwapBuffers();
        gspWaitForVBlank();

        if ((physical_held & (KEY_X | KEY_Y)) == (KEY_X | KEY_Y))
            break;
    }

    gfxExit();
    return 0;
}
