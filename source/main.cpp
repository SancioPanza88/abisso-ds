/*------------------------------------------------------------------------------
    ABISSO DS — blocco 3: classi, mostri e combattimento.

    Schermo superiore: mondo (BG bitmap 16bpp, tile 16px = 16x12 tile visibili)
    Schermo inferiore: console di debug/HUD testuale

    Logica da index.html: updateMonsterAI (:1835), performAttack (:3330),
    hostDealMonsterHit (:2884), handleRespawn, CLASSES (:1020), MONSTER_TYPES (:1112).
------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>
#include <time.h>

#include "world.hpp"
#include "game.hpp"

#define TILE_PX 16
#define SCR_W   256
#define SCR_H   192
#define CAM_TW  (SCR_W / TILE_PX)   // 16 tile visibili
#define CAM_TH  (SCR_H / TILE_PX)   // 12 tile visibili

static u16* const fb = (u16*)BG_GFX;

// --- colori (RGB15 5-5-5) ---
static const u16 C_BLACK       = RGB15( 1, 1, 1 );
static const u16 C_UNVIS_WALL  = RGB15( 2, 2, 3 );
static const u16 C_UNVIS_FLOOR = RGB15( 3, 3, 2 );
static const u16 C_VIS_WALL    = RGB15( 7, 5, 8 );   // pietra
static const u16 C_VIS_FLOOR   = RGB15(10, 8, 5 );   // pavimento terra
static const u16 C_VIS_STAIRS  = RGB15(20, 14, 6);   // scale dorate
static const u16 C_TORCH       = RGB15(31, 20, 6 );
static const u16 C_HERO        = RGB15(31, 26, 12);
static const u16 C_BOLT_PLAYER = RGB15(20, 26, 31);
static const u16 C_BOLT_MONST  = RGB15(22, 15, 26);
static const u16 C_HP_BG       = RGB15( 6,  2,  2);
static const u16 C_HP_FG       = RGB15(31, 10, 10);
static const u16 C_MP_BG       = RGB15( 2,  4,  7);
static const u16 C_MP_FG       = RGB15(10, 20, 31);

struct Game {
    abisso::Layout layout;
    std::vector<uint8_t> visible;
    std::vector<uint8_t> visited;
    abisso::GameState gs;
    abisso::Rng combatRng;
    int camX = 0, camY = 0;
    int depth = 1;
    uint32_t worldSeed = 0;
    int lastFovTx = -1, lastFovTy = -1;
    int frame = 0;
    int classIdx = 0;
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
    g.camX = (int)(g.gs.player.x - CAM_TW / 2.0);
    g.camY = (int)(g.gs.player.y - CAM_TH / 2.0);
    if (g.camX < 0) g.camX = 0;
    if (g.camY < 0) g.camY = 0;
    if (g.camX > g.layout.w - CAM_TW) g.camX = g.layout.w - CAM_TW;
    if (g.camY > g.layout.h - CAM_TH) g.camY = g.layout.h - CAM_TH;
}

static inline int screenX(int tx) { return tx - g.camX; }
static inline int screenY(int ty) { return ty - g.camY; }

static u16 monsterColor(char key)
{
    switch (key) {
        case 'r': return RGB15(21, 17, 11);
        case 'b': return RGB15(19, 15, 22);
        case 'g': case 'k': case 'q': return RGB15(15, 21, 12);
        case 'j': return RGB15(11, 23, 17);
        case 'J': return RGB15( 6, 20, 13);
        case 's': return RGB15(25, 25, 23);
        case 'o': return RGB15(24, 15,  7);
        case 'z': return RGB15(15, 17, 11);
        case 'S': return RGB15(17,  7, 11);
        case 'W': return RGB15(15, 22, 25);
        case 'h': return RGB15(22, 25, 29);
        case 'C': return RGB15(25, 24, 22);
        case 'c': return RGB15(22, 15, 26);
        case 'm': return RGB15(25, 13,  9);
        case 'G': return RGB15(17, 17, 19);
        case 'D': return RGB15(29, 20,  7);
        case 'X': return RGB15(25, 22, 17);
        case 'L': return RGB15(17, 13, 31);
        case 'M': return RGB15(15, 31, 19);
        case 'R': return RGB15(24, 10,  7);
        case 'K': return RGB15(25, 21, 12);
    }
    return RGB15(31, 31, 31);
}

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

    // mostri visibili (blocco 2x2 tile, boss 3x3)
    for (const abisso::Monster& m : g.gs.monsters) {
        const int tx = (int)m.x, ty = (int)m.y;
        const size_t idx = static_cast<size_t>(ty) * g.layout.w + tx;
        if (!g.visible[idx]) continue;
        const int sx = screenX(tx), sy = screenY(ty);
        if (sx < -1 || sy < -1 || sx >= CAM_TW || sy >= CAM_TH) continue;
        const abisso::MonsterType& mt = *abisso::getMonsterType(m.type);
        const int sz = mt.boss ? 3 : 2;
        const u16 c = monsterColor(m.type);
        const int px0 = sx * TILE_PX + TILE_PX / 2 - (sz * TILE_PX) / 2;
        const int py0 = sy * TILE_PX + TILE_PX / 2 - (sz * TILE_PX) / 2;
        for (int yy = 0; yy < sz * TILE_PX; yy++)
            for (int xx = 0; xx < sz * TILE_PX; xx++)
                putPixel(px0 + xx, py0 + yy, c);
        // barra HP del mostro ferito
        if (m.hp < m.maxHp) {
            const int bw = sz * TILE_PX;
            const int frac = (int)(bw * m.hp / m.maxHp);
            for (int xx = 0; xx < bw; xx++)
                putPixel(px0 + xx, py0 - 3, xx < frac ? RGB15(31, 6, 6) : RGB15(20, 20, 20));
        }
    }

    // proiettili
    for (const abisso::Bolt& b : g.gs.bolts) {
        const int sx = (int)((b.x - g.camX) * TILE_PX) - 1;
        const int sy = (int)((b.y - g.camY) * TILE_PX) - 1;
        const u16 c = b.fromPlayer ? C_BOLT_PLAYER : C_BOLT_MONST;
        for (int yy = 0; yy < 3; yy++)
            for (int xx = 0; xx < 3; xx++)
                putPixel(sx + xx, sy + yy, c);
    }

    // eroe (con respiro come nello scheletro web: /6)
    const abisso::Player& p = g.gs.player;
    const int sx = (int)((p.x - g.camX) * TILE_PX) + TILE_PX / 2;
    const int sy = (int)((p.y - g.camY) * TILE_PX) + TILE_PX / 2;
    const int r = 6;
    const u16 heroC = ((g.frame / 6) % 2) ? C_HERO : RGB15(31, 24, 14);
    for (int yy = -r; yy < r; yy++)
        for (int xx = -r; xx < r; xx++)
            putPixel(sx + xx, sy + yy, heroC);
}

static void renderHud()
{
    const abisso::Player& p = g.gs.player;
    // barra HP (larghezza fissa 52px)
    const int bw = 52;
    const int frac = p.maxHp > 0 ? (int)(bw * p.hp / p.maxHp) : 0;
    for (int yy = 0; yy < 4; yy++)
        for (int xx = 0; xx < bw; xx++)
            putPixel(8 + xx, 4 + yy, xx < frac ? C_HP_FG : C_HP_BG);
    // barra MP (solo classi con mana)
    if (p.maxMp > 0) {
        const int mfrac = (int)(bw * p.mp / p.maxMp);
        for (int yy = 0; yy < 3; yy++)
            for (int xx = 0; xx < bw; xx++)
                putPixel(8 + xx, 9 + yy, xx < mfrac ? C_MP_FG : C_MP_BG);
    }
    // indicatore combattimento boss
    if (g.gs.bossFight && ((g.frame / 20) % 2 == 0)) {
        for (int i = 0; i < 6; i++)
            for (int yy = 0; yy < 8; yy++)
                putPixel(SCR_W / 2 - 24 + i * 8, 6 + yy, RGB15(31, 8, 8));
    }
    // banner di morte
    if (p.dead) {
        for (int yy = 0; yy < 10; yy++)
            for (int xx = 0; xx < 120; xx++)
                putPixel(SCR_W / 2 - 60 + xx, SCR_H / 2 - 5 + yy, RGB15(15, 3, 3));
    }
}

static void updateInput()
{
    scanKeys();
    const u32 held = keysHeld();
    abisso::Player& p = g.gs.player;
    double dx = 0, dy = 0;
    if (held & KEY_UP)    dy -= 1;
    if (held & KEY_DOWN)  dy += 1;
    if (held & KEY_LEFT)  dx -= 1;
    if (held & KEY_RIGHT) dx += 1;
    if (dx != 0 && dy != 0) { dx *= 0.70710678; dy *= 0.70710678; }
    if ((dx != 0 || dy != 0) && !p.dead) {
        p.fx = dx; p.fy = dy;
        const double speed = p.cls ? p.cls->speed : 3.6;
        abisso::tryMoveEntity(g.layout, p.x, p.y, dx, dy, 1.0 / 60.0, speed, 0.27);
    }
    if (keysDown() & KEY_A)
        abisso::playerAttack(g.gs, g.combatRng);
}

static void updateFov()
{
    const int tx = (int)g.gs.player.x, ty = (int)g.gs.player.y;
    if (tx == g.lastFovTx && ty == g.lastFovTy) return;
    g.lastFovTx = tx;
    g.lastFovTy = ty;
    abisso::computeFov(g.layout, tx, ty, g.visible, g.visited);
}

static void newRun(int classIdx)
{
    g.classIdx = classIdx;
    g.gs.player = abisso::Player();
    abisso::playerSetClass(g.gs.player, abisso::CLASS_KEYS[classIdx]);
    g.gs.depth = g.depth;
    g.gs.layout = &g.layout;
    g.gs.bossFight = false;
    g.gs.player.x = g.layout.spawn.x + 0.5;
    g.gs.player.y = g.layout.spawn.y + 0.5;
    abisso::makeMonsters(g.gs, g.combatRng);
    g.visible.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);
    g.visited.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);
    g.lastFovTx = g.lastFovTy = -1;
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

    // ---- SELF-TEST: pattern fisso sul framebuffer subito, prima di tutto ----
    for (int i = 0; i < SCR_W * SCR_H; i++) fb[i] = RGB15(31, 0, 0);

    iprintf("\x1b[0;0HABISSO DS");
    iprintf("\x1b[1;0H[A1] fb test rosso scritto");
    iprintf("\x1b[2;0HA: attacco  SELECT: classe");
    iprintf("\x1b[3;0HSTART: esci");

    g.worldSeed = randomSeedFromRtc();
    iprintf("\x1b[4;0H[A2] seed=%08x", g.worldSeed);

    g.combatRng = abisso::Rng(g.worldSeed ^ 0x9E3779B9u);
    g.depth = 1;
    g.layout = abisso::generateDepth(g.depth, "main", g.worldSeed);
    iprintf("\x1b[5;0H[A3] map %dx%d stanze:%d spawn:%d,%d", g.layout.w, g.layout.h,
            (int)g.layout.rooms.size(), g.layout.spawn.x, g.layout.spawn.y);
    newRun(0);
    iprintf("\x1b[6;0H[A4] mostri:%d  vis:%d  player %.1f,%.1f",
            (int)g.gs.monsters.size(), (int)g.visible.size(),
            g.gs.player.x, g.gs.player.y);

    while (pmMainLoop()) {
        swiWaitForVBlank();

        updateInput();
        const u32 down = keysDown();
        if (down & KEY_START) break;
        if (down & KEY_SELECT) {
            g.depth = 1;
            g.layout = abisso::generateDepth(g.depth, "main", g.worldSeed);
            newRun((g.classIdx + 1) % 9);
        }

        abisso::updateCombat(g.gs, g.combatRng, 1.0 / 60.0);
        updateFov();
        centerCamera();
        renderWorld();
        renderHud();

        if (g.frame % 30 == 0) {
            int visCnt = 0;
            for (size_t i = 0; i < g.visible.size(); i++)
                if (g.visible[i]) visCnt++;
            const abisso::Player& p = g.gs.player;
            iprintf("\x1b[7;0HF=%d cam=%d,%d vis=%d fov=%d,%d  ", g.frame,
                    g.camX, g.camY, visCnt, g.lastFovTx, g.lastFovTy);
            iprintf("\x1b[8;0HHP %d/%d  mostri:%d  p=%d,%d        ", p.hp, p.maxHp,
                    (int)g.gs.monsters.size(), (int)(p.x * 10), (int)(p.y * 10));
        }
        g.frame++;
    }

    return 0;
}
