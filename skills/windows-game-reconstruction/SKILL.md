---
name: windows-game-reconstruction
description: Reverse-engineer a 1990s Win32 (PE32 i386) game and reconstruct it for Linux without Wine. Use when running an original Windows game executable on Linux through a custom PE loader and the win32_layer (Win32/DirectX over SDL3), when binding a PE's import table to host implementations, when building a hybrid (original code + reconstructed code) build as a correctness oracle, or when porting a Windows game piece by piece. Living document: extended as each stage is reached.
---

# Reconstructing a Windows game on Linux

This skill is the method distilled from **ArmyMen2** (`~/git/retrofoundry/ArmyMen2`),
from which `~/git/win32_layer` was extracted. It is applied next to
**Commandos: Behind Enemy Lines** (`~/git/retrofoundry/Commandos`).
It is a living document: when a stage teaches something, edit it here
(see *Changelog* at the end). Sibling skill for DOS games: `dos-game-reconstruction`.

The shape of the work: **the original binary keeps running, and is replaced
function by function.** The original is the oracle; nothing is "done" because
it looks right on screen. The first stage below is what makes that possible.

## Stage 1: the hybrid layer (current goal)

**Goal:** a Linux program that loads the original Win32 `.exe` into its own
process and runs it **with no Wine and no Windows code**, every import the
game makes landing in `win32_layer`. Boot to the title screen with the
original's own CRT startup, unmodified.

```
 Linux i386 ELF (loader + libwin32_layer.a + SDL3)
   |-- mmap the PE image at its ImageBase (fixed address)
   |-- copy headers + sections
   |-- bind the IAT: slot = address of OUR implementation of module!name
   |-- set up fs (TEB) so SEH push/pop works
   |-- chdir to the game directory, set the command line
   `-- call AddressOfEntryPoint (the CRT's WinMainCRTStartup)
```

Reference implementation: `ArmyMen2/src/hybrid/loader.cpp` (~800 lines, of
which the loader proper is `WinMain`, `bind_imports`, `teb_attach`,
`make_stub`, `fault`). The layer side is `win32_layer/src/kernel32.cpp`:
`w32l_export_lookup(module, name)` is the table the loader asks.
### Standing rule: loaders are per game, the platform is shared

**Each game gets its own loader** in its own repo (ArmyMen2's
`src/hybrid/loader.cpp`, Commandos' own). Loaders are small and game-shaped:
image placement, the TEB the binary actually reads, detours, retail patches,
log hooks, trace switches. Copying the ArmyMen2 loader as a starting point and
bending it to the next binary is expected; do not contort a shared loader to
serve every game.

**The platform implementation is common and shared**: everything a loader
binds the IAT *to* (kernel32, user32, gdi32, ddraw, dsound, dinput, dplay,
winmm, the SDL host, the fixed heap, the CRT shims) lives once, in
`~/git/win32_layer`, and is consumed by every game, never forked. When
Commandos needs an import ArmyMen2 did not, **add it to `win32_layer`**, in
the same style and behind the same `w32l_export_lookup` table, and keep
ArmyMen2 building and agreeing with its oracle. A game-specific behaviour goes
behind a hook in `platform.h` (`w32l_app_info`, `w32l_host_*`), not an
`#ifdef GAME` inside the layer. If two loaders end up with identical code
(PE mapping, import binding, stubs), that is the cue to lift that part into
the layer as an optional helper, but only after the second game has proven
the shape.

### Recipe

1. **Triage the PE** before writing anything (`objdump -p`):
   - `Machine 0x14C`, `Magic 0x10B` (PE32 i386). Anything else, stop.
   - `ImageBase` and `SizeOfImage`. If the **relocation directory (entry 5) is
     empty** the image can only live at `ImageBase`: every absolute address in
     the code is then a stable VA, which is also what makes address-keyed
     reverse engineering possible. If it has relocations you may rebase.
   - Import directory: list every `DLL!name`. This is your work list. Count by
     DLL; the system DLLs (`kernel32 user32 gdi32 advapi32 shell32 ole32 winmm
     wsock32 shlwapi`) and the *third-party* ones (`mss32`, `smackw32`,
     `binkw32`, `ddraw` via COM) need different answers.
   - Ordinal imports (`0x80000000` bit set): resolve by ordinal table. `dsound`
     `#1` is `DirectSoundCreate`; `wsock32` imports are mostly ordinal.
   - Debug directory / PDB: usually absent. Do not plan on symbols.
   - Packer or protection (SecuROM, SafeDisc, UPX): if the entry point is not
     in `.text`, or the import table is tiny, solve that first; the loader
     cannot run a packed image.
2. **Place the ELF out of the way.** The ELF and the PE share one 32-bit
   address space. Use `MAP_FIXED_NOREPLACE` for the image and make the PE
   mapping fail loudly if the address is taken. ArmyMen2's image ends at
   `0x667000` and a non-PIE i386 ELF at `0x00700000` clears it. **A bigger
   image will collide**: Commandos' `comandos.exe` spans `0x400000-0x7E9000`,
   so the ELF must go higher (a linker script or `-Wl,-Ttext-segment=`, e.g.
   `0x10000000`), and the loader should check `[ImageBase, +SizeOfImage)`
   against `/proc/self/maps` and say which mapping is in the way.
3. **Map the whole image `RWX`** at first (the CRT writes its own `.data`,
   the IAT sits in `.rdata`, detours rewrite `.text`). Tighten per-section
   protection later only if something needs it.
4. **Bind every import slot** from `OriginalFirstThunk` names (fall back to
   `FirstThunk` when it is 0) into `FirstThunk`. Use `strcasecmp` on the
   module name (`KERNEL32.dll` vs `kernel32.dll`) and `strcmp` on the function.
5. **Make an unprovided import loud, not silent.** Give each missing import
   its own 16-byte stub (`push "module!name"; call die; int3`) that names it
   and exits. A shared trap that returns 0 turns `CreateFileA` into a game
   that cannot say what is wrong. The binding pass prints `N bound, M
   missing`; that count is the progress meter for the whole stage.
6. **Give the thread a TEB.** MSVC CRTs and SEH read `fs:[0]` (chain head,
   end marker `0xFFFFFFFF`) and `fs:[0x18]` (self). Allocate a page per
   thread, install it with `set_thread_area`, load the selector into `%fs`
   from a constructor that runs before `main`, and hook thread creation
   (`w32l_thread_attach`) so game threads get their own. Check what the binary
   actually reads: ArmyMen2 touched only `fs:[0]` at 340 sites. Other games
   read `fs:[0x30]` (PEB), `fs:[0x18]`, or `fs:[0x20]`/TLS slots; grep the
   disassembly for `fs:` before deciding how much TEB to fake.
7. **Match the process environment the CRT expects**: `chdir` into the game
   directory (the original reads its install dir with `getcwd` or relative
   paths); the command line through the layer; `GetModuleFileNameA` returning
   the layer's canonical Windows path; Windows paths (`C:\...`, backslashes,
   case-insensitive) translated by the layer's file functions. Linux's
   case-sensitive filesystem is a real hazard for assets named `FASE0013.WAD`
   vs `Fase0013.wad` — the Commandos installer ships mixed-case names.
