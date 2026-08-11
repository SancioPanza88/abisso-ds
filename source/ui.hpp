#pragma once

#include <nds.h>

namespace abisso {

struct GameState;

/* Touch button IDs */
enum TouchButton {
    TB_NONE = -1,
    TB_HP_POTION = 0,
    TB_MP_POTION = 1,
    TB_INTERACT = 2,
    TB_MAP = 3,
    TB_EQUIP = 4,
    TB_COUNT = 5
};

/* Inizializza il sistema UI sul sub-screen */
void uiInit();

/* Rendering del sistema UI sul sub-screen */
void uiRender(u16* subFb, int fbW, const GameState& g, int mapVisible);

/* Gestione input touch: ritorna il bottone premuto o TB_NONE */
int uiHandleTouch(touchPosition& touch, int screenW, int screenH);

/* Mostra/nasconde pannello mercante */
void uiShowMerchant(bool show);
bool uiIsMerchantVisible();

/* Mostra/nasconde pannello equipaggiamento */
void uiShowEquip(bool show);
bool uiIsEquipVisible();

/* Aggiorna testo del bottone interact */
void uiSetInteractLabel(const char* label);

} // namespace abisso
