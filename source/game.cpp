#include "game.hpp"

#include <cmath>
#include <cstring>

namespace abisso {

int scaledStat(int base, int depth, double factor)
{
    // index.html:1160: Math.round(base*(1+factor*(depth-1)))
    return (int)std::lround(base * (1.0 + factor * (depth - 1)));
}

/* ---------------------------------------------------------------------
   Classi — ordine di inserimento del web (CLASSES + prof + nuovi eroi)
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
    p.atkT = 0;
    p.abilityT = 0;
    p.dead = false;
}

/* ---------------------------------------------------------------------
   Mostri — ordine del web (MONSTER_TYPES, boss in fondo)
   --------------------------------------------------------------------- */
static const MonsterType MONSTER_TABLE[] = {
    { 'r', "ratto",          7,  2,  2.6, 5.5, 0.85,  1,  3, false, false, false },
    { 'b', "pipistrello",    8,  2,  2.9, 6.5, 0.85,  1,  3, false, true,  false },
    { 'g', "goblin",        14,  3,  2.3, 6.0, 0.85,  2,  5, false, false, false },
    { 'j', "melma",          9,  2,  1.4, 4.5, 0.85,  1,  2, false, false, false },
    { 'J', "gelatina",      22,  3,  1.2, 4.5, 0.85,  2,  4, false, false, false },
    { 's', "scheletro",     20,  4,  1.9, 6.0, 0.85,  3,  6, false, false, false },
    { 'o', "orco",          32,  6,  2.0, 6.5, 0.85,  4,  9, false, false, false },
    { 'z', "zombie",        26,  5,  1.3, 5.0, 0.85,  3,  6, false, false, false },
    { 'S', "ragno gigante", 24,  5,  3.0, 7.5, 0.85,  4,  8, false, false, false },
    { 'W', "spettro",       28,  6,  2.7, 8.0, 0.85,  5, 10, false, false, false },
    { 'k', "serpente",      10,  2,  2.5, 5.5, 0.85,  1,  3, false, false, false },
    { 'h', "arpia",          9,  2,  2.9, 6.5, 0.85,  1,  3, false, true,  false },
    { 'C', "cavaliere",     26,  5,  1.8, 6.0, 0.85,  3,  6, false, false, false },
    { 'c', "cultista",      18,  4,  2.0, 6.5, 3.00,  3,  6, true,  false, false },
    { 'm', "mantide",       22,  4,  3.2, 7.5, 0.85,  4,  8, false, false, false },
    { 'q', "sciamano",      20,  4,  1.9, 7.0, 2.60,  3,  7, true,  false, false },
    { 'G', "golem",         40,  7,  1.4, 5.0, 0.85,  6, 12, false, false, false },
    { 'D', "Drago",        120, 11,  2.3, 11.0, 0.85, 40, 70, false, false, true },
    { 'X', "Golem di Pietra", 150, 12,  1.6, 12.0, 0.85, 220, 300, false, false, true },
    { 'L', "Lich",         135, 11,  2.1, 12.0, 0.85, 200, 280, false, false, true },
    { 'M', "Regina Melme", 130, 10,  1.5, 10.0, 0.85, 190, 260, false, false, true },
    { 'R', "Re Ragno",     140, 12,  2.6, 12.0, 0.85, 210, 290, false, false, true },
    { 'K', "Re dei ratti", 120, 11,  2.9, 12.0, 0.85, 200, 280, false, false, true },
};

const MonsterType* getMonsterType(char key)
{
    for (const MonsterType& t : MONSTER_TABLE)
        if (t.key == key) return &t;
    return &MONSTER_TABLE[0];
}

int monsterWeight(char key, int depth)
{
    // rispecchia le lambda weight del web (index.html:1113-1136)
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
    // index.html:1152 — stesso ordine di chiavi del web (non-boss)
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
   Spawn (index.html:1823 makeMonster + generazione iniziale)
   --------------------------------------------------------------------- */
static char bossTypeForDepth(int depth)
{
    switch (depth) {          // piani-boss: 5,10,15,20,25,30
        case 5:  return 'M';
        case 10: return 'D';
        case 15: return 'R';
        case 20: return 'X';
        case 25: return 'K';
        case 30: return 'L';
    }
    return 0;
}

static Monster makeMonsterAt(char type, double x, double y, int depth)
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
    return m;
}

void makeMonsters(GameState& g, Rng& rng)
{
    const Layout& l = *g.layout;
    g.monsters.clear();
    g.bolts.clear();
    for (const Pt& s : l.monsterSpawnSpots) {
        const char type = pickMonsterType(rng, g.depth);
        g.monsters.push_back(makeMonsterAt(type, s.x + 0.5, s.y + 0.5, g.depth));
    }
    const char bkey = bossTypeForDepth(g.depth);
    if (bkey && l.hasBossRoom) {
        g.monsters.push_back(makeMonsterAt(
            bkey,
            l.bossRoom.x + l.bossRoom.w / 2.0,
            l.bossRoom.y + l.bossRoom.h / 2.0,
            g.depth));
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
    // index.html:3330 performAttack
    Player& p = g.player;
    if (p.dead) return;
    if (p.atkT > 0) return;
    const ClassInfo& c = *p.cls;
    if (c.maxMp > 0 && c.manaCost && p.mp < c.manaCost) return;
    p.atkT = c.atkCooldown;
    if (c.manaCost) p.mp -= c.manaCost;

    // auto-mira verso il nemico più vicino (index.html:3342-3348)
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
        p.fx = dx / d;
        p.fy = dy / d;
    }

    if (!c.ranged) {
        // fendente ad arco frontale (index.html:3358-3372)
        for (Monster& m : g.monsters) {
            const double dx = m.x - p.x, dy = m.y - p.y;
            const double d = std::sqrt(dx * dx + dy * dy);
            if (d > c.range) continue;
            const double nx = dx / (d != 0 ? d : 1), ny = dy / (d != 0 ? d : 1);
            if (nx * p.fx + ny * p.fy < std::cos(c.arc)) continue;
            int amount = rollDamage(c, rng);
            if (c.crit > 0 && rng.next() < c.crit) amount *= 2;
            m.hp -= amount;
            if (m.hp <= 0) {
                const MonsterType& t = *getMonsterType(m.type);
                p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
            }
        }
    } else {
        // proiettile (index.html:3384-3397)
        Bolt b;
        b.x = p.x; b.y = p.y;
        b.vx = p.fx * c.projSpeed;
        b.vy = p.fy * c.projSpeed;
        b.life = 1.6;
        b.r = 0.4;
        b.dmg = rollDamage(c, rng);
        b.fromPlayer = true;
        g.bolts.push_back(b);
    }
}

static void updateMonster(Monster& m, GameState& g, Rng& rng, double dt)
{
    const MonsterType& t = *getMonsterType(m.type);
    const Layout& l = *g.layout;
    Player& p = g.player;

    // boss dormiente: resta confinato nella tana finché la lotta non scatta
    if (t.boss && l.hasBossRoom && !g.bossFight) {
        const BossRoom& br = l.bossRoom;
        m.x = std::min(std::max(m.x, br.x + 0.45), br.x + br.w - 0.45);
        m.y = std::min(std::max(m.y, br.y + 0.45), br.y + br.h - 0.45);
        m.state = 'i';
        m.atkCd = 0;
        return;
    }

    m.atkCd = std::max(0.0, m.atkCd - dt);

    const double dx = p.x - m.x, dy = p.y - m.y;
    const double d2 = dx * dx + dy * dy;

    if (d2 < m.aggro * m.aggro) {
        m.state = 'c';
        const double d = std::sqrt(d2);
        const double vx = dx / d, vy = dy / d;
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
            if (t.ranged) {
                // cultista/sciamano: magia a distanza (index.html:1943-1949)
                const double sp = 3.2;
                const double l2 = std::sqrt(d2);
                Bolt b;
                b.x = m.x; b.y = m.y;
                b.vx = vx * sp; b.vy = vy * sp;
                b.life = std::min(1.9, l2 / sp + 0.35);
                b.r = 0.45;
                b.dmg = m.dmg;
                b.fromPlayer = false;
                g.bolts.push_back(b);
                m.atkCd = 2.1 + rng.next() * 0.6;
            } else {
                // colpo ravvicinato (index.html:1961-1965)
                m.atkCd = 1.05 + rng.next() * 0.3;
                if (!p.dead) {
                    int amount = m.dmg + (int)std::floor(rng.next() * 2);
                    p.hp -= amount;
                    if (p.hp <= 0) {
                        p.hp = 0;
                        p.dead = true;
                        p.respawnT = 10.0;
                    }
                }
            }
        }
    } else {
        // vagabondaggio (index.html:1968-1979)
        m.state = 'w';
        m.wanderT -= dt;
        if (m.wanderT <= 0) {
            m.wanderT = 2 + rng.next() * 3;
            const double ang = rng.next() * 3.14159265358979 * 2;
            m.wx = m.x + std::cos(ang) * 3;
            m.wy = m.y + std::sin(ang) * 3;
        }
        const double wx = m.wx - m.x, wy = m.wy - m.y;
        const double d = std::sqrt(wx * wx + wy * wy);
        if (d > 0.35) {
            tryMoveEntity(l, m.x, m.y, wx / d, wy / d, dt * 0.55, m.speed, 0.27);
            m.fx = wx / d; m.fy = wy / d;
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
                    if (m.hp <= 0) {
                        const MonsterType& t = *getMonsterType(m.type);
                        p.gold += t.goldMin + (int)std::floor(rng.next() * (t.goldMax - t.goldMin + 1));
                    }
                    used = true;
                    break;
                }
            }
        } else if (!p.dead) {
            const double dx = p.x - b.x, dy = p.y - b.y;
            if (dx * dx + dy * dy <= b.r * b.r) {
                p.hp -= b.dmg;
                if (p.hp <= 0) {
                    p.hp = 0;
                    p.dead = true;
                    p.respawnT = 10.0;
                }
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
    if (p.dead) {
        p.respawnT -= dt;
        if (p.respawnT <= 0) {
            // respawn come nel web (index.html handleRespawn): scala + piene, oro a zero
            p.hp = p.maxHp;
            p.mp = p.maxMp;
            p.gold = 0;
            p.dead = false;
            if (g.layout) {
                p.x = g.layout->spawn.x + 0.5;
                p.y = g.layout->spawn.y + 0.5;
            }
        }
        return;
    }
    p.atkT = std::max(0.0, p.atkT - dt);
    p.abilityT = std::max(0.0, p.abilityT - dt);
    if (p.maxMp > 0 && p.mp < p.maxMp)
        p.mp = std::min(p.maxMp, p.mp + (int)(dt * 0.8));

    // il combattimento col boss scatta quando si entra nell'arena
    if (!g.bossFight && g.layout && g.layout->hasBossRoom) {
        const BossRoom& br = g.layout->bossRoom;
        if (p.x >= br.x && p.x <= br.x + br.w &&
            p.y >= br.y && p.y <= br.y + br.h) {
            g.bossFight = true;
        }
    }

    for (Monster& m : g.monsters)
        updateMonster(m, g, rng, dt);

    updateBolts(g, rng, dt);

    // rimozione mostri morti
    std::vector<Monster> alive;
    alive.reserve(g.monsters.size());
    for (const Monster& m : g.monsters) {
        const MonsterType& t = *getMonsterType(m.type);
        if (m.hp > 0) alive.push_back(m);
        else if (t.boss) g.bossFight = false;
    }
    g.monsters.swap(alive);
}

}  // namespace abisso


