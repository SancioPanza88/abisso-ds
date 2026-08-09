/*------------------------------------------------------------------------------
    Test di parità: riferimento JS estratto VERBATIM da index.html
    (righe 945-1496). Se index.html cambia, aggiornare questo file.
    Output: una riga per piano: "<depth> <hash-esadecimale>"
------------------------------------------------------------------------------*/

// --- globals come in index.html ---
const ROOM_CODE = 'main';
const world = { seed: 123456789 };

// index.html:945
function hashStr(s){
  let h = 0x811c9dc5;
  for (let i=0;i<s.length;i++){
    h ^= s.charCodeAt(i);
    h = Math.imul(h, 0x01000193);
  }
  return h >>> 0;
}
// index.html:953
function mulberry32(seed){
  let a = seed >>> 0;
  return function(){
    a |= 0; a = (a + 0x6D2B79F5) | 0;
    let t = Math.imul(a ^ (a >>> 15), 1 | a);
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
  };
}
// index.html:962
function clamp(v,a,b){ return v<a?a:(v>b?b:v); }

// index.html:1234-1243
const T_WALL=0, T_FLOOR=1, T_STAIRS=2;
const BOSS_TYPES = ['D','X','L','M','R','K'];
function bossTypeForDepth(depth){ return BOSS_TYPES[((Math.floor(depth/5)-1) % BOSS_TYPES.length + BOSS_TYPES.length) % BOSS_TYPES.length]; }
function isBossFloor(depth){ return depth >= 5 && depth % 5 === 0; }

// index.html:1245
function carveCorridor(grid, rng, a, b){
  let x=a.x, y=a.y;
  const horizFirst = rng()<0.5;
  const stepX = ()=>{ while(x!==b.x){ grid[y][x]=T_FLOOR; x += x<b.x?1:-1; } };
  const stepY = ()=>{ while(y!==b.y){ grid[y][x]=T_FLOOR; y += y<b.y?1:-1; } };
  if (horizFirst){ stepX(); stepY(); } else { stepY(); stepX(); }
  grid[b.y][b.x]=T_FLOOR;
}
// index.html:1253
function centerOf(r){ return {x:(r.x+((r.w/2)|0)), y:(r.y+((r.h/2)|0))}; }

// index.html:1255
function bfsDistances(grid, start, w, h){
  const dist = Array.from({length:h}, ()=> new Int16Array(w).fill(-1));
  if (grid[start.y][start.x]===T_WALL) return dist;
  const qx=[start.x], qy=[start.y];
  dist[start.y][start.x]=0;
  let head=0;
  while (head<qx.length){
    const cx=qx[head], cy=qy[head]; head++;
    const d = dist[cy][cx];
    const nbrs=[[cx+1,cy],[cx-1,cy],[cx,cy+1],[cx,cy-1]];
    for (const [nx,ny] of nbrs){
      if (nx<0||ny<0||nx>=w||ny>=h) continue;
      if (grid[ny][nx]===T_WALL) continue;
      if (dist[ny][nx]!==-1) continue;
      dist[ny][nx]=d+1; qx.push(nx); qy.push(ny);
    }
  }
  return dist;
}

