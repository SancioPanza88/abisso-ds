/*------------------------------------------------------------------------------
    ABISSO DS — main.cpp
    Port di index.html su Nintendo DS (libnds / devkitARM).

    Video: MODE_5_2D bitmap BG on VRAM_A + OAM sprites on VRAM_B.
    Verified from: devkitPro/nds-examples/Graphics/16bit_color_bmp + Sprites.
    RGB15 lacks bit 15 (alpha); BIT(15)|RGB15 needed for opaque pixels.
    Sprites: oamInit with SpriteMapping_Bmp_1D_128, SpriteColorFormat_Bmp.
    DMA used for framebuffer clear (verified: dmaFillWords from nds/dma.h).
------------------------------------------------------------------------------*/
#include <nds.h>
#include <cmath>
#include <cstring>
#include <algorithm>

#include "world.hpp"
#include "game.hpp"
#include "fx.hpp"
#include "ui.hpp"
#include "sfx.hpp"

using namespace abisso;

#define OC(r,g,b) (BIT(15) | RGB15(r,g,b))

static const int SCREEN_W = 256;
static const int SCREEN_H = 192;
static const int TILE_PX  = 16;
static const int SPR_PX   = 16;
static const int SPR_BYTES = SPR_PX * SPR_PX * 2;

static const char*  ROOM_CODE = "default";
static const uint32_t WORLD_SEED = 42;

/* --- Grit-generated bitmap externs (16x16 16-bit) --- */
extern "C" {
    extern const unsigned short hero_bardoBitmap[];
    extern const unsigned short hero_monacoBitmap[];
    extern const unsigned short hero_negromanteBitmap[];
    extern const unsigned short hero_paladinoBitmap[];
    extern const unsigned short mon_arpiaBitmap[];
    extern const unsigned short mon_cavaliereBitmap[];
    extern const unsigned short mon_cultistaBitmap[];
    extern const unsigned short mon_dragoBitmap[];
    extern const unsigned short mon_golemBitmap[];
    extern const unsigned short mon_mantideBitmap[];
    extern const unsigned short mon_orcoBitmap[];
    extern const unsigned short mon_sciamanoBitmap[];
    extern const unsigned short mon_serpenteBitmap[];
    extern const unsigned short mon_spettroBitmap[];
    extern const unsigned short boss_golemBitmap[];
    extern const unsigned short boss_lichBitmap[];
    extern const unsigned short boss_melmeBitmap[];
    extern const unsigned short boss_ragnoBitmap[];
    extern const unsigned short boss_rattiBitmap[];
    extern const unsigned short icon_goldBitmap[];
    extern const unsigned short icon_gem_blueBitmap[];
    extern const unsigned short icon_potion_hpBitmap[];
    extern const unsigned short icon_potion_manaBitmap[];
    extern const unsigned short icon_shield_buffBitmap[];
}

/* --- OAM sprite GFX pointers --- */
static u16* sprHero[4];
static u16* sprMonster[256];
static u16* sprItem[8];
static u16* sprDefault;

static u16* loadSpr(const void* data)
{
    u16* gfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_Bmp);
    if (gfx) dmaCopy(data, gfx, SPR_BYTES);
    return gfx;
}

