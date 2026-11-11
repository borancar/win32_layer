---
name: windows-game-reconstruction
description: Reverse-engineer a 1990s Win32 (PE32 i386) game and reconstruct it for Linux without Wine. Use when running an original Windows game executable on Linux through a custom PE loader and the win32_layer (Win32/DirectX over SDL3), when binding a PE's import table to host implementations, when building a hybrid (original code + reconstructed code) build as a correctness oracle, or when porting a Windows game piece by piece. Living document: extended as each stage is reached.
---

# Reconstructing a Windows game on Linux

Keep the original binary running and replace it function by function. Treat
the original as the oracle: call nothing done because it looks right on screen.

- Reference port: ArmyMen2 (`https://github.com/borancar/ArmyMen2`). Platform:
  `https://github.com/borancar/win32_layer`. DOS games: use the `dos-game-reconstruction` skill.
- When a stage teaches something, write it into the matching file under
  `docs/`, keep this file to instructions, and add one line to `CHANGELOG.md`.
- **Send what you learn back.** If you are not the repository owner, open a
  pull request against https://github.com/borancar/win32_layer as
  `CONTRIBUTING.md` (beside this file) describes: new matching ideas, tags
  confirmed on another compiler, pitfalls, missing imports.

## Stage 1: the hybrid layer

Build a Linux i386 program that loads the original `.exe` into its own process
and runs it with no Wine and no Windows code, every import bound to
`win32_layer`. Boot to the title screen through the original's own CRT startup,
unmodified.

```
 Linux i386 ELF (loader + libwin32_layer.a + SDL3)
   |-- mmap the PE image at its ImageBase (fixed address)
   |-- copy headers + sections
   |-- bind the IAT: slot = address of OUR implementation of module!name
   |-- set up fs (TEB) so SEH push/pop works
   |-- chdir to the game directory, set the command line
   `-- call AddressOfEntryPoint (the CRT's WinMainCRTStartup)
```

Start from `ArmyMen2/src/hybrid/loader.cpp` (`WinMain`, `bind_imports`,
`teb_attach`, `make_stub`, `fault`). Resolve imports through
`w32l_export_lookup(module, name)` in `win32_layer/src/kernel32.cpp`.

### Loader per game, platform shared

- Write one loader per game, in the game's repo. Copy the ArmyMen2 loader and
  bend it: image placement, TEB, detours, retail patches, log hooks, traces.
- Put everything the IAT binds to in `https://github.com/borancar/win32_layer`. Never fork the layer.
- When a game needs an import the layer lacks, add it to the layer in the same
  style, behind `w32l_export_lookup`, and keep ArmyMen2 building and agreeing
  with its oracle.
- Put game-specific behaviour behind a `platform.h` hook (`w32l_app_info`,
  `w32l_host_*`). Do not write `#ifdef GAME` in the layer.
- Lift loader code into the layer only once two loaders carry the same code.

### Recipe

1. **Triage the PE** (`objdump -p`) before writing anything.
   - Require `Machine 0x14C`, `Magic 0x10B`. Stop on anything else.
   - Note `ImageBase` and `SizeOfImage`. If the relocation directory (entry 5)
     is empty, load at `ImageBase` only and treat every address as a stable VA.
   - List every `DLL!name` from the import directory; that is the work list.
     Count by DLL and separate system DLLs from third-party ones (`mss32`,
     `smackw32`, `binkw32`, `ddraw` via COM); for the third-party ones see
     `docs/middleware.md`.
   - Resolve ordinal imports (`0x80000000` bit) from an ordinal table
     (`dsound #1` is `DirectSoundCreate`; `wsock32` is mostly ordinal).
   - Do not plan on symbols or a PDB.
   - If the entry point is outside `.text` or the import table is tiny, the
     image is packed or protected: solve that first.
2. **Place the ELF clear of the PE and of the layer's fixed heap**
   (`0x0C000000-0x16000000`). Link at `0x30000000` unless `SizeOfImage` says
   otherwise. Map the image with `MAP_FIXED_NOREPLACE`; on failure print which
   `/proc/self/maps` entry overlaps `[ImageBase, +SizeOfImage)`.
3. **Map the whole image `RWX`.** Tighten per section only when needed.
4. **Bind every import slot** from `OriginalFirstThunk` (fall back to
   `FirstThunk` when 0) into `FirstThunk`. Compare module names with
   `strcasecmp`, function names with `strcmp`.
5. **Give each missing import its own stub** (`push "module!name"; call die;
   int3`) that names it and exits. Never share a trap that returns 0. Print
   `N bound, M missing` and drive the stage by that count.
6. **Give each thread a TEB.** Grep the disassembly for `fs:` and fake what it
   reads: `fs:[0]` (SEH chain, end marker `0xFFFFFFFF`), `fs:[0x18]` (self),
   `fs:[0x30]` (PEB), TLS slots. Allocate a page per thread, install it with
   `set_thread_area`, load `%fs` in a constructor that runs before `main`, and
   hook thread creation (`w32l_thread_attach`).
7. **Set the process environment the CRT expects.** `chdir` into the game
   directory, pass the command line through the layer, answer
   `GetModuleFileNameA` with the layer's canonical Windows path, and route
   every path through the layer's file functions (backslashes, drive letters,
   case-insensitive names).
