/*------------------------------------------------------------------------------
    ABISSO DS — blocco 2: mappa reale generata, movimento continuo, FOV.

    Schermo superiore: mondo (BG bitmap 16bpp, tile 16px = 16x12 tile visibili)
    Schermo inferiore: console di debug (HUD in arrivo)

    Logica da index.html: canOccupy/tryMoveEntity (:1527-1563),
    castRay/updateFOV (:3745-3785), spawn (:1698).
------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>
#include <time.h>

#include "world.hpp"

#define TILE_PX 16
#define SCR_W   256
#define SCR_H   192
#define CAM_TW  (SCR_W / TILE_PX)   // 16 tile visibili
#define CAM_TH  (SCR_H / TILE_PX)   // 12 tile visibili

static u16* const fb = (u16*)BG_GFX;

// --- colori (RGB15 5-5-5) ---
static const u16 C_BLACK      = RGB15( 1, 1, 1 );
static const u16 C_UNVIS_WALL = RGB15( 2, 2, 3 );
static const u16 C_UNVIS_FLOOR= RGB15( 3, 3, 2 );
static const u16 C_VIS_WALL   = RGB15( 7, 5, 8 );   // pietra
static const u16 C_VIS_FLOOR  = RGB15(10, 8, 5 );   // pavimento terra
static const u16 C_VIS_STAIRS = RGB15(20, 14, 6);   // scale dorate
static const u16 C_TORCH      = RGB15(31, 20, 6 );
static const u16 C_HERO       = RGB15(31, 26, 12);

struct Game {
    abisso::Layout layout;
    std::vector<uint8_t> visible;
    std::vector<uint8_t> visited;
    double px = 0, py = 0;
    double speed = 3.6;              // velocità di test (guerriero web)
    int camX = 0, camY = 0;
    int depth = 1;
    uint32_t worldSeed = 0;
    int lastFovTx = -1, lastFovTy = -1;
    int frame = 0;
};

static Game g;

static uint32_t randomSeedFromRtc()
{
    // seed del mondo: ora Unix dall'RTC (esempio ufficiale time/RealTimeClock)
    return (uint32_t)(time(NULL) & 0xFFFFFFFFu);
}

static void putPixel(int x, int y, u16 color)
{
    if (x < 0 || x >= SCR_W || y < 0 || y >= SCR_H) return;
    fb[y * SCR_W + x] = color;
}

static void fillTile(int tx, int ty, u16 color)
{
    for (int yy = 0; yy < TILE_PX; yy++)
        for (int xx = 0; xx < TILE_PX; xx++)
            fb[(ty * TILE_PX + yy) * SCR_W + (tx * TILE_PX + xx)] = color;
}

static void clearScreen(u16 color)
{
    for (int i = 0; i < SCR_W * SCR_H; i++) fb[i] = color;
}

static void centerCamera()
{
    g.camX = (int)(g.px - CAM_TW / 2.0);
    g.camY = (int)(g.py - CAM_TH / 2.0);
    if (g.camX < 0) g.camX = 0;
    if (g.camY < 0) g.camY = 0;
    if (g.camX > g.layout.w - CAM_TW) g.camX = g.layout.w - CAM_TW;
    if (g.camY > g.layout.h - CAM_TH) g.camY = g.layout.h - CAM_TH;
}

// tile (tx,ty) del mondo -> coordinate schermo (sx,sy) in tile
static inline int screenX(int tx) { return tx - g.camX; }
static inline int screenY(int ty) { return ty - g.camY; }

static void renderWorld()
{
    clearScreen(C_BLACK);
    const size_t n = static_cast<size_t>(g.layout.w) * g.layout.h;
    if (g.visible.size() != n || g.visited.size() != n) return;

    for (int ty = g.camY; ty < g.camY + CAM_TH; ty++) {
        for (int tx = g.camX; tx < g.camX + CAM_TW; tx++) {
            const size_t idx = static_cast<size_t>(ty) * g.layout.w + tx;
            const int sx = screenX(tx), sy = screenY(ty);
            if (sx < 0 || sy < 0 || sx >= CAM_TW || sy >= CAM_TH) continue;
            const uint8_t tile = g.layout.grid[idx];
            if (g.visited[idx]) {
                if (g.visible[idx]) {
                    if (tile == abisso::T_STAIRS) fillTile(sx, sy, C_VIS_STAIRS);
                    else if (tile == abisso::T_WALL) fillTile(sx, sy, C_VIS_WALL);
                    else fillTile(sx, sy, C_VIS_FLOOR);
                } else {
                    if (tile == abisso::T_WALL) fillTile(sx, sy, C_UNVIS_WALL);
                    else fillTile(sx, sy, C_UNVIS_FLOOR);
                }
            }
        }
    }

    // torce visibili: puntino caldo
    for (const abisso::Pt& t : g.layout.torches) {
        const size_t idx = static_cast<size_t>(t.y) * g.layout.w + t.x;
        if (!g.visible[idx]) continue;
        const int sx = screenX(t.x), sy = screenY(t.y);
        if (sx < 0 || sy < 0 || sx >= CAM_TW || sy >= CAM_TH) continue;
        putPixel(sx * TILE_PX + 7, sy * TILE_PX + 7, C_TORCH);
        putPixel(sx * TILE_PX + 8, sy * TILE_PX + 7, C_TORCH);
        putPixel(sx * TILE_PX + 7, sy * TILE_PX + 8, C_TORCH);
        putPixel(sx * TILE_PX + 8, sy * TILE_PX + 8, C_TORCH);
    }

    // eroe (con respiro come nello scheletro web: /6)
    const int sx = (int)((g.px - g.camX) * TILE_PX) + TILE_PX / 2;
    const int sy = (int)((g.py - g.camY) * TILE_PX) + TILE_PX / 2;
    const int r = 6;
    const u16 heroC = ((g.frame / 6) % 2) ? C_HERO : RGB15(31, 24, 14);
    for (int yy = -r; yy < r; yy++)
        for (int xx = -r; xx < r; xx++)
            putPixel(sx + xx, sy + yy, heroC);
}

static void updateInput()
{
    scanKeys();
    const u32 held = keysHeld();
    double dx = 0, dy = 0;
    if (held & KEY_UP)    dy -= 1;
    if (held & KEY_DOWN)  dy += 1;
    if (held & KEY_LEFT)  dx -= 1;
    if (held & KEY_RIGHT) dx += 1;
    if (dx != 0 && dy != 0) { dx *= 0.70710678; dy *= 0.70710678; }
    if (dx != 0 || dy != 0) {
        abisso::tryMoveEntity(g.layout, g.px, g.py, dx, dy, 1.0 / 60.0,
                              g.speed, 0.27);
    }
}

static void updateFov()
{
    const int tx = (int)g.px, ty = (int)g.py;
    if (tx == g.lastFovTx && ty == g.lastFovTy) return;
    g.lastFovTx = tx;
    g.lastFovTy = ty;
    abisso::computeFov(g.layout, tx, ty, g.visible, g.visited);
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
//------------------------------------------------------------------------------
{
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);

    consoleDemoInit();

    bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    iprintf("\x1b[0;0HABISSO DS");
    iprintf("\x1b[2;0HBottom: debug");
    iprintf("\x1b[3;0HD-pad muove, START esce");

    g.worldSeed = randomSeedFromRtc();
    g.depth = 1;
    g.layout = abisso::generateDepth(g.depth, "main", g.worldSeed);
    g.px = g.layout.spawn.x + 0.5;
    g.py = g.layout.spawn.y + 0.5;
    g.visible.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);
    g.visited.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);

    while (pmMainLoop()) {
        swiWaitForVBlank();

        if (keysDown() & KEY_START) break;

        updateInput();
        updateFov();
        centerCamera();
        renderWorld();

        if (g.frame % 30 == 0) {
            iprintf("\x1b[5;0HPiano %d  x=%.1f y=%.1f        ", g.depth, g.px, g.py);
            iprintf("\x1b[6;0Hseed=%08x  cam=%d,%d        ", g.worldSeed, g.camX, g.camY);
            iprintf("\x1b[7;0Hvis=%d  freq=%d      ", g.frame, 60);
        }
        g.frame++;
    }

    return 0;
}