static void initSprites()
{
    for (int i = 0; i < 256; i++) sprMonster[i] = 0;
    for (int i = 0; i < 8; i++) sprItem[i] = 0;
    for (int i = 0; i < 4; i++) sprHero[i] = 0;

    sprHero[0] = loadSpr(hero_bardoBitmap);
    sprHero[1] = loadSpr(hero_monacoBitmap);
    sprHero[2] = loadSpr(hero_negromanteBitmap);
    sprHero[3] = loadSpr(hero_paladinoBitmap);

    sprMonster['o'] = loadSpr(mon_orcoBitmap);
    sprMonster['W'] = loadSpr(mon_spettroBitmap);
    sprMonster['k'] = loadSpr(mon_serpenteBitmap);
    sprMonster['h'] = loadSpr(mon_arpiaBitmap);
    sprMonster['C'] = loadSpr(mon_cavaliereBitmap);
    sprMonster['c'] = loadSpr(mon_cultistaBitmap);
    sprMonster['m'] = loadSpr(mon_mantideBitmap);
    sprMonster['q'] = loadSpr(mon_sciamanoBitmap);
    sprMonster['G'] = loadSpr(mon_golemBitmap);
    sprMonster['D'] = loadSpr(mon_dragoBitmap);
    sprMonster['S'] = loadSpr(boss_ragnoBitmap);
    sprMonster['X'] = loadSpr(boss_golemBitmap);
    sprMonster['L'] = loadSpr(boss_lichBitmap);
    sprMonster['M'] = loadSpr(boss_melmeBitmap);
    sprMonster['R'] = loadSpr(boss_ragnoBitmap);
    sprMonster['K'] = loadSpr(boss_rattiBitmap);

    sprItem[GI_GOLD]        = loadSpr(icon_goldBitmap);
    sprItem[GI_GEM]         = loadSpr(icon_gem_blueBitmap);
    sprItem[GI_POTION]      = loadSpr(icon_potion_hpBitmap);
    sprItem[GI_MANA_POTION] = loadSpr(icon_potion_manaBitmap);
    sprItem[GI_POWER]       = loadSpr(icon_shield_buffBitmap);
    sprItem[GI_EQUIP]       = loadSpr(icon_shield_buffBitmap);
}

static int heroIdx(const char* key)
{
    if (!key) return 0;
    if (std::strcmp(key, "paladino") == 0)   return 3;
    if (std::strcmp(key, "negromante") == 0) return 2;
    if (std::strcmp(key, "monaco") == 0)     return 1;
    if (std::strcmp(key, "bardo") == 0)      return 0;
    if (std::strcmp(key, "mago") == 0)       return 2;
    if (std::strcmp(key, "prof") == 0)       return 3;
    if (std::strcmp(key, "guerriero") == 0)  return 3;
    if (std::strcmp(key, "ladro") == 0)      return 0;
    if (std::strcmp(key, "ranger") == 0)     return 0;
    return 0;
}

/* --- Tile colors --- */
static u16 tileColor(uint8_t t)
{
    switch (t) {
        case T_WALL:   return OC( 8,  8, 12);
        case T_FLOOR:  return OC(20, 18, 15);
        case T_STAIRS: return OC(20, 28, 20);
        default:       return OC( 4,  4,  6);
    }
}

/* --- Monster fallback colors (for types without sprites) --- */
static u16 monsterColor(char type, bool boss)
{
    if (boss) return OC(28, 8, 8);
    switch (type) {
        case 'r': return OC(14, 10,  8);
        case 'b': return OC(10,  8, 14);
        case 'g': return OC(10, 16,  8);
        case 'j': return OC( 8, 18,  8);
        case 'J': return OC(12, 22, 12);
        case 's': return OC(22, 22, 22);
        case 'z': return OC(12, 14, 10);
        default:  return OC(28, 8, 8);
    }
}

static void drawRect(u16* fb, int fbW, int x, int y, int w, int h, u16 color)
{
    for (int j = 0; j < h; j++) {
        int row = (y + j) * fbW + x;
        for (int i = 0; i < w; i++) {
            int px = x + i, py = y + j;
            if (px >= 0 && px < fbW && py >= 0 && py < SCREEN_H)
                fb[row + i] = color;
        }
    }
}

static void dmaClearFb(u16* fb, u16 color)
{
    const u32 fill = (u32)color | ((u32)color << 16);
    dmaFillWords(&fill, fb, SCREEN_W * SCREEN_H * sizeof(u16));
}