8. **Install a fault handler** that prints EIP, whether it is inside the image,
   the registers and 16 stack dwords. Inside the image: look up the
   disassembly. Outside: suspect a bad import or a missing stub.
9. **Call the entry point.** It does not return.

### Verifying stage 1

- Reach the first real frame with zero missing imports, or with a named list.
- Log every import the game calls (`W32L_*_TRACE`) and build the layer from
  that traffic.
- Compare frames by hash on identical input: `W32L_LOCKSTEP=1`, `W32L_REPLAY`,
  `W32L_FRAMELOG`.
- Run with one canonical environment and `W32L_FIXED_HEAP` so allocations land
  at the same addresses every run.

### Known pitfalls

Read `docs/pitfalls.md` before writing a loader. Add new pitfalls there.

### Stubbing a whole module (`win32_layer/src/stubs.cpp`)

- Do not hand-write signatures for a DLL you are not implementing yet; let
  `w32l_stub_lookup` generate a stack-balancing, logging stub per name.
- Take argument bytes from stdcall-decorated names (`_AIL_open_stream@12`).
- Give ordinal imports a name and arg-bytes table (see `kWinsock`).
- Answer failure where zero means success (`socket` -> -1, `WSAStartup` ->
  `WSASYSNOTREADY`).
- Implement allocators for real (`AIL_mem_alloc_lock` -> `malloc`).
- Use the log of first calls as the work list for the real implementation.

### Middleware and libraries

When the game imports a third-party DLL (`mss32`, `smackw32`, `binkw32`), or
you need an outside library to stand in for one, read `docs/middleware.md`
first: which provider to use, how to wire it into the loader, and what its
licence allows. Stub the module (above) until then. Record each new provider
there.

## Project scaffolding

Create at the start: `CLAUDE.md` (from `templates/CLAUDE.md`, fill `<...>`),
`STATUS.md`, `docs/` (`00-recon.md`, `01-loader.md`),
`reconstruct/{README.md,LICENSE,Makefile,src/win32/}`, `tools/`.

- Keep `CLAUDE.md` to policy. Put status in `STATUS.md` and every finding in
  `docs/`, indexed from `CLAUDE.md`.
- Keep `reconstruct/` to the decompiled game: sources in `reconstruct/src/`,
  its own `Makefile`, `README.md` and `LICENSE`, no loader, layer or
  reverse-engineering detail.
- Put decompiled functions that call Win32 in `reconstruct/src/win32/`.
- Update this skill and `win32_layer` as you learn. With push access, push
  straight to `master`; without it, open a pull request (`CONTRIBUTING.md`).
