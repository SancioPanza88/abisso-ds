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
    extern const unsigned short wall_stoneBitmap[];
    extern const unsigned short floor_stoneBitmap[];
    extern const unsigned short stairsBitmap[];
    extern const unsigned short torchBitmap[];
}

/* --- Back buffer: draw to main RAM, DMA to VRAM at VBlank (eliminates tearing) --- */
static u16 backBuf[SCREEN_W * SCREEN_H];

/* --- OAM sprite GFX pointers --- */
static u16* sprHero[4];
static u16* sprMonster[256];
static u16* sprItem[8];

static u16* loadSpr(const void* data)
{
    u16* gfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_Bmp);
    if (!gfx) return 0;
    dmaCopy(data, gfx, SPR_BYTES);
    for (int i = 0; i < SPR_PX * SPR_PX; i++) {
        if ((gfx[i] & 0x7FFF) == 0)
            gfx[i] = 0x0000;
        else
            gfx[i] |= BIT(15);
    }
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

/* --- Hash for per-tile shade variation (matches HTML) --- */
static inline u32 tileHash(int x, int y)
{
    u32 h = (u32)(x * 374761393u + y * 668265263u);
    h = (h ^ (h >> 13)) * 1274126177u;
    return h;
}

/* --- Blit a 16x16 texture to framebuffer at tile position with shade --- */
static void blitTileShaded(u16* fb, int fbW, int sx, int sy,
                            const u16* tex, int shadeMul)
{
    if (!tex) return;
    for (int j = 0; j < TILE_PX; j++) {
        const int py = sy + j;
        if (py < 0 || py >= SCREEN_H) continue;
        const int texRow = j * TILE_PX;
        for (int i = 0; i < TILE_PX; i++) {
            const int px = sx + i;
            if (px < 0 || px >= fbW) continue;
            u16 raw = tex[texRow + i];
            /* magenta = transparent */
            if ((raw & 0x7FFF) == 0x7C1F) continue;
            /* apply shade: shadeMul is 0-256 fixed-point (256 = full bright) */
            int r = ((raw >> 10) & 0x1F);
            int g = ((raw >>  5) & 0x1F);
            int b = ( raw        & 0x1F);
            r = (r * shadeMul) >> 8;
            g = (g * shadeMul) >> 8;
            b = (b * shadeMul) >> 8;
            fb[py * fbW + px] = BIT(15) | RGB15(r, g, b);
        }
    }
}

/* --- Draw a filled 16x16 tile with bevels (walls get 3D effect) --- */
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
    dmaFillWords(fill, fb, SCREEN_W * SCREEN_H * sizeof(u16));
}

/* --- Minimap rendering (drawn on main FB, bottom-right corner) --- */
static void drawMinimap(u16* fb, int fbW, const Layout& l,
                         const Player& p, const std::vector<Monster>& monsters,
                         const std::vector<uint8_t>& visited)
{
    const int MM_SIZE = 60;
    const int MM_X = fbW - MM_SIZE - 4;
    const int MM_Y = 4;
    const int mmW = l.w;
    const int mmH = l.h;
    if (mmW <= 0 || mmH <= 0) return;

    const float scaleX = (float)MM_SIZE / mmW;
    const float scaleY = (float)MM_SIZE / mmH;
    const float scale = std::min(scaleX, scaleY);
    const int drawW = (int)(mmW * scale);
    const int drawH = (int)(mmH * scale);
    const int ox = MM_X + (MM_SIZE - drawW) / 2;
    const int oy = MM_Y + (MM_SIZE - drawH) / 2;

    /* background */
    drawRect(fb, fbW, MM_X - 1, MM_Y - 1, MM_SIZE + 2, MM_SIZE + 2, OC(0, 0, 0));

    /* tiles */
    for (int ty = 0; ty < mmH; ty++) {
        for (int tx = 0; tx < mmW; tx++) {
            const size_t idx = (size_t)ty * mmW + tx;
            if (!visited.empty() && !visited[idx]) continue;
            const int px = ox + (int)(tx * scale);
            const int py = oy + (int)(ty * scale);
            const int pw = std::max(1, (int)((tx + 1) * scale) - (int)(tx * scale));
            const int ph = std::max(1, (int)((ty + 1) * scale) - (int)(ty * scale));
            uint8_t tile = l.grid[idx];
            u16 c;
            if (tile == T_WALL) c = OC(6, 6, 8);
            else if (tile == T_STAIRS) c = OC(20, 26, 6);
            else c = OC(14, 12, 10);
            drawRect(fb, fbW, px, py, pw, ph, c);
        }
    }

    /* stairs marker (gold) */
    {
        const int px = ox + (int)(l.stairsPos.x * scale);
        const int py = oy + (int)(l.stairsPos.y * scale);
        drawRect(fb, fbW, px, py, 2, 2, OC(31, 26, 6));
    }

    /* monsters as red dots (only visible ones) */
    for (const Monster& m : monsters) {
        const int mtx = (int)m.x;
        const int mty = (int)m.y;
        if (mtx >= 0 && mtx < mmW && mty >= 0 && mty < mmH) {
            const size_t idx = (size_t)mty * mmW + mtx;
            if (!visited.empty() && !visited[idx]) continue;
            const int px = ox + (int)(m.x * scale);
            const int py = oy + (int)(m.y * scale);
            drawRect(fb, fbW, px, py, 2, 2, OC(31, 6, 6));
        }
    }

    /* player as white dot */
    {
        const int px = ox + (int)(p.x * scale);
        const int py = oy + (int)(p.y * scale);
        drawRect(fb, fbW, px - 1, py - 1, 3, 3, OC(31, 31, 31));
    }
}

