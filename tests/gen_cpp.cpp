/*------------------------------------------------------------------------------
    Test di parità lato C++ (host): genera i piani 1..30 con lo stesso seed
    del test JS e stampa lo stesso formato "<depth> <hash>".
    Compila su host:  g++ -std=c++17 -O2 -I source tests/gen_cpp.cpp source/world.cpp
------------------------------------------------------------------------------*/
#include <cstdio>
#include <cstdint>

#include "world.hpp"

int main()
{
    for (int depth = 1; depth <= 30; depth++) {
        abisso::Layout l = abisso::generateDepth(depth, "main", 123456789u);
        const std::vector<uint8_t> bytes = abisso::serializeLayout(l);
        const uint32_t h = abisso::fnv1a(bytes);
        std::printf("%d %08x\n", depth, h);
    }
    return 0;
}
