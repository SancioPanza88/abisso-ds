/*------------------------------------------------------------------------------
    ABISSO DS — blocco finale: tutti i sistemi integrati.
    Schermo superiore: mondo (BG bitmap 16bpp, tile 16px)
    Schermo inferiore: HUD touch, minimap, pannelli
------------------------------------------------------------------------------*/
#include <nds.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include <cmath>

#include "world.hpp"
#include "game.hpp"
#include "fx.hpp"
#include "ui.hpp"
#include "sfx.hpp"

#define TILE_PX 16
#define SCR_W   256
#define SCR_H   192
#define CAM_TW  (SCR_W / TILE_PX)
#define CAM_TH  (SCR_H / TILE_PX)

static u16* const fb = (u16*)BG_GFX;

/* --- colori RGB15 --- */
static const u16 C_BLACK       = RGB15( 1, 1, 1 );
static const u16 C_UNVIS_WALL  = RGB15( 2, 2, 3 );
static const u16 C_UNVIS_FLOOR = RGB15( 3, 3, 2 );
static const u16 C_VIS_WALL    = RGB15( 7, 5, 8 );
static const u16 C_VIS_FLOOR   = RGB15(10, 8, 5 );
static const u16 C_VIS_STAIRS  = RGB15(20, 14, 6);
static const u16 C_TORCH       = RGB15(31, 20, 6 );
static const u16 C_HERO        = RGB15(31, 26, 12);
static const u16 C_BOLT_PLAYER = RGB15(20, 26, 31);
static const u16 C_BOLT_MONST  = RGB15(22, 15, 26);
static const u16 C_CHEST_CLOSED = RGB15(21, 17, 6);
static const u16 C_CHEST_OPEN  = RGB15(14, 11, 5);
static const u16 C_MERCHANT    = RGB15(21, 18, 10);
static const u16 C_GOLD_ITEM   = RGB15(31, 26, 6);
static const u16 C_GEM_ITEM    = RGB15(10, 20, 31);
static const u16 C_POTION_ITEM = RGB15(31, 10, 10);
static const u16 C_MANA_ITEM   = RGB15(10, 20, 31);
static const u16 C_POWER_ITEM  = RGB15(20, 28, 15);
static const u16 C_EQUIP_ITEM  = RGB15(18, 10, 31);
static const u16 C_BOSS_GATE   = RGB15(28, 10, 5);

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
    bool showMap = false;
    bool showEquip = false;
    bool merchantOpen = false;
};

static Game g;