/* --- Class selection screen (sub screen) --- */
static int selectClass()
{
    const int NUM_CLASSES = 9;
    const char* names[] = {
        "Guerriero", "Ladro", "Mago", "Ranger", "Prof",
        "Paladino", "Negromante", "Bardo", "Monaco"
    };
    const char* abils[] = {
        "Carica", "Passo Furtivo", "Onda d'Urto", "Raffica",
        "Colpo Caricato", "Muro Sacro", "Drenaggio", "Canto", "Onda di Chi"
    };
    const int hp[]  = {16, 10,  9, 11, 22, 18, 10, 12, 14};
    const int dmg[] = { 3,  2,  4,  2,  5,  3,  4,  2,  3};
    const bool rng[] = {0, 0, 1, 1, 1, 0, 1, 0, 0};

    int sel = 0;
    bool confirmed = false;

    /* draw initial sub screen */
    for (int i = 0; i < 24; i++) iprintf("\x1b[%d;0H                          ", i);
    iprintf("\x1b[0;0H");
    iprintf(" == SCELTA EROE ==\n\n");
    for (int i = 0; i < NUM_CLASSES; i++) {
        iprintf(" %c %s\n", i == sel ? '>' : ' ', names[i]);
    }
    iprintf("\n A:Conferma\n");

    while (pmMainLoop() && !confirmed) {
        scanKeys();
        const int down = keysDown();

        if (down & KEY_UP) {
            sel = (sel - 1 + NUM_CLASSES) % NUM_CLASSES;
        }
        if (down & KEY_DOWN) {
            sel = (sel + 1) % NUM_CLASSES;
        }
        if (down & KEY_A || down & KEY_START) {
            confirmed = true;
        }

        /* redraw selection list */
        iprintf("\x1b[2;0H");
        for (int i = 0; i < NUM_CLASSES; i++) {
            iprintf(" %c %-10s   \n", i == sel ? '>' : ' ', names[i]);
        }
        /* show details for selected class */
        iprintf("\x1b[13;0H");
        iprintf("                    \n");
        iprintf(" HP: %-3d  DMG: %-3d \n", hp[sel], dmg[sel]);
        iprintf(" %s%s      \n", abils[sel], rng[sel] ? " (ranged)" : " (melee)");
        iprintf("                    \n");
        for (int i = 18; i < 24; i++) {
            iprintf("\x1b[%d;0H                          ", i);
        }
        iprintf("\x1b[18;0H A:Conferma        \n");

        swiWaitForVBlank();
    }
    return sel;
}

