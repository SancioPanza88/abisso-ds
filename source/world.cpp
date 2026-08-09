/*------------------------------------------------------------------------------
    ABISSO DS — port di generateDepth in C++ (parità col web).
    L'ordine di ogni singola chiamata a Rng::next() deve coincidere con
    l'ordine delle chiamate a rng() in index.html:1275.
------------------------------------------------------------------------------*/
#include "world.hpp"

#include <cmath>

namespace abisso {

static const char* const BOSS_TYPES = "DXLMRK";

uint32_t hashStr(const std::string& s)
{
    uint32_t h = 0x811c9dc5u;
    for (size_t i = 0; i < s.size(); i++) {
        h ^= static_cast<uint8_t>(s[i]);
        h *= 0x01000193u; // Math.imul(h, 0x01000193)
    }
    return h;
}

double Rng::next()
{
    // a |= 0; a = (a + 0x6D2B79F5) | 0;
    a_ += 0x6D2B79F5u;
    // let t = Math.imul(a ^ (a >>> 15), 1 | a);
    uint32_t t = (a_ ^ (a_ >> 15)) * (1 | a_);
    // t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;  (il t finale è quello vecchio)
    uint32_t oldT = t;
    t = (t + (t ^ (t >> 7)) * (61 | t)) ^ oldT;
    // return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
}

static int frand(Rng& r, double m)      // Math.floor(rng()*m), m>0
{
    return static_cast<int>(r.next() * m);
}

static bool brng(Rng& r, double p)      // rng() < p
{
    return r.next() < p;
}

static int clampV(int v, int a, int b)
{
    return v < a ? a : (v > b ? b : v);
}

static int max1(int v)
{
    return v < 1 ? 1 : v;
}

static char bossTypeForDepth(int depth)
{
    // BOSS_TYPES[((Math.floor(depth/5)-1) % 6 + 6) % 6]
    int idx = (depth / 5 - 1) % 6;
    if (idx < 0) idx += 6;
    return BOSS_TYPES[idx];
}

static bool isBossFloor(int depth)
{
    return depth >= 5 && depth % 5 == 0;
}

static void carveCorridor(Layout& L, Rng& rng, Pt a, Pt b)
{
    int x = a.x, y = a.y;
    bool horizFirst = brng(rng, 0.5);
    auto stepX = [&]() {
        while (x != b.x) {
            L.grid[static_cast<size_t>(y) * L.w + x] = T_FLOOR;
            x += x < b.x ? 1 : -1;
        }
    };
    auto stepY = [&]() {
        while (y != b.y) {
            L.grid[static_cast<size_t>(y) * L.w + x] = T_FLOOR;
            y += y < b.y ? 1 : -1;
        }
    };
    if (horizFirst) { stepX(); stepY(); } else { stepY(); stepX(); }
    L.grid[static_cast<size_t>(b.y) * L.w + b.x] = T_FLOOR;
}

static Pt centerOf(const Room& r)
{
    return { r.x + (r.w / 2), r.y + (r.h / 2) };
}

static std::vector<int> bfsDistances(const Layout& L, Pt start)
{
    std::vector<int> dist(static_cast<size_t>(L.w) * L.h, -1);
    if (L.grid[static_cast<size_t>(start.y) * L.w + start.x] == T_WALL)
        return dist;
    std::vector<int> qx, qy;
    qx.push_back(start.x);
    qy.push_back(start.y);
    dist[static_cast<size_t>(start.y) * L.w + start.x] = 0;
    size_t head = 0;
    while (head < qx.size()) {
        int cx = qx[head], cy = qy[head];
        head++;
        int d = dist[static_cast<size_t>(cy) * L.w + cx];
        static const int dxs[4] = { 1, -1, 0, 0 };
        static const int dys[4] = { 0, 0, 1, -1 };
        for (int k = 0; k < 4; k++) {
            int nx = cx + dxs[k], ny = cy + dys[k];
            if (nx < 0 || ny < 0 || nx >= L.w || ny >= L.h) continue;
            if (L.grid[static_cast<size_t>(ny) * L.w + nx] == T_WALL) continue;
            if (dist[static_cast<size_t>(ny) * L.w + nx] != -1) continue;
            dist[static_cast<size_t>(ny) * L.w + nx] = d + 1;
            qx.push_back(nx);
            qy.push_back(ny);
        }
    }
    return dist;
}

Layout generateDepth(int depth, const std::string& roomCode, uint32_t worldSeed)
{
    const std::string seedStr = roomCode + "::" + std::to_string(worldSeed) + "::layout::" + std::to_string(depth);
    Rng rng(hashStr(seedStr));

    Layout L;
    L.depth = depth;
    L.w = clampV(78 + depth * 3, 78, 130);
    L.h = clampV(44 + depth * 2, 44, 74);
    L.grid.assign(static_cast<size_t>(L.w) * L.h, T_WALL);

    std::vector<Room>& rooms = L.rooms;
    const int maxRooms = 11 + static_cast<int>(std::floor(depth * 0.7));
    int attempts = 0;
    while (static_cast<int>(rooms.size()) < maxRooms && attempts < 500) {
        attempts++;
        const int rw = 4 + frand(rng, 7);
        const int rh = 3 + frand(rng, 5);
        const int rx = 1 + frand(rng, L.w - rw - 2);
        const int ry = 1 + frand(rng, L.h - rh - 2);
        bool ok = true;
        for (const Room& r : rooms) {
            if (rx - 1 < r.x + r.w + 1 && rx + rw + 1 > r.x - 1 &&
                ry - 1 < r.y + r.h + 1 && ry + rh + 1 > r.y - 1) {
                ok = false;
                break;
            }
        }
        if (!ok) continue;
        Room room{ rx, ry, rw, rh };
        rooms.push_back(room);
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++)
                L.grid[static_cast<size_t>(y) * L.w + x] = T_FLOOR;
    }
    if (rooms.empty()) {
        const int rw = 8, rh = 6, rx = (L.w >> 1) - 4, ry = (L.h >> 1) - 3;
        rooms.push_back(Room{ rx, ry, rw, rh });
        for (int y = ry; y < ry + rh; y++)
            for (int x = rx; x < rx + rw; x++)
                L.grid[static_cast<size_t>(y) * L.w + x] = T_FLOOR;
    }

