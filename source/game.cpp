#include "game.hpp"
#include "sfx.hpp"

#include <cmath>
#include <cstring>
#include <algorithm>

namespace abisso {

int scaledStat(int base, int depth, double factor)
{
    return (int)std::lround(base * (1.0 + factor * (depth - 1)));
}

/* ---------------------------------------------------------------------
   Classi
   --------------------------------------------------------------------- */
static const ClassInfo CLASS_TABLE[] = {
    { "guerriero",  "Guerriero",  16,  0, 0,  3.6, 0.50, 1.55, 1.45,  3,  6, false, 0.00,  0 },
    { "ladro",      "Ladro",      10,  0, 0,  4.6, 0.32, 1.35, 1.15,  2,  4, false, 0.28,  0 },
    { "mago",       "Mago",        9, 14, 2,  3.2, 0.75, 8.00, 0,     4,  8,  true,  0.00, 11 },
    { "ranger",     "Ranger",     11,  0, 0,  3.9, 0.42, 9.00, 0,     2,  5,  true,  0.00, 15 },
    { "prof",       "Prof",       22,  0, 0,  3.6, 0.55, 8.50, 0,     5,  9,  true,  0.00, 14 },
    { "paladino",   "Paladino",   18,  0, 0,  3.3, 0.60, 1.50, 1.30,  3,  5, false, 0.00,  0 },
    { "negromante", "Negromante", 10, 14, 2,  3.2, 0.70, 8.00, 0,     4,  7,  true,  0.00, 11 },
    { "bardo",      "Bardo",      12,  0, 0,  4.0, 0.35, 1.40, 1.10,  2,  4, false, 0.15,  0 },
    { "monaco",     "Monaco",     14,  0, 0,  4.4, 0.30, 1.30, 1.25,  3,  5, false, 0.20,  0 },
};

const char* const CLASS_KEYS[9] = {
    "guerriero", "ladro", "mago", "ranger", "prof",
    "paladino", "negromante", "bardo", "monaco",
};

const ClassInfo* getClass(const char* key)
{
    for (const ClassInfo& c : CLASS_TABLE)
        if (std::strcmp(c.key, key) == 0) return &c;
    return &CLASS_TABLE[0];
}

void playerSetClass(Player& p, const char* key)
{
    p.cls = getClass(key);
    p.maxHp = p.cls->hp;
    p.hp = p.cls->hp;
    p.maxMp = p.cls->maxMp;
    p.mp = p.cls->maxMp;
    p.gold = 0;
    p.potions = 1;
    p.manaPotions = 0;
    p.atkT = 0;
    p.abilityT = 0;
    p.invulnT = 0;
    p.poisonT = 0;
    p.dead = false;
    p.buffRage = 0; p.buffShield = 0; p.buffHaste = 0; p.buffFocus = 0;
    for (int i = 0; i < EQ_SLOT_COUNT; i++) p.equip[i] = EquipItem();
    applyEquipBonus(p);
}

/* ---------------------------------------------------------------------
   Mostri
   --------------------------------------------------------------------- */
static const MonsterType MONSTER_TABLE[] = {
    { 'r', "ratto",          7,  2,  2.6, 5.5, 0.85,  1,  3, false, false, false, false, false, false, false, false, false, false, false },
    { 'b', "pipistrello",    8,  2,  2.9, 6.5, 0.85,  1,  3, false, true,  false, false, false, false, false, false, false, false, false },
    { 'g', "goblin",        14,  3,  2.3, 6.0, 0.85,  2,  5, false, false, false, false, false, false, false, false, false, false, false },
    { 'j', "melma",          9,  2,  1.4, 4.5, 0.85,  1,  2, false, false, false, false, false, false, false, false, false, false, false },
    { 'J', "gelatina",      22,  3,  1.2, 4.5, 0.85,  2,  4, false, false, false, true,  false, false, false, false, false, false, false },
    { 's', "scheletro",     20,  4,  1.9, 6.0, 0.85,  3,  6, false, false, false, false, false, false, false, false, false, false, false },
    { 'o', "orco",          32,  6,  2.0, 6.5, 0.85,  4,  9, false, false, false, false, false, false, false, false, false, false, false },
    { 'z', "zombie",        26,  5,  1.3, 5.0, 0.85,  3,  6, false, false, false, false, false, false, false, false, false, false, false },
    { 'S', "ragno gigante", 24,  5,  3.0, 7.5, 0.85,  4,  8, false, false, false, false, false, false, false, true,  false, false, false },
    { 'W', "spettro",       28,  6,  2.7, 8.0, 0.85,  5, 10, false, false, false, false, false, false, false, false, false, true,  false },
    { 'k', "serpente",      10,  2,  2.5, 5.5, 0.85,  1,  3, false, false, false, false, true,  false, false, true,  false, false, false },
    { 'h', "arpia",          9,  2,  2.9, 6.5, 0.85,  1,  3, false, true,  false, false, false, true,  false, false, false, false, false },
    { 'C', "cavaliere",     26,  5,  1.8, 6.0, 0.85,  3,  6, false, false, false, false, false, false, true,  false, false, false, false },
    { 'c', "cultista",      18,  4,  2.0, 6.5, 3.00,  3,  6, true,  false, false, false, false, false, false, false, false, false, false },
    { 'm', "mantide",       22,  4,  3.2, 7.5, 0.85,  4,  8, false, false, false, false, false, false, false, false, true,  false, false },
    { 'q', "sciamano",      20,  4,  1.9, 7.0, 2.60,  3,  7, true,  false, false, false, false, false, false, false, false, false, true  },
    { 'G', "golem",         40,  7,  1.4, 5.0, 0.85,  6, 12, false, false, false, false, false, false, false, false, false, true,  false },
    { 'D', "Drago",        120, 11,  2.3, 11.0, 0.85, 40, 70, false, false, true,  false, false, false, false, false, false, false, false },
    { 'X', "Golem di Pietra", 150, 12,  1.6, 12.0, 0.85, 220, 300, false, false, true,  false, false, false, false, false, false, true,  false },
    { 'L', "Lich",         135, 11,  2.1, 12.0, 0.85, 200, 280, false, false, true,  false, false, false, false, false, false, false, false },
    { 'M', "Regina Melme", 130, 10,  1.5, 10.0, 0.85, 190, 260, false, false, true,  false, false, false, false, false, false, false, false },
    { 'R', "Re Ragno",     140, 12,  2.6, 12.0, 0.85, 210, 290, false, false, true,  false, false, false, false, true,  false, false, false },
    { 'K', "Re dei ratti", 120, 11,  2.9, 12.0, 0.85, 200, 280, false, false, true,  false, false, false, false, false, false, false, false },
};

const MonsterType* getMonsterType(char key)
{
    for (const MonsterType& t : MONSTER_TABLE)
        if (t.key == key) return &t;
    return &MONSTER_TABLE[0];
}

int monsterWeight(char key, int depth)
{
    switch (key) {
        case 'r': return depth <= 10 ? 10 - depth : 0;
        case 'b': return depth <= 9  ?  9 - depth : 0;
        case 'g': return 8;
        case 'j': return depth >= 2 ? 6 : 2;
        case 'J': return depth >= 2 ? 5 : 1;
        case 's': return depth >= 2 ? 9 : 2;
        case 'o': return depth >= 3 ? 8 : 1;
        case 'z': return depth >= 3 ? 7 : 1;
        case 'S': return depth >= 4 ? 7 : 0;
        case 'W': return depth >= 5 ? 6 : 0;
        case 'k': return 7;
        case 'h': return depth <= 11 ? 11 - depth : 0;
        case 'C': return depth >= 2 ? 8 : 1;
        case 'c': return depth >= 3 ? 6 : 1;
        case 'm': return depth >= 4 ? 6 : 0;
        case 'q': return depth >= 4 ? 5 : 0;
        case 'G': return depth >= 4 ? 5 : 0;
    }
    return 0;
}

char pickMonsterType(Rng& rng, int depth)
{
    static const char WEB_ORDER[] = { 'r','b','g','j','J','s','o','z','S','W','k','h','C','c','m','q','G' };
    int total = 0;
    for (char k : WEB_ORDER) total += monsterWeight(k, depth);
    if (total <= 0) total = 1;
    double r = rng.next() * total;
    for (char k : WEB_ORDER) {
        r -= monsterWeight(k, depth);
        if (r <= 0) return k;
    }
    return WEB_ORDER[sizeof(WEB_ORDER) - 1];
}

/* ---------------------------------------------------------------------
   Affix (index.html:2285-2294)
   --------------------------------------------------------------------- */
char pickAffix(int depth, Rng& rng)
{
    const double chance = std::min(0.4, 0.1 + depth * 0.018);
    if (rng.next() > chance) return 0;
    static const char AFFIXES[] = { 'f', 'e', 'r' };
    return AFFIXES[(int)(rng.next() * 3) % 3];
}

/* ---------------------------------------------------------------------
   Equipaggiamento (index.html:2316-2363)
   --------------------------------------------------------------------- */
static const char* const EQUIP_SLOT_NAMES[] = { "Elmo", "Collana", "Armatura", "Anello", "Gambali" };
static const char* const EQUIP_RARITY_NAMES[] = { "Comune", "Raro", "Epico", "Leggendario" };
static const int RARITY_WEIGHTS[] = { 100, 32, 9, 2 };
static const double RARITY_MULT[] = { 1.0, 1.35, 1.6, 2.2 };
static const int RARITY_STAT_COUNT[] = { 1, 1, 2, 3 };

const char* equipSlotName(EquipSlot s) { return EQUIP_SLOT_NAMES[(int)s]; }
const char* equipRarityName(EquipRarity r) { return EQUIP_RARITY_NAMES[(int)r]; }

int equipPower(const EquipItem& item)
{
    return item.stats.hp + item.stats.dmgPct + item.stats.speedPct + item.stats.armorPct;
}

int equipRarityWeight(EquipRarity r, int depth)
{
    if (r == EQ_COMUNE) return RARITY_WEIGHTS[(int)r];
    return (int)(RARITY_WEIGHTS[(int)r] * (1.0 + depth * 0.045));
}

static EquipRarity pickRarity(int depth, Rng& rng)
{
    int total = 0;
    for (int i = 0; i < 4; i++) total += equipRarityWeight((EquipRarity)i, depth);
    double r = rng.next() * total;
    for (int i = 0; i < 4; i++) {
        r -= equipRarityWeight((EquipRarity)i, depth);
        if (r <= 0) return (EquipRarity)i;
    }
    return EQ_COMUNE;
}

EquipItem makeEquipItem(int depth, Rng& rng)
{
    EquipItem item;
    item.slot = (EquipSlot)((int)(rng.next() * EQ_SLOT_COUNT) % EQ_SLOT_COUNT);
    item.rarity = pickRarity(depth, rng);
    const int nStats = RARITY_STAT_COUNT[(int)item.rarity];
    const double mult = RARITY_MULT[(int)item.rarity];
    /* hp bonus */
    if (nStats >= 1) {
        item.stats.hp = (int)std::lround((4 + depth * 1.6) * mult);
        if (nStats >= 2) item.stats.dmgPct = (int)std::lround((5 + depth * 1.1) * mult);
        if (nStats >= 3) item.stats.armorPct = (int)std::lround((4 + depth * 0.9) * mult);
    }
    if (nStats == 1) {
        /* single stat: random pick */
        const int pick = (int)(rng.next() * 4) % 4;
        if (pick == 0) item.stats.hp = (int)std::lround((4 + depth * 1.6) * mult);
        else if (pick == 1) { item.stats.hp = 0; item.stats.dmgPct = (int)std::lround((5 + depth * 1.1) * mult); }
        else if (pick == 2) { item.stats.hp = 0; item.stats.speedPct = (int)std::lround((3 + depth * 0.5) * mult); }
        else { item.stats.hp = 0; item.stats.armorPct = (int)std::lround((4 + depth * 0.9) * mult); }
    }
    return item;
}

EquipItem makePremiumEquip(Rng& rng)
{
    EquipItem item;
    item.slot = (EquipSlot)((int)(rng.next() * EQ_SLOT_COUNT) % EQ_SLOT_COUNT);
    item.rarity = rng.next() < 0.45 ? EQ_LEGGENARIO : EQ_EPICO;
    const int nStats = RARITY_STAT_COUNT[(int)item.rarity];
    const double mult = RARITY_MULT[(int)item.rarity];
    /* boss chest: depth 10 base for premium stats */
    const int d = 10;
    item.stats.hp = (int)std::lround((4 + d * 1.6) * mult);
    if (nStats >= 2) item.stats.dmgPct = (int)std::lround((5 + d * 1.1) * mult);
    if (nStats >= 3) item.stats.armorPct = (int)std::lround((4 + d * 0.9) * mult);
    if (nStats == 1) {
        item.stats.hp = (int)std::lround((4 + d * 1.6) * mult);
        item.stats.dmgPct = 0; item.stats.speedPct = 0; item.stats.armorPct = 0;
    }
    return item;
}

EquipStat computeEquipBonus(const Player& p)
{
    EquipStat bonus;
    for (int i = 0; i < EQ_SLOT_COUNT; i++) {
        const EquipItem& it = p.equip[i];
        bonus.hp += it.stats.hp;
        bonus.dmgPct += it.stats.dmgPct;
        bonus.speedPct += it.stats.speedPct;
        bonus.armorPct += it.stats.armorPct;
    }
    return bonus;
}

void applyEquipBonus(Player& p)
{
    const EquipStat bonus = computeEquipBonus(p);
    const int oldMaxHp = p.maxHp;
    p.maxHp = p.cls->hp + bonus.hp;
    if (p.hp == oldMaxHp || p.hp > p.maxHp) p.hp = p.maxHp;
}

void tryEquipOrSalvage(Player& p, const EquipItem& item, Rng& rng)
{
    const EquipItem& current = p.equip[item.slot];
    const int newPower = equipPower(item);
    const int oldPower = equipPower(current);
    if (newPower >= oldPower) {
        p.equip[item.slot] = item;
        applyEquipBonus(p);
        p.hp = std::min(p.maxHp, p.hp + std::max(0, item.stats.hp - current.stats.hp));
    } else {
        const int salvage = 3 + (int)(newPower * 0.6);
        p.gold += salvage;
    }
}

/* ---------------------------------------------------------------------
   Potenziamenti (index.html:2306-2314)
   --------------------------------------------------------------------- */
const PowerupDef POWERUP_DEFS[PU_COUNT] = {
    { "Furia", 12.0 },
    { "Scudo", 10.0 },
    { "Fretta", 12.0 },
    { "Concentrazione", 10.0 },
};

void applyPowerup(Player& p, int buffType)
{
    if (buffType < 0 || buffType >= PU_COUNT) return;
    switch (buffType) {
        case PU_RAGE:   p.buffRage   = POWERUP_DEFS[buffType].duration; break;
        case PU_SHIELD: p.buffShield = POWERUP_DEFS[buffType].duration; break;
        case PU_HASTE:  p.buffHaste  = POWERUP_DEFS[buffType].duration; break;
        case PU_FOCUS:  p.buffFocus  = POWERUP_DEFS[buffType].duration; break;
    }
}

void tickBuffs(Player& p, double dt)
{
    if (p.buffRage > 0)   p.buffRage   = std::max(0.0, p.buffRage - dt);
    if (p.buffShield > 0) p.buffShield = std::max(0.0, p.buffShield - dt);
    if (p.buffHaste > 0)  p.buffHaste  = std::max(0.0, p.buffHaste - dt);
    if (p.buffFocus > 0)  p.buffFocus  = std::max(0.0, p.buffFocus - dt);
}

/* ---------------------------------------------------------------------
   Effetti schermo
   --------------------------------------------------------------------- */
void addShake(GameState& g, double intensity)
{
    g.shakeIntensity = std::max(g.shakeIntensity, intensity);
    g.shakeT = std::max(g.shakeT, 0.15);
}

void addHitstop(GameState& g, double dur)
{
    g.hitstopT = std::max(g.hitstopT, dur);
}

void addDamageFlash(GameState& g)
{
    g.damageFlashT = 0.35;
}

void spawnFloatText(GameState& g, double x, double y, const char* text, int colorIdx)
{
    if ((int)g.floatTexts.size() >= 32) return;
    FloatingText ft;
    ft.x = x; ft.y = y;
    ft.life = 1.1; ft.vy = -0.9; ft.colorIdx = colorIdx;
    int i = 0;
    while (text[i] && i < 11) { ft.text[i] = text[i]; i++; }
    ft.text[i] = '\0';
    g.floatTexts.push_back(ft);
}

/* ---------------------------------------------------------------------
   Spawn (index.html:1823 makeMonster + generazione iniziale)
   --------------------------------------------------------------------- */
static char bossTypeForDepthGame(int depth)
{
    switch (depth) {
        case 5:  return 'M';
        case 10: return 'D';
        case 15: return 'R';
        case 20: return 'X';
        case 25: return 'K';
        case 30: return 'L';
    }
    return 0;
}

static Monster makeMonsterAt(char type, double x, double y, int depth, char affix)
{
    const MonsterType& t = *getMonsterType(type);
    Monster m;
    m.type = type;
    m.x = x;
    m.y = y;
    m.maxHp = scaledStat(t.hp, depth, 0.16);
    m.hp = m.maxHp;
    m.dmg = scaledStat(t.dmg, depth, 0.11);
    m.speed = t.speed;
    m.aggro = t.aggro;
    m.range = t.range;
    m.affix = affix;
    if (affix == 'f') m.speed *= 1.6;
    if (type == 'J') m.splitLeft = 1;
    return m;
}

void makeMonsters(GameState& g, Rng& rng)
{
    const Layout& l = *g.layout;
    g.monsters.clear();
    g.bolts.clear();
    for (const Pt& s : l.monsterSpawnSpots) {
        const char type = pickMonsterType(rng, g.depth);
        const char affix = pickAffix(g.depth, rng);
        g.monsters.push_back(makeMonsterAt(type, s.x + 0.5, s.y + 0.5, g.depth, affix));
    }
    const char bkey = bossTypeForDepthGame(g.depth);
    if (bkey && l.hasBossRoom) {
        g.monsters.push_back(makeMonsterAt(
            bkey,
            l.bossRoom.x + l.bossRoom.w / 2.0,
            l.bossRoom.y + l.bossRoom.h / 2.0,
            g.depth, 0));
    }
}

/* ---------------------------------------------------------------------
   Oggetti a terra (index.html:3266-3281)
   --------------------------------------------------------------------- */
void makeInitialItems(GameState& g, const Layout& l, int depth, Rng& rng)
{
    g.items.clear();
    /* treasure spots: gold o gem */
    for (const TreasureSpot& t : l.treasureSpots) {
        GroundItem gi;
        gi.x = t.x + 0.5; gi.y = t.y + 0.5;
        gi.id = g.nextItemId++;
        if (t.gem) {
            gi.kind = GI_GEM;
            gi.amount = 15 + (int)(rng.next() * 10 * depth);
        } else {
            gi.kind = GI_GOLD;
            gi.amount = 3 + (int)(rng.next() * 6 * depth);
        }
        g.items.push_back(gi);
    }
    /* 1 powerup random da powerupSpots */
    if (l.powerupSpots.size() > 0) {
        const Pt& spot = l.powerupSpots[(int)(rng.next() * l.powerupSpots.size()) % l.powerupSpots.size()];
        GroundItem gi;
        gi.x = spot.x + 0.5; gi.y = spot.y + 0.5;
        gi.id = g.nextItemId++;
        gi.kind = GI_POWER;
        gi.powerupType = (int)(rng.next() * PU_COUNT) % PU_COUNT;
        g.items.push_back(gi);
    }
    /* potion spots */
    for (const PotionSpot& p : l.potionSpots) {
        GroundItem gi;
        gi.x = p.x + 0.5; gi.y = p.y + 0.5;
        gi.id = g.nextItemId++;
        gi.kind = p.mana ? GI_MANA_POTION : GI_POTION;
        gi.amount = 1;
        g.items.push_back(gi);
    }
}

void checkItemPickup(GameState& g)
{
    Player& p = g.player;
    if (p.dead) return;
    const double px = p.x, py = p.y;
    std::vector<GroundItem> remaining;
    remaining.reserve(g.items.size());
    for (const GroundItem& it : g.items) {
        const double dx = it.x - px, dy = it.y - py;
        if (dx * dx + dy * dy < 0.62 * 0.62) {
            switch (it.kind) {
                case GI_GOLD:
                    p.gold += it.amount;
                    spawnFloatText(g, p.x, p.y - 0.7, ("+" + std::to_string(it.amount) + " Au").c_str(), 1);
                    break;
                case GI_GEM:
                    p.gold += it.amount;
                    spawnFloatText(g, p.x, p.y - 0.7, ("+" + std::to_string(it.amount) + " Au").c_str(), 4);
                    break;
                case GI_POTION:
                    p.potions++;
                    spawnFloatText(g, p.x, p.y - 0.7, "+Pozione", 2);
                    break;
                case GI_MANA_POTION:
                    p.manaPotions++;
                    spawnFloatText(g, p.x, p.y - 0.7, "+PozMana", 4);
                    break;
                case GI_POWER:
                    applyPowerup(p, it.powerupType);
                    spawnFloatText(g, p.x, p.y - 0.7, POWERUP_DEFS[it.powerupType].name, 5);
                    break;
                case GI_EQUIP:
                    tryEquipOrSalvage(p, it.equip, *g.rng);
                    spawnFloatText(g, p.x, p.y - 0.7, equipRarityName(it.equip.rarity),
                                   it.equip.rarity >= EQ_EPICO ? (it.equip.rarity == EQ_LEGGENARIO ? 6 : 5) : 0);
                    break;
            }
            continue; /* pick up */
        }
        remaining.push_back(it);
    }
    g.items.swap(remaining);
}

/* ---------------------------------------------------------------------
   Forzieri (index.html:4143-4161)
   --------------------------------------------------------------------- */
int openChest(GameState& g, Rng& rng, const std::string& chestId, bool isBoss)
{
    /* boss chest */
    if (isBoss) {
        if (!g.bossDead) return -1; /* locked */
        const int gold = 60 + (int)(rng.next() * 25 * g.depth);
        g.player.gold += gold;
        g.player.potions++;
        g.player.manaPotions++;
        EquipItem eq = makePremiumEquip(rng);
        tryEquipOrSalvage(g.player, eq, rng);
        return gold;
    }
    /* regular chest */
    const int gold = 6 + (int)(rng.next() * 9 * g.depth);
    g.player.gold += gold;
    if (rng.next() < 0.32) g.player.potions++;
    if (rng.next() < 0.24) g.player.manaPotions++;
    if (rng.next() < 0.20) {
        EquipItem eq = makeEquipItem(g.depth, rng);
        tryEquipOrSalvage(g.player, eq, rng);
    }
    return gold;
}

/* ---------------------------------------------------------------------
   Mercante (index.html:4163-4228)
   --------------------------------------------------------------------- */
int merchantPrice(int kind, int depth)
{
    switch (kind) {
        case 0: return 12 + depth * 2;  /* pozione salute */
        case 1: return 12 + depth * 2;  /* pozione mana */
        case 2: return 20 + depth * 4;  /* powerup */
        case 3: return 45 + depth * 8;  /* equipaggiamento */
    }
    return 999;
}

bool buyFromMerchant(GameState& g, Rng& rng, int kind)
{
    Player& p = g.player;
    const int cost = merchantPrice(kind, g.depth);
    if (p.gold < cost) return false;
    p.gold -= cost;
    switch (kind) {
        case 0: p.potions++; break;
        case 1: p.manaPotions++; break;
        case 2: { int bt = (int)(rng.next() * PU_COUNT) % PU_COUNT; applyPowerup(p, bt); break; }
        case 3: { EquipItem eq = makeEquipItem(g.depth, rng); tryEquipOrSalvage(p, eq, rng); break; }
    }
    return true;
}

/* ---------------------------------------------------------------------
   Pozioni (index.html:4906-4929)
   --------------------------------------------------------------------- */
void drinkPotion(Player& p)
{
    if (p.dead || p.potions <= 0 || p.hp >= p.maxHp) return;
    p.potions--;
    const int heal = (int)std::lround(p.maxHp * 0.55) + 2;
    p.hp = std::min(p.maxHp, p.hp + heal);
}

void drinkManaPotion(Player& p)
{
    if (p.dead || !p.cls || p.cls->maxMp <= 0 || p.manaPotions <= 0 || p.mp >= p.maxMp) return;
    p.manaPotions--;
    const int restore = (int)std::lround(p.maxMp * 0.6) + 1;
    p.mp = std::min(p.maxMp, p.mp + restore);
}

/* ---------------------------------------------------------------------
   Drop mostri (index.html:3425-3437)
   --------------------------------------------------------------------- */
static void spawnDropAt(GameState& g, double x, double y, GroundItemKind kind, int amount)
{
    GroundItem gi;
    gi.x = x; gi.y = y;
    gi.id = g.nextItemId++;
    gi.kind = kind;
    gi.amount = amount;
    g.items.push_back(gi);
}

void spawnMonsterDrops(GameState& g, Rng& rng, const Monster& m)
{
    const MonsterType& t = *getMonsterType(m.type);
    const int gold = t.goldMin + (int)(rng.next() * (t.goldMax - t.goldMin + 1));
    spawnDropAt(g, std::floor(m.x), std::floor(m.y), GI_GOLD, gold);
    if (rng.next() < 0.16)
        spawnDropAt(g, std::floor(m.x), std::floor(m.y), GI_POTION, 1);
    if (rng.next() < 0.11)
        spawnDropAt(g, std::floor(m.x), std::floor(m.y), GI_MANA_POTION, 1);
    if (rng.next() < (t.boss ? 0.95 : 0.07)) {
        GroundItem gi;
        gi.x = std::floor(m.x); gi.y = std::floor(m.y);
        gi.id = g.nextItemId++;
        gi.kind = GI_EQUIP;
        gi.equip = makeEquipItem(g.depth, rng);
        g.items.push_back(gi);
    }
}

/* ---------------------------------------------------------------------
   Discesa piano (index.html:3330-3367)
   --------------------------------------------------------------------- */
void advanceDepth(GameState& g, const Layout& newLayout, Rng& rng)
{
    g.depth++;
    g.layout = &newLayout;
    g.bossActive = false;
    g.bossDead = false;
    g.bossFight = false;
    g.bolts.clear();
    g.items.clear();
    g.floatTexts.clear();
    g.depthFadeT = 0.55;
    g.depthFadeDir = 1;
    spawnFloatText(g, g.player.x, g.player.y - 1.0, ("Piano " + std::to_string(g.depth)).c_str(), 1);
}

/* ---------------------------------------------------------------------
   Morte e respawn (index.html:4986-5005)
   --------------------------------------------------------------------- */
void handleDeath(Player& p)
{
    p.dead = true;
    p.respawnT = 3.2;
}

void handleRespawn(GameState& g)
{
    Player& p = g.player;
    p.dead = false;
    p.gold = 0;
    p.potions = 0;
    p.manaPotions = 0;
    for (int i = 0; i < EQ_SLOT_COUNT; i++) p.equip[i] = EquipItem();
    p.buffRage = 0; p.buffShield = 0; p.buffHaste = 0; p.buffFocus = 0;
    p.hp = p.cls->hp; p.maxHp = p.cls->hp;
    p.mp = p.cls->maxMp; p.maxMp = p.cls->maxMp;
    applyEquipBonus(p);
    if (g.layout) {
        p.x = g.layout->spawn.x + 0.5;
        p.y = g.layout->spawn.y + 0.5;
    }
}

/* ---------------------------------------------------------------------
   Combattimento
   --------------------------------------------------------------------- */
static int rollDamage(const ClassInfo& c, Rng& rng)
{
    return c.dmgMin + (int)std::floor(rng.next() * (c.dmgMax - c.dmgMin + 1));
}

void playerAttack(GameState& g, Rng& rng)
{
    Player& p = g.player;
    if (p.dead) return;
    if (p.atkT > 0) return;
    const ClassInfo& c = *p.cls;
    if (c.maxMp > 0 && c.manaCost && p.mp < c.manaCost) return;
    p.atkT = c.atkCooldown;
    if (c.manaCost) p.mp -= c.manaCost;
    if (p.buffFocus > 0) p.atkT *= 0.5;

    const double autoRange = c.ranged ? c.range : std::max(c.range + 0.7, 2.1);
    const Monster* nearest = nullptr;
    double bestD = autoRange * autoRange;
    for (const Monster& m : g.monsters) {
        const double dx = m.x - p.x, dy = m.y - p.y;
        const double d = dx * dx + dy * dy;
        if (d < bestD) { bestD = d; nearest = &m; }
    }
    if (nearest) {
        const double dx = nearest->x - p.x, dy = nearest->y - p.y;
        const double d = std::sqrt(dx * dx + dy * dy);
        if (d > 0.001) { p.fx = dx / d; p.fy = dy / d; }
    }

    if (!c.ranged) {
        for (Monster& m : g.monsters) {
            const double dx = m.x - p.x, dy = m.y - p.y;
            const double d = std::sqrt(dx * dx + dy * dy);
            if (d > c.range) continue;
            const double nx = dx / (d != 0 ? d : 1), ny = dy / (d != 0 ? d : 1);
            if (nx * p.fx + ny * p.fy < std::cos(c.arc)) continue;
            int amount = rollDamage(c, rng);
            if (p.buffRage > 0) amount = (int)(amount * 1.4);
            if (c.crit > 0 && rng.next() < c.crit) amount *= 2;
            m.hp -= amount;
            spawnFloatText(g, m.x, m.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
            if (m.hp <= 0) {
                const MonsterType& t = *getMonsterType(m.type);
                p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
                spawnMonsterDrops(g, rng, m);
            }
        }
    } else {
        Bolt b;
        b.x = p.x; b.y = p.y;
        b.vx = p.fx * c.projSpeed;
        b.vy = p.fy * c.projSpeed;
        b.life = 1.6;
        b.r = 0.4;
        b.dmg = rollDamage(c, rng);
        if (p.buffRage > 0) b.dmg = (int)(b.dmg * 1.4);
        b.fromPlayer = true;
        g.bolts.push_back(b);
    }
}

static void dealMeleeDamageToPlayer(GameState& g, Monster& m, double mult, Rng& rng)
{
    Player& p = g.player;
    if (p.dead || p.invulnT > 0) return;
    int amount = m.dmg + (int)std::floor(rng.next() * 2);
    amount = (int)(amount * mult);
    if (p.buffShield > 0) amount /= 2;
    const EquipStat eq = computeEquipBonus(p);
    amount = (int)(amount * std::max(0.2, 1.0 - eq.armorPct / 100.0));
    amount = std::max(1, amount);
    p.hp -= amount;
    p.invulnT = 0.45;
    m.atkAnimT = 0.3;
    addDamageFlash(g);
    addShake(g, 0.22);
    spawnFloatText(g, p.x, p.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
    spawnBurst(g.particles, m.x + m.fx * 0.4, m.y + m.fy * 0.4, 1, 0.2, 0.2, 3);
    const MonsterType& mt = *getMonsterType(m.type);
    if (mt.poison || m.type == 'S' || m.type == 'k' || m.type == 'R')
        p.poisonT = 3.0;
    if (mt.lifesteal) {
        m.hp = std::min(m.maxHp, m.hp + amount / 2);
        std::string healStr = "+" + std::to_string(amount / 2);
        spawnFloatText(g, m.x, m.y - 0.5, healStr.c_str(), 2);
    }
    if (p.hp <= 0) handleDeath(p);
}

static void bossAI(Monster& m, GameState& g, Rng& rng, double dt)
{
    const MonsterType& t = *getMonsterType(m.type);
    const Layout& l = *g.layout;
    Player& p = g.player;

    if (t.boss && l.hasBossRoom && !g.bossFight) {
        const BossRoom& br = l.bossRoom;
        m.x = std::min(std::max(m.x, br.x + 0.45), br.x + br.w - 0.45);
        m.y = std::min(std::max(m.y, br.y + 0.45), br.y + br.h - 0.45);
        m.state = 'i';
        m.atkCd = 0;
        m.bossMove = 0;
        return;
    }

    if (m.affix == 'r' && m.hp < m.maxHp)
        m.hp = std::min(m.maxHp, m.hp + dt * 0.045);

    m.atkCd = std::max(0.0, m.atkCd - dt);
    m.bossCdFb = std::max(0.0, m.bossCdFb - dt);
    m.bossCdFly = std::max(0.0, m.bossCdFly - dt);
    m.bossCdSummon = std::max(0.0, m.bossCdSummon - dt);
    m.bossCdStomp = std::max(0.0, m.bossCdStomp - dt);
    m.bossCdCharge = std::max(0.0, m.bossCdCharge - dt);

    const double dx = p.x - m.x, dy = p.y - m.y;
    const double d2 = dx * dx + dy * dy;
    const double d = std::sqrt(d2 + 0.0001);
    const double vx = dx / d, vy = dy / d;
    m.fx = vx; m.fy = vy;

    m.state = 'c';

    if (m.bossMove == 0) {
        if (d > 1.4) {
            tryMoveEntity(l, m.x, m.y, vx, vy, dt, m.speed * 0.7, 0.45);
        }
        double roll = rng.next();
        if (roll < 0.15 && m.atkCd <= 0) {
            m.bossMove = 1; m.bossPhase = 0; m.bossTimer = 0; m.bossHitDone = false;
        } else if (roll < 0.30 && m.bossCdFb <= 0) {
            m.bossMove = 3; m.bossPhase = 0; m.bossTimer = 0; m.bossHitDone = false;
        } else if (roll < 0.45 && m.bossCdFly <= 0) {
            m.bossMove = 4; m.bossPhase = 0; m.bossTimer = 0;
            m.bossFlyT = 0; m.bossLift = 0; m.bossFlying = true; m.bossDiveHit = false;
        } else if (roll < 0.55 && m.bossCdStomp <= 0) {
            m.bossMove = 6; m.bossPhase = 0; m.bossTimer = 0; m.bossHitDone = false;
        } else if (roll < 0.65 && m.bossCdSummon <= 0) {
            m.bossMove = 7; m.bossPhase = 0; m.bossTimer = 0;
        } else if (roll < 0.75 && m.bossCdCharge <= 0) {
            m.bossMove = 8; m.bossPhase = 0; m.bossTimer = 0;
            m.bossHitDone = false; m.bossWpx = p.x; m.bossWpy = p.y;
        } else if (d < 2.0 && m.atkCd <= 0) {
            m.bossMove = 2; m.bossPhase = 0; m.bossTimer = 0; m.bossHitDone = false;
        }
    }

    m.bossTimer += dt;
    double chargeMult = 1.0;
    double stompRadius = 2.2;
    if (m.type == 'X') { chargeMult = 1.1; stompRadius = 3.2; }
    if (m.type == 'K') chargeMult = 1.2;

    switch (m.bossMove) {
        case 1: {
            if (m.bossPhase == 0) {
                m.bossWindT += dt;
                if (m.bossWindT >= 0.4) {
                    m.bossPhase = 1; m.bossTimer = 0;
                }
            } else if (m.bossPhase == 1) {
                if (!m.bossHitDone && d < 1.5) {
                    dealMeleeDamageToPlayer(g, m, 1.2 * chargeMult, rng);
                    m.bossHitDone = true;
                }
                if (m.bossTimer > 0.3) { m.bossMove = 0; m.atkCd = 3.5; m.bossWindT = 0; }
            }
        } break;
        case 2: {
            if (m.bossPhase == 0) {
                m.bossWindT += dt;
                if (m.bossWindT >= 0.8) { m.bossPhase = 1; m.bossTimer = 0; }
            } else if (m.bossPhase == 1) {
                if (m.bossTimer > 0.25 && !m.bossHitDone) {
                    if (d < 1.7 && vx * p.fx + vy * p.fy > 0) {
                        dealMeleeDamageToPlayer(g, m, 1.0, rng);
                    }
                    m.bossHitDone = true;
                }
                if (m.bossTimer > 0.7) { m.bossMove = 0; m.bossCdFb = 4.5; m.bossWindT = 0; }
            }
        } break;
        case 3: {
            if (m.bossPhase == 0) {
                Bolt fb;
                fb.x = m.x; fb.y = m.y;
                fb.vx = vx * 4.0; fb.vy = vy * 4.0;
                fb.life = 2.5; fb.r = 0.8;
                fb.dmg = (int)(m.dmg * 1.3);
                fb.fromPlayer = false;
                g.bolts.push_back(fb);
                m.bossPhase = 1; m.bossTimer = 0;
            } else {
                if (m.bossTimer > 1.5) { m.bossMove = 0; m.bossCdFb = 5.0; }
            }
        } break;
        case 4: {
            if (m.bossPhase == 0) {
                m.bossFlyT += dt;
                m.bossLift = std::min(1.5, m.bossLift + dt * 2.5);
                if (m.bossFlyT >= 0.6) { m.bossPhase = 1; m.bossTimer = 0; }
            } else if (m.bossPhase == 1) {
                m.bossWpx = p.x; m.bossWpy = p.y;
                if (m.bossTimer >= 0.3) { m.bossPhase = 2; m.bossTimer = 0; }
            } else if (m.bossPhase == 2) {
                const double ddx = m.bossWpx - m.x, ddy = m.bossWpy - m.y;
                const double dd = std::sqrt(ddx * ddx + ddy * ddy);
                if (dd > 0.1) {
                    tryMoveEntity(l, m.x, m.y, ddx / dd, ddy / dd, dt, 10.0, 0.45);
                }
                if (!m.bossDiveHit && d < 1.5) {
                    dealMeleeDamageToPlayer(g, m, 1.3, rng);
                    m.bossDiveHit = true;
                }
                if (dd < 0.4 || m.bossTimer > 1.0) {
                    m.bossMove = 0; m.bossCdFly = 6.0;
                    m.bossFlying = false; m.bossLift = 0;
                }
            }
        } break;
        case 6: {
            if (m.bossPhase == 0) {
                m.bossWindT += dt;
                if (m.bossWindT >= 0.5) {
                    m.bossPhase = 1; m.bossTimer = 0;
                    addShake(g, 0.5);
                }
            } else if (m.bossPhase == 1) {
                if (!m.bossHitDone && d < stompRadius) {
                    dealMeleeDamageToPlayer(g, m, 1.5, rng);
                    m.bossHitDone = true;
                }
                if (m.bossTimer > 0.3) { m.bossMove = 0; m.bossCdStomp = 4.0; m.bossWindT = 0; }
            }
        } break;
        case 7: {
            if (m.bossPhase == 0 && m.bossTimer > 0.5) {
                int count = 1 + (rng.next() < 0.4 ? 1 : 0);
                for (int k = 0; k < count; k++) {
                    const double ang = rng.next() * 6.283;
                    Monster nm = makeMonsterAt('g', m.x + std::cos(ang) * 1.5,
                        m.y + std::sin(ang) * 1.5, g.depth, 0);
                    nm.state = 'c';
                    g.monsters.push_back(nm);
                }
                m.bossPhase = 1; m.bossTimer = 0;
            } else if (m.bossPhase == 1) {
                if (m.bossTimer > 0.5) { m.bossMove = 0; m.bossCdSummon = 12.0; }
            }
        } break;
        case 8: {
            if (m.bossPhase == 0) {
                m.bossWindT += dt;
                if (m.bossWindT >= 0.8) {
                    m.bossPhase = 1; m.bossTimer = 0;
                }
            } else if (m.bossPhase == 1) {
                const double cdx = m.bossWpx - m.x, cdy = m.bossWpy - m.y;
                const double cd = std::sqrt(cdx * cdx + cdy * cdy);
                if (cd > 0.1) {
                    tryMoveEntity(l, m.x, m.y, cdx / cd, cdy / cd, dt, 12.0, 0.45);
                }
                if (!m.bossHitDone && d < 1.5) {
                    dealMeleeDamageToPlayer(g, m, 1.8 * chargeMult, rng);
                    m.bossHitDone = true;
                }
                if (cd < 0.4 || m.bossTimer > 1.0) {
                    m.bossMove = 0; m.bossCdCharge = 4.0; m.bossWindT = 0;
                }
            }
        } break;
        default: break;
    }
    if (m.bossMove == 0) m.bossWindT = 0;
}

static void updateMonster(Monster& m, GameState& g, Rng& rng, double dt)
{
    const MonsterType& t = *getMonsterType(m.type);
    const Layout& l = *g.layout;
    Player& p = g.player;

    m.staggerT = std::max(0.0, m.staggerT - dt);
    m.atkAnimT = std::max(0.0, m.atkAnimT - dt);
    m.dentT = std::max(0.0, m.dentT - dt);

    if (t.boss) { bossAI(m, g, rng, dt); return; }

    if (m.affix == 'r' && m.hp < m.maxHp)
        m.hp = std::min(m.maxHp, m.hp + dt * 0.045);

    m.atkCd = std::max(0.0, m.atkCd - dt);

    const double dx = p.x - m.x, dy = p.y - m.y;
    const double d2 = dx * dx + dy * dy;
    const double d = std::sqrt(d2 + 0.0001);
    const double vx = dx / d, vy = dy / d;

    if (m.dashing) {
        m.dashT += dt;
        const double progress = m.dashT / m.dashDur;
        const double mx = m.dashTX - m.x, my = m.dashTY - m.y;
        const double md = std::sqrt(mx * mx + my * my);
        if (md > 0.05) {
            tryMoveEntity(l, m.x, m.y, mx / md, my / md, dt, m.dashSpeed, 0.27);
            m.fx = mx / md; m.fy = my / md;
        }
        if (!m.dashHit && d < 0.9) {
            dealMeleeDamageToPlayer(g, m, 1.3, rng);
            m.dashHit = true;
        }
        if (progress >= 1.0 || md < 0.15) {
            m.dashing = false;
            m.staggerT = 2.0;
        }
        return;
    }

    if (m.staggerT > 0) { m.state = 'i'; return; }

    if (m.winding) {
        m.windT += dt;
        if (m.windFx == 'd') {
            if (m.windT >= 0.35) {
                m.winding = false;
                m.dashing = true;
                m.dashT = 0;
                m.dashDur = 0.4;
                m.dashSpeed = 10.0;
                m.dashTX = p.x; m.dashTY = p.y;
                m.dashHit = false;
                m.atkCd = 4.5;
            }
        } else if (m.windFx == 's') {
            if (m.windT >= 0.3) {
                m.winding = false;
                m.dashing = true;
                m.dashT = 0;
                m.dashDur = 0.55;
                m.dashSpeed = 9.0;
                const double predict = 1.0;
                m.dashTX = p.x + p.fx * predict * 0.3;
                m.dashTY = p.y + p.fy * predict * 0.3;
                m.dashHit = false;
                m.atkCd = 5.0;
            }
        } else if (m.windFx == 'c') {
            if (m.windT >= 0.5) {
                m.winding = false;
                if (d < 1.7) {
                    if (vx * m.fx + vy * m.fy > std::cos(1.5708 / 2.0)) {
                        dealMeleeDamageToPlayer(g, m, 1.4, rng);
                    }
                }
                m.staggerT = 2.0;
                m.atkCd = 5.0;
            }
        } else if (m.windFx == 'S') {
            if (m.windT >= 0.4) {
                m.winding = false;
                addShake(g, 0.4);
                if (d < 2.2) {
                    dealMeleeDamageToPlayer(g, m, 1.8, rng);
                }
                m.staggerT = 2.5;
                m.atkCd = 4.0;
            }
        }
        return;
    }

    if (m.dentT > 0) {
        return;
    }

    if (d2 < m.aggro * m.aggro) {
        m.state = 'c';
        m.fx = vx; m.fy = vy;

        const double reach = t.range > 0 ? t.range : 0.85;
        if (d > reach) {
            double mx = vx, my = vy;
            if (t.erratic && rng.next() < 0.03) {
                mx += (rng.next() - 0.5) * 1.4;
                my += (rng.next() - 0.5) * 1.4;
            }
            tryMoveEntity(l, m.x, m.y, mx, my, dt, m.speed, 0.27);
        } else if (m.atkCd <= 0) {
            double specialRoll = rng.next();
            if (t.dash && specialRoll < 0.25) {
                m.winding = true; m.windT = 0; m.windFx = 'd';
            } else if (t.swoop && specialRoll < 0.35) {
                m.winding = true; m.windT = 0; m.windFx = 's';
            } else if (t.cone && specialRoll < 0.45) {
                m.winding = true; m.windT = 0; m.windFx = 'c';
            } else if (t.stomp && specialRoll < 0.55) {
                m.winding = true; m.windT = 0; m.windFx = 'S';
            } else if (t.dbl) {
                m.atkCd = 1.0;
                m.atkAnimT = 0.3;
                if (!p.dead && p.invulnT <= 0) {
                    dealMeleeDamageToPlayer(g, m, 0.8, rng);
                    m.dentT = 0.2;
                    dealMeleeDamageToPlayer(g, m, 0.8, rng);
                }
            } else if (t.ranged) {
                const double sp = 3.2;
                Bolt b;
                b.x = m.x; b.y = m.y;
                b.vx = vx * sp; b.vy = vy * sp;
                b.life = std::min(1.9, d / sp + 0.35);
                b.r = 0.45;
                b.dmg = m.dmg;
                b.fromPlayer = false;
                g.bolts.push_back(b);
                m.atkCd = 2.1 + rng.next() * 0.6;
            } else {
                m.atkCd = 1.05 + rng.next() * 0.3;
                dealMeleeDamageToPlayer(g, m, 1.0, rng);
            }
        }
    } else {
        m.state = 'w';
        m.wanderT -= dt;
        if (m.wanderT <= 0) {
            m.wanderT = 2 + rng.next() * 3;
            const double ang = rng.next() * 3.14159265358979 * 2;
            m.wx = m.x + std::cos(ang) * 3;
            m.wy = m.y + std::sin(ang) * 3;
        }
        const double wx = m.wx - m.x, wy = m.wy - m.y;
        const double dd = std::sqrt(wx * wx + wy * wy);
        if (dd > 0.35) {
            tryMoveEntity(l, m.x, m.y, wx / dd, wy / dd, dt * 0.55, m.speed, 0.27);
            m.fx = wx / dd; m.fy = wy / dd;
        }
    }
}

static void updateBolts(GameState& g, Rng& rng, double dt)
{
    Player& p = g.player;
    std::vector<Bolt> alive;
    alive.reserve(g.bolts.size());
    for (Bolt& b : g.bolts) {
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.life -= dt;
        if (b.life <= 0) continue;
        bool used = false;
        if (b.fromPlayer) {
            for (Monster& m : g.monsters) {
                const double dx = m.x - b.x, dy = m.y - b.y;
                if (dx * dx + dy * dy <= b.r * b.r) {
                    m.hp -= b.dmg;
                    spawnFloatText(g, m.x, m.y - 0.5, ("-" + std::to_string(b.dmg)).c_str(), 3);
                    if (m.hp <= 0) {
                        const MonsterType& t = *getMonsterType(m.type);
                        p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
                        spawnMonsterDrops(g, rng, m);
                    }
                    used = true;
                    break;
                }
            }
        } else if (!p.dead && p.invulnT <= 0) {
            const double dx = p.x - b.x, dy = p.y - b.y;
            if (dx * dx + dy * dy <= b.r * b.r) {
                int amount = b.dmg;
                if (p.buffShield > 0) amount = amount / 2;
                const EquipStat eq = computeEquipBonus(p);
                amount = (int)(amount * std::max(0.2, 1.0 - eq.armorPct / 100.0));
                amount = std::max(1, amount);
                p.hp -= amount;
                p.invulnT = 0.45;
                addDamageFlash(g);
                addShake(g, 0.22);
                spawnFloatText(g, p.x, p.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
                if (p.hp <= 0) handleDeath(p);
                used = true;
            }
        }
        if (!used) alive.push_back(b);
    }
    g.bolts.swap(alive);
}

void updateCombat(GameState& g, Rng& rng, double dt)
{
    Player& p = g.player;

    /* global time for animations */
    g.gameTime += dt;
    g.torchPhase += dt * 3.0;

    /* hitstop */
    if (g.hitstopT > 0) { g.hitstopT -= dt; return; }

    /* shake decay */
    if (g.shakeT > 0) g.shakeT -= dt;
    if (g.shakeT <= 0) g.shakeIntensity = 0;

    /* damage flash decay */
    if (g.damageFlashT > 0) g.damageFlashT -= dt;

    /* boss death flash decay */
    if (g.bossDeathFlashT > 0) g.bossDeathFlashT -= dt;

    /* depth fade */
    if (g.depthFadeT > 0) g.depthFadeT -= dt;

    /* floating texts */
    for (int i = (int)g.floatTexts.size() - 1; i >= 0; i--) {
        g.floatTexts[i].life -= dt;
        g.floatTexts[i].y += g.floatTexts[i].vy * dt;
        if (g.floatTexts[i].life <= 0) g.floatTexts.erase(g.floatTexts.begin() + i);
    }

    /* invulnerability */
    if (p.invulnT > 0) p.invulnT -= dt;

    /* poison */
    if (p.poisonT > 0) {
        p.poisonT -= dt;
        p.hp -= (int)(dt * 1.0 + 0.5);
        if (p.hp <= 0) handleDeath(p);
    }

    /* respawn */
    if (p.dead) {
        p.respawnT -= dt;
        if (p.respawnT <= 0) handleRespawn(g);
        return;
    }

    p.atkT = std::max(0.0, p.atkT - dt);
    p.abilityT = std::max(0.0, p.abilityT - dt);
    if (p.maxMp > 0 && p.mp < p.maxMp)
        p.mp = std::min(p.maxMp, p.mp + (int)(dt * 0.8));

    tickBuffs(p, dt);

    /* boss fight trigger */
    if (!g.bossFight && g.layout && g.layout->hasBossRoom) {
        const BossRoom& br = g.layout->bossRoom;
        if (p.x >= br.x && p.x <= br.x + br.w &&
            p.y >= br.y && p.y <= br.y + br.h) {
            g.bossFight = true;
            g.bossActive = true;
            applyBossGates(*const_cast<Layout*>(g.layout), true);
            addShake(g, 0.25);
        }
    }

    for (Monster& m : g.monsters)
        updateMonster(m, g, rng, dt);

    updateBolts(g, rng, dt);

    /* rimozione mostri morti */
    std::vector<Monster> alive;
    alive.reserve(g.monsters.size());
    for (const Monster& m : g.monsters) {
        const MonsterType& t = *getMonsterType(m.type);
        if (m.hp > 0) {
            alive.push_back(m);
        } else {
            /* esplosivo */
            if (m.affix == 'e') {
                const double dx = p.x - m.x, dy = p.y - m.y;
                if (dx * dx + dy * dy < 1.8 * 1.8 && !p.dead && p.invulnT <= 0) {
                    int amount = 8 + (int)(g.depth * 1.5);
                    if (p.buffShield > 0) amount /= 2;
                    p.hp -= amount;
                    p.invulnT = 0.45;
                    addDamageFlash(g);
                    addShake(g, 0.3);
                    spawnFloatText(g, p.x, p.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
                    if (p.hp <= 0) handleDeath(p);
                }
                addShake(g, 0.4);
            }
            /* gelatina split */
            if (t.split && m.splitLeft > 0) {
                for (int k = 0; k < 2; k++) {
                    Monster nm = makeMonsterAt('j', m.x + (k ? 0.3 : -0.3), m.y, g.depth, 0);
                    nm.hp = std::lround(nm.hp * 0.55);
                    nm.maxHp = nm.hp;
                    nm.dmg = (int)(nm.dmg * 0.7);
                    nm.splitLeft = 0;
                    g.monsters.push_back(nm);
                }
            }
            if (t.boss) {
                g.bossFight = false;
                g.bossActive = false;
                g.bossDead = true;
                g.bossDeathFlashT = 0.35;
                g.hitstopT = 0.22;
                applyBossGates(*const_cast<Layout*>(g.layout), false);
                addShake(g, 1.2);
                spawnBurst(g.particles, m.x, m.y, 1.0, 0.8, 0.2, 12);
                spawnBurst(g.particles, m.x - 0.4, m.y, 1.0, 0.6, 0.2, 12);
                spawnBurst(g.particles, m.x + 0.4, m.y, 1.0, 0.8, 0.2, 12);
                for (int k = 0; k < 8; k++) {
                    Particle gp;
                    gp.x = m.x; gp.y = m.y;
                    double ang = k * 0.785;
                    gp.vx = std::cos(ang) * 4.0; gp.vy = std::sin(ang) * 4.0 - 2.0;
                    gp.r = 1.0; gp.g = 0.84; gp.b = 0.0;
                    gp.size = 0.08; gp.maxLife = 0.8; gp.life = 0.8; gp.type = 0;
                    g.particles.push_back(gp);
                }
            }
            spawnBurst(g.particles, m.x, m.y, 1.0, 0.6, 0.2, 5);
            spawnMonsterDrops(g, rng, m);
        }
    }
    g.monsters.swap(alive);

    updateParticles(g.particles, dt);
}

/* ---------------------------------------------------------------------
    Abilità di classe (index.html: ability definitions)
    Ogni classe ha un'abilità speciale attivata con F/L/Y
    --------------------------------------------------------------------- */
void useAbility(GameState& g, Rng& rng)
{
    Player& p = g.player;
    if (p.dead || p.abilityT > 0) return;
    const ClassInfo& c = *p.cls;
    const double ABIL_CD = 4.0;

    const char* key = c.key;

    if (std::strcmp(key, "guerriero") == 0) {
        /* Carica: dash forward hitting all enemies, 1.3x dmg, brief invuln */
        const double dashDist = 2.5;
        const double nx = p.x + p.fx * dashDist;
        const double ny = p.y + p.fy * dashDist;
        if (canOccupy(*g.layout, nx, ny, 0.35)) {
            p.x = nx; p.y = ny;
        }
        p.invulnT = 0.6;
        for (Monster& m : g.monsters) {
            const double dx = m.x - p.x, dy = m.y - p.y;
            const double d = std::sqrt(dx * dx + dy * dy);
            if (d < 1.8) {
                int amount = c.dmgMin + (int)std::floor(rng.next() * (c.dmgMax - c.dmgMin + 1));
                amount = (int)(amount * 1.3);
                if (p.buffRage > 0) amount = (int)(amount * 1.4);
                m.hp -= amount;
                spawnFloatText(g, m.x, m.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
                if (m.hp <= 0) {
                    const MonsterType& t = *getMonsterType(m.type);
                    p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
                    spawnMonsterDrops(g, rng, m);
                }
            }
        }
        addShake(g, 0.3);
        sfxAbility();
    }
    else if (std::strcmp(key, "ladro") == 0) {
        /* Passo Furtivo: teleport to nearest enemy, guaranteed crit 1.9x, 1.4s invis */
        const Monster* nearest = nullptr;
        double bestD = 1e9;
        for (const Monster& m : g.monsters) {
            const double dx = m.x - p.x, dy = m.y - p.y;
            const double d = dx * dx + dy * dy;
            if (d < bestD) { bestD = d; nearest = &m; }
        }
        if (nearest && bestD < 15.0 * 15.0) {
            p.x = nearest->x - p.fx * 0.8;
            p.y = nearest->y - p.fy * 0.8;
            if (!canOccupy(*g.layout, p.x, p.y, 0.35)) {
                p.x = nearest->x + p.fx * 0.8;
                p.y = nearest->y + p.fy * 0.8;
            }
            p.invulnT = 1.4;
            for (Monster& m : g.monsters) {
                const double dx = m.x - p.x, dy = m.y - p.y;
                const double d = std::sqrt(dx * dx + dy * dy);
                if (d < 1.5) {
                    int amount = c.dmgMin + (int)std::floor(rng.next() * (c.dmgMax - c.dmgMin + 1));
                    amount = (int)(amount * 1.9);
                    if (p.buffRage > 0) amount = (int)(amount * 1.4);
                    m.hp -= amount;
                    spawnFloatText(g, m.x, m.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
                    if (m.hp <= 0) {
                        const MonsterType& t = *getMonsterType(m.type);
                        p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
                        spawnMonsterDrops(g, rng, m);
                    }
                }
            }
        }
        addShake(g, 0.2);
        sfxAbility();
    }
    else if (std::strcmp(key, "mago") == 0) {
        /* Onda d'Urto: AoE explosion 2.3 radius, 1.2x dmg, costs 6 mana */
        if (p.mp < 6) return;
        p.mp -= 6;
        for (Monster& m : g.monsters) {
            const double dx = m.x - p.x, dy = m.y - p.y;
            const double d = std::sqrt(dx * dx + dy * dy);
            if (d < 2.3) {
                int amount = c.dmgMin + (int)std::floor(rng.next() * (c.dmgMax - c.dmgMin + 1));
                amount = (int)(amount * 1.2);
                if (p.buffRage > 0) amount = (int)(amount * 1.4);
                m.hp -= amount;
                spawnFloatText(g, m.x, m.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
                if (m.hp <= 0) {
                    const MonsterType& t = *getMonsterType(m.type);
                    p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
                    spawnMonsterDrops(g, rng, m);
                }
            }
        }
        addShake(g, 0.35);
        sfxAbility();
    }
    else if (std::strcmp(key, "ranger") == 0) {
        /* Raffica: 5 arrows in fan spread ±0.42 rad, 0.7x dmg each */
        for (int i = 0; i < 5; i++) {
            const double spread = (i - 2) * 0.42 / 2.0;
            const double angle = std::atan2(p.fy, p.fx) + spread;
            Bolt b;
            b.x = p.x; b.y = p.y;
            b.vx = std::cos(angle) * c.projSpeed;
            b.vy = std::sin(angle) * c.projSpeed;
            b.life = 1.2;
            b.r = 0.35;
            b.dmg = (int)(rollDamage(c, rng) * 0.7);
            if (p.buffRage > 0) b.dmg = (int)(b.dmg * 1.4);
            b.fromPlayer = true;
            g.bolts.push_back(b);
        }
        sfxAbility();
    }
    else if (std::strcmp(key, "paladino") == 0) {
        /* Muro Sacro: shield buff 5s */
        p.buffShield = 5.0;
        spawnFloatText(g, p.x, p.y - 0.7, "SCUDO", 4);
        sfxAbility();
    }
    else if (std::strcmp(key, "negromante") == 0) {
        /* Drenaggio d'Anima: AoE 2.4 radius, heals 50% dmg dealt, costs 6 mana */
        if (p.mp < 6) return;
        p.mp -= 6;
        int totalDmg = 0;
        for (Monster& m : g.monsters) {
            const double dx = m.x - p.x, dy = m.y - p.y;
            const double d = std::sqrt(dx * dx + dy * dy);
            if (d < 2.4) {
                int amount = c.dmgMin + (int)std::floor(rng.next() * (c.dmgMax - c.dmgMin + 1));
                if (p.buffRage > 0) amount = (int)(amount * 1.4);
                m.hp -= amount;
                totalDmg += amount;
                spawnFloatText(g, m.x, m.y - 0.5, ("-" + std::to_string(amount)).c_str(), 3);
                if (m.hp <= 0) {
                    const MonsterType& t = *getMonsterType(m.type);
                    p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
                    spawnMonsterDrops(g, rng, m);
                }
            }
        }
        if (totalDmg > 0) {
            const int heal = totalDmg / 2;
            p.hp = std::min(p.maxHp, p.hp + heal);
            spawnFloatText(g, p.x, p.y - 0.7, ("+" + std::to_string(heal)).c_str(), 2);
        }
        addShake(g, 0.25);
        sfxAbility();
    }
    else if (std::strcmp(key, "bardo") == 0) {
        /* Canto d'Ispirazione: rage buff +40% dmg for 8s */
        p.buffRage = 8.0;
        spawnFloatText(g, p.x, p.y - 0.7, "FURIA", 3);
        sfxAbility();
    }
    else if (std::strcmp(key, "monaco") == 0) {
        /* Onda di Chi: piercing projectile 1.5x dmg, hits all on path */
        Bolt b;
        b.x = p.x; b.y = p.y;
        b.vx = p.fx * c.projSpeed;
        b.vy = p.fy * c.projSpeed;
        b.life = 1.8;
        b.r = 0.5;
        b.dmg = (int)(rollDamage(c, rng) * 1.5);
        if (p.buffRage > 0) b.dmg = (int)(b.dmg * 1.4);
        b.fromPlayer = true;
        g.bolts.push_back(b);
        sfxAbility();
    }
    else if (std::strcmp(key, "prof") == 0) {
        /* Colpo Caricato: 0.8s charge then 2x dmg plasma shot */
        p.abilityT = ABIL_CD;
        Bolt b;
        b.x = p.x; b.y = p.y;
        b.vx = p.fx * c.projSpeed * 1.3;
        b.vy = p.fy * c.projSpeed * 1.3;
        b.life = 1.4;
        b.r = 0.55;
        b.dmg = rollDamage(c, rng) * 2;
        if (p.buffRage > 0) b.dmg = (int)(b.dmg * 1.4);
        b.fromPlayer = true;
        g.bolts.push_back(b);
        addShake(g, 0.2);
        sfxAbility();
        return; /* early return since we set abilityT manually */
    }
    else {
        return; /* unknown class, no ability */
    }

    p.abilityT = ABIL_CD;
}

void spawnBurst(std::vector<Particle>& parts, double x, double y, double r, double g, double b, int count)
{
    for (int i = 0; i < count; i++) {
        Particle p;
        p.x = x; p.y = y;
        const double ang = (double)i / (double)count * 6.28318;
        const double spd = 1.5 + (i % 3) * 0.5;
        p.vx = std::cos(ang) * spd;
        p.vy = std::sin(ang) * spd - 1.0;
        p.r = r; p.g = g; p.b = b;
        p.size = 0.06 + (i % 3) * 0.02;
        p.maxLife = 0.3 + (i % 4) * 0.1;
        p.life = p.maxLife;
        p.type = 0;
        parts.push_back(p);
    }
}

void updateParticles(std::vector<Particle>& parts, double dt)
{
    for (int i = (int)parts.size() - 1; i >= 0; i--) {
        Particle& p = parts[i];
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vy += 3.0 * dt;
        p.life -= dt;
        if (p.life <= 0) parts.erase(parts.begin() + i);
    }
}

}  // namespace abisso
