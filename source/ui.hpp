#pragma once

#include <nds.h>

namespace abisso {

struct GameState;

enum TouchButton {
    TB_NONE = -1,
    TB_HP_POTION = 0,
    TB_MP_POTION = 1,
    TB_INTERACT = 2,
    TB_MAP = 3,
    TB_EQUIP = 4,
    TB_COUNT = 5
};

void uiInit();
void uiRender(u16* subFb, int fbW, const GameState& g, int mapVisible);
int uiHandleTouch(touchPosition& touch, int screenW, int screenH);

void uiShowMerchant(bool show);
bool uiIsMerchantVisible();

void uiShowEquip(bool show);
bool uiIsEquipVisible();

void uiSetInteractLabel(const char* label);

} // namespace abisso
