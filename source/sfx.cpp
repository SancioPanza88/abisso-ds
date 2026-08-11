#include "sfx.hpp"
#include <nds/arm9/sound.h>
#include <nds/arm9/input.h>
#include <cstring>
#include <cmath>

namespace abisso {

/* --- Procedural PCM sound buffers (8-bit unsigned, 11025 Hz) --- */
static const int SFX_RATE = 11025;
static const int SFX_MAX = 2048;
static u8 sfxBuf[SFX_MAX];

static void generateTone(int len, double freq, double volume, int type)
{
    for (int i = 0; i < len && i < SFX_MAX; i++) {
        double t = (double)i / SFX_RATE;
        double env = 1.0 - (double)i / len;
        double sample = 0;
        if (type == 0) {
            /* sine */
            sample = std::sin(6.283 * freq * t) * env * volume;
        } else if (type == 1) {
            /* square */
            sample = (std::sin(6.283 * freq * t) > 0 ? 1.0 : -1.0) * env * volume;
        } else if (type == 2) {
            /* noise */
            sample = (((i * 73 + 17) & 0xFF) / 128.0 - 1.0) * env * volume;
        } else if (type == 3) {
            /* descending sweep */
            double ff = freq * (1.0 - (double)i / len * 0.6);
            sample = (std::sin(6.283 * ff * t) > 0 ? 1.0 : -1.0) * env * volume;
        }
        sfxBuf[i] = (u8)(128 + (int)(sample * 127));
    }
}

static void playSfx(int len)
{
    soundEnable();
    soundPlaySample(sfxBuf, SoundFormat_8Bit, len, SFX_RATE, 127, 64, false, 0);
}

void sfxInit() { soundEnable(); }

void sfxSwing()    { generateTone(400, 600, 0.4, 1); playSfx(400); }
void sfxHit()      { generateTone(200, 300, 0.5, 2); playSfx(200); }
void sfxCrit()     { generateTone(300, 800, 0.6, 0); playSfx(300); }
void sfxKill()     { generateTone(500, 200, 0.5, 3); playSfx(500); }
void sfxBossKill() { generateTone(1200, 80, 0.6, 3); playSfx(1200); }
void sfxHurt()     { generateTone(300, 250, 0.5, 2); playSfx(300); }
void sfxDeath()    { generateTone(1500, 120, 0.6, 3); playSfx(1500); }
void sfxPickup()   { generateTone(200, 900, 0.3, 0); playSfx(200); }
void sfxGem()      { generateTone(300, 1200, 0.3, 0); playSfx(300); }
void sfxPower()    { generateTone(400, 600, 0.35, 0); playSfx(400); }
void sfxPotion()   { generateTone(500, 500, 0.3, 0); playSfx(500); }
void sfxMana()     { generateTone(500, 700, 0.3, 0); playSfx(500); }
void sfxChest()    { generateTone(600, 400, 0.35, 0); playSfx(600); }
void sfxStair()    { generateTone(800, 350, 0.3, 0); playSfx(800); }
void sfxStep()     { generateTone(80, 150, 0.15, 2); playSfx(80); }
void sfxBossRoar() { generateTone(1500, 60, 0.6, 3); playSfx(1500); }
void sfxAbility()  { generateTone(600, 500, 0.4, 1); playSfx(600); }
void sfxBoom()     { generateTone(800, 100, 0.6, 2); playSfx(800); }
void sfxRevive()   { generateTone(800, 440, 0.3, 0); playSfx(800); }

} // namespace abisso
