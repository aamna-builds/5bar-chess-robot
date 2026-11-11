// Node tests for the browser simulator core:  node simulation/tests/test_core.mjs
import { createRequire } from 'module';
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';

const require = createRequire(import.meta.url);
const here = path.dirname(fileURLToPath(import.meta.url));
const FB = require('../web/core.js');
const CFG = require('../web/config.js');

let pass = 0, fail = 0;
const check = (ok, msg) => { if (ok) pass++; else { fail++; console.log('FAIL', msg); } };

// 1. perft (standard reference counts)
const PERFT = [
  [FB.START, [20, 400, 8902]],
  ['r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1', [48, 2039, 97862]],
  ['8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1', [14, 191, 2812, 43238]],
  ['r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1', [6, 264, 9467]],
  ['rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8', [44, 1486, 62379]],
];
for (const [fen, counts] of PERFT) {
  const p = FB.Position.fromFen(fen);
  check(p.fen() === fen, 'fen round trip ' + fen);
  counts.forEach((n, i) => check(FB.perft(p, i + 1) === n, `perft ${fen} d${i + 1}`));
}

// 2. kinematics agrees with the MATLAB table
const R = FB.makeRobot(CFG.v2);
const table = fs.readFileSync(path.join(here, '../../matlab/output/square_table.csv'), 'utf8').trim().split('\n').slice(1);
for (const line of table) {
  const [name, x, y, l, r, ls, rs] = line.split(',');
  if (name.startsWith('grave')) continue;
  const p = R.squareXY(name);
  check(Math.abs(p[0] - +x) < 0.01 && Math.abs(p[1] - +y) < 0.01, 'square ' + name);
  const th = R.ik(p);
  check(Math.abs(th[0] * 180 / Math.PI - +l) < 0.002 && Math.abs(th[1] * 180 / Math.PI - +r) < 0.002, 'ik ' + name);
  const st = R.steps(th);
  check(Math.abs(st[0] - +ls) <= 1 && Math.abs(st[1] - +rs) <= 1, 'steps ' + name);
  const q = R.fk(th);
  check(FB.dist(p, q) < 1e-9, 'fk ' + name);
  check(R.squareAt(p) === name, 'squareAt ' + name);
}
check(R.isSafe(R.home()), 'home pose safe');
check(R.graveSlots(true).length >= 15 && R.graveSlots(false).length >= 15, 'graveyard slots');

// 3. planner: example games and random games, every path re-checked
function verify(planner, before, plan) {
  const occ = [];
  before.board.forEach((p, i) => { if (p) occ.push(R.squareXY(FB.sqName(i))); });
  for (const q of planner.parked) occ.push(q.p);
  for (const t of plan.transfers) {
    const s = t.waypoints[0], e = t.waypoints[t.waypoints.length - 1];
    const others = occ.filter((o) => FB.dist(o, s) > 1e-6 && FB.dist(o, e) > 1e-6);
    check(FB.pathClear(t.waypoints, others, R.clearance), 'clear ' + t.label);
    check(FB.pathSafe(R, t.waypoints), 'safe ' + t.label);
    const i = occ.findIndex((o) => FB.dist(o, s) < 1e-6); if (i >= 0) occ.splice(i, 1);
    occ.push(e);
  }
}
for (const f of ['italian_game.txt', 'special_moves.txt']) {
  const pos = FB.Position.fromFen(), pl = new FB.Planner(R);
  for (const mv of fs.readFileSync(path.join(here, '../../host/examples', f), 'utf8').trim().split(/\s+/)) {
    const before = pos.copy();
    verify(pl, before, pl.plan(pos, mv));
  }
}
let seed = 2026;
const rng = () => ((seed = (seed * 1103515245 + 12345) % 2147483648) / 2147483648);
for (let g = 0; g < 6; g++) {
  const pos = FB.Position.fromFen(), pl = new FB.Planner(R);
  for (let k = 0; k < 120 && !pos.outcome(); k++) {
    const L = pos.legal(), m = L[Math.floor(rng() * L.length)];
    const before = pos.copy();
    verify(pl, before, pl.plan(pos, FB.uci(m)));
  }
}

// 4. illegal move leaves the position untouched
{
  const pos = FB.Position.fromFen(), pl = new FB.Planner(R), fen = pos.fen();
  let threw = false;
  try { pl.plan(pos, 'e2e5'); } catch (e) { threw = true; }
  check(threw && pos.fen() === fen, 'illegal move rejected');
}

// 5. promotion fetches a captured queen back from the graveyard
{
  const pos = FB.Position.fromFen('3qk3/P7/8/8/8/8/8/3QK3 w - - 0 1'), pl = new FB.Planner(R);
  pl.plan(pos, 'd1d8'); pl.plan(pos, 'e8d8');
  const plan = pl.plan(pos, 'a7a8q');
  check(plan.transfers[plan.transfers.length - 1].label.startsWith('Q graveyard'), 'promotion fetch');
}

// 6. the opponent finds mate in one and returns legal moves
{
  const pos = FB.Position.fromFen('6k1/5ppp/8/8/8/8/5PPP/R5K1 w - - 0 1');
  const m = FB.bestMove(pos, 2, () => 0);
  const c = pos.copy(); c.push(m);
  check(c.outcome() === 'checkmate', 'finds back-rank mate, got ' + m);
}

// 7. trajectory ends where the path ends
{
  const w = [[0, 200], [50, 200], [50, 300]];
  const T = FB.trajectory(w, 120, 400);
  check(FB.dist(T.at(T.T), w[2]) < 1e-9 && FB.dist(T.at(0), w[0]) < 1e-9, 'trajectory ends');
}

console.log(`simulator core tests: ${pass} passed, ${fail} failed`);
process.exit(fail ? 1 : 0);
