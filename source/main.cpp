/*------------------------------------------------------------------------------
    ABISSO DS — main.cpp
    Port di index.html su Nintendo DS (libnds / devkitARM).

    Video init verificato su: devkitPro/nds-examples/Graphics/Backgrounds/
    16bit_color_bmp/source/template.cpp (source ufficiale GitHub).
    Pattern verificato: videoSetMode → vramSetBankA → consoleDemoInit →
    bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0,0) → BG_GFX.
    BIT(15) necessario per pixel opachi in 16-bit bitmap (libnds video.h:
    RGB15(r,g,b) = ((r)|((g)<<5)|((b)<<10)) — bit 15 NON impostato).
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

/* Opaque 16-bit color — BIT(15) sets alpha in NDS 16-bit bitmap mode */
#define OC(r,g,b) (BIT(15) | RGB15(r,g,b))

static const int SCREEN_W = 256;
static const int SCREEN_H = 192;
static const int TILE_PX  = 16;

/* Default room/seed — in the full version these come from a menu */
static const char*  ROOM_CODE = "default";
static const uint32_t WORLD_SEED = 42;

/* --- Tile colors (verified against index.html renderTile) --- */
static u16 tileColor(uint8_t t)
{
    switch (t) {
        case T_WALL:   return OC( 8,  8, 12);
        case T_FLOOR:  return OC(20, 18, 15);
        case T_STAIRS: return OC(20, 28, 20);
        default:       return OC( 4,  4,  6);
    }
}

/* --- Monster display colors (index.html renderMonster) --- */
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
        case 'o': return OC(12, 14,  8);
        case 'z': return OC(12, 14, 10);
        case 'S': return OC(16, 10,  8);
        case 'W': return OC(18, 18, 22);
        case 'k': return OC(10, 14,  8);
        case 'h': return OC(18, 14, 20);
        case 'C': return OC(18, 18, 20);
        case 'c': return OC(14, 10, 14);
        case 'm': return OC(14, 18, 10);
        case 'q': return OC(16, 12, 16);
        case 'G': return OC(14, 12, 10);
        default:  return OC(28, 8, 8);
    }
}

/* --- Item display colors --- */
static u16 itemColor(GroundItemKind k)
{
    switch (k) {
        case GI_GOLD:        return OC(31, 26,  6);
        case GI_GEM:         return OC(10, 20, 31);
        case GI_POTION:      return OC(31, 10, 10);
        case GI_MANA_POTION: return OC(10, 10, 31);
        case GI_POWER:       return OC(31, 20,  6);
        case GI_EQUIP:       return OC(18, 10, 31);
        default:             return OC(31, 31, 31);
    }
}

/* Draw a filled rectangle on the 16-bit bitmap framebuffer */
static void drawRect(u16* fb, int fbW, int x, int y, int w, int h, u16 color)
{
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int px = x + i, py = y + j;
            if (px >= 0 && px < fbW && py >= 0 && py < SCREEN_H)
                fb[py * fbW + px] = color;
        }
    }
}