8. **Install a fault handler** that prints the faulting EIP, whether it is
   inside the image, all general registers and 16 stack dwords, then
   continues the search. A crash at an address inside the image is a
   disassembly lookup; one outside means a bad import or a missing stub.
9. **Call the entry point.** `WinMainCRTStartup` ends in `ExitProcess` and
   does not return.

### Verifying stage 1

Boot reaches the first real frame **with zero missing imports**, or with a
named list of missing ones you are working through. Then:
- log every import the game actually calls (a `W32L_*_TRACE`-style switch in
  the layer) so the layer is built from observed traffic, not guesswork;
- compare against the same exe run under Wine on an identical input
  (`W32L_LOCKSTEP=1`, `W32L_REPLAY`, `W32L_FRAMELOG` give deterministic,
  hash-comparable frames);
- keep one canonical environment and fixed-address heap
  (`W32L_FIXED_HEAP`) so allocations land at the same addresses every run,
  otherwise differential comparison drowns in pointer noise.

### Known pitfalls (from ArmyMen2)

- 64-bit host types: the port targets i386 deliberately (`-m32`), because
  the original assumes 4-byte pointers and `long == 32`. Stay 32-bit for the
  hybrid. Use `<stdint.h>` fixed-width types in reconstructed code.
- Calling conventions: the image's functions are `__cdecl`, `__stdcall`,
  `__fastcall` or bare register conventions. Layer exports must match
  (`callconv.h`, `WINAPI`). A mismatched `stdcall`/`cdecl` corrupts the stack
  one call later, far from the cause.
- Some call sites reach a function with **no argument frame** (a stubbed
  logger folded onto a bare `ret`). Validate pointers before using them.
- Detours: copy a whole instruction boundary (>= 5 bytes), `jmp` back,
  and leave the original body running, so the original stays the oracle.
- Patched retail builds exist (GOG removes or alters code). Patch back only
  when the expected byte is found, and say so when it is not.

### Stubbing a whole module (shared layer, `src/stubs.cpp`)

