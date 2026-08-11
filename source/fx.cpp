#include "fx.hpp"
#include "game.hpp"

#include <cmath>
#include <cstring>

namespace abisso {

/* Opaque 16-bit color macro — BIT(15) sets the alpha bit in NDS 16-bit
   bitmap mode. Without it, RGB15() produces a TRANSPARENT pixel. */
#define OPAQUE_RGB15(r,g,b) (BIT(15) | RGB15(r,g,b))

/* colori RGB15 per floating text */
static const u16 FT_COLORS[] = {
    OPAQUE_RGB15(31,31,31), /* 0 white */
    OPAQUE_RGB15(31,26, 6), /* 1 gold */
    OPAQUE_RGB15(12,28,10), /* 2 green */
    OPAQUE_RGB15(31, 8, 8), /* 3 red */
    OPAQUE_RGB15(10,20,31), /* 4 blue */
    OPAQUE_RGB15(18,10,31), /* 5 epic purple */
    OPAQUE_RGB15(31,18, 4), /* 6 legendary orange */
};

void fxUpdateShake(GameState& g, int& camX, int& camY)
{
    if (g.shakeT > 0 && g.shakeIntensity > 0) {
        const double intensity = g.shakeIntensity * (g.shakeT / 0.15);
        static unsigned int seed = 12345;
        seed = seed * 1103515245 + 12345;
        const int sx = ((seed >> 16) & 0x1F) - 16;
        seed = seed * 1103515245 + 12345;
        const int sy = ((seed >> 16) & 0x1F) - 16;
        camX += (int)(sx * intensity);
        camY += (int)(sy * intensity);
    }
}

void fxApplyDamageFlash(u16* fb, int fbW, int fbH, double flashT)
{
    if (flashT <= 0) return;
    const int alpha = (int)(flashT / 0.35 * 6);
    if (alpha <= 0) return;
    const u16 red = OPAQUE_RGB15(31, 0, 0);
    for (int y = 0; y < fbH; y++) {
        for (int x = 0; x < fbW; x++) {
            const double dx = (x - fbW / 2.0) / (fbW / 2.0);
            const double dy = (y - fbH / 2.0) / (fbH / 2.0);
            const double dist = std::sqrt(dx * dx + dy * dy);
            if (dist > 1.0 - alpha * 0.1) {
                fb[y * fbW + x] = red;
            }
        }
    }
}

int fxGetDepthFadeAlpha(double fadeT, double fadeMax)
{
    if (fadeT <= 0) return 0;
    double t = fadeT / fadeMax;
    if (t > 1.0) t = 1.0;
    return (int)(t * 31);
}

void fxRenderFloatTexts(u16* fb, int fbW, int camX, int camY,
                        const GameState& g)
{
    for (const FloatingText& ft : g.floatTexts) {
        if (ft.life <= 0) continue;
        const int sx = (int)((ft.x - camX) * 16) + 8;
        const int sy = (int)((ft.y - camY) * 16);
        const double alpha = ft.life / 1.1;
        if (alpha <= 0 || sx < -40 || sx > fbW + 40 || sy < 0 || sy > 192) continue;
        const u16 color = FT_COLORS[ft.colorIdx % 7];
        const int len = (int)std::strlen(ft.text);
        for (int c = 0; c < len && c < 11; c++) {
            const int cx = sx + c * 6;
            const char ch = ft.text[c];
            for (int py = 0; py < 5; py++) {
                for (int px = 0; px < 5; px++) {
                    int bit = 0;
                    if (ch >= '0' && ch <= '9') {
                        static const unsigned char DIGITS[] = {
                            0x0E,0x11,0x13,0x15,0x0E,
                            0x04,0x0C,0x04,0x04,0x0E,
                            0x0E,0x11,0x06,0x08,0x1F,
                            0x1F,0x02,0x06,0x02,0x1F,
                            0x02,0x06,0x0A,0x1F,0x02,
                            0x1F,0x10,0x1E,0x01,0x1E,
                            0x06,0x08,0x0E,0x11,0x0E,
                            0x1F,0x01,0x02,0x04,0x08,
                            0x0E,0x11,0x0E,0x11,0x0E,
                            0x0E,0x11,0x0F,0x01,0x0E,
                        };
                        bit = (DIGITS[(ch - '0') * 5 + py] >> (4 - px)) & 1;
                    } else if (ch == '+') {
                        bit = (py == 2 && px >= 1 && px <= 3) ||
                              (px == 2 && py >= 1 && py <= 3);
                    } else if (ch == '-') {
                        bit = (py == 2 && px >= 1 && px <= 3);
                    } else if (ch == 'A' || ch == 'a') {
                        static const unsigned char A[] = {0x04,0x0A,0x11,0x1F,0x11};
                        bit = (A[py] >> (4 - px)) & 1;
                    } else if (ch == 'u' || ch == 'U') {
                        static const unsigned char U[] = {0x11,0x11,0x11,0x11,0x0E};
                        bit = (U[py] >> (4 - px)) & 1;
                    } else if (ch == 'P') {
                        static const unsigned char P[] = {0x1E,0x11,0x1E,0x10,0x10};
                        bit = (P[py] >> (4 - px)) & 1;
                    } else if (ch == 'z') {
                        static const unsigned char z[] = {0x1F,0x02,0x04,0x08,0x1F};
                        bit = (z[py] >> (4 - px)) & 1;
                    } else if (ch == 'i') {
                        bit = (py == 0 && px == 2) || (py >= 1 && py <= 4 && px == 2);
                    } else if (ch == 'n') {
                        static const unsigned char n[] = {0x00,0x16,0x19,0x11,0x11};
                        bit = (n[py] >> (4 - px)) & 1;
                    } else if (ch == 'o') {
                        static const unsigned char o[] = {0x00,0x0E,0x11,0x11,0x0E};
                        bit = (o[py] >> (4 - px)) & 1;
                    } else if (ch == 'e') {
                        static const unsigned char e[] = {0x0E,0x11,0x1E,0x10,0x0E};
                        bit = (e[py] >> (4 - px)) & 1;
                    } else if (ch == 'l') {
                        static const unsigned char l[] = {0x0C,0x04,0x04,0x04,0x0E};
                        bit = (l[py] >> (4 - px)) & 1;
                    } else if (ch == 't') {
                        static const unsigned char t[] = {0x08,0x1C,0x08,0x08,0x06};
                        bit = (t[py] >> (4 - px)) & 1;
                    } else if (ch == 'm') {
                        static const unsigned char m[] = {0x00,0x16,0x15,0x15,0x11};
                        bit = (m[py] >> (4 - px)) & 1;
                    } else if (ch == 'p') {
                        static const unsigned char p[] = {0x00,0x16,0x19,0x1E,0x10};
                        bit = (p[py] >> (4 - px)) & 1;
                    } else if (ch == 'c') {
                        static const unsigned char cc[] = {0x00,0x0E,0x10,0x10,0x0E};
                        bit = (cc[py] >> (4 - px)) & 1;
                    } else if (ch == ' ') {
                        bit = 0;
                    }
                    if (bit) {
                        const int drawX = cx + px;
                        const int drawY = sy + py;
                        if (drawX >= 0 && drawX < fbW && drawY >= 0 && drawY < 192) {
                            fb[drawY * fbW + drawX] = color;
                        }
                    }
                }
            }
        }
    }
}

} // namespace abisso
