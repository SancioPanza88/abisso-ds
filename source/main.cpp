/*------------------------------------------------------------------------------
    ABISSO DS — scheletro iniziale

    Schermo superiore: mondo di gioco (BG bitmap 16bpp 256x192)
    Schermo inferiore: console di debug (HUD futura)

    API verificate su nds-examples ufficiali (devkitPro):
    - templates/arm9            (Makefile + main.c, while(pmMainLoop()))
    - Graphics/Backgrounds/16bit_color_bmp (MODE_5_2D, BgType_Bmp16,
      BgSize_B16_256x256, VRAM_A_MAIN_BG, BG_GFX, consoleDemoInit)
------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>

#define TILE_PX 16
#define MAP_W   16
#define MAP_H   12

// Framebuffer del BG bitmap del motore grafico principale
static u16* const fb = (u16*)BG_GFX;

static int heroTx = MAP_W / 2;
static int heroTy = MAP_H / 2;

static void putPixel(int x, int y, u16 color)
{
    if (x < 0 || x >= 256 || y < 0 || y >= 192) return;
    fb[y * 256 + x] = color;
}

static void fillTile(int tx, int ty, u16 color)
{
    for (int yy = 0; yy < TILE_PX; yy++)
        for (int xx = 0; xx < TILE_PX; xx++)
            putPixel(tx * TILE_PX + xx, ty * TILE_PX + yy, color);
}

static void drawTestScene(void)
{
    // sfondo notturno
    for (int y = 0; y < 192; y++)
        for (int x = 0; x < 256; x++)
            putPixel(x, y, RGB15(4, 2, 1));

    // griglia a scacchi tipo "pavimento dell'abisso"
    for (int ty = 0; ty < MAP_H; ty++) {
        for (int tx = 0; tx < MAP_W; tx++) {
            fillTile(tx, ty, (tx + ty) % 2 ? RGB15(8, 6, 4) : RGB15(10, 8, 5));
        }
    }

    // muri perimetrali
    for (int tx = 0; tx < MAP_W; tx++) {
        fillTile(tx, 0, RGB15(6, 4, 8));
        fillTile(tx, MAP_H - 1, RGB15(6, 4, 8));
    }
    for (int ty = 0; ty < MAP_H; ty++) {
        fillTile(0, ty, RGB15(6, 4, 8));
        fillTile(MAP_W - 1, ty, RGB15(6, 4, 8));
    }
}

static void drawHero(void)
{
    fillTile(heroTx, heroTy, RGB15(31, 20, 10));
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
//------------------------------------------------------------------------------
{
    videoSetMode(MODE_5_2D);        // main engine: BG bitmap 16bpp
    videoSetModeSub(MODE_0_2D);     // sub engine: testo
    vramSetBankA(VRAM_A_MAIN_BG);   // VRAM bank A -> BG del main engine

    consoleDemoInit();              // console sul touch screen (sub)

    bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    iprintf("\x1b[0;0HABISSO DS - scheletro");
    iprintf("\x1b[2;0HTop: mondo  Bottom: HUD");
    iprintf("\x1b[3;0HD-pad muove, START esce");

    drawTestScene();

    int frame = 0;

    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();

        u32 held = keysHeld();
        u32 pressed = keysDown();

        if (pressed & KEY_START) break;

        if (held & KEY_UP)    heroTy = (heroTy - 1 + MAP_H) % MAP_H;
        if (held & KEY_DOWN)  heroTy = (heroTy + 1) % MAP_H;
        if (held & KEY_LEFT)  heroTx = (heroTx - 1 + MAP_W) % MAP_W;
        if (held & KEY_RIGHT) heroTx = (heroTx + 1) % MAP_W;

        drawTestScene();
        drawHero();

        // il punto che tremola in basso: un "respiro" del gioco
        int resp = (frame / 6) % 2;
        fillTile(heroTx, heroTy, resp ? RGB15(31, 20, 10) : RGB15(31, 24, 14));

        touchPosition touch;
        touchRead(&touch);
        iprintf("\x1b[5;0HF=%d touch=%d,%d      ", frame, touch.px, touch.py);

        frame++;
    }

    return 0;
}