int main(void)
{
    /* === Main screen: MODE_5 + OAM sprites === */
    videoSetMode(MODE_5_2D | DISPLAY_SPR_ACTIVE);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    int bgMain = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* mainFb = bgGetGfxPtr(bgMain);

    /* === Sub screen: MODE_0 text console === */
    videoSetModeSub(MODE_0_2D);
    consoleDemoInit();

    /* === OAM init for bitmap sprites === */
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);

    /* === Load all sprites into VRAM_B === */
    initSprites();

    /* Clear main screen */
    dmaClearFb(mainFb, OC(4, 4, 6));

    /* --- Init game state --- */
    Rng rng(WORLD_SEED);
    Layout layout = generateDepth(1, ROOM_CODE, WORLD_SEED);

    GameState g;
    g.layout    = &layout;
    g.rng       = &rng;
    g.worldSeed = WORLD_SEED;
    g.depth     = 1;

    playerSetClass(g.player, "guerriero");
    g.player.x = layout.spawn.x + 0.5;
    g.player.y = layout.spawn.y + 0.5;

    makeMonsters(g, rng);
    makeInitialItems(g, layout, 1, rng);

    sfxInit();
    uiInit();

    int prevTileX = (int)g.player.x;
    int prevTileY = (int)g.player.y;
    int lastDepth = -1;
    bool needSubRedraw = true;

    while (pmMainLoop()) {
        scanKeys();
        const int held  = keysHeld();
        const int down  = keysDown();
        const double dt = 1.0 / 60.0;

        if (g.hitstopT > 0) { g.hitstopT -= dt; continue; }

        /* --- Player input --- */
        if (!g.player.dead) {
            double dx = 0, dy = 0;
            if (held & KEY_UP)    dy -= 1;
            if (held & KEY_DOWN)  dy += 1;
            if (held & KEY_LEFT)  dx -= 1;
            if (held & KEY_RIGHT) dx += 1;

            if (dx != 0 || dy != 0) {
                double len = std::sqrt(dx * dx + dy * dy);
                dx /= len; dy /= len;
                double spd = 3.5;
                if (g.player.buffHaste > 0) spd *= 1.35;
                const EquipStat eq = computeEquipBonus(g.player);
                spd *= 1.0 + eq.speedPct / 100.0;
                tryMoveEntity(layout, g.player.x, g.player.y,
                              dx, dy, dt, spd, 0.35);
                g.player.fx = dx;
                g.player.fy = dy;
                needSubRedraw = true;
            }

            if (down & KEY_A) { playerAttack(g, rng); needSubRedraw = true; }
            if (down & KEY_B) { drinkPotion(g.player); needSubRedraw = true; }
            if (down & KEY_X) { drinkManaPotion(g.player); needSubRedraw = true; }

            int curTX = (int)g.player.x;
            int curTY = (int)g.player.y;
            if (curTX != prevTileX || curTY != prevTileY) {
                if (layout.grid[curTY * layout.w + curTX] == T_STAIRS) {
                    int newDepth = g.depth + 1;
                    Layout newLayout = generateDepth(newDepth, ROOM_CODE, WORLD_SEED);
                    advanceDepth(g, newLayout, rng);
                    layout = newLayout;
                    g.layout = &layout;
                    makeMonsters(g, rng);
                    makeInitialItems(g, layout, newDepth, rng);
                    g.player.x = layout.spawn.x + 0.5;
                    g.player.y = layout.spawn.y + 0.5;
                    needSubRedraw = true;
                }
            }
            prevTileX = curTX;
            prevTileY = curTY;
        }

        /* --- Touch screen input --- */
        if (down & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            int btn = uiHandleTouch(touch, SCREEN_W, SCREEN_H);
            switch (btn) {
                case TB_HP_POTION: drinkPotion(g.player); needSubRedraw = true; break;
                case TB_MP_POTION: drinkManaPotion(g.player); needSubRedraw = true; break;
                case TB_MAP: break;
                case TB_EQUIP: uiShowEquip(!uiIsEquipVisible()); break;
                default: break;
            }
        }

        /* --- Update game logic --- */
        updateCombat(g, rng, dt);
        checkItemPickup(g);
        tickBuffs(g.player, dt);

        /* --- Camera --- */
        int camX = (int)(g.player.x * TILE_PX) - SCREEN_W / 2;
        int camY = (int)(g.player.y * TILE_PX) - SCREEN_H / 2;
        fxUpdateShake(g, camX, camY);

        const int mapPixW = layout.w * TILE_PX;
        const int mapPixH = layout.h * TILE_PX;
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
        if (camX > mapPixW - SCREEN_W) camX = mapPixW - SCREEN_W;
        if (camY > mapPixH - SCREEN_H) camY = mapPixH - SCREEN_H;

        /* --- Render BG layer (tiles, effects) --- */
        dmaClearFb(mainFb, OC(4, 4, 6));

        int tx0 = camX / TILE_PX;
        int ty0 = camY / TILE_PX;
        int tx1 = (camX + SCREEN_W) / TILE_PX + 1;
        int ty1 = (camY + SCREEN_H) / TILE_PX + 1;
        if (tx0 < 0) tx0 = 0;
        if (ty0 < 0) ty0 = 0;
        if (tx1 >= layout.w) tx1 = layout.w - 1;
        if (ty1 >= layout.h) ty1 = layout.h - 1;

        for (int ty = ty0; ty <= ty1; ty++) {
            for (int tx = tx0; tx <= tx1; tx++) {
                const int sx = tx * TILE_PX - camX;
                const int sy = ty * TILE_PX - camY;
                const u16 c = tileColor(layout.grid[ty * layout.w + tx]);
                drawRect(mainFb, SCREEN_W, sx, sy, TILE_PX, TILE_PX, c);
            }
        }

        /* Draw torches on BG */
        for (const Pt& t : layout.torches) {
            const int sx = t.x * TILE_PX - camX + 4;
            const int sy = t.y * TILE_PX - camY + 4;
            drawRect(mainFb, SCREEN_W, sx, sy, 8, 8, OC(28, 22, 6));
        }

        /* Draw bolts on BG */
        for (const Bolt& b : g.bolts) {
            const int sx = (int)(b.x * TILE_PX) - camX - 2;
            const int sy = (int)(b.y * TILE_PX) - camY - 2;
            drawRect(mainFb, SCREEN_W, sx, sy, 4, 4,
                     b.fromPlayer ? OC(31, 26, 6) : OC(31, 8, 8));
        }

        /* Floating texts on BG */
        fxRenderFloatTexts(mainFb, SCREEN_W, camX, camY, g);

        /* Depth fade */
        if (g.depthFadeT > 0) {
            const int alpha = fxGetDepthFadeAlpha(g.depthFadeT, 0.55);
            if (alpha > 0) {
                u16 fadeColor = OC(alpha, alpha, alpha);
                dmaClearFb(mainFb, fadeColor);
            }
        }

        /* --- OAM sprites: clear, then set all visible entities --- */
        oamClear(&oamMain, 0, 0);
        int oid = 0;

        /* Items as OAM sprites */
        for (const GroundItem& gi : g.items) {
            if (oid >= 128) break;
            const int sx = (int)(gi.x * TILE_PX) - camX - 8;
            const int sy = (int)(gi.y * TILE_PX) - camY - 8;
            if (sx < -16 || sx > SCREEN_W + 16 || sy < -16 || sy > SCREEN_H + 16) continue;
            u16* gfx = sprItem[gi.kind];
            if (gfx) {
                oamSet(&oamMain, oid++, sx, sy, 2, 15,
                       SpriteSize_16x16, SpriteColorFormat_Bmp,
                       gfx, -1, false, false, false, false, false);
            } else {
                drawRect(mainFb, SCREEN_W, sx + 5, sy + 5, 6, 6,
                         gi.kind == GI_GOLD ? OC(31,26,6) :
                         gi.kind == GI_GEM ? OC(10,20,31) :
                         gi.kind == GI_POTION ? OC(31,10,10) : OC(10,10,31));
            }
        }

        /* Monsters as OAM sprites */
        for (const Monster& m : g.monsters) {
            if (oid >= 128) break;
            const MonsterType& mt = *getMonsterType(m.type);
            const int sx = (int)(m.x * TILE_PX) - camX - 8;
            const int sy = (int)(m.y * TILE_PX) - camY - 8;
            if (sx < -16 || sx > SCREEN_W + 16 || sy < -16 || sy > SCREEN_H + 16) continue;
            u16* gfx = sprMonster[(unsigned char)m.type];
            if (gfx) {
                oamSet(&oamMain, oid++, sx, sy, 1, 15,
                       SpriteSize_16x16, SpriteColorFormat_Bmp,
                       gfx, -1, false, false, false, false, false);
            } else {
                drawRect(mainFb, SCREEN_W, sx + 3, sy + 3, 10, 10,
                         monsterColor(m.type, mt.boss));
            }
            /* HP bar on BG (above sprite, not covered by OAM) */
            if (m.hp < m.maxHp) {
                const int barW = 10;
                const int hpW = (int)(barW * m.hp / m.maxHp);
                drawRect(mainFb, SCREEN_W, sx + 3, sy + 1, hpW, 1, OC(31, 8, 8));
                drawRect(mainFb, SCREEN_W, sx + 3 + hpW, sy + 1, barW - hpW, 1, OC(6, 2, 2));
            }
        }

        /* Player as OAM sprite */
        if (oid < 128) {
            const int sx = (int)(g.player.x * TILE_PX) - camX - 8;
            const int sy = (int)(g.player.y * TILE_PX) - camY - 8;
            u16* gfx = sprHero[heroIdx(g.player.cls->key)];
            if (gfx && !g.player.dead) {
                oamSet(&oamMain, oid++, sx, sy, 0, 15,
                       SpriteSize_16x16, SpriteColorFormat_Bmp,
                       gfx, -1, false, false, false, false, false);
            } else {
                const u16 pc = g.player.dead ? OC(12, 8, 8) : OC(10, 28, 10);
                drawRect(mainFb, SCREEN_W, sx + 2, sy + 2, 12, 12, pc);
            }
        }

        /* --- VBlank: apply OAM changes --- */
        swiWaitForVBlank();
        oamUpdate(&oamMain);

        /* --- Sub screen: only redraw when values change --- */
        if (needSubRedraw || g.depth != lastDepth) {
            iprintf("\x1b[2J");
            iprintf("  ABISSO DS\n");
            iprintf("  Piano %d\n\n", g.depth);
            if (g.player.dead) {
                iprintf("  SEI MORTO!\n");
                iprintf("  Respawn: %.0fs\n", g.player.respawnT);
            } else {
                iprintf("  HP: %d/%d\n", g.player.hp, g.player.maxHp);
                if (g.player.maxMp > 0)
                    iprintf("  MP: %d/%d\n", g.player.mp, g.player.maxMp);
                iprintf("  Oro: %d\n", g.player.gold);
                iprintf("  Pozioni: %d\n", g.player.potions);
                if (g.player.buffRage > 0)   iprintf("  FURIA\n");
                if (g.player.buffShield > 0) iprintf("  SCUDO\n");
                if (g.player.buffHaste > 0)  iprintf("  FRETTA\n");
                if (g.player.buffFocus > 0)  iprintf("  FOCUS\n");
            }
            iprintf("\n  D-Pad:Muovi A:Attacca\n");
            iprintf("  B:PozHP X:PozMP\n");
            needSubRedraw = false;
            lastDepth = g.depth;
        }
    }

    return 0;
}