- Keep `STATUS.md` near 100 lines: goal, numbers, rules in force, how to work,
  known defects, next steps. Replace stale numbers, delete what is done, move
  findings to `docs/`. Prune it when it passes 150 lines, after every
  milestone and before briefing agents.
- Brief agents with a short `docs/PORTING.md` (rules, tooling, how to derive a
  function). Do not tell them to read the status or this skill whole.

## Later stages

Read the file for the stage you are in.

| stage | file |
|---|---|
| Replacing functions piecewise and verifying a port (side by side, Lua control socket, hot patching, the hook DLL) | `docs/verifying-a-port.md` |
| What the first full port taught (four builds, differential testing, signatures, layouts, drift checks) | `docs/armymen2-lessons.md` |
| Running the original compiler on Linux (VC8 `cl.exe`, `link.exe`) | `docs/original-compiler.md` |
| Byte matching: ideas with their survival status, per compiler | `docs/byte-matching.md` |
| Register allocation, per compiler | `docs/vc6-regalloc.md`, `docs/vc8-regalloc.md` |

## The byte-match loop

Read with the `kuna-decompiler` skill, write with the original compiler,
compare bytes. Before the first function, read the header of
`docs/byte-matching.md` (*Read this first*: the rules and the symptom index).
Do not read that file whole; go from the index to a section, or grep it for the
instruction pattern. The register notes are `docs/vc6-regalloc.md` and
`docs/vc8-regalloc.md`.

1. **Identify the compiler from the Rich header** (product id and build per
   object count). Check that the extracted tool reports that build, and list
   every revision that stamps the same build (RTM, each service pack).
2. **Export once:** `kuna decompile-project game/X.exe --stream --jobs auto -o
   ../X-kuna/X.kuna`. Find the function in `index.jsonl`, read its
   `c_offset`/`c_len` block and its `.asm`. Check widths, signedness and calls
   against the asm, not the C.
3. **Transcribe** into `reconstruct/src/`, compile with the original flags,
   and compare the function's bytes with the image, relocation targets checked
   by name.
4. **On a mismatch, re-read the asm, then look the difference up in
   `docs/byte-matching.md`** (its symptom index, or grep the instruction
   pattern) before changing the C. Read the entries tagged for your compiler
   first; if none fits, try the other compilers' entries as hypotheses. When
   one of those matches, add your compiler to its tag (`[vc6]` becomes
   `[vc6, vc8]`); when it fails, note that under the entry. When a family of
   near misses resists every spelling, try the other compiler revisions before
   tuning further.
5. **Tune Kuna only when its C is wrong:** one `--option` at a time
   (`kuna docs options`); `--assert` for known prototypes (`--jobs 1`, no
   `--stream`).
6. **Keep a screen** (functions identical, bytes unmatched) and re-run it on
   every change. Count distinct addresses, after a clean build.
7. **Re-read section 1 of `docs/byte-matching.md` before trusting a score**
   or declaring a function unmatchable.
8. **Record every matching idea in `docs/byte-matching.md`, not here:** the
   compiler tag (`[vc6]`, `[vc8]`, `[vc8 rtm]`), the function, the figures
   before and after, and its status. Search the log for a later revert first.

## Where things live

| | |
|---|---|
| Layer | `https://github.com/borancar/win32_layer` (`README.md` lists the modules and `W32L_*` switches) |
| Reference loader | `https://github.com/borancar/ArmyMen2/blob/master/src/hybrid/loader.cpp` |
| Reference build | `ArmyMen2/Makefile`, target `hybrid` (`HYBRID_*` variables) |
| ArmyMen2 policy and findings | `ArmyMen2/CLAUDE.md`, `docs/`, `STATUS.md` |
| Commandos | `https://github.com/retro-foundry-games/CommandosBEL` (`game/` is the extracted GOG 1.1, build 23145) |
| Changes to this skill | `CHANGELOG.md` |
| How to contribute | `CONTRIBUTING.md` |