/* --- Interact with nearby objects --- */
static bool tryInteract(GameState& g, Rng& rng)
{
    Player& p = g.player;
    if (p.dead) return false;
    const Layout& l = *g.layout;
    const int ptx = (int)p.x;
    const int pty = (int)p.y;

    /* stairs */
    if (l.grid[(size_t)pty * l.w + ptx] == T_STAIRS) {
        return true; /* handled by caller */
    }

    /* chest */
    for (const ChestSpot& cs : l.chestSpots) {
        if (std::abs(cs.x - ptx) <= 1 && std::abs(cs.y - pty) <= 1) {
            if (g.chestsOpened.count(cs.id) == 0) {
                g.chestsOpened.insert(cs.id);
                const bool isBoss = cs.id.substr(0, 5) == "cboss";
                int gold = openChest(g, rng, cs.id, isBoss);
                if (gold >= 0) {
                    spawnFloatText(g, p.x, p.y - 0.7, ("+" + std::to_string(gold) + " Au").c_str(), 1);
                    sfxChest();
                    return true;
                }
            }
        }
    }

    /* merchant */
    if (std::abs(l.merchantPos.x - ptx) <= 1 && std::abs(l.merchantPos.y - pty) <= 1) {
        uiShowMerchant(!uiIsMerchantVisible());
        return true;
    }

    return false;
}

/* --- Sub screen HUD text rendering --- */
static void renderSubHud(const GameState& g, bool forceRedraw, int& lastDepth)
{
    if (!forceRedraw && g.depth == lastDepth && !uiIsMerchantVisible() && !uiIsEquipVisible())
        return;

    const Player& p = g.player;
    char buf[64];

    /* If merchant overlay is active, show that */
    if (uiIsMerchantVisible()) {
        /* Use cursor positioning to avoid flicker */
        iprintf("\x1b[0;0H");  /* cursor to 0,0 */
        iprintf(" == MERCANTE ==  \n\n");
        iprintf(" Oro: %-5d       \n\n", p.gold);
        iprintf(" 1)Poz HP:  %3dAu \n", merchantPrice(0, g.depth));
        iprintf(" 2)Poz MP:  %3dAu \n", merchantPrice(1, g.depth));
        iprintf(" 3)Potenza: %3dAu \n", merchantPrice(2, g.depth));
        iprintf(" 4)Equip:   %3dAu \n\n", merchantPrice(3, g.depth));
        iprintf(" A:Compr1 B:Compr2\n");
        iprintf(" X:Compr3 Y:Compr4\n");
        iprintf(" L/R:Chiudi       \n");
        for (int i = 11; i < 24; i++) {
            iprintf("\x1b[%d;0H                    ", i);
        }
        lastDepth = g.depth;
        return;
    }

    /* If equipment overlay is active, show that */
    if (uiIsEquipVisible()) {
        iprintf("\x1b[0;0H");
        iprintf(" == EQUIPAGGIAMENTO == \n\n");
        for (int i = 0; i < EQ_SLOT_COUNT; i++) {
            const EquipItem& eq = p.equip[i];
            const char* slotName = equipSlotName((EquipSlot)i);
            if (eq.rarity > 0 || eq.stats.hp > 0) {
                iprintf(" %s:%-11s \n", slotName, equipRarityName(eq.rarity));
                iprintf("  HP:%-3d DMG:%-3d%% \n", eq.stats.hp, eq.stats.dmgPct);
            } else {
                iprintf(" %s:---           \n", slotName);
                iprintf("                   \n");
            }
        }
        iprintf("\n L/R:Chiudi       \n");
        for (int i = 14; i < 24; i++) {
            iprintf("\x1b[%d;0H                    ", i);
        }
        lastDepth = g.depth;
        return;
    }

    /* Normal HUD */
    iprintf("\x1b[0;0H");
    iprintf("  ABISSO DS   \n");
    iprintf("  Piano %-2d    \n\n", g.depth);
    if (p.dead) {
        iprintf("  SEI MORTO!   \n");
        std::snprintf(buf, sizeof(buf), "  Respawn: %.0fs  ", p.respawnT);
        iprintf("%s\n", buf);
    } else {
        std::snprintf(buf, sizeof(buf), "  HP: %d/%d     ", p.hp, p.maxHp);
        iprintf("%s\n", buf);
        if (p.maxMp > 0) {
            std::snprintf(buf, sizeof(buf), "  MP: %d/%d     ", p.mp, p.maxMp);
            iprintf("%s\n", buf);
        } else {
            iprintf("               \n");
        }
        std::snprintf(buf, sizeof(buf), "  Oro:%-5d     ", p.gold);
        iprintf("%s\n", buf);
        std::snprintf(buf, sizeof(buf), "  PozHP:%d MP:%d  ", p.potions, p.manaPotions);
        iprintf("%s\n", buf);
        int by = 10;
        if (p.buffRage > 0)   { iprintf("\x1b[%d;0H FURIA  %.0fs  ", by++, p.buffRage); }
        if (p.buffShield > 0) { iprintf("\x1b[%d;0H SCUDO  %.0fs  ", by++, p.buffShield); }
        if (p.buffHaste > 0)  { iprintf("\x1b[%d;0H FRETTA %.0fs  ", by++, p.buffHaste); }
        if (p.buffFocus > 0)  { iprintf("\x1b[%d;0H FOCUS  %.0fs  ", by++, p.buffFocus); }
        if (g.bossFight)      { iprintf("\x1b[%d;0H *** BOSS ***  ", by++); }
        /* clear remaining buff lines */
        for (int i = by; i < 14; i++) {
            iprintf("\x1b[%d;0H               ", i);
        }
    }
    iprintf("\x1b[15;0H DP:Muovi A:Atk ");
    iprintf("\x1b[16;0H B:PozHP X:PozMP ");
    iprintf("\x1b[17;0H Y:Use L:Abil   ");
        lastDepth = g.depth;
}