static uint32_t randomSeedFromRtc()
{
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

    /* boss gates: tile speciali */
    if (g.gs.bossActive && g.layout.hasBossRoom) {
        for (const abisso::Pt& gate : g.layout.bossRoom.gates) {
            const int sx = screenX(gate.x), sy = screenY(gate.y);
            if (sx < 0 || sy < 0 || sx >= CAM_TW || sy >= CAM_TH) continue;
            fillTile(sx, sy, C_BOSS_GATE);
        }
    }

    /* torce */
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

    /* forzieri */
    for (const abisso::ChestSpot& c : g.layout.chestSpots) {
        const size_t idx = static_cast<size_t>(c.y) * g.layout.w + c.x;
        if (!g.visible[idx]) continue;
        const int sx = screenX(c.x), sy = screenY(c.y);
        if (sx < 0 || sy < 0 || sx >= CAM_TW || sy >= CAM_TH) continue;
        const bool opened = g.gs.chestsOpened.find(c.id) != g.gs.chestsOpened.end();
        const u16 color = opened ? C_CHEST_OPEN : C_CHEST_CLOSED;
        fillTile(sx, sy, color);
    }

    /* mercante */
    {
        const abisso::Pt& mp = g.layout.merchantPos;
        const size_t idx = static_cast<size_t>(mp.y) * g.layout.w + mp.x;
        if (g.visible[idx]) {
            const int sx = screenX(mp.x), sy = screenY(mp.y);
            if (sx >= 0 && sy >= 0 && sx < CAM_TW && sy < CAM_TH)
                fillTile(sx, sy, C_MERCHANT);
        }
    }

    /* oggetti a terra */
    for (const abisso::GroundItem& it : g.gs.items) {
        const int tx = (int)it.x, ty = (int)it.y;
        const size_t idx = static_cast<size_t>(ty) * g.layout.w + tx;
        if (!g.visible[idx]) continue;
        const int sx = screenX(tx), sy = screenY(ty);
        if (sx < 0 || sy < 0 || sx >= CAM_TW || sy >= CAM_TH) continue;
        u16 color;
        switch (it.kind) {
            case abisso::GI_GOLD:        color = C_GOLD_ITEM; break;
            case abisso::GI_GEM:         color = C_GEM_ITEM; break;
            case abisso::GI_POTION:      color = C_POTION_ITEM; break;
            case abisso::GI_MANA_POTION: color = C_MANA_ITEM; break;
            case abisso::GI_POWER:       color = C_POWER_ITEM; break;
            case abisso::GI_EQUIP:       color = C_EQUIP_ITEM; break;
            default: color = RGB15(31,31,31); break;
        }
        /* small dot for items */
        putPixel(sx * TILE_PX + 7, sy * TILE_PX + 7, color);
        putPixel(sx * TILE_PX + 8, sy * TILE_PX + 7, color);
        putPixel(sx * TILE_PX + 7, sy * TILE_PX + 8, color);
        putPixel(sx * TILE_PX + 8, sy * TILE_PX + 8, color);
    }

    /* mostri visibili */
    for (const abisso::Monster& m : g.gs.monsters) {
        const int tx = (int)m.x, ty = (int)m.y;
        const size_t idx = static_cast<size_t>(ty) * g.layout.w + tx;
        if (!g.visible[idx]) continue;
        const int sx = screenX(tx), sy = screenY(ty);
        if (sx < -1 || sy < -1 || sx >= CAM_TW || sy >= CAM_TH) continue;
        const abisso::MonsterType& mt = *abisso::getMonsterType(m.type);
        const int sz = mt.boss ? 3 : 2;
        u16 c = monsterColor(m.type);
        /* affix indicator */
        if (m.affix == 'f') c = RGB15(25, 25, 5);  /* veloce: yellow */
        if (m.affix == 'e') c = RGB15(31, 15, 5);  /* esplosivo: orange */
        if (m.affix == 'r') c = RGB15(10, 25, 10); /* rigenerante: green */
        const int px0 = sx * TILE_PX + TILE_PX / 2 - (sz * TILE_PX) / 2;
        const int py0 = sy * TILE_PX + TILE_PX / 2 - (sz * TILE_PX) / 2;
        for (int yy = 0; yy < sz * TILE_PX; yy++)
            for (int xx = 0; xx < sz * TILE_PX; xx++)
                putPixel(px0 + xx, py0 + yy, c);
        /* barra HP mostro ferito */
        if (m.hp < m.maxHp) {
            const int bw = sz * TILE_PX;
            const int frac = (int)(bw * m.hp / m.maxHp);
            for (int xx = 0; xx < bw; xx++)
                putPixel(px0 + xx, py0 - 3, xx < frac ? RGB15(31, 6, 6) : RGB15(20, 20, 20));
        }
    }

    /* proiettili */
    for (const abisso::Bolt& b : g.gs.bolts) {
        const int sx = (int)((b.x - g.camX) * TILE_PX) - 1;
        const int sy = (int)((b.y - g.camY) * TILE_PX) - 1;
        const u16 c = b.fromPlayer ? C_BOLT_PLAYER : C_BOLT_MONST;
        for (int yy = 0; yy < 3; yy++)
            for (int xx = 0; xx < 3; xx++)
                putPixel(sx + xx, sy + yy, c);
    }

    /* eroe (con respiro) */
    const abisso::Player& p = g.gs.player;
    const int hsx = (int)((p.x - g.camX) * TILE_PX) + TILE_PX / 2;
    const int hsy = (int)((p.y - g.camY) * TILE_PX) + TILE_PX / 2;
    const int r = 6;
    const u16 heroC = ((g.frame / 6) % 2) ? C_HERO : RGB15(31, 24, 14);
    for (int yy = -r; yy < r; yy++)
        for (int xx = -r; xx < r; xx++)
            putPixel(hsx + xx, hsy + yy, heroC);

    /* floating text */
    abisso::fxRenderFloatTexts(fb, SCR_W, g.camX, g.camY, g.gs);

    /* damage flash overlay */
    if (g.gs.damageFlashT > 0)
        abisso::fxApplyDamageFlash(fb, SCR_W, SCR_H, g.gs.damageFlashT);

    /* depth fade overlay */
    if (g.gs.depthFadeT > 0) {
        const int alpha = abisso::fxGetDepthFadeAlpha(g.gs.depthFadeT, 0.55);
        if (alpha > 0) {
            for (int y = 0; y < SCR_H; y++)
                for (int x = 0; x < SCR_W; x++)
                    fb[y * SCR_W + x] = RGB15(alpha, alpha, alpha);
        }
    }
}

