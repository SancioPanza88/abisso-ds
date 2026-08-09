#pragma once

#include <stdint.h>
#include <vector>

#include "world.hpp"

namespace abisso {

/* ---------------------------------------------------------------------
   Classi (index.html:1020 CLASSES + SPECIAL_CHARS + NEW_HEROES)
   --------------------------------------------------------------------- */
struct ClassInfo {
    const char* key;
    const char* name;
    int hp, maxMp, manaCost;
    double speed, atkCooldown, range, arc;
    int dmgMin, dmgMax;
    bool ranged;
    double crit;
    double projSpeed;
};
const ClassInfo* getClass(const char* key);
extern const char* const CLASS_KEYS[9];

/* ---------------------------------------------------------------------
   Tipi di mostro (index.html:1112 MONSTER_TYPES)
   --------------------------------------------------------------------- */
struct MonsterType {
    char key;
    const char* name;
    int hp, dmg;
    double speed, aggro, range;
    int goldMin, goldMax;
    bool ranged, erratic, boss;
};
const MonsterType* getMonsterType(char key);
int monsterWeight(char key, int depth);          // index.html:1113-1136 weight
char pickMonsterType(Rng& rng, int depth);       // index.html:1152
int scaledStat(int base, int depth, double factor); // index.html:1160

/* ---------------------------------------------------------------------
   Stato di gioco
   --------------------------------------------------------------------- */
struct Player {
    const ClassInfo* cls = nullptr;
    double x = 0, y = 0;
    double fx = 0, fy = 1;
    int hp = 10, maxHp = 10;
    int mp = 0, maxMp = 0;
    int gold = 0;
    double atkT = 0, abilityT = 0, respawnT = 0;
    bool dead = false;
};

struct Monster {
    char type = 'r';
    double x = 0, y = 0;
    double hp = 1, maxHp = 1;
    int dmg = 1;
    double speed = 2, aggro = 5, range = 0.85, atkCd = 0;
    char state = 'i';        // i idle, w wander, c chase
    double wx = 0, wy = 0, wanderT = 0;
    double fx = 0, fy = 1;
};

struct Bolt {
    double x, y, vx, vy, life, r;
    int dmg;
    bool fromPlayer;
};

struct GameState {
    const Layout* layout = nullptr;
    Player player;
    std::vector<Monster> monsters;
    std::vector<Bolt> bolts;
    bool bossFight = false;
    int depth = 1;
};

void makeMonsters(GameState& g, Rng& rng);        // spawn da layout
void updateCombat(GameState& g, Rng& rng, double dt);
void playerAttack(GameState& g, Rng& rng);        // index.html:3330
void playerSetClass(Player& p, const char* key);  // applica le statistiche di classe

}  // namespace abisso