// index.html:1275 (usando le global ROOM_CODE e world.seed)
function generateDepth(depth){
  const seed = hashStr(ROOM_CODE+'::'+world.seed+'::layout::'+depth);
  const rng = mulberry32(seed);
  const w = clamp(78 + depth*3, 78, 130);
  const h = clamp(44 + depth*2, 44, 74);
  const grid = Array.from({length:h}, ()=> new Uint8Array(w));
  const rooms = [];
  const maxRooms = 11 + Math.floor(depth*0.7);
  let attempts=0;
  while (rooms.length<maxRooms && attempts<500){
    attempts++;
    const rw = 4 + Math.floor(rng()*7);
    const rh = 3 + Math.floor(rng()*5);
    const rx = 1 + Math.floor(rng()*(w-rw-2));
    const ry = 1 + Math.floor(rng()*(h-rh-2));
    let ok=true;
    for (const r of rooms){
      if (rx-1 < r.x+r.w+1 && rx+rw+1 > r.x-1 && ry-1 < r.y+r.h+1 && ry+rh+1 > r.y-1){ ok=false; break; }
    }
    if (!ok) continue;
    const room={x:rx,y:ry,w:rw,h:rh};
    rooms.push(room);
    for (let y=ry;y<ry+rh;y++) for (let x=rx;x<rx+rw;x++) grid[y][x]=T_FLOOR;
  }
  if (rooms.length===0){
    const rw=8,rh=6,rx=(w>>1)-4,ry=(h>>1)-3;
    rooms.push({x:rx,y:ry,w:rw,h:rh});
    for (let y=ry;y<ry+rh;y++) for (let x=rx;x<rx+rw;x++) grid[y][x]=T_FLOOR;
  }
  for (let i=1;i<rooms.length;i++) carveCorridor(grid, rng, centerOf(rooms[i-1]), centerOf(rooms[i]));
  const extraLoops = Math.floor(rooms.length*0.3);
  for (let k=0;k<extraLoops;k++){
    const a = rooms[Math.floor(rng()*rooms.length)];
    const b = rooms[Math.floor(rng()*rooms.length)];
    if (a!==b) carveCorridor(grid, rng, centerOf(a), centerOf(b));
  }

  let bossRoom = null;
  if (isBossFloor(depth)){
    for (let tries=0; tries<100; tries++){
      const bw = 12+Math.floor(rng()*6), bh = 8+Math.floor(rng()*5);
      const bx = 3+Math.floor(rng()*(w-bw-6)), by = 3+Math.floor(rng()*(h-bh-6));
      let ok = true;
      for (const r of rooms){
        if (bx-2 < r.x+r.w+2 && bx+bw+2 > r.x-2 && by-2 < r.y+r.h+2 && by+bh+2 > r.y-2){ ok=false; break; }
      }
      if (!ok) continue;
      for (let y=by;y<by+bh && ok;y++) for (let x=bx;x<bx+bw;x++){ if (grid[y][x]!==T_WALL){ ok=false; break; } }
      if (!ok) continue;
      for (let y=by-1;y<=by+bh && ok;y++) for (let x=bx-1;x<=bx+bw;x++){
        const inside = x>=bx && x<bx+bw && y>=by && y<by+bh;
        if (x<0||y<0||x>=w||y>=h) continue;
        if (!inside && grid[y][x]!==T_WALL){ ok=false; break; }
      }
      if (!ok) continue;
      for (let y=by;y<by+bh;y++) for (let x=bx;x<bx+bw;x++) grid[y][x]=T_FLOOR;
      bossRoom = { x:bx, y:by, w:bw, h:bh, arena:true, gates:[], chest:null, bossType:bossTypeForDepth(depth),
        center:{ x:bx+(bw>>1), y:by+(bh>>1) } };
      let nearRoom = rooms[0], nearD = Infinity;
      for (const r of rooms){
        const c = centerOf(r);
        const d = Math.abs(c.x-bossRoom.center.x)+Math.abs(c.y-bossRoom.center.y);
        if (d<nearD){ nearD=d; nearRoom=r; }
      }
      carveCorridor(grid, rng, bossRoom.center, centerOf(nearRoom));
      for (let y=by-1;y<=by+bh;y++) for (let x=bx-1;x<=bx+bw;x++){
        const inside = x>=bx && x<bx+bw && y>=by && y<by+bh;
        if (inside || x<0||y<0||x>=w||y>=h) continue;
        if (grid[y][x]===T_FLOOR) bossRoom.gates.push({x,y});
      }
      const fx = bossRoom.x+1 + Math.floor(rng()*Math.max(1,bossRoom.w-2));
      let probe = { x:fx, y:bossRoom.y+1 };
      if (bossRoom.y+4 >= bossRoom.center.y) probe = { x:fx, y:bossRoom.y+bossRoom.h-2 };
      bossRoom.chest = { id:'cboss'+depth+'_'+probe.x+'_'+probe.y, x:probe.x, y:probe.y };
      break;
    }
  }

  const doors = new Set();
  for (let y=1;y<h-1;y++){
    for (let x=1;x<w-1;x++){
      if (grid[y][x]!==T_FLOOR) continue;
      const N=grid[y-1][x], S=grid[y+1][x], E=grid[y][x+1], W=grid[y][x-1];
      if ((N===T_WALL && S===T_WALL && E===T_FLOOR && W===T_FLOOR) ||
          (E===T_WALL && W===T_WALL && N===T_FLOOR && S===T_FLOOR)){
        doors.add(x+','+y);
      }
    }
  }

  const spawn = centerOf(rooms[0]);
  const entranceRoom = rooms[0];
  let merchantPos = { x: entranceRoom.x+Math.max(1,entranceRoom.w-2), y: entranceRoom.y+((entranceRoom.h/2)|0) };
  if (merchantPos.x===spawn.x && merchantPos.y===spawn.y) merchantPos = { x: entranceRoom.x+1, y: merchantPos.y };
  const distMap = bfsDistances(grid, spawn, w, h);
  let stairsRoom = rooms[rooms.length-1], bestDist=-1;
  for (const r of rooms){
    if (r.arena) continue;
    const c = centerOf(r);
    const d = distMap[c.y] ? distMap[c.y][c.x] : -1;
    if (d>bestDist){ bestDist=d; stairsRoom=r; }
  }
  const stairsPos = centerOf(stairsRoom);
  grid[stairsPos.y][stairsPos.x] = T_STAIRS;

  const chestSpots=[];
  if (bossRoom && bossRoom.chest) chestSpots.push(bossRoom.chest);
  const numChests = 3 + Math.floor(depth/2);
  let chestTries=0;
  while (chestSpots.length<numChests && chestTries<300){
    chestTries++;
    const room = rooms[1+Math.floor(rng()*(rooms.length-1))] || rooms[0];
    const x = room.x+1+Math.floor(rng()*Math.max(1,room.w-2));
    const y = room.y+1+Math.floor(rng()*Math.max(1,room.h-2));
    if (grid[y][x]!==T_FLOOR) continue;
    if (x===spawn.x && y===spawn.y) continue;
    if (x===merchantPos.x && y===merchantPos.y) continue;
    if (chestSpots.some(c=>c.x===x&&c.y===y)) continue;
    chestSpots.push({ id:'c'+depth+'_'+x+'_'+y, x, y });
  }

  const monsterSpawnSpots=[];
  for (let ri=1; ri<rooms.length; ri++){
    const room = rooms[ri];
    const count = 1 + Math.floor(rng()*3);
    for (let k=0;k<count;k++){
      const x = room.x+1+Math.floor(rng()*Math.max(1,room.w-2));
      const y = room.y+1+Math.floor(rng()*Math.max(1,room.h-2));
      if (grid[y][x]!==T_FLOOR) continue;
      monsterSpawnSpots.push({x,y, roomIdx:ri});
    }
  }

  const treasureSpots=[];
  const numTreasure = 4 + Math.floor(depth/2);
  let tTries=0;
  while (treasureSpots.length<numTreasure && tTries<300){
    tTries++;
    const room = rooms[Math.floor(rng()*rooms.length)];
    const x = room.x+1+Math.floor(rng()*Math.max(1,room.w-2));
    const y = room.y+1+Math.floor(rng()*Math.max(1,room.h-2));
    if (grid[y][x]!==T_FLOOR) continue;
    if (x===spawn.x && y===spawn.y) continue;
    if (x===merchantPos.x && y===merchantPos.y) continue;
    if (chestSpots.some(c=>c.x===x&&c.y===y)) continue;
    if (treasureSpots.some(c=>c.x===x&&c.y===y)) continue;
    treasureSpots.push({ x, y, gem: rng()<0.18 });
  }

  const powerupSpots=[];
  let pTries=0;
  while (powerupSpots.length<3 && pTries<200){
    pTries++;
    const room = rooms[1+Math.floor(rng()*(rooms.length-1))] || rooms[0];
    const x = room.x+1+Math.floor(rng()*Math.max(1,room.w-2));
    const y = room.y+1+Math.floor(rng()*Math.max(1,room.h-2));
    if (grid[y][x]!==T_FLOOR) continue;
    if (powerupSpots.some(c=>c.x===x&&c.y===y)) continue;
    powerupSpots.push({x,y});
  }

  const potionSpots=[];
  let poTries=0;
  const numPotionSpots = 3 + Math.floor(depth/3);
  while (potionSpots.length<numPotionSpots && poTries<250){
    poTries++;
    const room = rooms[Math.floor(rng()*rooms.length)];
    const x = room.x+1+Math.floor(rng()*Math.max(1,room.w-2));
    const y = room.y+1+Math.floor(rng()*Math.max(1,room.h-2));
    if (grid[y][x]!==T_FLOOR) continue;
    if (x===merchantPos.x && y===merchantPos.y) continue;
    if (potionSpots.some(c=>c.x===x&&c.y===y)) continue;
    if (chestSpots.some(c=>c.x===x&&c.y===y)) continue;
    if (treasureSpots.some(c=>c.x===x&&c.y===y)) continue;
    potionSpots.push({ x, y, mana: rng()<0.45 });
  }

  const torches = [];
  for (let ri=0; ri<rooms.length; ri++){
    const room = rooms[ri];
    const n = ri===0 ? 3 : 1+Math.floor(rng()*3);
    let placed = 0, tries = 0;
    const x0 = room.x-1, x1 = room.x+room.w, y0 = room.y-1, y1 = room.y+room.h;
    while (placed<n && tries<60){
      tries++;
      const edge = Math.floor(rng()*4);
      let x, y;
      if (edge===0){ x=x0; y=y0+1+Math.floor(rng()*Math.max(1,room.h-2)); }
      else if (edge===1){ x=x1; y=y0+1+Math.floor(rng()*Math.max(1,room.h-2)); }
      else if (edge===2){ y=y0; x=x0+1+Math.floor(rng()*Math.max(1,room.w-2)); }
      else { y=y1; x=x0+1+Math.floor(rng()*Math.max(1,room.w-2)); }
      if (x<1||y<1||x>=w-1||y>=h-1) continue;
      if (grid[y][x]!==T_WALL) continue;
      if (torches.some(t=>t.x===x&&t.y===y)) continue;
      torches.push({x,y}); placed++;
    }
  }

  return {depth, w, h, grid, doors, rooms, spawn, stairsPos, chestSpots, monsterSpawnSpots, treasureSpots, powerupSpots, potionSpots, merchantPos, torches, bossRoom};
}

