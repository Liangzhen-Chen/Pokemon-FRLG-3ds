#include "frlg_video_3ds.h"

#include <3ds.h>
#include <string.h>

enum {
    TOP_WIDTH = 400,
    TOP_HEIGHT = 240,
    TOP_BYTES_PER_PIXEL = 3,
    GBA_X_OFFSET = (TOP_WIDTH - FRLG_GBA_SCREEN_WIDTH) / 2,
    GBA_Y_OFFSET = (TOP_HEIGHT - FRLG_GBA_SCREEN_HEIGHT) / 2
};

void frlg_video_3ds_blit_centered(const FrlgRgb8 *pixels)
{
    u8 *framebuffer = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
    int y;
    int x;

    memset(framebuffer, 0, TOP_WIDTH * TOP_HEIGHT * TOP_BYTES_PER_PIXEL);
    if (pixels == NULL)
        return;

    for (y = 0; y < FRLG_GBA_SCREEN_HEIGHT; y++)
    {
        for (x = 0; x < FRLG_GBA_SCREEN_WIDTH; x++)
        {
            const FrlgRgb8 color = pixels[y * FRLG_GBA_SCREEN_WIDTH + x];
            const int screen_x = GBA_X_OFFSET + x;
            const int screen_y = GBA_Y_OFFSET + y;
            const size_t offset = (size_t)(screen_x * TOP_HEIGHT + (TOP_HEIGHT - 1 - screen_y)) * TOP_BYTES_PER_PIXEL;

            framebuffer[offset] = color.blue;
            framebuffer[offset + 1] = color.green;
            framebuffer[offset + 2] = color.red;
        }
    }
}
