#include "ui.hpp"
#include "game.hpp"

#include <cstdio>
#include <cstring>

namespace abisso {

/* Touch button zones (256x192 sub-screen) */
struct TouchZone {
    int x1, y1, x2, y2;
};

static const TouchZone BUTTON_ZONES[TB_COUNT] = {
    {   8,  72, 120, 100 },  /* TB_HP_POTION */
    { 136,  72, 248, 100 },  /* TB_MP_POTION */
    {  48, 108, 208, 138 },  /* TB_INTERACT */
    {   8, 148,  78, 178 },  /* TB_MAP */
    { 178, 148, 248, 178 },  /* TB_EQUIP */
};

static bool s_merchantVisible = false;
static bool s_equipVisible = false;
static char s_interactLabel[32] = "INTERAGISCI";
static int s_lastFrame = -1;

void uiInit()
{
    s_merchantVisible = false;
    s_equipVisible = false;
    std::strcpy(s_interactLabel, "INTERAGISCI");
}

void uiSetInteractLabel(const char* label)
{
    std::strncpy(s_interactLabel, label, 31);
    s_interactLabel[31] = '\0';
}

void uiShowMerchant(bool show) { s_merchantVisible = show; }
bool uiIsMerchantVisible() { return s_merchantVisible; }
void uiShowEquip(bool show) { s_equipVisible = show; }
bool uiIsEquipVisible() { return s_equipVisible; }

/* Draw a single pixel character at position */
static inline void putPixel(u16* fb, int fbW, int x, int y, u16 color)
{
    if (x >= 0 && x < fbW && y >= 0 && y < 192)
        fb[y * fbW + x] = color;
}

/* Draw a filled rectangle */
static void fillRect(u16* fb, int fbW, int x, int y, int w, int h, u16 color)
{
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            putPixel(fb, fbW, x + i, y + j, color);
}

/* Draw a button with text */
static void drawButton(u16* fb, int fbW, const TouchZone& z, const char* label, u16 bgColor, u16 borderColor, bool enabled)
{
    fillRect(fb, fbW, z.x1, z.y1, z.x2 - z.x1, z.y2 - z.y1, bgColor);
    /* border */
    for (int i = z.x1; i < z.x2; i++) {
        putPixel(fb, fbW, i, z.y1, borderColor);
        putPixel(fb, fbW, i, z.y2 - 1, borderColor);
    }
    for (int j = z.y1; j < z.y2; j++) {
        putPixel(fb, fbW, z.x1, j, borderColor);
        putPixel(fb, fbW, z.x2 - 1, j, borderColor);
    }
    if (!enabled) {
        /* dim button */
        fillRect(fb, fbW, z.x1 + 1, z.y1 + 1, z.x2 - z.x1 - 2, z.y2 - z.y1 - 2, RGB15(3, 3, 3));
    }
}

/* Simple text renderer: 3x5 font */
static const unsigned char FONT3x5[96][5] = {
    /* ASCII 32+ */
    {0x00,0x00,0x00,0x00,0x00}, /* space */
    {0x04,0x04,0x04,0x00,0x04}, /* ! */
    {0x0A,0x0A,0x00,0x00,0x00}, /* " */
    {0x0A,0x1F,0x0A,0x1F,0x0A}, /* # */
    {0x0E,0x10,0x0E,0x01,0x1E}, /* $ */
    {0x11,0x02,0x04,0x08,0x11}, /* % */
    {0x06,0x09,0x06,0x0A,0x05}, /* & */
    {0x04,0x04,0x00,0x00,0x00}, /* ' */
    {0x02,0x04,0x04,0x04,0x02}, /* ( */
    {0x08,0x04,0x04,0x04,0x08}, /* ) */
    {0x0A,0x04,0x1F,0x04,0x0A}, /* * */
    {0x00,0x04,0x0E,0x04,0x00}, /* + */
    {0x00,0x00,0x00,0x04,0x08}, /* , */
    {0x00,0x00,0x1F,0x00,0x00}, /* - */
    {0x00,0x00,0x00,0x00,0x04}, /* . */
    {0x01,0x02,0x04,0x08,0x10}, /* / */
    {0x0E,0x11,0x13,0x15,0x0E}, /* 0 */
    {0x04,0x0C,0x04,0x04,0x0E}, /* 1 */
    {0x0E,0x11,0x06,0x08,0x1F}, /* 2 */
    {0x1F,0x02,0x06,0x02,0x1F}, /* 3 */
    {0x02,0x06,0x0A,0x1F,0x02}, /* 4 */
    {0x1F,0x10,0x1E,0x01,0x1E}, /* 5 */
    {0x06,0x08,0x0E,0x11,0x0E}, /* 6 */
    {0x1F,0x01,0x02,0x04,0x08}, /* 7 */
    {0x0E,0x11,0x0E,0x11,0x0E}, /* 8 */
    {0x0E,0x11,0x0F,0x01,0x0E}, /* 9 */
    {0x00,0x04,0x00,0x04,0x00}, /* : */
    {0x00,0x04,0x00,0x04,0x08}, /* ; */
    {0x02,0x04,0x08,0x04,0x02}, /* < */
    {0x00,0x0E,0x00,0x0E,0x00}, /* = */
    {0x08,0x04,0x02,0x04,0x08}, /* > */
    {0x0E,0x11,0x02,0x00,0x04}, /* ? */
    {0x0E,0x11,0x15,0x15,0x0E}, /* @ */
    {0x0E,0x11,0x1F,0x11,0x11}, /* A */
    {0x1E,0x11,0x1E,0x11,0x1E}, /* B */
    {0x0E,0x10,0x10,0x10,0x0E}, /* C */
    {0x1E,0x11,0x11,0x11,0x1E}, /* D */
    {0x1F,0x10,0x1E,0x10,0x1F}, /* E */
    {0x1F,0x10,0x1E,0x10,0x10}, /* F */
    {0x0E,0x10,0x13,0x11,0x0E}, /* G */
    {0x11,0x11,0x1F,0x11,0x11}, /* H */
    {0x0E,0x04,0x04,0x04,0x0E}, /* I */
    {0x01,0x01,0x01,0x11,0x0E}, /* J */
    {0x11,0x12,0x1C,0x12,0x11}, /* K */
    {0x10,0x10,0x10,0x10,0x1F}, /* L */
    {0x11,0x1B,0x15,0x11,0x11}, /* M */
    {0x11,0x19,0x15,0x13,0x11}, /* N */
    {0x0E,0x11,0x11,0x11,0x0E}, /* O */
    {0x1E,0x11,0x1E,0x10,0x10}, /* P */
    {0x0E,0x11,0x15,0x12,0x0D}, /* Q */
    {0x1E,0x11,0x1E,0x12,0x11}, /* R */
    {0x0F,0x10,0x0E,0x01,0x1E}, /* S */
    {0x1F,0x04,0x04,0x04,0x04}, /* T */
    {0x11,0x11,0x11,0x11,0x0E}, /* U */
    {0x11,0x11,0x11,0x0A,0x04}, /* V */
    {0x11,0x11,0x15,0x1B,0x11}, /* W */
    {0x11,0x0A,0x04,0x0A,0x11}, /* X */
    {0x11,0x0A,0x04,0x04,0x04}, /* Y */
    {0x1F,0x02,0x04,0x08,0x1F}, /* Z */
};

static void drawText(u16* fb, int fbW, int x, int y, const char* text, u16 color)
{
    int cx = x;
    for (int i = 0; text[i]; i++) {
        const char ch = text[i];
        if (ch < 32 || ch > 127) { cx += 4; continue; }
        const unsigned char* glyph = FONT3x5[ch - 32];
        for (int py = 0; py < 5; py++) {
            for (int px = 0; px < 3; px++) {
                if ((glyph[py] >> (2 - px)) & 1)
                    putPixel(fb, fbW, cx + px, y + py, color);
            }
        }
        cx += 4;
    }
}

static void drawBar(u16* fb, int fbW, int x, int y, int w, int h, int frac, u16 fg, u16 bg)
{
    fillRect(fb, fbW, x, y, w, h, bg);
    fillRect(fb, fbW, x, y, (int)(w * frac / 100), h, fg);
}

void uiRender(u16* subFb, int fbW, const GameState& g, int mapVisible)
{
    const Player& p = g.player;

    /* background */
    fillRect(subFb, fbW, 0, 0, fbW, 192, RGB15(2, 2, 3));

    /* HP bar */
    const int hpFrac = p.maxHp > 0 ? (p.hp * 100 / p.maxHp) : 0;
    drawBar(subFb, fbW, 8, 4, 100, 6, hpFrac, RGB15(31, 10, 10), RGB15(6, 2, 2));
    char buf[64];
    std::snprintf(buf, sizeof(buf), "HP %d/%d", p.hp, p.maxHp);
    drawText(subFb, fbW, 8, 12, buf, RGB15(31, 15, 15));

    /* MP bar */
    if (p.maxMp > 0) {
        const int mpFrac = (p.mp * 100 / p.maxMp);
        drawBar(subFb, fbW, 8, 22, 100, 5, mpFrac, RGB15(10, 20, 31), RGB15(2, 4, 7));
        std::snprintf(buf, sizeof(buf), "MP %d/%d", p.mp, p.maxMp);
        drawText(subFb, fbW, 8, 29, buf, RGB15(15, 22, 31));
    }

    /* Gold */
    std::snprintf(buf, sizeof(buf), "Au:%d", p.gold);
    drawText(subFb, fbW, 8, 38, buf, RGB15(31, 26, 6));

    /* Potions counts */
    std::snprintf(buf, sizeof(buf), "P:%d M:%d", p.potions, p.manaPotions);
    drawText(subFb, fbW, 8, 47, buf, RGB15(20, 20, 20));

    /* Depth */
    std::snprintf(buf, sizeof(buf), "Piano %d", g.depth);
    drawText(subFb, fbW, 120, 38, buf, RGB15(20, 18, 15));

    /* Buffs */
    int bx = 8;
    if (p.buffRage > 0) { drawText(subFb, fbW, bx, 56, "FURIA", RGB15(31, 10, 10)); bx += 28; }
    if (p.buffShield > 0) { drawText(subFb, fbW, bx, 56, "SCUDO", RGB15(10, 20, 31)); bx += 30; }
    if (p.buffHaste > 0) { drawText(subFb, fbW, bx, 56, "FRETTA", RGB15(31, 26, 6)); bx += 36; }
    if (p.buffFocus > 0) { drawText(subFb, fbW, bx, 56, "FOCUS", RGB15(15, 28, 31)); bx += 30; }

    /* Touch buttons */
    const u16 btnBg = RGB15(6, 5, 4);
    const u16 btnBorder = RGB15(14, 12, 9);

    /* HP Potion button */
    drawButton(subFb, fbW, BUTTON_ZONES[TB_HP_POTION],
               "POZ HP", btnBg, btnBorder, p.potions > 0 && p.hp < p.maxHp);
    drawText(subFb, fbW, BUTTON_ZONES[TB_HP_POTION].x1 + 4,
             BUTTON_ZONES[TB_HP_POTION].y1 + 10, "POZ HP", RGB15(31, 15, 15));

    /* MP Potion button */
    drawButton(subFb, fbW, BUTTON_ZONES[TB_MP_POTION],
               "POZ MP", btnBg, btnBorder, p.manaPotions > 0 && p.maxMp > 0 && p.mp < p.maxMp);
    drawText(subFb, fbW, BUTTON_ZONES[TB_MP_POTION].x1 + 4,
             BUTTON_ZONES[TB_MP_POTION].y1 + 10, "POZ MP", RGB15(15, 22, 31));

    /* Interact button */
    const u16 intBg = RGB15(8, 12, 6);
    drawButton(subFb, fbW, BUTTON_ZONES[TB_INTERACT], s_interactLabel, intBg, RGB15(14, 20, 10), true);
    drawText(subFb, fbW, BUTTON_ZONES[TB_INTERACT].x1 + 16,
             BUTTON_ZONES[TB_INTERACT].y1 + 10, s_interactLabel, RGB15(20, 28, 15));

    /* Map toggle */
    drawButton(subFb, fbW, BUTTON_ZONES[TB_MAP], "MAP", btnBg, btnBorder, true);
    drawText(subFb, fbW, BUTTON_ZONES[TB_MAP].x1 + 12,
             BUTTON_ZONES[TB_MAP].y1 + 10, "MAP", RGB15(20, 20, 20));

    /* Equipment toggle */
    drawButton(subFb, fbW, BUTTON_ZONES[TB_EQUIP], "EQUIP", btnBg, btnBorder, true);
    drawText(subFb, fbW, BUTTON_ZONES[TB_EQUIP].x1 + 8,
             BUTTON_ZONES[TB_EQUIP].y1 + 10, "EQUIP", RGB15(20, 20, 20));

    /* Minimap area (bottom 54 pixels) */
    if (mapVisible) {
        fillRect(subFb, fbW, 0, 138, fbW, 54, RGB15(1, 1, 2));
        drawText(subFb, fbW, 4, 140, "MINIMAP", RGB15(10, 10, 10));
    }

    /* Equipment panel overlay */
    if (s_equipVisible) {
        fillRect(subFb, fbW, 0, 0, fbW, 192, RGB15(4, 3, 2));
        drawText(subFb, fbW, 4, 4, "EQUIPAGGIAMENTO", RGB15(31, 26, 6));
        for (int i = 0; i < EQ_SLOT_COUNT; i++) {
            const EquipItem& eq = p.equip[i];
            const char* slotName = equipSlotName((EquipSlot)i);
            drawText(subFb, fbW, 8, 16 + i * 14, slotName, RGB15(15, 14, 12));
            if (eq.rarity > 0 || eq.stats.hp > 0) {
                u16 rarityColor = RGB15(15, 15, 15);
                if (eq.rarity == EQ_RARO) rarityColor = RGB15(10, 20, 31);
                if (eq.rarity == EQ_EPICO) rarityColor = RGB15(18, 10, 31);
                if (eq.rarity == EQ_LEGGENARIO) rarityColor = RGB15(31, 18, 4);
                std::snprintf(buf, sizeof(buf), "%s HP:%d DMG:%d%%",
                              equipRarityName(eq.rarity), eq.stats.hp, eq.stats.dmgPct);
                drawText(subFb, fbW, 8, 21 + i * 14, buf, rarityColor);
            } else {
                drawText(subFb, fbW, 8, 21 + i * 14, "---", RGB15(8, 8, 8));
            }
        }
        drawText(subFb, fbW, 4, 178, "TOCCA PER CHIUDERE", RGB15(10, 10, 10));
    }

    /* Merchant panel overlay */
    if (s_merchantVisible) {
        fillRect(subFb, fbW, 0, 0, fbW, 192, RGB15(4, 3, 2));
        drawText(subFb, fbW, 4, 4, "MERCANTE", RGB15(31, 26, 6));
        std::snprintf(buf, sizeof(buf), "Oro: %d", p.gold);
        drawText(subFb, fbW, 4, 14, buf, RGB15(31, 26, 6));

        const int prices[] = { merchantPrice(0, g.depth), merchantPrice(1, g.depth),
                               merchantPrice(2, g.depth), merchantPrice(3, g.depth) };
        const char* items[] = { "Pozione HP", "Pozione MP", "Potenziamento", "Equipaggiamento" };
        const u16 colors[] = { RGB15(31,15,15), RGB15(15,22,31), RGB15(31,26,6), RGB15(20,20,20) };

        for (int i = 0; i < 4; i++) {
            const int y = 28 + i * 16;
            const bool canBuy = p.gold >= prices[i];
            const u16 textColor = canBuy ? colors[i] : RGB15(8, 8, 8);
            drawText(subFb, fbW, 8, y, items[i], textColor);
            std::snprintf(buf, sizeof(buf), "%d Au", prices[i]);
            drawText(subFb, fbW, 140, y, buf, canBuy ? RGB15(31,26,6) : RGB15(8,8,8));
        }
        drawText(subFb, fbW, 4, 178, "TOCCA PER CHIUDERE", RGB15(10, 10, 10));
    }

    /* Death overlay */
    if (p.dead) {
        fillRect(subFb, fbW, 0, 0, fbW, 192, RGB15(15, 3, 3));
        drawText(subFb, fbW, 64, 80, "SEI MORTO", RGB15(31, 8, 8));
        std::snprintf(buf, sizeof(buf), "Respawn: %.0fs", p.respawnT);
        drawText(subFb, fbW, 56, 96, buf, RGB15(20, 12, 12));
        drawText(subFb, fbW, 32, 112, "Oro e equip persi!", RGB15(20, 10, 10));
    }
}

int uiHandleTouch(touchPosition& touch, int screenW, int screenH)
{
    if (s_merchantVisible || s_equipVisible) return TB_NONE;
    const int tx = touch.px;
    const int ty = touch.py;
    for (int i = 0; i < TB_COUNT; i++) {
        const TouchZone& z = BUTTON_ZONES[i];
        if (tx >= z.x1 && tx <= z.x2 && ty >= z.y1 && ty <= z.y2)
            return i;
    }
    return TB_NONE;
}

} // namespace abisso