When a game imports a middleware or subsystem DLL you are not implementing
yet, do **not** hand-write signatures. `w32l_export_lookup` falls through to
`w32l_stub_lookup`, which generates a function per name that balances the
stack, answers a fixed value and logs its first call.

- **stdcall-decorated names carry their own arg bytes** (`_AIL_open_stream@12`
  pops 12), so a whole module such as `mss32.dll` is covered with no table.
  Get this wrong and the stack is corrupted one call later, far from the cause.
- **Ordinal imports** (`wsock32`) need a name/arg-bytes table (see `kWinsock`).
- **Answer failure, not zero, where zero means success** (`socket` -> -1,
  `WSAStartup` -> `WSASYSNOTREADY`) so the game takes its no-network path.
- **Allocators are real** (`AIL_mem_alloc_lock`/`free_lock` -> malloc/free): a
  zero there is a NULL the game dereferences.
- The first call of every stub is logged. That log is the work list for the
  real implementation.

### Third-party providers and licensing

- **Miles (`mss32`)**: OpenMiles (`github.com/maci0/openmiles`) is the real
  provider. It is **GPLv3**. **Decided (2026-11-04): the Commandos project is
  GPLv3, and `win32_layer` is GPL v2 or (at your option) any later version**
  (stated in its README; the repo carries only the v2 `LICENSE` text) so the
  two combine, the combined binary being GPLv3. ArmyMen2 vendors a GPL-2.0 copy
  of the layer and is unaffected until it takes the new one. Do not copy
  OpenMiles code into the layer itself; link it from the game.
- **Smacker (`smackw32`)**: libsmacker, as ArmyMen2 does. Commandos does not
  import it (its films are `.avi`).

### Pitfalls found booting a second game (Commandos)

