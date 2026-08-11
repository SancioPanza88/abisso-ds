#include "sfx.hpp"
#include <nds/arm9/sound.h>
#include <cstring>
#include <cmath>

namespace abisso {

static const int SFX_RATE = 16384;
static const int SFX_MAX = 4096;
static u8 sfxBuf[SFX_MAX];

static void generateTone(int len, double freq, double volume, int type)
{
    if (len > SFX_MAX) len = SFX_MAX;
    for (int i = 0; i < len; i++) {
        double t = (double)i / SFX_RATE;
        double env = 1.0 - (double)i / len;
        env = env * env;
        double sample = 0;
        if (type == 0) {
            sample = std::sin(6.283 * freq * t) * env * volume;
        } else if (type == 1) {
            sample = (std::sin(6.283 * freq * t) > 0 ? 1.0 : -1.0) * env * volume;
        } else if (type == 2) {
            sample = (((i * 73 + 17) & 0xFF) / 128.0 - 1.0) * env * volume;
        } else if (type == 3) {
            double ff = freq * (1.0 - (double)i / len * 0.6);
            sample = (std::sin(6.283 * ff * t) > 0 ? 1.0 : -1.0) * env * volume;
        }
        sfxBuf[i] = (u8)(128 + (int)(sample * 120));
    }
}

static void playSfx(int len)
{
    if (len > SFX_MAX) len = SFX_MAX;
    soundEnable();
    soundPlaySample(sfxBuf, SoundFormat_8Bit, len, SFX_RATE, 127, 64, false, 0);
}

void sfxInit() { soundEnable(); }

void sfxSwing()    { generateTone(600, 600, 0.5, 1); playSfx(600); }
void sfxHit()      { generateTone(500, 350, 0.6, 2); playSfx(500); }
void sfxCrit()     { generateTone(700, 900, 0.7, 0); playSfx(700); }
void sfxKill()     { generateTone(900, 200, 0.6, 3); playSfx(900); }
void sfxBossKill() { generateTone(2048, 80, 0.7, 3); playSfx(2048); }
void sfxHurt()     { generateTone(600, 250, 0.6, 2); playSfx(600); }
void sfxDeath()    { generateTone(2048, 100, 0.7, 3); playSfx(2048); }
void sfxPickup()   { generateTone(400, 1000, 0.4, 0); playSfx(400); }
void sfxGem()      { generateTone(600, 1400, 0.4, 0); playSfx(600); }
void sfxPower()    { generateTone(800, 600, 0.45, 0); playSfx(800); }
void sfxPotion()   { generateTone(700, 500, 0.4, 0); playSfx(700); }
void sfxMana()     { generateTone(700, 800, 0.4, 0); playSfx(700); }
void sfxChest()    { generateTone(1000, 400, 0.45, 0); playSfx(1000); }
void sfxStair()    { generateTone(1200, 350, 0.4, 0); playSfx(1200); }
void sfxStep()     { generateTone(120, 150, 0.2, 2); playSfx(120); }
void sfxBossRoar() { generateTone(2048, 50, 0.7, 3); playSfx(2048); }
void sfxAbility()  { generateTone(800, 500, 0.5, 1); playSfx(800); }
void sfxBoom()     { generateTone(1500, 80, 0.7, 2); playSfx(1500); }
void sfxRevive()   { generateTone(1200, 440, 0.4, 0); playSfx(1200); }

} // namespace abisso