int main(void)
{
    /* === Main screen: MODE_5 + OAM sprites === */
    videoSetMode(MODE_5_2D | DISPLAY_SPR_ACTIVE);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankB(VRAM_B_MAIN_SPRITE);
    int bgMain = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    u16* vramFb = bgGetGfxPtr(bgMain);

    /* === Sub screen: MODE_0 text console === */
    videoSetModeSub(MODE_0_2D);
    consoleDemoInit();

    /* === OAM init for bitmap sprites === */
    oamInit(&oamMain, SpriteMapping_Bmp_1D_128, false);

    /* === Load all sprites into VRAM_B === */
    initSprites();

    /* Clear back buffer */
    dmaClearFb(backBuf, OC(4, 4, 6));

    /* --- Init game state --- */
    Rng rng(WORLD_SEED);
    Layout layout = generateDepth(1, ROOM_CODE, WORLD_SEED);

    GameState g;
    g.layout    = &layout;
    g.rng       = &rng;
    g.worldSeed = WORLD_SEED;
    g.depth     = 1;

    /* --- Class selection on sub screen --- */
    int classIdx = selectClass();
    playerSetClass(g.player, CLASS_KEYS[classIdx]);
    g.player.x = layout.spawn.x + 0.5;
    g.player.y = layout.spawn.y + 0.5;

    makeMonsters(g, rng);
    makeInitialItems(g, layout, 1, rng);

    sfxInit();
    uiInit();

    /* FOV state */
    const size_t mapSize = (size_t)layout.w * layout.h;
    std::vector<uint8_t> fovVisible(mapSize, 0);
    std::vector<uint8_t> fovVisited(mapSize, 0);
    computeFov(layout, (int)g.player.x, (int)g.player.y, fovVisible, fovVisited);

    int prevTileX = (int)g.player.x;
    int prevTileY = (int)g.player.y;
    int lastDepth = -1;
    bool needSubRedraw = true;
    bool mapVisible = false;

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
            if (down & KEY_B) { drinkPotion(g.player); sfxPotion(); needSubRedraw = true; }
            if (down & KEY_X) { drinkManaPotion(g.player); sfxMana(); needSubRedraw = true; }
            if (down & KEY_L) { useAbility(g, rng); needSubRedraw = true; }

            /* Y button = interact (chest/merchant/stairs) */
            if (down & KEY_Y) {
                if (uiIsMerchantVisible()) {
                    uiShowMerchant(false);
                } else if (uiIsEquipVisible()) {
                    uiShowEquip(false);
                } else {
                    const Layout& l = *g.layout;
                    const int ptx = (int)g.player.x;
                    const int pty = (int)g.player.y;
                    bool didInteract = false;
                    /* stairs */
                    if (!didInteract && l.grid[(size_t)pty * l.w + ptx] == T_STAIRS) {
                        int newDepth = g.depth + 1;
                        Layout newLayout = generateDepth(newDepth, ROOM_CODE, WORLD_SEED);
                        advanceDepth(g, newLayout, rng);
                        layout = newLayout;
                        g.layout = &layout;
                        makeMonsters(g, rng);
                        makeInitialItems(g, layout, newDepth, rng);
                        g.player.x = layout.spawn.x + 0.5;
                        g.player.y = layout.spawn.y + 0.5;
                        const size_t ns = (size_t)layout.w * layout.h;
                        fovVisible.assign(ns, 0);
                        fovVisited.assign(ns, 0);
                        computeFov(layout, (int)g.player.x, (int)g.player.y,
                                   fovVisible, fovVisited);
                        sfxStair();
                        didInteract = true;
                    }
                    if (!didInteract) {
                        tryInteract(g, rng);
                    }
                }
                needSubRedraw = true;
            }

            /* R button = toggle equip overlay */
            if (down & KEY_R) {
                if (uiIsMerchantVisible()) {
                    uiShowMerchant(false);
                } else {
                    uiShowEquip(!uiIsEquipVisible());
                }
                needSubRedraw = true;
            }

            int curTX = (int)g.player.x;
            int curTY = (int)g.player.y;
            if (curTX != prevTileX || curTY != prevTileY) {
                /* recompute FOV on tile change */
                computeFov(layout, curTX, curTY, fovVisible, fovVisited);

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
                    /* reset FOV */
                    const size_t ns = (size_t)layout.w * layout.h;
                    fovVisible.assign(ns, 0);
                    fovVisited.assign(ns, 0);
                    computeFov(layout, (int)g.player.x, (int)g.player.y,
                               fovVisible, fovVisited);
                    sfxStair();
                }
                needSubRedraw = true;
            }
            prevTileX = curTX;
            prevTileY = curTY;
        }

        /* --- Merchant touch input (buy items) --- */
        if (uiIsMerchantVisible() && (down & KEY_TOUCH)) {
            touchPosition touch;
            touchRead(&touch);
            const int ty = touch.py;
            /* map touch Y zones to merchant items */
            if (ty >= 44 && ty < 60)  buyFromMerchant(g, rng, 0);
            else if (ty >= 60 && ty < 76)  buyFromMerchant(g, rng, 1);
            else if (ty >= 76 && ty < 92)  buyFromMerchant(g, rng, 2);
            else if (ty >= 92 && ty < 108) buyFromMerchant(g, rng, 3);
            else if (ty > 140) uiShowMerchant(false);
            needSubRedraw = true;
        }
        /* equip overlay: touch to close */
        else if (uiIsEquipVisible() && (down & KEY_TOUCH)) {
            uiShowEquip(false);
            needSubRedraw = true;
        }
        /* normal touch screen input */
        else if (down & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            int btn = uiHandleTouch(touch, SCREEN_W, SCREEN_H);
            switch (btn) {
                case TB_HP_POTION: drinkPotion(g.player); sfxPotion(); needSubRedraw = true; break;
                case TB_MP_POTION: drinkManaPotion(g.player); sfxMana(); needSubRedraw = true; break;
                case TB_INTERACT:
                    tryInteract(g, rng);
                    needSubRedraw = true;
                    break;
                case TB_MAP:
                    mapVisible = !mapVisible;
                    needSubRedraw = true;
                    break;
                case TB_EQUIP: uiShowEquip(!uiIsEquipVisible()); needSubRedraw = true; break;
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

        /* --- Render BG layer (tiles, fog of war, effects) --- */
        dmaClearFb(backBuf, OC(4, 4, 6));

        /* torch flicker: dual-frequency sine like HTML */
        const double flick = 0.9 + 0.1 * std::sin(g.torchPhase * 3.1)
                                + 0.04 * std::sin(g.torchPhase * 7.7);
        const int flickMul = (int)(flick * 256.0);
        const int dimMul = 118; /* fog: ~46% brightness */

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
                const size_t idx = (size_t)ty * layout.w + tx;
                const uint8_t tile = layout.grid[idx];
                if (fovVisible[idx]) {
                    if (tile == T_WALL) {
                        blitTileShaded(backBuf, SCREEN_W, sx, sy, wall_stoneBitmap, flickMul);
                        /* 3D bevel on walls */
                        u16 hi = BIT(15) | RGB15(14, 12, 9);
                        u16 lo = BIT(15) | RGB15(2, 2, 2);
                        drawRect(backBuf, SCREEN_W, sx, sy, TILE_PX, 2, hi);
                        drawRect(backBuf, SCREEN_W, sx, sy + 14, TILE_PX, 2, lo);
                    } else if (tile == T_STAIRS) {
                        blitTileShaded(backBuf, SCREEN_W, sx, sy, stairsBitmap, flickMul);
                    } else {
                        blitTileShaded(backBuf, SCREEN_W, sx, sy, floor_stoneBitmap, flickMul);
                    }
                } else if (fovVisited[idx]) {
                    /* fog: dim version */
                    if (tile == T_WALL) {
                        blitTileShaded(backBuf, SCREEN_W, sx, sy, wall_stoneBitmap, dimMul);
                    } else if (tile == T_STAIRS) {
                        blitTileShaded(backBuf, SCREEN_W, sx, sy, stairsBitmap, dimMul);
                    } else {
                        blitTileShaded(backBuf, SCREEN_W, sx, sy, floor_stoneBitmap, dimMul);
                    }
                } else {
                    drawRect(backBuf, SCREEN_W, sx, sy, TILE_PX, TILE_PX, OC(1, 1, 2));
                }
            }
        }

        /* Draw torches on BG (only if visible, with flicker) */
        for (const Pt& t : layout.torches) {
            if (t.x >= 0 && t.x < layout.w && t.y >= 0 && t.y < layout.h) {
                if (fovVisible[(size_t)t.y * layout.w + t.x]) {
                    /* per-torch phase + dual-frequency flicker */
                    const double ph = ((t.x * 7 + t.y * 13) % 20) / 20.0;
                    const double tfl = 0.75 + 0.25 * std::sin(g.torchPhase * 9 + ph * 6.28)
                                         + 0.08 * std::sin(g.torchPhase * 23 + ph * 13);
                    const int tflI = (int)(tfl * 255);
                    blitTileShaded(backBuf, SCREEN_W, t.x * TILE_PX - camX,
                                   t.y * TILE_PX - camY, torchBitmap, tflI);
                }
            }
        }

        /* Draw bolts on BG */
        for (const Bolt& b : g.bolts) {
            const int sx = (int)(b.x * TILE_PX) - camX - 2;
            const int sy = (int)(b.y * TILE_PX) - camY - 2;
            drawRect(backBuf, SCREEN_W, sx, sy, 4, 4,
                     b.fromPlayer ? OC(31, 26, 6) : OC(31, 8, 8));
        }

        /* Particles on BG */
        for (const Particle& part : g.particles) {
            const int sx = (int)(part.x * TILE_PX) - camX;
            const int sy = (int)(part.y * TILE_PX) - camY;
            if (sx < -4 || sx >= SCREEN_W + 4 || sy < -4 || sy >= SCREEN_H + 4) continue;
            const double lifeRatio = part.life / part.maxLife;
            const int sz = std::max(1, (int)(part.size * TILE_PX * lifeRatio));
            const int r = (int)(part.r * lifeRatio * 31);
            const int g2 = (int)(part.g * lifeRatio * 31);
            const int b = (int)(part.b * lifeRatio * 31);
            const u16 pc = OC(std::min(31, r), std::min(31, g2), std::min(31, b));
            if (part.type == 1) {
                /* shard: rotated rectangle */
                drawRect(backBuf, SCREEN_W, sx, sy, sz, sz / 2 + 1, pc);
            } else if (part.type == 2) {
                /* smoke: larger, faint */
                const int ssz = sz + (int)(TILE_PX * 0.3 * (1.0 - lifeRatio));
                drawRect(backBuf, SCREEN_W, sx - ssz / 4, sy - ssz / 4, ssz / 2, ssz / 2, pc);
            } else {
                drawRect(backBuf, SCREEN_W, sx, sy, sz, sz, pc);
            }
        }

        /* Floating texts on BG */
        fxRenderFloatTexts(backBuf, SCREEN_W, camX, camY, g);

        /* Depth fade */
        if (g.depthFadeT > 0) {
            const int alpha = fxGetDepthFadeAlpha(g.depthFadeT, 0.55);
            if (alpha > 0) {
                u16 fadeColor = OC(alpha, alpha, alpha);
                dmaClearFb(backBuf, fadeColor);
            }
        }

        /* Damage flash overlay — smooth red vignette */
        if (g.damageFlashT > 0) {
            const int a = (int)(g.damageFlashT / 0.35 * 24);
            if (a > 0) {
                for (int y = 0; y < SCREEN_H; y += 2) {
                    for (int x = 0; x < SCREEN_W; x += 2) {
                        const double dx = (x - SCREEN_W / 2.0) / (SCREEN_W / 2.0);
                        const double dy = (y - SCREEN_H / 2.0) / (SCREEN_H / 2.0);
                        const double dist = dx * dx + dy * dy;
                        if (dist > 0.35) {
                            const int amt = (int)((dist - 0.35) / 0.65 * a);
                            if (amt > 0) {
                                const int idx = y * SCREEN_W + x;
                                int or2 = ((backBuf[idx] >> 10) & 0x1F);
                                int og  = ((backBuf[idx] >>  5) & 0x1F);
                                int ob  = ( backBuf[idx]        & 0x1F);
                                or2 = std::min(31, or2 + amt);
                                backBuf[idx] = BIT(15) | RGB15(or2, og, ob);
                                backBuf[idx + 1] = backBuf[idx];
                                backBuf[idx + SCREEN_W] = backBuf[idx];
                                backBuf[idx + SCREEN_W + 1] = backBuf[idx];
                            }
                        }
                    }
                }
            }
        }

        /* Boss death flash: warm white overlay decaying */
        if (g.bossDeathFlashT > 0) {
            const double k = g.bossDeathFlashT / 0.35;
            const int a = (int)(k * 14);
            if (a > 0) {
                drawRect(backBuf, SCREEN_W, 0, 0, SCREEN_W, SCREEN_H,
                         OC(std::min(31, 28 + a / 2), std::min(31, 23 + a / 3), std::min(31, 12 + a / 4)));
            }
        }

        /* --- OAM sprites: clear, then set all visible entities --- */
        oamClear(&oamMain, 0, 0);
        int oid = 0;

        /* Items as OAM sprites (only if visible in FOV) */
        for (const GroundItem& gi : g.items) {
            if (oid >= 128) break;
            const int mtx = (int)gi.x;
            const int mty = (int)gi.y;
            if (mtx < 0 || mtx >= layout.w || mty < 0 || mty >= layout.h) continue;
            if (!fovVisible[(size_t)mty * layout.w + mtx]) continue;
            const int sx = (int)(gi.x * TILE_PX) - camX - 8;
            const int sy = (int)(gi.y * TILE_PX) - camY - 8;
            if (sx < -16 || sx > SCREEN_W + 16 || sy < -16 || sy > SCREEN_H + 16) continue;
            u16* gfx = sprItem[gi.kind];
            if (gfx) {
                oamSet(&oamMain, oid++, sx, sy, 2, 15,
                       SpriteSize_16x16, SpriteColorFormat_Bmp,
                       gfx, -1, false, false, false, false, false);
            } else {
                drawRect(backBuf, SCREEN_W, sx + 5, sy + 5, 6, 6,
                         gi.kind == GI_GOLD ? OC(31,26,6) :
                         gi.kind == GI_GEM ? OC(10,20,31) :
                         gi.kind == GI_POTION ? OC(31,10,10) : OC(10,10,31));
            }
        }

        /* Monsters as OAM sprites (only if visible in FOV) */
        for (const Monster& m : g.monsters) {
            if (oid >= 128) break;
            const MonsterType& mt = *getMonsterType(m.type);
            const int mtx = (int)m.x;
            const int mty = (int)m.y;
            if (mtx < 0 || mtx >= layout.w || mty < 0 || mty >= layout.h) continue;
            if (!fovVisible[(size_t)mty * layout.w + mtx]) continue;
            /* walk bob + windup flash + boss lift */
            int offY = 0;
            const bool mMoving = (m.state == 'c' || m.state == 'w');
            if (mMoving) {
                offY = (int)(std::sin(g.gameTime * 10.0 + m.x * 5.3 + m.y * 2.7) * 2.0);
            }
            if (m.winding) {
                offY -= 2;
            }
            if (m.bossFlying) {
                offY -= 6;
            }
            /* attack squash stretch */
            int offX = 0;
            if (m.atkAnimT > 0) {
                offY -= 2;
            }
            const int sx = (int)(m.x * TILE_PX) - camX - 8 + offX;
            const int sy = (int)(m.y * TILE_PX) - camY - 8 + offY;
            if (sx < -16 || sx > SCREEN_W + 16 || sy < -16 || sy > SCREEN_H + 16) continue;
            u16* gfx = sprMonster[(unsigned char)m.type];
            if (gfx) {
                const bool mhflip = (m.fx < -0.01);
                oamSet(&oamMain, oid++, sx, sy, 1, 15,
                       SpriteSize_16x16, SpriteColorFormat_Bmp,
                       gfx, -1, false, false, mhflip, false, false);
            } else {
                drawRect(backBuf, SCREEN_W, sx + 3, sy + 3, 10, 10,
                         monsterColor(m.type, mt.boss));
            }
            /* HP bar on BG (above sprite, not covered by OAM) */
            if (m.hp < m.maxHp) {
                const int barW = 10;
                const int hpW = (int)(barW * m.hp / m.maxHp);
                drawRect(backBuf, SCREEN_W, sx + 3, sy + 1, hpW, 1, OC(31, 8, 8));
                drawRect(backBuf, SCREEN_W, sx + 3 + hpW, sy + 1, barW - hpW, 1, OC(6, 2, 2));
            }
            /* windup flash — draw slightly larger so it peeks past sprite edges */
            if (m.winding) {
                drawRect(backBuf, SCREEN_W, sx - 1, sy - 1, 18, 18, OC(15, 3, 3));
            }
        }

        /* Player as OAM sprite (always visible) */
        if (oid < 128) {
            /* walk bob — only when moving */
            int poffY = 0;
            const bool isMoving = (g.player.fx != 0 || g.player.fy != 0);
            if (!g.player.dead && isMoving) {
                poffY = (int)(std::sin(g.gameTime * 12.0) * 1.5);
            }
            const int sx = (int)(g.player.x * TILE_PX) - camX - 8;
            const int sy = (int)(g.player.y * TILE_PX) - camY - 8 + poffY;
            u16* gfx = sprHero[heroIdx(g.player.cls->key)];
            if (gfx && !g.player.dead) {
                /* invulnerability blink */
                bool showSprite = true;
                if (g.player.invulnT > 0) {
                    showSprite = ((int)(g.player.invulnT * 12) % 2 == 0);
                }
                if (showSprite) {
                    const bool hflip = (g.player.fx < -0.01);
                    oamSet(&oamMain, oid++, sx, sy, 0, 15,
                           SpriteSize_16x16, SpriteColorFormat_Bmp,
                           gfx, -1, false, false, hflip, false, false);
                }
            } else {
                const u16 pc = g.player.dead ? OC(12, 8, 8) : OC(10, 28, 10);
                drawRect(backBuf, SCREEN_W, sx + 2, sy + 2, 12, 12, pc);
            }
        }

        /* Minimap overlay on main screen */
        if (mapVisible) {
            drawMinimap(backBuf, SCREEN_W, layout, g.player, g.monsters, fovVisited);
        }

        /* --- VBlank: DMA back buffer to VRAM, then apply OAM changes --- */
        swiWaitForVBlank();
        dmaCopy(backBuf, vramFb, SCREEN_W * SCREEN_H * sizeof(u16));
        oamUpdate(&oamMain);

        /* --- Sub screen HUD --- */
        {
            bool forceRedraw = needSubRedraw || g.depth != lastDepth ||
                               uiIsMerchantVisible() || uiIsEquipVisible();
            renderSubHud(g, forceRedraw, lastDepth);
            needSubRedraw = false;
        }
    }

    return 0;
}
