/*
 * 5-bar chess robot: simulation core for the browser.
 *
 * The same maths and rules as matlab/ and host/fivebar_chess, ported to
 * JavaScript so the robot can run live in a web page. Pure logic, no DOM:
 * the page (index.html) draws it, and simulation/tests/ run it under Node.
 *
 *   FB.makeRobot(cfgJson)  geometry, kinematics, board layout
 *   FB.Position            chess rules (castling, en passant, promotion, mate)
 *   FB.Planner             chess move -> collision-free magnet paths
 *   FB.trajectory          trapezoidal time scaling of a path
 *   FB.bestMove            small alpha-beta opponent
 */
(function (root) {
  'use strict';

  const DEG = Math.PI / 180;
  const FILES = 'abcdefgh';
  const dist = (a, b) => Math.hypot(a[0] - b[0], a[1] - b[1]);

  // ------------------------------------------------------------------ robot
  function makeRobot(c, overrides) {
    const g = Object.assign({
      L1: c.geometry.proximal_mm, L2: c.geometry.distal_mm, d: c.geometry.motor_spacing_mm,
      y0: c.board.near_edge_y_mm,
    }, overrides || {});
    const R = {
      L1: g.L1, L2: g.L2, d: g.d, y0: g.y0,
      sq: c.board.square_mm, rankNear: c.board.rank_nearest_motors, aSide: c.board.a_file_side,
      pieceDia: c.piece.base_diameter_mm,
      graveOff: c.graveyard.x_offset_from_board_edge_mm, gravePitch: c.graveyard.pitch_mm,
      whiteSide: c.graveyard.white_side,
      stepsPerRev: c.drive.full_steps_per_rev * c.drive.microsteps * c.drive.gear_ratio,
      homeDeg: c.drive.home_deg.slice(),
      vmax: c.motion.max_speed_mm_s, amax: c.motion.accel_mm_s2,
      minTrans: c.safety.min_transmission_deg, minElbow: c.safety.min_elbow_deg,
    };
    R.m1 = [-R.d / 2, 0];
    R.m2 = [R.d / 2, 0];
    R.half = 4 * R.sq;
    R.clearance = R.pieceDia + 2;
    R.degPerStep = 360 / R.stepsPerRev;

    R.ik = function (p) {
      const out = [];
      for (const [m, s] of [[R.m1, 1], [R.m2, -1]]) {
        const dx = p[0] - m[0], dy = p[1] - m[1];
        const r = Math.hypot(dx, dy);
        if (r < 1e-9) return null;
        const cs = (R.L1 * R.L1 + r * r - R.L2 * R.L2) / (2 * R.L1 * r);
        if (cs > 1 || cs < -1) return null;
        out.push(Math.atan2(dy, dx) + s * Math.acos(cs));
      }
      return out;
    };
    R.elbows = function (th) {
      return [[R.m1[0] + R.L1 * Math.cos(th[0]), R.m1[1] + R.L1 * Math.sin(th[0])],
              [R.m2[0] + R.L1 * Math.cos(th[1]), R.m2[1] + R.L1 * Math.sin(th[1])]];
    };
    R.fk = function (th) {
      const [b1, b2] = R.elbows(th);
      const vx = b2[0] - b1[0], vy = b2[1] - b1[1], D = Math.hypot(vx, vy);
      if (D < 1e-9 || D > 2 * R.L2) return null;
      const h = Math.sqrt(Math.max(0, R.L2 * R.L2 - D * D / 4));
      return [(b1[0] + b2[0]) / 2 - h * vy / D, (b1[1] + b2[1]) / 2 + h * vx / D];
    };
    const ang = (u, v) => Math.atan2(Math.abs(u[0] * v[1] - u[1] * v[0]), u[0] * v[0] + u[1] * v[1]) / DEG;
    R.quality = function (p) {
      const th = R.ik(p);
      if (!th) return { ok: false, safe: false, margin: -Infinity };
      const [b1, b2] = R.elbows(th);
      const trans = ang([b1[0] - p[0], b1[1] - p[1]], [b2[0] - p[0], b2[1] - p[1]]);
      const e1 = ang([R.m1[0] - b1[0], R.m1[1] - b1[1]], [p[0] - b1[0], p[1] - b1[1]]);
      const e2 = ang([R.m2[0] - b2[0], R.m2[1] - b2[1]], [p[0] - b2[0], p[1] - b2[1]]);
      const elbow = Math.min(e1, e2);
      const tm = Math.min(trans, 180 - trans), em = Math.min(elbow, 180 - elbow);
      return { ok: true, th, b1, b2, trans, elbow, margin: Math.min(tm, em),
               safe: tm >= R.minTrans && em >= R.minElbow };
    };
    R.isSafe = (p) => R.quality(p).safe;
    R.steps = (th) => th.map((t, i) => Math.round((t / DEG - R.homeDeg[i]) / R.degPerStep));
    R.home = () => R.fk(R.homeDeg.map((a) => a * DEG));

    R.squareXY = function (name) {
      const f = FILES.indexOf(name[0]), r = +name[1] - 1;
      const col = R.aSide === '+x' ? 7 - f : f;
      const row = R.rankNear === 8 ? 7 - r : r;
      return [(col - 3.5) * R.sq, R.y0 + (row + 0.5) * R.sq];
    };
    R.squareAt = function (p) {           // inverse of squareXY (null if off-board)
      const col = Math.floor(p[0] / R.sq + 4), row = Math.floor((p[1] - R.y0) / R.sq);
      if (col < 0 || col > 7 || row < 0 || row > 7) return null;
      const f = R.aSide === '+x' ? 7 - col : col;
      const r = R.rankNear === 8 ? 7 - row : row;
      return FILES[f] + (r + 1);
    };
    R.graveSlots = function (white) {
      const side = ((R.whiteSide === '+x') === white) ? 1 : -1;
      const x = side * (R.half + R.graveOff);
      const out = [];
      for (let y = R.y0 + R.pieceDia / 2; y <= R.y0 + 8 * R.sq - R.pieceDia / 2 + 1e-9; y += R.gravePitch) {
        if (R.isSafe([x, y])) out.push([x, y]);
      }
      return out;
    };
    return R;
  }

  // ------------------------------------------------------------------ chess
  const KN = [[1, 2], [2, 1], [2, -1], [1, -2], [-1, -2], [-2, -1], [-2, 1], [-1, 2]];
  const KG = [[1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1], [0, -1], [1, -1]];
  const RD = [[1, 0], [-1, 0], [0, 1], [0, -1]];
  const BD = [[1, 1], [1, -1], [-1, 1], [-1, -1]];
  const START = 'rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1';
  const isW = (p) => p === p.toUpperCase();
  const on = (f, r) => f >= 0 && f < 8 && r >= 0 && r < 8;
  const sqName = (i) => FILES[i % 8] + (Math.floor(i / 8) + 1);
  const sqIndex = (n) => FILES.indexOf(n[0]) + 8 * (+n[1] - 1);
  const uci = (m) => sqName(m.from) + sqName(m.to) + (m.promo || '');

  class Position {
    constructor() { this.board = new Array(64).fill(null); this.white = true; this.castling = 'KQkq'; this.ep = null; this.half = 0; this.full = 1; }
    static fromFen(fen = START) {
      const p = new Position(), parts = fen.trim().split(/\s+/), rows = parts[0].split('/');
      if (rows.length !== 8) throw new Error('bad FEN');
      rows.forEach((row, r) => {
        let f = 0;
        for (const ch of row) {
          if (/\d/.test(ch)) f += +ch; else { p.board[(7 - r) * 8 + f] = ch; f++; }
        }
        if (f !== 8) throw new Error('bad FEN rank');
      });
      p.white = parts[1] === 'w';
      p.castling = parts[2] === '-' ? '' : parts[2];
      p.ep = parts[3] === '-' ? null : sqIndex(parts[3]);
      if (parts.length >= 6) { p.half = +parts[4]; p.full = +parts[5]; }
      return p;
    }
    fen() {
      const rows = [];
      for (let r = 7; r >= 0; r--) {
        let s = '', e = 0;
        for (let f = 0; f < 8; f++) {
          const pc = this.board[r * 8 + f];
          if (!pc) e++; else { s += (e || '') + pc; e = 0; }
        }
        rows.push(s + (e || ''));
      }
      return [rows.join('/'), this.white ? 'w' : 'b', this.castling || '-',
              this.ep === null ? '-' : sqName(this.ep), this.half, this.full].join(' ');
    }
    copy() {
      const p = new Position();
      p.board = this.board.slice(); p.white = this.white; p.castling = this.castling;
      p.ep = this.ep; p.half = this.half; p.full = this.full; return p;
    }
    attacked(sq, byW) {
      const f = sq % 8, r = sq >> 3, b = this.board;
      const pr = byW ? r - 1 : r + 1;
      for (const df of [-1, 1]) if (on(f + df, pr) && b[pr * 8 + f + df] === (byW ? 'P' : 'p')) return true;
      for (const [df, dr] of KN) if (on(f + df, r + dr) && b[(r + dr) * 8 + f + df] === (byW ? 'N' : 'n')) return true;
      for (const [df, dr] of KG) if (on(f + df, r + dr) && b[(r + dr) * 8 + f + df] === (byW ? 'K' : 'k')) return true;
      for (const [dirs, kinds] of [[RD, 'RQ'], [BD, 'BQ']]) {
        const k = byW ? kinds : kinds.toLowerCase();
        for (const [df, dr] of dirs) {
          let ff = f + df, rr = r + dr;
          while (on(ff, rr)) {
            const p = b[rr * 8 + ff];
            if (p) { if (k.includes(p)) return true; break; }
            ff += df; rr += dr;
          }
        }
      }
      return false;
    }
    kingSq(w) { return this.board.indexOf(w ? 'K' : 'k'); }
    inCheck() { return this.attacked(this.kingSq(this.white), !this.white); }
    pseudo() {
      const b = this.board, w = this.white, mv = [];
      const addP = (fr, to, last) => { if (last) for (const pr of 'qrbn') mv.push({ from: fr, to, promo: pr }); else mv.push({ from: fr, to }); };
      for (let sq = 0; sq < 64; sq++) {
        const p = b[sq];
        if (!p || isW(p) !== w) continue;
        const f = sq % 8, r = sq >> 3, k = p.toUpperCase();
        if (k === 'P') {
          const st = w ? 1 : -1, start = w ? 1 : 6, last = w ? 7 : 0, r1 = r + st;
          if (on(f, r1) && !b[r1 * 8 + f]) {
            addP(sq, r1 * 8 + f, r1 === last);
            const r2 = r + 2 * st;
            if (r === start && !b[r2 * 8 + f]) mv.push({ from: sq, to: r2 * 8 + f });
          }
          for (const df of [-1, 1]) {
            if (!on(f + df, r1)) continue;
            const t = r1 * 8 + f + df, q = b[t];
            if ((q && isW(q) !== w) || t === this.ep) addP(sq, t, r1 === last);
          }
        } else if (k === 'N' || k === 'K') {
          for (const [df, dr] of (k === 'N' ? KN : KG)) {
            if (!on(f + df, r + dr)) continue;
            const t = (r + dr) * 8 + f + df, q = b[t];
            if (!q || isW(q) !== w) mv.push({ from: sq, to: t });
          }
          if (k === 'K') this._castles(mv, sq);
        } else {
          const dirs = k === 'B' ? BD : k === 'R' ? RD : BD.concat(RD);
          for (const [df, dr] of dirs) {
            let ff = f + df, rr = r + dr;
            while (on(ff, rr)) {
              const t = rr * 8 + ff, q = b[t];
              if (!q) mv.push({ from: sq, to: t });
              else { if (isW(q) !== w) mv.push({ from: sq, to: t }); break; }
              ff += df; rr += dr;
            }
          }
        }
      }
      return mv;
    }
    _castles(mv, ks) {
      const w = this.white, home = w ? 4 : 60, b = this.board;
      if (ks !== home || this.attacked(home, !w)) return;
      const [K, Q, rook] = w ? ['K', 'Q', 'R'] : ['k', 'q', 'r'];
      if (this.castling.includes(K) && b[home + 3] === rook && !b[home + 1] && !b[home + 2] &&
          !this.attacked(home + 1, !w) && !this.attacked(home + 2, !w)) mv.push({ from: home, to: home + 2 });
      if (this.castling.includes(Q) && b[home - 4] === rook && !b[home - 1] && !b[home - 2] && !b[home - 3] &&
          !this.attacked(home - 1, !w) && !this.attacked(home - 2, !w)) mv.push({ from: home, to: home - 2 });
    }
    legal() {
      const out = [];
      for (const m of this.pseudo()) {
        const n = this.copy(); n._apply(m);
        if (!n.attacked(n.kingSq(this.white), n.white)) out.push(m);
      }
      return out;
    }
    describe(m) {
      const p = this.board[m.from], info = { move: m, piece: p, captured: null, capturedOn: null, rookFrom: null, rookTo: null, promoted: null };
      const t = this.board[m.to];
      if (t) { info.captured = t; info.capturedOn = m.to; }
      else if (p.toUpperCase() === 'P' && m.to === this.ep && m.from % 8 !== m.to % 8) {
        const c = isW(p) ? m.to - 8 : m.to + 8; info.captured = this.board[c]; info.capturedOn = c;
      }
      if (p.toUpperCase() === 'K' && Math.abs(m.to - m.from) === 2) {
        if (m.to > m.from) { info.rookFrom = m.from + 3; info.rookTo = m.from + 1; }
        else { info.rookFrom = m.from - 4; info.rookTo = m.from - 1; }
      }
      if (m.promo) info.promoted = isW(p) ? m.promo.toUpperCase() : m.promo;
      return info;
    }
    _apply(m) {
      const info = this.describe(m), b = this.board;
      if (info.capturedOn !== null) b[info.capturedOn] = null;
      b[m.from] = null;
      b[m.to] = info.promoted || info.piece;
      if (info.rookFrom !== null) { b[info.rookTo] = b[info.rookFrom]; b[info.rookFrom] = null; }
      for (const [s, rt] of [[4, 'KQ'], [60, 'kq'], [0, 'Q'], [7, 'K'], [56, 'q'], [63, 'k']]) {
        if (m.from === s || m.to === s) this.castling = [...this.castling].filter((c) => !rt.includes(c)).join('');
      }
      this.ep = (info.piece.toUpperCase() === 'P' && Math.abs(m.to - m.from) === 16) ? (m.from + m.to) / 2 : null;
      this.half = (info.piece.toUpperCase() === 'P' || info.captured) ? 0 : this.half + 1;
      if (!this.white) this.full++;
      this.white = !this.white;
      return info;
    }
    push(m) {
      if (typeof m === 'string') m = { from: sqIndex(m.slice(0, 2)), to: sqIndex(m.slice(2, 4)), promo: m[4] || undefined };
      const L = this.legal();
      let hit = L.find((x) => x.from === m.from && x.to === m.to && (x.promo || null) === (m.promo || null));
      if (!hit && !m.promo) hit = L.find((x) => x.from === m.from && x.to === m.to && x.promo === 'q');
      if (!hit) throw new Error('illegal move ' + uci(m));
      return this._apply(hit);
    }
    outcome() {
      if (!this.legal().length) return this.inCheck() ? 'checkmate' : 'stalemate';
      if (this.half >= 100) return 'fifty-move rule';
      const rest = this.board.filter((p) => p && p.toUpperCase() !== 'K').map((p) => p.toUpperCase()).sort().join('');
      if (rest === '' || rest === 'B' || rest === 'N') return 'insufficient material';
      return null;
    }
  }

  function perft(pos, depth) {
    if (depth === 0) return 1;
    let n = 0;
    for (const m of pos.legal()) { const c = pos.copy(); c._apply(m); n += perft(c, depth - 1); }
    return n;
  }

  // ---------------------------------------------------------------- planner
  function segPointDist(p, a, b) {
    const dx = b[0] - a[0], dy = b[1] - a[1], L2 = dx * dx + dy * dy;
    const t = L2 === 0 ? 0 : Math.max(0, Math.min(1, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / L2));
    return Math.hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy);
  }
  const pathLength = (w) => w.slice(1).reduce((s, p, i) => s + dist(w[i], p), 0);
  function pathClear(w, obstacles, clearance) {
    for (let i = 0; i + 1 < w.length; i++) for (const o of obstacles) if (segPointDist(o, w[i], w[i + 1]) < clearance - 1e-9) return false;
    return true;
  }
  function pathSafe(R, w, step = 2) {
    for (let i = 0; i + 1 < w.length; i++) {
      const a = w[i], b = w[i + 1], n = Math.max(1, Math.ceil(dist(a, b) / step));
      for (let k = 0; k <= n; k++) { const t = k / n; if (!R.isSafe([a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1])])) return false; }
    }
    return true;
  }
  function dedupe(w) {
    const out = [w[0]];
    for (const p of w.slice(1)) if (dist(p, out[out.length - 1]) > 1e-9) out.push(p);
    for (let i = 1; i < out.length - 1;) {
      const [a, b, c] = [out[i - 1], out[i], out[i + 1]];
      if (Math.abs((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])) < 1e-9) out.splice(i, 1); else i++;
    }
    return out;
  }
  function laneCandidates(R, a, b) {
    const h = R.sq / 2, routes = [];
    const corners = (p) => [[-1, -1], [-1, 1], [1, -1], [1, 1]].map(([sx, sy]) => [p[0] + sx * h, p[1] + sy * h]);
    for (const c1 of corners(a)) for (const c2 of corners(b)) {
      routes.push(dedupe([a, c1, [c2[0], c1[1]], c2, b]));
      routes.push(dedupe([a, c1, [c1[0], c2[1]], c2, b]));
    }
    for (const ex of [-R.half, R.half]) for (const c1 of [a].concat(corners(a))) for (const c2 of [b].concat(corners(b))) {
      routes.push(dedupe([a, c1, [ex, c1[1]], [ex, c2[1]], c2, b]));
    }
    return routes;
  }
  function route(R, a, b, obstacles) {
    const direct = [a, b];
    if (pathClear(direct, obstacles, R.clearance) && pathSafe(R, direct)) return direct;
    const cands = laneCandidates(R, a, b).sort((x, y) => pathLength(x) - pathLength(y));
    for (const w of cands) if (pathClear(w, obstacles, R.clearance) && pathSafe(R, w)) return w;
    throw new Error('no safe path from ' + a.map((v) => v.toFixed(0)) + ' to ' + b.map((v) => v.toFixed(0)));
  }

  class Planner {
    constructor(R) {
      this.R = R;
      this.slots = { true: R.graveSlots(true), false: R.graveSlots(false) };
      this.parked = [];                       // [{p:[x,y], piece}]
    }
    obstacles(pos, ignore) {
      const pts = [];
      pos.board.forEach((p, i) => { if (p && !ignore.has(i)) pts.push(this.R.squareXY(sqName(i))); });
      return pts.concat(this.parked.map((q) => q.p));
    }
    freeSlot(white) {
      for (const s of this.slots[white]) if (!this.parked.some((q) => dist(q.p, s) < 1e-6)) return s;
      throw new Error('graveyard full');
    }
    toGrave(pos, sq, piece) {
      const a = this.R.squareXY(sqName(sq)), slot = this.freeSlot(isW(piece));
      const w = route(this.R, a, slot, this.obstacles(pos, new Set([sq])));
      this.parked.push({ p: slot, piece });
      return { piece, label: `${piece} ${sqName(sq)} → graveyard`, waypoints: w };
    }
    fromGrave(pos, piece, sq) {
      const i = this.parked.findIndex((q) => q.piece === piece);
      if (i < 0) return null;
      const slot = this.parked[i].p;
      this.parked.splice(i, 1);
      const w = route(this.R, slot, this.R.squareXY(sqName(sq)), this.obstacles(pos, new Set([sq])));
      return { piece, label: `${piece} graveyard → ${sqName(sq)}`, waypoints: w };
    }
    plan(pos, move) {
      const snapshot = pos.copy();
      const info = pos.push(move);                 // throws on illegal moves
      const m = info.move, R = this.R, work = snapshot.copy(), transfers = [], notes = [];
      const parkedBefore = this.parked.slice();
      try {
        if (info.capturedOn !== null) { transfers.push(this.toGrave(work, info.capturedOn, info.captured)); work.board[info.capturedOn] = null; }
        const w = route(R, R.squareXY(sqName(m.from)), R.squareXY(sqName(m.to)), this.obstacles(work, new Set([m.from])));
        transfers.push({ piece: info.piece, label: `${info.piece} ${sqName(m.from)}–${sqName(m.to)}`, waypoints: w });
        work.board[m.to] = info.piece; work.board[m.from] = null;
        if (info.rookFrom !== null) {
          const rook = work.board[info.rookFrom];
          const wr = route(R, R.squareXY(sqName(info.rookFrom)), R.squareXY(sqName(info.rookTo)), this.obstacles(work, new Set([info.rookFrom])));
          transfers.push({ piece: rook, label: `${rook} ${sqName(info.rookFrom)}–${sqName(info.rookTo)} (castling)`, waypoints: wr });
          work.board[info.rookTo] = rook; work.board[info.rookFrom] = null;
        }
        if (info.promoted) {
          transfers.push(this.toGrave(work, m.to, info.piece));
          work.board[m.to] = null;
          const f = this.fromGrave(work, info.promoted, m.to);
          if (f) transfers.push(f); else notes.push(`A ${info.promoted} is placed on ${sqName(m.to)} by hand.`);
          work.board[m.to] = info.promoted;
        }
      } catch (e) {
        // undo the chess move so the game state stays consistent
        Object.assign(pos, snapshot.copy());
        this.parked = parkedBefore;
        throw e;
      }
      return { move: uci(m), info, transfers, notes };
    }
  }

  // -------------------------------------------------------------- motion
  function trajectory(w, vmax, amax) {
    // returns {T, at(t) -> [x,y]} ; each segment rest-to-rest trapezoid
    const segs = [];
    let T = 0;
    for (let i = 0; i + 1 < w.length; i++) {
      const L = dist(w[i], w[i + 1]);
      if (L < 1e-9) continue;
      let ta = vmax / amax, tc, vp;
      if (amax * ta * ta > L) { ta = Math.sqrt(L / amax); vp = amax * ta; tc = 0; }
      else { vp = vmax; tc = (L - amax * ta * ta) / vmax; }
      const Ts = 2 * ta + tc;
      segs.push({ a: w[i], b: w[i + 1], L, ta, tc, vp, t0: T, Ts });
      T += Ts;
    }
    function at(t) {
      if (!segs.length) return w[0].slice();
      if (t >= T) return w[w.length - 1].slice();
      let s = segs.find((g) => t < g.t0 + g.Ts) || segs[segs.length - 1];
      const x = t - s.t0;
      let d;
      if (x < s.ta) d = 0.5 * amax * x * x;
      else if (x < s.ta + s.tc) d = 0.5 * amax * s.ta * s.ta + s.vp * (x - s.ta);
      else { const r = s.Ts - x; d = s.L - 0.5 * amax * r * r; }
      const u = d / s.L;
      return [s.a[0] + u * (s.b[0] - s.a[0]), s.a[1] + u * (s.b[1] - s.a[1])];
    }
    return { T, at };
  }

  // ------------------------------------------------------------------- AI
  const VAL = { P: 100, N: 320, B: 330, R: 500, Q: 900, K: 0 };
  // centralisation bonus per square (white's view), small and symmetric
  const CENTER = [];
  for (let i = 0; i < 64; i++) { const f = i % 8, r = i >> 3; CENTER.push(14 - 2 * (Math.abs(3.5 - f) + Math.abs(3.5 - r))); }
  function evaluate(pos) {
    let s = 0;
    pos.board.forEach((p, i) => {
      if (!p) return;
      const k = p.toUpperCase(), w = isW(p);
      let v = VAL[k];
      if (k === 'N' || k === 'B') v += CENTER[i] + ((i >> 3) === (w ? 0 : 7) ? -18 : 0);   // develop minor pieces
      if (k === 'P') v += (w ? (i >> 3) - 1 : 6 - (i >> 3)) * 3 + CENTER[i] / 2;
      if (k === 'K') v += (pos.full < 30 ? -CENTER[i] : CENTER[i]);
      s += w ? v : -v;
    });
    return pos.white ? s : -s;
  }
  function search(pos, depth, alpha, beta, ply) {
    const moves = pos.legal();
    if (!moves.length) return pos.inCheck() ? -100000 + ply : 0;
    if (depth === 0) return evaluate(pos);
    moves.sort((a, b) => (pos.board[b.to] ? VAL[pos.board[b.to].toUpperCase()] : 0) - (pos.board[a.to] ? VAL[pos.board[a.to].toUpperCase()] : 0));
    for (const m of moves) {
      const c = pos.copy(); c._apply(m);
      const v = -search(c, depth - 1, -beta, -alpha, ply + 1);
      if (v >= beta) return beta;
      if (v > alpha) alpha = v;
    }
    return alpha;
  }
  function bestMove(pos, depth = 2, rng = Math.random) {
    const moves = pos.legal();
    let best = [], bestV = -Infinity;
    for (const m of moves) {
      const c = pos.copy(); c._apply(m);
      const v = -search(c, depth - 1, -Infinity, -(bestV - 8), 1) + rng() * 8;   // a little variety
      if (v > bestV) { bestV = v; best = [m]; }
    }
    return best[0] ? uci(best[0]) : null;
  }

  const FB = { makeRobot, Position, perft, Planner, route, pathClear, pathSafe, trajectory, bestMove,
               sqName, sqIndex, uci, START, isW, dist };
  if (typeof module !== 'undefined' && module.exports) module.exports = FB;
  else root.FB = FB;
})(typeof window !== 'undefined' ? window : globalThis);