    for (size_t i = 1; i < rooms.size(); i++)
        carveCorridor(L, rng, centerOf(rooms[i - 1]), centerOf(rooms[i]));

    const int extraLoops = static_cast<int>(std::floor(rooms.size() * 0.3));
    for (int k = 0; k < extraLoops; k++) {
        const Room& a = rooms[frand(rng, static_cast<double>(rooms.size()))];
        const Room& b = rooms[frand(rng, static_cast<double>(rooms.size()))];
        if (&a != &b) carveCorridor(L, rng, centerOf(a), centerOf(b));
    }

    if (isBossFloor(depth)) {
        for (int tries = 0; tries < 100; tries++) {
            const int bw = 12 + frand(rng, 6);
            const int bh = 8 + frand(rng, 5);
            const int bx = 3 + frand(rng, L.w - bw - 6);
            const int by = 3 + frand(rng, L.h - bh - 6);
            bool ok = true;
            for (const Room& r : rooms) {
                if (bx - 2 < r.x + r.w + 2 && bx + bw + 2 > r.x - 2 &&
                    by - 2 < r.y + r.h + 2 && by + bh + 2 > r.y - 2) {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;
            for (int y = by; y < by + bh && ok; y++)
                for (int x = bx; x < bx + bw; x++)
                    if (L.grid[static_cast<size_t>(y) * L.w + x] != T_WALL) { ok = false; break; }
            if (!ok) continue;
            for (int y = by - 1; y <= by + bh && ok; y++)
                for (int x = bx - 1; x <= bx + bw; x++) {
                    const bool inside = x >= bx && x < bx + bw && y >= by && y < by + bh;
                    if (x < 0 || y < 0 || x >= L.w || y >= L.h) continue;
                    if (!inside && L.grid[static_cast<size_t>(y) * L.w + x] != T_WALL) { ok = false; break; }
                }
            if (!ok) continue;
            for (int y = by; y < by + bh; y++)
                for (int x = bx; x < bx + bw; x++)
                    L.grid[static_cast<size_t>(y) * L.w + x] = T_FLOOR;
            L.hasBossRoom = true;
            L.bossRoom.x = bx;
            L.bossRoom.y = by;
            L.bossRoom.w = bw;
            L.bossRoom.h = bh;
            L.bossRoom.bossType = bossTypeForDepth(depth);
            L.bossRoom.center = { bx + (bw >> 1), by + (bh >> 1) };
            Room* nearRoom = &rooms[0];
            int nearD = INT32_MAX;
            for (Room& r : rooms) {
                const Pt c = centerOf(r);
                const int d = std::abs(c.x - L.bossRoom.center.x) + std::abs(c.y - L.bossRoom.center.y);
                if (d < nearD) { nearD = d; nearRoom = &r; }
            }
            carveCorridor(L, rng, L.bossRoom.center, centerOf(*nearRoom));
            for (int y = by - 1; y <= by + bh; y++)
                for (int x = bx - 1; x <= bx + bw; x++) {
                    const bool inside = x >= bx && x < bx + bw && y >= by && y < by + bh;
                    if (inside || x < 0 || y < 0 || x >= L.w || y >= L.h) continue;
                    if (L.grid[static_cast<size_t>(y) * L.w + x] == T_FLOOR)
                        L.bossRoom.gates.push_back({ x, y });
                }
            const int fx = L.bossRoom.x + 1 + frand(rng, max1(L.bossRoom.w - 2));
            Pt probe{ fx, L.bossRoom.y + 1 };
            if (L.bossRoom.y + 4 >= L.bossRoom.center.y)
                probe = { fx, L.bossRoom.y + L.bossRoom.h - 2 };
            L.bossRoom.chest = {
                "cboss" + std::to_string(depth) + "_" + std::to_string(probe.x) + "_" + std::to_string(probe.y),
                probe.x, probe.y
            };
            break;
        }
    }

    // door heuristic
    for (int y = 1; y < L.h - 1; y++) {
        for (int x = 1; x < L.w - 1; x++) {
            if (L.grid[static_cast<size_t>(y) * L.w + x] != T_FLOOR) continue;
            const uint8_t N = L.grid[static_cast<size_t>(y - 1) * L.w + x];
            const uint8_t S = L.grid[static_cast<size_t>(y + 1) * L.w + x];
            const uint8_t E = L.grid[static_cast<size_t>(y) * L.w + x + 1];
            const uint8_t W = L.grid[static_cast<size_t>(y) * L.w + x - 1];
            if ((N == T_WALL && S == T_WALL && E == T_FLOOR && W == T_FLOOR) ||
                (E == T_WALL && W == T_WALL && N == T_FLOOR && S == T_FLOOR)) {
                L.doors.push_back({ x, y });
            }
        }
    }

    L.spawn = centerOf(rooms[0]);
    const Room& entranceRoom = rooms[0];
    // safeZone non servita in formato punti: la calcolerà il gioco
    L.merchantPos = { entranceRoom.x + max1(entranceRoom.w - 2),
                      entranceRoom.y + (entranceRoom.h / 2) };
    if (L.merchantPos.x == L.spawn.x && L.merchantPos.y == L.spawn.y)
        L.merchantPos = { entranceRoom.x + 1, L.merchantPos.y };

    const std::vector<int> distMap = bfsDistances(L, L.spawn);
    const Room* stairsRoom = &rooms[rooms.size() - 1];
    int bestDist = -1;
    for (const Room& r : rooms) {
        const Pt c = centerOf(r);
        const int d = distMap[static_cast<size_t>(c.y) * L.w + c.x];
        if (d > bestDist) { bestDist = d; stairsRoom = &r; }
    }
    L.stairsPos = centerOf(*stairsRoom);
    L.grid[static_cast<size_t>(L.stairsPos.y) * L.w + L.stairsPos.x] = T_STAIRS;

    if (L.hasBossRoom)
        L.chestSpots.push_back(L.bossRoom.chest);
    const int numChests = 3 + depth / 2;
    int chestTries = 0;
    while (static_cast<int>(L.chestSpots.size()) < numChests && chestTries < 300) {
        chestTries++;
        // JS: rooms[1+Math.floor(rng()*(rooms.length-1))] || rooms[0]
        //     (rng() viene consumata SEMPRE, anche se l'indice esce dai limiti)
        int ridx = 1 + frand(rng, static_cast<double>(rooms.size() - 1));
        const Room& room = ridx < static_cast<int>(rooms.size()) ? rooms[ridx] : rooms[0];
        const int x = room.x + 1 + frand(rng, max1(room.w - 2));
        const int y = room.y + 1 + frand(rng, max1(room.h - 2));
        if (L.grid[static_cast<size_t>(y) * L.w + x] != T_FLOOR) continue;
        if (x == L.spawn.x && y == L.spawn.y) continue;
        if (x == L.merchantPos.x && y == L.merchantPos.y) continue;
        bool dup = false;
        for (const ChestSpot& c : L.chestSpots)
            if (c.x == x && c.y == y) { dup = true; break; }
        if (dup) continue;
        L.chestSpots.push_back({ "c" + std::to_string(depth) + "_" + std::to_string(x) + "_" + std::to_string(y), x, y });
    }

    for (size_t ri = 1; ri < rooms.size(); ri++) {
        const Room& room = rooms[ri];
        const int count = 1 + frand(rng, 3);
        for (int k = 0; k < count; k++) {
            const int x = room.x + 1 + frand(rng, max1(room.w - 2));
            const int y = room.y + 1 + frand(rng, max1(room.h - 2));
            if (L.grid[static_cast<size_t>(y) * L.w + x] != T_FLOOR) continue;
            L.monsterSpawnSpots.push_back({ x, y });
        }
    }

    const int numTreasure = 4 + depth / 2;
    int tTries = 0;
    while (static_cast<int>(L.treasureSpots.size()) < numTreasure && tTries < 300) {
        tTries++;
        const Room& room = rooms[frand(rng, static_cast<double>(rooms.size()))];
        const int x = room.x + 1 + frand(rng, max1(room.w - 2));
        const int y = room.y + 1 + frand(rng, max1(room.h - 2));
        if (L.grid[static_cast<size_t>(y) * L.w + x] != T_FLOOR) continue;
        if (x == L.spawn.x && y == L.spawn.y) continue;
        if (x == L.merchantPos.x && y == L.merchantPos.y) continue;
        bool dup = false;
        for (const ChestSpot& c : L.chestSpots)
            if (c.x == x && c.y == y) { dup = true; break; }
        if (dup) continue;
        bool dup2 = false;
        for (const TreasureSpot& c : L.treasureSpots)
            if (c.x == x && c.y == y) { dup2 = true; break; }
        if (dup2) continue;
        L.treasureSpots.push_back({ x, y, brng(rng, 0.18) });
    }

    int pTries = 0;
    while (static_cast<int>(L.powerupSpots.size()) < 3 && pTries < 200) {
        pTries++;
        int ridx = 1 + frand(rng, static_cast<double>(rooms.size() - 1));
        const Room& room = ridx < static_cast<int>(rooms.size()) ? rooms[ridx] : rooms[0];
        const int x = room.x + 1 + frand(rng, max1(room.w - 2));
        const int y = room.y + 1 + frand(rng, max1(room.h - 2));
        if (L.grid[static_cast<size_t>(y) * L.w + x] != T_FLOOR) continue;
        bool dup = false;
        for (const Pt& c : L.powerupSpots)
            if (c.x == x && c.y == y) { dup = true; break; }
        if (dup) continue;
        L.powerupSpots.push_back({ x, y });
    }

    const int numPotionSpots = 3 + depth / 3;
    int poTries = 0;
    while (static_cast<int>(L.potionSpots.size()) < numPotionSpots && poTries < 250) {
        poTries++;
        const Room& room = rooms[frand(rng, static_cast<double>(rooms.size()))];
        const int x = room.x + 1 + frand(rng, max1(room.w - 2));
        const int y = room.y + 1 + frand(rng, max1(room.h - 2));
        if (L.grid[static_cast<size_t>(y) * L.w + x] != T_FLOOR) continue;
        if (x == L.merchantPos.x && y == L.merchantPos.y) continue;
        bool dup = false;
        for (const PotionSpot& c : L.potionSpots)
            if (c.x == x && c.y == y) { dup = true; break; }
        if (dup) continue;
        bool dup2 = false;
        for (const ChestSpot& c : L.chestSpots)
            if (c.x == x && c.y == y) { dup2 = true; break; }
        if (dup2) continue;
        bool dup3 = false;
        for (const TreasureSpot& c : L.treasureSpots)
            if (c.x == x && c.y == y) { dup3 = true; break; }
        if (dup3) continue;
        L.potionSpots.push_back({ x, y, brng(rng, 0.45) });
    }

    for (size_t ri = 0; ri < rooms.size(); ri++) {
        const Room& room = rooms[ri];
        const int n = ri == 0 ? 3 : 1 + frand(rng, 3);
        int placed = 0, tries = 0;
        const int x0 = room.x - 1, x1 = room.x + room.w, y0 = room.y - 1, y1 = room.y + room.h;
        while (placed < n && tries < 60) {
            tries++;
            const int edge = frand(rng, 4);
            int x, y;
            if (edge == 0)      { x = x0; y = y0 + 1 + frand(rng, max1(room.h - 2)); }
            else if (edge == 1) { x = x1; y = y0 + 1 + frand(rng, max1(room.h - 2)); }
            else if (edge == 2) { y = y0; x = x0 + 1 + frand(rng, max1(room.w - 2)); }
            else                { y = y1; x = x0 + 1 + frand(rng, max1(room.w - 2)); }
            if (x < 1 || y < 1 || x >= L.w - 1 || y >= L.h - 1) continue;
            if (L.grid[static_cast<size_t>(y) * L.w + x] != T_WALL) continue;
            bool dup = false;
            for (const Pt& t : L.torches)
                if (t.x == x && t.y == y) { dup = true; break; }
            if (dup) continue;
            L.torches.push_back({ x, y });
            placed++;
        }
    }

    return L;
}

/* --- movimento (index.html:1520-1563) --- */

bool isWalkableTile(const Layout& l, int tx, int ty)
{
    if (tx < 0 || ty < 0 || tx >= l.w || ty >= l.h) return false;
    return l.grid[static_cast<size_t>(ty) * l.w + tx] != T_WALL;
}

bool canOccupy(const Layout& l, double x, double y, double r)
{
    const double pts[4][2] = {
        { x - r, y - r }, { x + r, y - r }, { x - r, y + r }, { x + r, y + r }
    };
    for (int i = 0; i < 4; i++) {
        const int cx = static_cast<int>(std::floor(pts[i][0]));
        const int cy = static_cast<int>(std::floor(pts[i][1]));
        if (!isWalkableTile(l, cx, cy)) return false;
    }
    return true;
}

void tryMoveEntity(const Layout& l, double& x, double& y,
                   double dx, double dy, double dt,
                   double speed, double radius)
{
    const double step = speed * dt;
    const double nx = x + dx * step;
    const double ny = y + dy * step;
    const bool movedX = canOccupy(l, nx, y, radius);
    if (movedX) x = nx;
    const bool movedY = canOccupy(l, x, ny, radius);
    if (movedY) y = ny;

    // assistenza d'ingresso nei corridoi (come index.html:1549)
    if (dx != 0.0 && dy != 0.0) {
        if (!movedX) {
            const double targetY = std::floor(y) + 0.5;
            const double diff = targetY - y;
            const double nudged = y + (diff < 0 ? -1 : 1) * std::min(std::fabs(diff), step * 1.6);
            if (canOccupy(l, nx, nudged, radius)) { x = nx; y = nudged; }
        }
        if (!movedY) {
            const double targetX = std::floor(x) + 0.5;
            const double diff = targetX - x;
            const double nudged = x + (diff < 0 ? -1 : 1) * std::min(std::fabs(diff), step * 1.6);
            if (canOccupy(l, nudged, ny, radius)) { x = nudged; y = ny; }
        }
    }
}

/* --- campo visivo (index.html:3745-3785) --- */

static void castRay(const Layout& l, int x0, int y0, int x1, int y1,
                    std::vector<uint8_t>& vis)
{
    int dx = std::abs(x1 - x0), dy = -std::abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, x = x0, y = y0;
    while (true) {
        if (x >= 0 && y >= 0 && x < l.w && y < l.h) {
            vis[static_cast<size_t>(y) * l.w + x] = 1;
            if (l.grid[static_cast<size_t>(y) * l.w + x] == T_WALL) break;
        } else break;
        if (x == x1 && y == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x += sx; }
        if (e2 <= dx) { err += dx; y += sy; }
    }
}

void computeFov(const Layout& l, int px, int py,
                std::vector<uint8_t>& visible, std::vector<uint8_t>& visited)
{
    const size_t n = static_cast<size_t>(l.w) * l.h;
    if (visible.size() != n) visible.assign(n, 0);
    if (visited.size() != n) visited.assign(n, 0);
    std::fill(visible.begin(), visible.end(), 0);

    const int R = static_cast<int>(std::ceil(FOV_RADIUS));
    for (int yy = py - R; yy <= py + R; yy++) {
        for (int xx = px - R; xx <= px + R; xx++) {
            if (xx < 0 || yy < 0 || xx >= l.w || yy >= l.h) continue;
            const int ddx = xx - px, ddy = yy - py;
            if (ddx * ddx + ddy * ddy > FOV_RADIUS * FOV_RADIUS) continue;
            castRay(l, px, py, xx, yy, visible);
        }
    }
    for (size_t i = 0; i < n; i++)
        visited[i] |= visible[i];
}

/* --- serializzazione canonica (test di parità) --- */static void putU8(std::vector<uint8_t>& d, uint32_t v)
{
    d.push_back(static_cast<uint8_t>(v & 0xFF));
}

static void putU32(std::vector<uint8_t>& d, uint32_t v)
{
    d.push_back(static_cast<uint8_t>(v & 0xFF));
    d.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    d.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    d.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

static void putPt(std::vector<uint8_t>& d, const Pt& p)
{
    putU32(d, static_cast<uint32_t>(p.x));
    putU32(d, static_cast<uint32_t>(p.y));
}

std::vector<uint8_t> serializeLayout(const Layout& L)
{
    std::vector<uint8_t> d;
    putU32(d, static_cast<uint32_t>(L.w));
    putU32(d, static_cast<uint32_t>(L.h));
    putU32(d, static_cast<uint32_t>(L.grid.size()));
    for (uint8_t t : L.grid) putU8(d, t);
    putU32(d, static_cast<uint32_t>(L.rooms.size()));
    for (const Room& r : L.rooms) {
        putU32(d, static_cast<uint32_t>(r.x));
        putU32(d, static_cast<uint32_t>(r.y));
        putU32(d, static_cast<uint32_t>(r.w));
        putU32(d, static_cast<uint32_t>(r.h));
    }
    putU32(d, static_cast<uint32_t>(L.doors.size()));
    for (const Pt& p : L.doors) putPt(d, p);
    putPt(d, L.spawn);
    putPt(d, L.stairsPos);
    putPt(d, L.merchantPos);
    putU32(d, static_cast<uint32_t>(L.chestSpots.size()));
    for (const ChestSpot& c : L.chestSpots) putPt(d, { c.x, c.y });
    putU32(d, static_cast<uint32_t>(L.monsterSpawnSpots.size()));
    for (const Pt& p : L.monsterSpawnSpots) putPt(d, p);
    putU32(d, static_cast<uint32_t>(L.treasureSpots.size()));
    for (const TreasureSpot& t : L.treasureSpots) {
        putPt(d, { t.x, t.y });
        putU8(d, t.gem ? 1 : 0);
    }
    putU32(d, static_cast<uint32_t>(L.powerupSpots.size()));
    for (const Pt& p : L.powerupSpots) putPt(d, p);
    putU32(d, static_cast<uint32_t>(L.potionSpots.size()));
    for (const PotionSpot& p : L.potionSpots) {
        putPt(d, { p.x, p.y });
        putU8(d, p.mana ? 1 : 0);
    }
    putU32(d, static_cast<uint32_t>(L.torches.size()));
    for (const Pt& p : L.torches) putPt(d, p);
    putU8(d, L.hasBossRoom ? 1 : 0);
    if (L.hasBossRoom) {
        putU32(d, static_cast<uint32_t>(L.bossRoom.x));
        putU32(d, static_cast<uint32_t>(L.bossRoom.y));
        putU32(d, static_cast<uint32_t>(L.bossRoom.w));
        putU32(d, static_cast<uint32_t>(L.bossRoom.h));
        putU32(d, static_cast<uint32_t>(L.bossRoom.gates.size()));
        for (const Pt& g : L.bossRoom.gates) putPt(d, g);
        putPt(d, { L.bossRoom.chest.x, L.bossRoom.chest.y });
        putU8(d, static_cast<uint8_t>(L.bossRoom.bossType));
        putPt(d, L.bossRoom.center);
    }
    return d;
}

uint32_t fnv1a(const std::vector<uint8_t>& data)
{
    uint32_t h = 0x811c9dc5u;
    for (uint8_t b : data) {
        h ^= b;
        h *= 0x01000193u;
    }
    return h;
}

} // namespace abisso
