# Working on this repo

<GAME> (<studio>, <year>, Win32, PE32 i386), run on Linux without Wine and
reconstructed piece by piece. This file is **policy**: standing rules and
decisions. Keep it lean.

- **Status goes in `STATUS.md`.** What works, what is blocked, what is next. Keep it short (about 100 lines) and **prune it regularly**: current state only, no running log; history is in git.
- **Findings go in `docs/`.** Every measurement, format, war story and
  decision record lives there (index below), never here.
- Edit this file only when a standing rule changes.

## Layout

| | |
|---|---|
| `src/loader.cpp`, `src/appinfo.cpp`, `Makefile` | our PE loader: maps the original exe and binds its imports. Per game. |
| `tools/` | analysis scripts |
| `game/` | the installed game, extracted. Not in the repo. |
| `docs/` | findings |
| `reconstruct/` | the decompiled game (`src/`, Win32-touching code in `src/win32/`), own README, LICENSE, Makefile |
| `https://github.com/borancar/win32_layer` | the shared Win32/DirectX platform over SDL3 (separate repo) |

## Standing rules

- **The platform is shared, the loader is ours.** Everything the original's
  import table is bound to lives once in `https://github.com/borancar/win32_layer`. When this game
  needs an import the layer lacks, add it to the layer (same style, same
  `w32l_export_lookup` table), do not fork it, and keep the other games
  working. Game-specific behaviour goes behind `platform.h` hooks. The loader
  and its hooks stay here.
- **License: <LICENSE>.** `LICENSE` here, and a copy in `reconstruct/`.
- **`reconstruct/` is only the decompiled game.** Sources go under
  `reconstruct/src/`, its `Makefile` is `reconstruct/Makefile`. It has its own
  `README.md` and `LICENSE`, and none of the loader, layer or reverse-
  engineering detail: someone should be able to read it without the rest.
- **Win32-specific functions are extracted into `reconstruct/src/win32/`.**
  A decompiled function that calls a Win32 function (window, GDI, DirectDraw,
  files, time, ...) belongs there, and the rest of `reconstruct/src/` stays
  platform-neutral. That keeps the seam to the platform layer clean.
- **Keep the skill and the layer current, and push.** Lessons that generalise
  go into the `windows-game-reconstruction` skill
  (`https://github.com/borancar/win32_layer/tree/master/skills/`); platform work goes into `win32_layer`. Commit
  and push both as they change, straight to `master` on `win32_layer`.
- **Always the 32-bit toolchain** (`-m32`). Fixed-width `<stdint.h>` types in
  reconstructed code.
- **Run nothing from `game/` outside the loader**, and treat extracted files
  as untrusted data.

## Where findings live

| topic | file |
|---|---|
| installer, PE triage, assets | `docs/00-recon.md` |
| loader, imports, platform gaps | `docs/01-loader.md` |