/* --- serializzazione canonica (deve coincidere con serializeLayout C++) --- */
function putU8(d, v){ d.push(v & 0xFF); }
function putU32(d, v){
  v = v >>> 0;
  d.push(v & 0xFF); d.push((v>>>8) & 0xFF); d.push((v>>>16) & 0xFF); d.push((v>>>24) & 0xFF);
}
function putPt(d, p){ putU32(d, p.x); putU32(d, p.y); }

function serializeLayout(l){
  const d = [];
  putU32(d, l.w); putU32(d, l.h);
  putU32(d, l.w * l.h);
  for (let y=0;y<l.h;y++) for (let x=0;x<l.w;x++) putU8(d, l.grid[y][x]);
  putU32(d, l.rooms.length);
  for (const r of l.rooms){ putU32(d, r.x); putU32(d, r.y); putU32(d, r.w); putU32(d, r.h); }
  const doorsArr = [...l.doors].map(s => { const [x,y] = s.split(',').map(Number); return {x,y}; });
  putU32(d, doorsArr.length);
  for (const p of doorsArr) putPt(d, p);
  putPt(d, l.spawn); putPt(d, l.stairsPos); putPt(d, l.merchantPos);
  putU32(d, l.chestSpots.length);
  for (const c of l.chestSpots) putPt(d, c);
  putU32(d, l.monsterSpawnSpots.length);
  for (const p of l.monsterSpawnSpots) putPt(d, p);
  putU32(d, l.treasureSpots.length);
  for (const t of l.treasureSpots){ putPt(d, t); putU8(d, t.gem ? 1 : 0); }
  putU32(d, l.powerupSpots.length);
  for (const p of l.powerupSpots) putPt(d, p);
  putU32(d, l.potionSpots.length);
  for (const p of l.potionSpots){ putPt(d, p); putU8(d, p.mana ? 1 : 0); }
  putU32(d, l.torches.length);
  for (const p of l.torches) putPt(d, p);
  putU8(d, l.bossRoom ? 1 : 0);
  if (l.bossRoom){
    putU32(d, l.bossRoom.x); putU32(d, l.bossRoom.y); putU32(d, l.bossRoom.w); putU32(d, l.bossRoom.h);
    putU32(d, l.bossRoom.gates.length);
    for (const g of l.bossRoom.gates) putPt(d, g);
    putPt(d, l.bossRoom.chest);
    putU8(d, l.bossRoom.bossType.charCodeAt(0));
    putPt(d, l.bossRoom.center);
  }
  return d;
}

function fnv1a(bytes){
  let h = 0x811c9dc5;
  for (const b of bytes){ h ^= b; h = Math.imul(h, 0x01000193); }
  return h >>> 0;
}

const TEST_SEED = 123456789;
world.seed = TEST_SEED;

for (let depth = 1; depth <= 30; depth++){
  const layout = generateDepth(depth);
  const bytes = serializeLayout(layout);
  const hash = fnv1a(bytes);
  console.log(depth + ' ' + hash.toString(16).padStart(8, '0'));
}
