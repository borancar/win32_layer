# Contributing to the windows-game-reconstruction skill

The skill and the platform it drives live in one repository:
https://github.com/borancar/win32_layer (the skill under
`skills/windows-game-reconstruction/`). If you used the skill on a game and it
taught you something, send it back as a pull request.

## What to send

| you found | put it in |
|---|---|
| a source shape, method or search that moved a byte match, or one that failed | `docs/byte-matching.md`, in the section for the symptom |
| another compiler's idea working (or inverting) on yours | the existing entry in `docs/byte-matching.md`: add your tag to its bracket, or one line under it |
| a register-allocation observation | `docs/vc6-regalloc.md`, `docs/vc8-regalloc.md`, or a new `docs/<tag>-regalloc.md` |
| a loader or platform pitfall | `docs/pitfalls.md`, under the game that found it |
| a middleware provider and how to wire it | `docs/middleware.md` |
| a correction to a step in `SKILL.md` | `SKILL.md`, as an instruction, without the backstory |
| a missing Win32 or DirectX import | the layer itself (`src/`, exported through `w32l_export_lookup`) |

## Rules for an entry in `docs/byte-matching.md`

- Tag it with the compiler: an existing tag, or a new one. For a new tag add a
  row to the tag table in *Read this first* and the exact build and flags to
  *Tags, statuses and figures*. A service pack is a different compiler.
- State the image pattern and the source shape that produces it. Keep it
  general: no project names needed.
- Give it a status: **held**, **number win**, **reverted/superseded** or **no
  effect**. Say how far it was checked.
- Before recording a win, check whether you later undid it. Reversals are the
  most useful entries; put them in section 1 with the rule they teach.
- Merge with an existing entry where the idea is the same; do not add a
  duplicate.

## What not to send

- No game data, executables, disassembly dumps or decompiler output of
  copyrighted games. Short instruction patterns that illustrate an idiom are
  fine.
- No leaked or proprietary source, quoted or paraphrased.
- No game-specific behaviour inside the layer: put it behind a `platform.h`
  hook in your own loader.

## Opening the pull request

1. Fork https://github.com/borancar/win32_layer and branch from `master`.
2. Make the change. Keep `SKILL.md` to instructions; detail goes under `docs/`.
3. Add one dated line to `skills/windows-game-reconstruction/CHANGELOG.md`.
4. For a layer change, build it (`make`) and say which game exercised it. The
   layer must keep serving the games that already use it.
5. Commit with a subject that starts `skill:` for skill changes, or names the
   module for layer changes.
6. Open the pull request against `master`:
   `gh pr create --repo borancar/win32_layer --base master`. Say which game
   and which compiler build the change comes from.

The layer is GPL-2.0-or-later; contributions are accepted under the same
licence.
