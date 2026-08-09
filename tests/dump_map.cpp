#include <cstdio>
#include <vector>
#include "world.hpp"

int main()
{
    abisso::Layout l = abisso::generateDepth(1, "main", 0x6a78ac8b);

    std::printf("map %dx%d rooms=%d spawn=(%d,%d)\n", l.w, l.h,
                (int)l.rooms.size(), l.spawn.x, l.spawn.y);

    for (const auto& r : l.rooms)
        std::printf("  room x=%d y=%d w=%d h=%d\n", r.x, r.y, r.w, r.h);

    // ASCII della zona attorno allo spawn (15x15)
    const int ox = l.spawn.x, oy = l.spawn.y;
    for (int dy = -7; dy <= 7; dy++) {
        for (int dx = -7; dx <= 7; dx++) {
            const int tx = ox + dx, ty = oy + dy;
            if (tx < 0 || ty < 0 || tx >= l.w || ty >= l.h) { std::printf("#"); continue; }
            const char c = (l.grid[(size_t)ty * l.w + tx] == abisso::T_WALL) ? '#' : '.';
            if (tx == ox && ty == oy) std::printf("P");
            else std::printf("%c", c);
        }
        std::printf("\n");
    }

    // vis simulato allo spawn
    std::vector<uint8_t> vis((size_t)l.w * l.h, 0), visited((size_t)l.w * l.h, 0);
    abisso::computeFov(l, ox, oy, vis, visited);
    int n = 0;
    for (uint8_t v : vis) if (v) n++;
    std::printf("vis@spawn=%d\n", n);

    // vis simulato in un corridoio generico e a meta mappa
    for (int y = 5; y < l.h - 5; y++)
        for (int x = 5; x < l.w - 5; x++) {
            if (l.grid[(size_t)y * l.w + x] != abisso::T_FLOOR) continue;
            if (l.grid[(size_t)y * l.w + x - 1] == abisso::T_WALL &&
                l.grid[(size_t)y * l.w + x + 1] == abisso::T_WALL) {
                abisso::computeFov(l, x, y, vis, visited);
                int c = 0;
                for (uint8_t v : vis) if (v) c++;
                std::printf("corridor at (%d,%d) vis=%d\n", x, y, c);
                break;
            }
        }

    // dove finisce il giocatore se cammina a destra/giu dallo spawn per 3 sec
    double px = l.spawn.x + 0.5, py = l.spawn.y + 0.5;
    const double speed = 3.6;
    for (int f = 0; f < 60 * 3; f++) {
        abisso::tryMoveEntity(l, px, py, 0.7071, 0.7071, 1.0 / 60.0, speed, 0.27);
    }
    std::printf("after 3s walking SE: (%.1f,%.1f) tile=(%d,%d)\n", px, py, (int)px, (int)py);
    abisso::computeFov(l, (int)px, (int)py, vis, visited);
    n = 0;
    for (uint8_t v : vis) if (v) n++;
    std::printf("vis@there=%d\n", n);

    return 0;
}
