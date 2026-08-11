#pragma once

#include <stdint.h>
#include <set>
#include <string>
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
    bool ranged, erratic, boss, split;
};
const MonsterType* getMonsterType(char key);
int monsterWeight(char key, int depth);          // index.html:1113-1136 weight
char pickMonsterType(Rng& rng, int depth);       // index.html:1152
int scaledStat(int base, int depth, double factor); // index.html:1160

/* ---------------------------------------------------------------------
   Equipaggiamento (index.html:2316-2363)
   --------------------------------------------------------------------- */
enum EquipSlot { EQ_HELM, EQ_NECKLACE, EQ_ARMOR, EQ_RING, EQ_GREAVES, EQ_SLOT_COUNT };
enum EquipRarity { EQ_COMUNE, EQ_RARO, EQ_EPICO, EQ_LEGGENARIO };
struct EquipStat { int hp = 0, dmgPct = 0, speedPct = 0, armorPct = 0; };
struct EquipItem {
    EquipSlot slot = EQ_HELM;
    EquipRarity rarity = EQ_COMUNE;
    EquipStat stats;
};
int equipPower(const EquipItem& item);
int equipRarityWeight(EquipRarity r, int depth);
EquipItem makeEquipItem(int depth, Rng& rng);
EquipItem makePremiumEquip(Rng& rng);
const char* equipRarityName(EquipRarity r);
const char* equipSlotName(EquipSlot s);

/* ---------------------------------------------------------------------
   Potenziamenti temporanei (index.html:2306-2314)
   --------------------------------------------------------------------- */
enum PowerupType { PU_RAGE, PU_SHIELD, PU_HASTE, PU_FOCUS, PU_COUNT };
struct PowerupDef { const char* name; double duration; };
extern const PowerupDef POWERUP_DEFS[PU_COUNT];

/* ---------------------------------------------------------------------
   Oggetti a terra (index.html:3266-3281)
   --------------------------------------------------------------------- */
enum GroundItemKind { GI_GOLD, GI_GEM, GI_POTION, GI_MANA_POTION, GI_POWER, GI_EQUIP };
struct GroundItem {
    double x = 0, y = 0;
    GroundItemKind kind = GI_GOLD;
    int amount = 0;          // gold/gem amount
    int powerupType = 0;     // PowerupType index
    EquipItem equip;         // for GI_EQUIP
    uint32_t id = 0;         // unique ID to prevent double-pickup
};

/* ---------------------------------------------------------------------
   Testo fluttuante (index.html:4527)
   --------------------------------------------------------------------- */
struct FloatingText {
    double x = 0, y = 0;
    double life = 1.1;
    double vy = -0.9;
    int colorIdx = 0;        // 0=white, 1=gold, 2=green, 3=red, 4=blue, 5=epic, 6=legendary
    char text[12] = {};
};

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
    int potions = 1;
    int manaPotions = 0;
    double atkT = 0, abilityT = 0, respawnT = 0;
    double invulnT = 0;
    double poisonT = 0;
    bool dead = false;
    /* buff temporanei */
    double buffRage = 0, buffShield = 0, buffHaste = 0, buffFocus = 0;
    /* equipaggiamento permanente */
    EquipItem equip[EQ_SLOT_COUNT] = {};
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
    char affix = 0;          // 0=none, 'f'=veloce, 'e'=esplosivo, 'r'=rigenerante
    int splitLeft = 0;       // gelatina: copie rimanenti
    /* boss AI */
    int bossMove = 0;        // 0=idle,1=claw,2=breath,3=fireball,4=fly,5=dive,6=stomp,7=summon,8=charge
    int bossPhase = 0;       // sotto-fase della mossa
    double bossTimer = 0;
    double bossCdB = 0, bossCdFb = 0, bossCdFly = 0, bossCdSummon = 0, bossCdStomp = 0, bossCdCharge = 0;
    double bossFbx = 0, bossFby = 0, bossFbvx = 0, bossFbvy = 0;
    double bossWindT = 0;
    bool bossHitDone = false;
    double bossWpx = 0, bossWpy = 0;
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
    std::vector<GroundItem> items;
    std::vector<FloatingText> floatTexts;
    bool bossFight = false;
    bool bossActive = false;   // gates sealed
    bool bossDead = false;
    int depth = 1;
    uint32_t worldSeed = 0;
    uint32_t nextItemId = 1;
    /* effetti schermo */
    double shakeT = 0, shakeIntensity = 0;
    double hitstopT = 0;
    double damageFlashT = 0;
    double depthFadeT = 0;
    int depthFadeDir = 0;     // 0=none, 1=fade-in, 2=fade-out
    std::set<std::string> chestsOpened;
    Rng* rng = nullptr;       // puntatore al rng globale (da main)
};

/* --- spawn e update --- */
void makeInitialItems(GameState& g, const Layout& l, int depth, Rng& rng);
void makeMonsters(GameState& g, Rng& rng);
void updateCombat(GameState& g, Rng& rng, double dt);
void playerAttack(GameState& g, Rng& rng);
void playerSetClass(Player& p, const char* key);

/* --- oggetti a terra --- */
void checkItemPickup(GameState& g);

/* --- forzieri --- */
int openChest(GameState& g, Rng& rng, const std::string& chestId, bool isBoss);

/* --- mercante --- */
int merchantPrice(int kind, int depth);
bool buyFromMerchant(GameState& g, Rng& rng, int kind);

/* --- pozioni --- */
void drinkPotion(Player& p);
void drinkManaPotion(Player& p);

/* --- equipaggiamento --- */
void tryEquipOrSalvage(Player& p, const EquipItem& item, Rng& rng);
EquipStat computeEquipBonus(const Player& p);
void applyEquipBonus(Player& p);

/* --- potenziamenti --- */
void applyPowerup(Player& p, int buffType);
void tickBuffs(Player& p, double dt);

/* --- drop mostri --- */
void spawnMonsterDrops(GameState& g, Rng& rng, const Monster& m);

/* --- affix --- */
char pickAffix(int depth, Rng& rng);

/* --- discesa --- */
void advanceDepth(GameState& g, const Layout& newLayout, Rng& rng);

/* --- morte e respawn --- */
void handleDeath(Player& p);
void handleRespawn(GameState& g);

/* --- effetti --- */
void addShake(GameState& g, double intensity);
void addHitstop(GameState& g, double dur);
void addDamageFlash(GameState& g);
void spawnFloatText(GameState& g, double x, double y, const char* text, int colorIdx);

}  // namespace abisso