- **Where the ELF goes.** It shares the address space with the PE *and* with
  the layer's fixed heap (`0x0C000000-0x16000000`). ArmyMen2's `0x700000` hits
  a big PE; `0x10000000` hits the heap (`fixedheap: cannot map the heap at
  0xc000000: File exists`). Link at `0x30000000` (clear of both, leaves
  brk's ASLR slack). Re-derive from `SizeOfImage` and the layer's constants.
- **`GetProcAddress` must answer NULL for what is not there.** The MSVC 7+
  CRT (`_mtinit`) probes `FlsAlloc/FlsFree/FlsGetValue/FlsSetValue` and falls
  back to `Tls*` only on NULL. A trap that answers non-NULL makes it call a
  function that is not there (symptom: a call to `0x00000000` from the CRT).
  The layer claims Windows 98 SE, which has no `Fls*`. Check which CRT the
  exe was built with by reading its startup, not by assuming the 1998 one: a
  re-released exe (GOG 2018 here) can be a modern rebuild.
- **Many imports are one-liners the layer already had.** Before writing a
  function, check whether it is implemented but missing from the export table.
- **Two stubs for the same name are a bug**; `w32l_export_lookup` falls
  through to `w32l_stub_lookup` only after the table.

### From "imports bound" to the first menu (Commandos)

- **Window extra bytes.** C++ window classes store `this` with
  `SetWindowLong(hwnd, 0, this)`. If `cbWndExtra` is not honoured every message
  goes to `DefWindowProc` and the game looks dead. Make `RegisterClass` /
  `Get/SetWindowLong` keep the extra bytes.
- **Find the next missing thing with an import tracer.** The loader can bind
  every import through a thunk that logs the call and its return address
  (`pushad; lea eax,[esp+32]; push eax; push info; call trace; add esp,8;
  popad; jmp fn`); `CMD_TRACE=1`. The last lines before an exit are the cause.
  Filter the message-loop noise (`PeekMessage`, `Tls*`, `Critical*`).
- **Look at the screen without a screen.** `SDL_VIDEODRIVER=dummy`,
  `W32L_FRAMELOG` (a hash per presented frame) and `W32L_FRAMEDUMP_AT` /
  `_TO` (a PPM of frame N). Convert and read it as an image. Use
  `env -u DBUS_SESSION_BUS_ADDRESS` (SDL blocks on the session bus) and
  `timeout -s KILL` (games ignore SIGTERM).
- **Colour depth is a property of the platform, not of the layer's
  implementation.** A game from 1998 may insist on 16-bit RGB565 (message:
  "DIRECTX 5 NOT FOUND"). `w32l_display_bpp` makes the display mode,
  `GetDeviceCaps`, the surface format and the present path agree. Also
  implement `CreateClipper` and `EnumDisplayModes`.
- **A real C runtime is picky about error codes.** `_open` on an empty file
  probes `SetFilePointer(-1, FILE_END)` and accepts only
  `ERROR_NEGATIVE_SEEK` (131); anything else ends in `_invoke_watson` and
  `TerminateProcess(0xC000000D)`. When the original dies in the CRT, strace
  the file calls, then match the Windows error code exactly.
- **Per-user folders must be real and writable** (`SHGetFolderPath`
  returns `./userdata/...`, created on demand), or the game's first config
  write fails.
- **Video is probably not Smacker.** Read the game's COM calls: a
  `CoCreateInstance` of `CLSID_AMMultiMediaStream` means DirectShow
  multimedia streaming. Disassemble the function around the call
  (`objdump -D -b binary -m i386 -M intel`) and list the vtable offsets it
  calls: that is the exact set of methods to implement (`amstream.cpp` has
  them for Commandos). Cinepak is simple enough to write from the format
  description (`cinepak.cpp`); an AVI's `movi` list has nested `LIST rec `
  groups.
- **The registry cannot be faked.** Implement it for real in the layer
  (`registry.cpp`) and persist it as a TOML file in the current directory
  (`registry.toml`, or `$W32L_REGISTRY`), read with tomlc99
  (`third_party/tomlc99`) and written by a small emitter. A stub that answers
  "not found" for everything hides what a game stores, and a stub that
  answers "ok" for everything makes it read garbage. REG_SZ is a TOML string,
  REG_DWORD an integer, everything else `["REG_x", data]`. Ask the owner how
  each type should be represented if the game needs one the layer does not
  handle yet.

## Project scaffolding and CLAUDE.md

Every game repo gets a lean `CLAUDE.md` that carries these standing rules
(`templates/CLAUDE.md` in this skill is the starting text; fill `<...>`):

- `CLAUDE.md` stays **lean**: policy only. Status goes in **`STATUS.md`**,
  every finding in **`docs/`** (indexed from `CLAUDE.md`).
- **Platform shared, loader per game** (see Stage 1).
- **`reconstruct/`** holds only the decompiled game: sources under
  `reconstruct/src/`, `reconstruct/Makefile`, its own `README.md` (no loader,
  layer or reverse-engineering detail) and its own `LICENSE`.
- **Win32-specific functions** (decompiled functions that call Win32) are
  extracted into **`reconstruct/src/win32/`**, so the rest stays
  platform-neutral and the layer seam is clean.
- **Keep updating this skill and `win32_layer`, and push** straight to
  `master` on `win32_layer`.

Create the scaffolding at the start: `CLAUDE.md`, `STATUS.md`, `docs/`
(`00-recon.md` for installer/PE/assets, `01-loader.md` for loader and
platform gaps), `reconstruct/{README.md,LICENSE,Makefile,src/win32/}`, `tools/`.

## Stage 2 and later (to be written as reached)

Planned, in order. Replace each stub with what was learned:
1. Boot a second game (Commandos) through **its own loader** over the
   **shared** layer. Record every import it needs that ArmyMen2 did not
   (`mss32`, `wsock32`, `shlwapi`, `shell32`, `ole32`, `advapi32`, more of
   `ddraw`/`winmm`) and which the layer had to grow.
2. Third-party middleware DLLs (Miles Sound System `mss32.dll`): provide a
   host implementation over SDL audio, or zero-answering traps like
   `smackw32` is today, decided per game by what it needs to start.
3. Per-routine replacement with differential checks (hybrid as oracle).
4. Frame-lockstep verification, asset format work, and so on.

## Where things live

| | |
|---|---|
| Layer | `~/git/win32_layer` (`README.md` lists the modules and `W32L_*` switches) |
| Reference loader | `~/git/retrofoundry/ArmyMen2/src/hybrid/loader.cpp` |
| Reference build | `ArmyMen2/Makefile`, target `hybrid` (`HYBRID_*` variables) |
| ArmyMen2 policy and findings | `ArmyMen2/CLAUDE.md`, `docs/`, `STATUS.md` |
| Commandos | `~/git/retrofoundry/Commandos/game/` (extracted GOG 1.1, build 23145) |

## Changelog

- 2026-11-04: created. Stage 1 specified from the ArmyMen2 loader.
  Commandos triage so far: `comandos.exe` is PE32 i386, base `0x400000`,
  no relocations, `SizeOfImage 0x3E9000`, imports DDRAW, WINMM, mss32,
  WSOCK32, SHLWAPI, KERNEL32, USER32, GDI32, ADVAPI32, SHELL32, ole32.
  `comandos_w10.exe` is byte-identical to it. Image collides with the
  `0x700000` ELF placement ArmyMen2 uses; the ELF must move.
- 2026-11-04: standing rule from the user: loaders may be per game; the
  platform implementation (`win32_layer`) must be common and shared.
- 2026-11-04: Commandos loader started: `~/git/retrofoundry/Commandos/src/loader.cpp`
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
