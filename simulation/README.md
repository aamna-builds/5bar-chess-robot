<sub>[← Back to README](../README.md)</sub>

# Live simulator

**▶ https://aamna-builds.github.io/5bar-chess-robot/**

The page is a single HTML file with no frameworks and no external libraries:

| File | What it does |
|---|---|
| `web/index.html` | the page: drawing, controls, animation |
| `web/core.js` | kinematics, chess rules, path planner, a small alpha-beta opponent, motion profiles. A port of `host/fivebar_chess` |
| `web/config.js` | generated from `config/*.json` by `tools/gen_web_config.py` |
| `tests/test_core.mjs` | perft counts, agreement with the MATLAB square table, planner collision and safety checks on example and random games |

Run it locally:

```bash
cd simulation/web && python3 -m http.server 8000      # then open http://localhost:8000
node simulation/tests/test_core.mjs                   # tests
```

`.github/workflows/pages.yml` publishes `simulation/web/` to GitHub Pages on every push that changes it.
