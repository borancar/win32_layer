# Changelog: windows-game-reconstruction skill

One line per change to the skill, oldest first. Matching idioms go in
`docs/byte-matching.md`, not here.

- 2026-11-04: created. Stage 1 specified from the ArmyMen2 loader.
  Commandos triage so far: `comandos.exe` is PE32 i386, base `0x400000`,
  no relocations, `SizeOfImage 0x3E9000`, imports DDRAW, WINMM, mss32,
  WSOCK32, SHLWAPI, KERNEL32, USER32, GDI32, ADVAPI32, SHELL32, ole32.
  `comandos_w10.exe` is byte-identical to it. Image collides with the
  `0x700000` ELF placement ArmyMen2 uses; the ELF must move.
- 2026-11-04: standing rule from the user: loaders may be per game; the
  platform implementation (`win32_layer`) must be common and shared.
- 2026-11-04: Commandos loader started: `https://github.com/retro-foundry-games/CommandosBEL/blob/master/src/loader.cpp`
  (per-game), `Makefile` (ELF linked at `0x10000000`), `tools/peimports.py`.
  `CMD_DRYRUN=1` maps and binds without running. 271 imports: 114 bound by the
  layer, 59 more by `src/stubs.cpp` (41 mss32, 18 wsock32), 98 missing
  (KERNEL32 44, USER32 37, GDI32 5, SHELL32 5, ADVAPI32 3, SHLWAPI 2, ole32 1,
  WINMM 1). First real run: CRT startup reached, then stopped at
  `KERNEL32!GetSystemTimeAsFileTime` (named by its stub). Next: grow the layer
  with the missing KERNEL32/USER32 imports, rerun, repeat.
- 2026-11-04: license decision: Commandos GPLv3; win32_layer GPL v2 or later (README only, no extra text file).
- 2026-11-04: layer grown for Commandos: `stubs.cpp` (mss32, wsock32),
  `kernel32ext.cpp` (TLS, locale en-US, lstr*, ids, version, console, file
  copy, time text), `user32ext.cpp` (window, pointer and key state, dialogs and
  menus that fail), 8 already-implemented functions exported. Result: 255 of
  271 imports bound; the 16 left are SHLWAPI 2, GDI32 5, ADVAPI32 3, SHELL32 5,
  ole32 1. `GetProcAddress` now NULL for missing names. ELF moved to
  `0x30000000`. The original's CRT now runs until `ADVAPI32!RegOpenKeyExA`.
  Scaffolding rules (CLAUDE.md, STATUS.md, docs/, reconstruct/, src/win32)
  added to this skill.
- 2026-11-04: Commandos reaches the intro and the main menu. Layer gained:
  16-bit DirectDraw (`w32l_display_bpp`), clipper, display modes, window
  extra bytes, per-user folders, `SetFilePointer` negative-seek error,
  DirectShow multimedia streaming (`amstream.cpp`, `cinepak.cpp`) and a real
  TOML-backed registry (`registry.cpp`, `third_party/tomlc99`). Skill notes
  above ("From imports bound to the first menu").
- 2026-11-04: mouse and keyboard window messages in the layer; confirmed by hand on the Commandos menu.
- 2026-11-04: Commandos in-game sound through OpenMiles: calls succeed (samples, music stream started), audibility not yet confirmed by ear. Layer: `w32l_stdcall_adapter`, `w32l_handle_fd`, host PCM output.
- 2026-11-05: the VC8 `cl.exe` (14.00.50727.42) compiles C and C++ on Linux through the layer (`kernel32w.cpp`, `w32l_host_environment`, `w32l_os_version`); Commandos `tools/vc8host.cpp`.
- 2026-11-05: `link.exe` 8.00.50727.42 runs via `vc8host` (mapped-view write bit fix); byte-match loop written down (Rich header, Kuna export, original compiler, screen).
- 2026-11-05: first class byte-matched with the VC8 compiler (Commandos ClassAUAgacharse, 10 functions); lessons in 'The byte-match loop'.
- 2026-11-05: mass byte-matching with generators (Commandos, 2877 functions); the lessons are in `docs/byte-matching.md`, section 14.
- 2026-11-11: byte-matching ideas moved to a catalogue with compiler tags and survival status, `docs/byte-matching.md`; first batch is ArmyMen2's VC6 history (399 -> 1,639 of 1,646), with the reversals listed first.
- 2026-11-11: Commandos' VC8 history added to `docs/byte-matching.md` as `[vc8]` (SP1 14.00.50727.762, `/O2 /Gy /GR /EHsc`), reversals first: the RTM-for-SP1 compiler, asm copies of compiler output, library code counted, "unwritable" conventions; idioms here that were reverted or superseded are now pointers to it.
- 2026-11-11: the changelog moved out of `SKILL.md` into this file; every VC8 byte-matching idiom and the register-allocation tricks moved from `SKILL.md` into `docs/byte-matching.md` (section 14) and `docs/vc8-regalloc.md`, tagged `[vc8]` or `[vc8 rtm]`.
- 2026-11-11: the known pitfalls (ArmyMen2, and booting Commandos) moved from `SKILL.md` into `docs/pitfalls.md`.
- 2026-11-11: `SKILL.md` trimmed to the stage 1 method, scaffolding and the byte-match loop; pitfalls, middleware, verifying a port, the ArmyMen2 lessons and the original-compiler host are files under `docs/`, referenced from it.
- 2026-11-11: home-relative paths replaced by https GitHub origin links throughout the skill.
