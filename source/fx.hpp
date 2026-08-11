#pragma once

#include <nds.h>

namespace abisso {

struct GameState;

/* Schermo shake: applica offset casuale alla camera */
void fxUpdateShake(GameState& g, int& camX, int& camY);

/* Flash rosso quando il giocatore subisce danno */
void fxApplyDamageFlash(u16* fb, int fbW, int fbH, double flashT);

/* Fade-to-black per cambio piano */
int fxGetDepthFadeAlpha(double fadeT, double fadeMax);

/* Floating text: rendering sul framebuffer bitmap */
void fxRenderFloatTexts(u16* fb, int fbW, int camX, int camY,
                        const GameState& g);

} // namespace abisso
