#pragma once
/*------------------------------------------------------------------------------
    ABISSO DS — core logico: strutture del mondo e generatore del dungeon.
    Port di index.html: hashStr (FNV-1a), mulberry32, generateDepth.
    NIENTE libnds qui: questo file deve compilare anche su host (test di
    parità con la versione web).
------------------------------------------------------------------------------*/
#include <cstdint>
#include <string>
#include <vector>

namespace abisso {

enum Tile : uint8_t {
    T_WALL  = 0,
    T_FLOOR = 1,
    T_STAIRS = 2,
};

struct Room {
    int x = 0, y = 0, w = 0, h = 0;
};

struct Pt {
    int x = 0, y = 0;
};

struct ChestSpot {
    std::string id;
    int x = 0, y = 0;
};

struct TreasureSpot {
    int x = 0, y = 0;
    bool gem = false;
};

struct PotionSpot {
    int x = 0, y = 0;
    bool mana = false;
};

struct BossRoom {
    int x = 0, y = 0, w = 0, h = 0;
    std::vector<Pt> gates;
    ChestSpot chest;
    char bossType = 'D';
    Pt center{};
};

struct Layout {
    int depth = 1;
    int w = 0, h = 0;
    std::vector<uint8_t> grid;              // h*w, indici grid[y*w+x]
    std::vector<Room> rooms;
    std::vector<Pt> doors;
    Pt spawn{};
    Pt stairsPos{};
    Pt merchantPos{};
    std::vector<ChestSpot> chestSpots;
    std::vector<Pt> monsterSpawnSpots;
    std::vector<TreasureSpot> treasureSpots;
    std::vector<Pt> powerupSpots;
    std::vector<PotionSpot> potionSpots;
    std::vector<Pt> torches;
    bool hasBossRoom = false;
    BossRoom bossRoom;
};

/* FNV-1a 32 bit (hashStr di index.html:945) */
uint32_t hashStr(const std::string& s);

/* mulberry32 (index.html:953): stessa sequenza dell'originale JS */
class Rng {
public:
    explicit Rng(uint32_t seed) : a_(seed) {}
    double next();
private:
    uint32_t a_;
};

/* generateDepth (index.html:1275). Il roomCode è la "stanza" scelta al login,
   worldSeed il seme del mondo: stessa combinazione => stessa mappa del web. */
Layout generateDepth(int depth, const std::string& roomCode, uint32_t worldSeed);

/* --- movimento (index.html:1520-1563) --- */
bool isWalkableTile(const Layout& l, int tx, int ty);
bool canOccupy(const Layout& l, double x, double y, double r);
/* sposta (x,y) di (dx,dy)*speed*dt con collisioni + assistenza d'ingresso
   nei corridoi; dx,dy devono essere un vettore direzione normalizzato */
void tryMoveEntity(const Layout& l, double& x, double& y,
                   double dx, double dy, double dt,
                   double speed, double radius);

/* --- campo visivo (index.html:3745-3785) --- */
constexpr double FOV_RADIUS = 7.4;
/* visibile = tile in linea di vista ora; visited = tile mai viste (fog of war).
   Ricomputare solo quando il tile del giocatore cambia (come updateFOV). */
void computeFov(const Layout& l, int px, int py,
                std::vector<uint8_t>& visible, std::vector<uint8_t>& visited);

/* Serializzazione canonica del layout (per i test di parità) */
std::vector<uint8_t> serializeLayout(const Layout& l);
uint32_t fnv1a(const std::vector<uint8_t>& data);

} // namespace abisso