static void newRun(int classIdx);

static void updateInput()
{
    scanKeys();
    u32 held = keysHeld();
    u32 down = keysDown();
    abisso::Player& p = g.gs.player;

    /* touch */
    if (keysHeld() & KEY_TOUCH) {
        touchPosition touch;
        touchRead(&touch);
        const int btn = abisso::uiHandleTouch(touch, 256, 192);
        switch (btn) {
            case abisso::TB_HP_POTION:
                abisso::drinkPotion(p);
                break;
            case abisso::TB_MP_POTION:
                abisso::drinkManaPotion(p);
                break;
            case abisso::TB_INTERACT:
                down |= KEY_B;
                break;
            case abisso::TB_MAP:
                g.showMap = !g.showMap;
                break;
            case abisso::TB_EQUIP:
                g.showEquip = !g.showEquip;
                abisso::uiShowEquip(g.showEquip);
                break;
        }
    }

    /* movement */
    double dx = 0, dy = 0;
    if (held & KEY_UP)    dy -= 1;
    if (held & KEY_DOWN)  dy += 1;
    if (held & KEY_LEFT)  dx -= 1;
    if (held & KEY_RIGHT) dx += 1;
    if (dx != 0 && dy != 0) { dx *= 0.70710678; dy *= 0.70710678; }
    if ((dx != 0 || dy != 0) && !p.dead) {
        p.fx = dx; p.fy = dy;
        double speed = p.cls ? p.cls->speed : 3.6;
        if (p.buffHaste > 0) speed *= 1.4;
        abisso::tryMoveEntity(g.layout, p.x, p.y, dx, dy, 1.0 / 60.0, speed, 0.27);
    }

    /* attack */
    if (down & KEY_A)
        abisso::playerAttack(g.gs, g.combatRng);

    /* B = interact (stairs, chest, merchant) */
    if (down & KEY_B) {
        const int tx = (int)p.x, ty = (int)p.y;
        /* stairs */
        if (g.layout.grid[(size_t)ty * g.layout.w + tx] == abisso::T_STAIRS) {
            abisso::advanceDepth(g.gs, g.layout, g.combatRng);
            g.depth = g.gs.depth;
            g.layout = abisso::generateDepth(g.depth, "main", g.worldSeed);
            g.gs.layout = &g.layout;
            abisso::makeInitialItems(g.gs, g.layout, g.depth, g.combatRng);
            abisso::makeMonsters(g.gs, g.combatRng);
            g.gs.player.x = g.layout.spawn.x + 0.5;
            g.gs.player.y = g.layout.spawn.y + 0.5;
            g.visible.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);
            g.visited.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);
            g.lastFovTx = g.lastFovTy = -1;
            abisso::sfxStair();
            return;
        }
        /* merchant */
        {
            const abisso::Pt& mp = g.layout.merchantPos;
            const double dmx = p.x - (mp.x + 0.5), dmy = p.y - (mp.y + 0.5);
            if (dmx * dmx + dmy * dmy < 2.1 * 2.1) {
                g.merchantOpen = !g.merchantOpen;
                abisso::uiShowMerchant(g.merchantOpen);
            }
        }
        /* chest */
        if (!g.merchantOpen) {
            for (const abisso::ChestSpot& c : g.layout.chestSpots) {
                if (g.gs.chestsOpened.find(c.id) != g.gs.chestsOpened.end()) continue;
                const double dcx = p.x - (c.x + 0.5), dcy = p.y - (c.y + 0.5);
                if (dcx * dcx + dcy * dcy < 2.1 * 2.1) {
                    bool isBoss = (g.layout.hasBossRoom && c.id == g.layout.bossRoom.chest.id);
                    int gold = abisso::openChest(g.gs, g.combatRng, c.id, isBoss);
                    g.gs.chestsOpened.insert(c.id);
                    if (gold >= 0) {
                        abisso::sfxChest();
                        abisso::spawnFloatText(g.gs, p.x, p.y - 0.7,
                            ("+" + std::to_string(gold) + " Au").c_str(), 1);
                    } else {
                        abisso::spawnFloatText(g.gs, p.x, p.y - 0.7, "SIGILLATO", 3);
                    }
                    break;
                }
            }
        }
    }

    /* L = HP potion, R = MP potion */
    if (down & KEY_L) { abisso::drinkPotion(p); abisso::sfxPotion(); }
    if (down & KEY_R) { abisso::drinkManaPotion(p); abisso::sfxMana(); }

    /* X = toggle map */
    if (down & KEY_X) {
        g.showMap = !g.showMap;
        abisso::uiShowEquip(false);
        g.showEquip = false;
    }

    /* Y = toggle equipment */
    if (down & KEY_Y) {
        g.showEquip = !g.showEquip;
        abisso::uiShowEquip(g.showEquip);
        g.showMap = false;
    }

    /* SELECT = cycle class */
    if (down & KEY_SELECT) {
        g.depth = 1;
        g.layout = abisso::generateDepth(g.depth, "main", g.worldSeed);
        g.gs.layout = &g.layout;
        newRun((g.classIdx + 1) % 9);
    }

    /* START = exit */
    /* (handled in main loop) */

    /* merchant touch: buy items */
    if (g.merchantOpen && (keysHeld() & KEY_TOUCH)) {
        touchPosition touch;
        touchRead(&touch);
        const int tx = touch.px;
        const int ty = touch.py;
        /* merchant item zones: y 28-88 */
        if (ty >= 28 && ty <= 44 && tx < 250) {
            abisso::buyFromMerchant(g.gs, g.combatRng, 0);
        } else if (ty >= 44 && ty <= 60 && tx < 250) {
            abisso::buyFromMerchant(g.gs, g.combatRng, 1);
        } else if (ty >= 60 && ty <= 76 && tx < 250) {
            abisso::buyFromMerchant(g.gs, g.combatRng, 2);
        } else if (ty >= 76 && ty <= 92 && tx < 250) {
            abisso::buyFromMerchant(g.gs, g.combatRng, 3);
        } else if (ty > 170) {
            g.merchantOpen = false;
            abisso::uiShowMerchant(false);
        }
    }

    /* equip panel: touch to close */
    if (g.showEquip && (keysHeld() & KEY_TOUCH)) {
        touchPosition touch;
        touchRead(&touch);
        if (touch.py > 170) {
            g.showEquip = false;
            abisso::uiShowEquip(false);
        }
    }
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
    g.gs.bossActive = false;
    g.gs.bossDead = false;
    g.gs.player.x = g.layout.spawn.x + 0.5;
    g.gs.player.y = g.layout.spawn.y + 0.5;
    abisso::makeInitialItems(g.gs, g.layout, g.depth, g.combatRng);
    abisso::makeMonsters(g.gs, g.combatRng);
    g.visible.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);
    g.visited.assign(static_cast<size_t>(g.layout.w) * g.layout.h, 0);
    g.lastFovTx = g.lastFovTy = -1;
    g.gs.chestsOpened.clear();
    g.gs.items.clear();
    g.gs.monsters.clear();
    g.gs.bolts.clear();
    g.gs.floatTexts.clear();
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleDemoInit();

    bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    /* sub-screen: bg0 per HUD testuale come fallback */
    bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

    abisso::sfxInit();
    abisso::uiInit();

    g.worldSeed = randomSeedFromRtc();
    g.combatRng = abisso::Rng(g.worldSeed ^ 0x9E3779B9u);
    g.gs.rng = &g.combatRng;
    g.depth = 1;
    g.layout = abisso::generateDepth(g.depth, "main", g.worldSeed);
    g.gs.layout = &g.layout;
    newRun(0);

    while (pmMainLoop()) {
        swiWaitForVBlank();

        /* hitstop: skip update */
        if (g.gs.hitstopT <= 0) {
            updateInput();
        }

    u32 down = keysDown();
        if (down & KEY_START) break;

        abisso::updateCombat(g.gs, g.combatRng, 1.0 / 60.0);
        updateFov();
        centerCamera();

        /* shake */
        int shakeCamX = g.camX, shakeCamY = g.camY;
        abisso::fxUpdateShake(g.gs, shakeCamX, shakeCamY);

        /* render top screen */
        renderWorld();

        /* render sub-screen UI */
        u16* const sub_fb = (u16*)BG_GFX_SUB;
        abisso::uiRender(sub_fb, 256, g.gs, g.showMap ? 1 : 0);

        g.frame++;
    }

    return 0;
}