int main(void)
{
    /* === Main screen: MODE_5 16-bit bitmap === */
    videoSetMode(MODE_5_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    int bgMain = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* mainFb = bgGetGfxPtr(bgMain);

    /* === Sub screen: MODE_0 text console === */
    videoSetModeSub(MODE_0_2D);
    consoleDemoInit();

    /* Clear main screen to opaque black (BIT(15) required!) */
    for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
        mainFb[i] = OC(4, 4, 6);

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

    while (pmMainLoop()) {
        swiWaitForVBlank();
        scanKeys();
        const int held  = keysHeld();
        const int down  = keysDown();
        const double dt = 1.0 / 60.0;

        /* --- Hitstop freeze --- */
        if (g.hitstopT > 0) { g.hitstopT -= dt; continue; }

        /* --- Player input (only when alive) --- */
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
            }

            /* A = attack */
            if (down & KEY_A)
                playerAttack(g, rng);

            /* B = drink HP potion */
            if (down & KEY_B)
                drinkPotion(g.player);

            /* X = drink MP potion */
            if (down & KEY_X)
                drinkManaPotion(g.player);

            /* Stairs: walk onto them → advance */
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
                case TB_HP_POTION: drinkPotion(g.player); break;
                case TB_MP_POTION: drinkManaPotion(g.player); break;
                case TB_MAP: break;
                case TB_EQUIP: uiShowEquip(!uiIsEquipVisible()); break;
                default: break;
            }
        }

        /* --- Update game logic --- */
        updateCombat(g, rng, dt);
        checkItemPickup(g);
        tickBuffs(g.player, dt);

        /* --- Render main screen (game world) --- */
        int camX = (int)(g.player.x * TILE_PX) - SCREEN_W / 2;
        int camY = (int)(g.player.y * TILE_PX) - SCREEN_H / 2;
        fxUpdateShake(g, camX, camY);

        const int mapPixW = layout.w * TILE_PX;
        const int mapPixH = layout.h * TILE_PX;
        if (camX < 0) camX = 0;
        if (camY < 0) camY = 0;
        if (camX > mapPixW - SCREEN_W) camX = mapPixW - SCREEN_W;
        if (camY > mapPixH - SCREEN_H) camY = mapPixH - SCREEN_H;

        /* Clear framebuffer */
        for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
            mainFb[i] = OC(4, 4, 6);

        /* Draw visible tiles */
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

        /* Draw items on ground */
        for (const GroundItem& gi : g.items) {
            const int sx = (int)(gi.x * TILE_PX) - camX - 3;
            const int sy = (int)(gi.y * TILE_PX) - camY - 3;
            drawRect(mainFb, SCREEN_W, sx, sy, 6, 6, itemColor(gi.kind));
        }

        /* Draw monsters */
        for (const Monster& m : g.monsters) {
            const MonsterType& mt = *getMonsterType(m.type);
            const int sx = (int)(m.x * TILE_PX) - camX - 5;
            const int sy = (int)(m.y * TILE_PX) - camY - 5;
            if (sx < -12 || sx > SCREEN_W + 12 || sy < -12 || sy > SCREEN_H + 12) continue;
            drawRect(mainFb, SCREEN_W, sx, sy, 10, 10, monsterColor(m.type, mt.boss));
            /* HP bar */
            if (m.hp < m.maxHp) {
                const int barW = 10;
                const int hpW = (int)(barW * m.hp / m.maxHp);
                drawRect(mainFb, SCREEN_W, sx, sy - 2, hpW, 1, OC(31, 8, 8));
                drawRect(mainFb, SCREEN_W, sx + hpW, sy - 2, barW - hpW, 1, OC(6, 2, 2));
            }
        }

        /* Draw bolts */
        for (const Bolt& b : g.bolts) {
            const int sx = (int)(b.x * TILE_PX) - camX - 2;
            const int sy = (int)(b.y * TILE_PX) - camY - 2;
            drawRect(mainFb, SCREEN_W, sx, sy, 4, 4,
                     b.fromPlayer ? OC(31, 26, 6) : OC(31, 8, 8));
        }

        /* Draw player */
        {
            const int sx = (int)(g.player.x * TILE_PX) - camX - 6;
            const int sy = (int)(g.player.y * TILE_PX) - camY - 6;
            const u16 pc = g.player.dead ? OC(12, 8, 8) : OC(10, 28, 10);
            drawRect(mainFb, SCREEN_W, sx, sy, 12, 12, pc);
            /* Facing direction indicator */
            const int fx = (int)(g.player.fx * 4);
            const int fy = (int)(g.player.fy * 4);
            drawRect(mainFb, SCREEN_W, sx + 5 + fx, sy + 5 + fy, 2, 2, OC(31, 31, 31));
        }

        /* Draw torches */
        for (const Pt& t : layout.torches) {
            const int sx = t.x * TILE_PX - camX + 4;
            const int sy = t.y * TILE_PX - camY + 4;
            drawRect(mainFb, SCREEN_W, sx, sy, 8, 8, OC(28, 22, 6));
        }

        /* Floating texts */
        fxRenderFloatTexts(mainFb, SCREEN_W, camX, camY, g);

        /* Damage flash */
        fxApplyDamageFlash(mainFb, SCREEN_W, SCREEN_H, g.damageFlashT);

        /* Depth fade */
        if (g.depthFadeT > 0) {
            const int alpha = fxGetDepthFadeAlpha(g.depthFadeT, 0.55);
            if (alpha > 0) {
                for (int i = 0; i < SCREEN_W * SCREEN_H; i++)
                    mainFb[i] = OC(alpha, alpha, alpha);
            }
        }

        /* --- Update sub screen console text --- */
        iprintf("\x1b[2J");  /* clear sub screen */
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
    }

    return 0;
}
