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
  *Wiring OpenMiles into a loader (learned on Commandos, Miles 3.6F):*
  build it for `x86-linux-gnu` as `libmss32.so` (two small Zig patches: export
  the `AIL_*` names undecorated on x86 Linux, and an `@alignCast` in
  `midi.zig`), `dlopen` it from the loader and bind each `_AIL_x@N` import to
  `w32l_stdcall_adapter(dlsym("AIL_x"), N)` (`src/adapter.cpp`: re-pushes the
  args, 16-byte aligns, saves ebx/esi/edi). **OpenMiles' `.winapi` functions
  are real stdcall on x86 Linux (callee pops)**; a loader wrapper that calls
  one must declare it `__attribute__((stdcall))` or its own `ret` pops the
  wrong slot and jumps to 0. The game may carry a *static copy of Miles
  internals* and poke the driver/sample structs when the first
  `AIL_waveOutOpen` succeeds (Commandos: mode 1); OpenMiles' layout differs, so
  make that first open fail and the game falls back to the pure-DLL path
  (mode 2). `AIL_open_stream("\\\\<HANDLE>")` names an open file by its
  Win32 handle: map it with `w32l_handle_fd` to `/proc/self/fd/<fd>`.
  Link the loader with `-Wl,-z,now` is harmless but not needed.
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
- **Input is window messages for a game that does not import DirectInput.**
  Post `WM_MOUSEMOVE` (coalesced, once per pump), the button messages with
  the button state in wParam, `WM_MOUSEWHEEL`, and key/char messages; also
  keep `GetCursorPos` and `GetKeyState` truthful, because such games poll
  them each frame. A scripted replay can verify hover but not clicks; have the
  owner click by hand.
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

### Running the original compiler (Commandos: VC8 `cl.exe`)

To byte-match a reconstruction, run the compiler the game was built with, on
the same layer (ArmyMen2 did it for VC6 with its own shim; VC8 reuses the layer):

- **Get it without installing:** read the install disc's `.msi` (File /
  Component / Directory tables, column-major, string pool) to map each cab entry's
  File key to its install path, then `cabextract` the wanted ones (Commandos:
  `tools/msitables.py`, `tools/extract-vc8.py`).
- **Host:** a PE loader that applies base relocations and binds imports between
  the images it loaded (VC8 links the *real* MSVCR80/MSVCP80, and two DLLs share a
  base), host overrides ahead of the layer for LoadLibrary / GetProcAddress /
  PE resources / FormatMessage-from-module, `DllMain` in dependency order, an EXE
  entry run. `cl` runs `c1`/`c1xx`/`c2` in-process as DLLs.
- **Layer settings a tool needs, a game must not have:** `w32l_host_environment`
  (the real environment, not the canonical block), `w32l_os_version` (NT 5.1; a
  2005 CRT refuses the layer's default Windows 98), `W32L_FIXED_HEAP=0` (the
  deterministic heap arena covers the compilers' image bases).
- **Pitfalls found:** `GetModuleFileNameW` must give a backslash path (the tool
  cuts it at the last `\`); set the command line before the first image starts
  (the CRT's `DllMain` caches it); `cl` reads a leading `/` as a switch, so run
  from the source directory with relative names; a message table (`clui.dll`)
  needs `FormatMessage(FROM_HMODULE)` or every diagnostic prints blank.

- **`link.exe` is a second `vc8host` process** (`cl` cannot spawn it): compile,
  then link. A linked CRT test EXE runs. `FILE_MAP_ALL_ACCESS` has the copy
  bit; the write bit must win or link's mapped output stays zero.

### The byte-match loop (Kuna + the original compiler)

Use the `kuna-decompiler` skill for the read side, the original compiler for the
write side, and compare bytes:

1. **Pick the compiler from the Rich header** (product id / build per object
   count; Commandos: VC8 build 50727 for cl/link/cvtres, plus a few CRT objects
   from other builds). Check the tool you extracted reports that build.
2. **Export once:** `kuna decompile-project game/X.exe --stream --jobs auto -o ../X-kuna/X.kuna`
   (Commandos: 885 functions in about 5 s). Search `index.jsonl` for the function,
   read its `c_offset`/`c_len` block, and its `.asm` for exact instruction bytes.
   Treat the C as an estimate; widths, signedness and calls are checked against
   the asm.
3. **Transcribe** into `reconstruct/src/`, compile with `tools/vc8cl /c` using
   the original flags, and compare the object's function bytes against the image
   (relocations masked). A mismatch is a hint about source shape, not about the
   compiler: re-read the asm before changing the C.
4. **Tune with Kuna only when the C is wrong**: `kuna docs options`, one
   `--option` at a time on the affected function; `--assert` for known
   prototypes (needs `--jobs 1`, no `--stream`).
5. Keep a screen (functions identical / bytes unmatched) as ArmyMen2's
   `tools/relinkscreen.py` does, and re-run it on every change.

**Worked result (Commandos `ClassAUAgacharse`, 10 of 10 functions identical)** and
the shapes that moved the match: `cl /O2 /Gy /GR /EHsc` with the default `/GS`;
`class` not `struct` (RTTI name `.?AV` vs `.?AU`); an `int` parameter, not `bool`,
when the code loads a dword; argument pushes hoisted above an earlier call are
normal; a shared `[edx+4]/[edx+8]` tail is an inlined helper; a register holding
`1` across a static guard is `int r = 1; ... return r;`; compare with relocations
masked and their targets checked (symbols, string literals, jump-table labels, EH
funclets, thunks followed through the incremental-link `jmp` table). The comparer
is `tools/bytematch.py` in the Commandos repo; copy it.

**More shapes that moved the match (Commandos Sim*/ClassAccion, 94 identical):**
- A destructor often lives in its own translation unit; compile it separately
  or inlining changes the caller. A class pair compiled together can inline
  what the original did not (split `ClassAccion` / `ClassAcUsuario`).
- Implicit vs explicit dtor changes vtable slot 0 and the scalar-deleting stub;
  keep a switch (`SIM_DTORS_INLINE`) while testing.
- A repeated tail such as `if (guardar) {...}` is an inline helper taking
  `(int, Class*, void*, int)`; write it once and call it.
- A diff of a few bytes that is a register swap (edi vs esi) is usually a
  parameter/local order, not different logic; permute before rewriting.
- Struct copies compile differently as ints than as floats (`fld/fstp` vs
  `mov`); copy with the member types the original used.
- A field read pair load/store ordering (`fstp [this+x]` before a pointer load)
  is a scheduler choice that source reordering did not move; record such
  functions as known near misses (`~name addr` in `symbols.tsv`) and move on.

**Do the stateless functions mechanically.** Decode vtable-slot bodies that are fixed by their bytes (`ret N`, return constant, field get/set, `lea`, `fld [const]`) and generate the C++ from a table of byte patterns (Commandos `tools/gentrivial.py`: 491 functions identical on the first build). Do this before hand-matching anything bigger.

**Template factories are real templates.** `ctorAccionT<T>::Crear` is `return new T(a,b,c,d)`; read the size from the `push imm` before `operator new` and the ctor from the `call`, then instantiate the template explicitly. 82 of 84 matched on the first build (Commandos `tools/genfabricas.py`).

**More fixed-shape families (Commandos, +350 identical):** scalar deleting destructors (`call dtor; if (f & 1) operator delete(this); return this`) and bare vptr-store constructors can be recognised by byte pattern and emitted per function with a placeholder owner; map vtable addresses as `extern "C"` arrays in the symbol table. Skip stubborn near misses and keep generating.

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
- 2026-11-04: mouse and keyboard window messages in the layer; confirmed by hand on the Commandos menu.
- 2026-11-04: Commandos in-game sound through OpenMiles: calls succeed (samples, music stream started), audibility not yet confirmed by ear. Layer: `w32l_stdcall_adapter`, `w32l_handle_fd`, host PCM output.
- 2026-11-05: the VC8 `cl.exe` (14.00.50727.42) compiles C and C++ on Linux through the layer (`kernel32w.cpp`, `w32l_host_environment`, `w32l_os_version`); Commandos `tools/vc8host.cpp`.
- 2026-11-05: `link.exe` 8.00.50727.42 runs via `vc8host` (mapped-view write bit fix); byte-match loop written down (Rich header, Kuna export, original compiler, screen).
- 2026-11-05: first class byte-matched with the VC8 compiler (Commandos ClassAUAgacharse, 10 functions); lessons in 'The byte-match loop'.
- 2026-11-05: mass byte-matching with generators (Commandos, 2877 functions). Lessons: (1) a callee reached with `ecx` untouched is a member call on `this`; `edx` used where you'd expect `ecx` means `this` is live (write a method, not a free function). (2) A generator must run over *unmatched plus already-generated* addresses, else its output file loses earlier matches. (3) Group unmatched functions by their instruction text with immediates masked: shapes repeat 5-30 times (forwarders, accessors, deleting destructors, `Serializar`, constructors) and one generator per shape beats hand work. (4) `~name` in the symbol map marks a near miss so the count stays honest; try two source forms (`if(a==K)` vs `switch`, temp vs direct) before marking one.
- Tail jmp after 'mov [esp+4],x' => the wrapper declares the stack arg it forwards (callee cleans it).
- Placeholder class names across many small .cpp files collide (ODR); compile fine, but the COMDAT of one is dropped by the linker/bytematch. Check for duplicate class names after each batch.
- Inline strcpy-to-global with 'sub edx,eax' diff indexing: write 'char c; const char*p=s; do { c=*p; g[p-s]=c; p++; } while(c);'. Local 'r=0; if(c) r=&a[i]; return r' hoists the xor above the cmp.
- Imports: name an IAT slot as __imp__<Name>@<N> in symbols.tsv; declare __declspec(dllimport) __stdcall in src/win32/. Walk the PE import table to map slot address -> name (peimports.py prints names only).
- Branch layout: original 'je Lend ... Lend: xor eax,eax; ret' => 'if (c) { work; return 1; } return 0;' (not an early 'if (!c) return 0;').
- float t = a*a+b*b; return (float)sqrt(t); produces the fstp [esp]/fld [esp] round-trip before call __CIsqrt (single-precision rounding); without the local it keeps the value in st0.
- Null path ending 'xor eax,eax; ret' means the function returns int (return o->M(); return 0;) even if the result looks unused.
- Two conditional jumps to one out-of-line block at the end (after the epilogue) = 'if (a && (i = F()) != -1) {...} else G();' (single else), not nested ifs.
- 2026-11-05 (Commandos, +700 functions after the generators): (1) Factory `Crear`/`CrearEdit` whose epilogue is merged (one `ret`, no `xor eax,eax` on the null path) returns **void**; the compiler only materialises 0 when the result is used, so check the return type before hunting an expression shape. (2) Scalar deleting destructors: write the real `virtual ~D()` with an inline `virtual ~Base() {}` (else a call to it appears) and map `??_G..` plus `??_7..`; when the destructor body calls out, declare the callees and `operator delete` `throw()` or the compiler keeps a separate destructor with an EH frame instead of inlining it into `??_G`. The same `throw()` removes the EH frame from constructors that `new` members (`Nc::Nc` in tiny221). (3) Constructors with member arrays call the vector constructor iterator (`push dtor; push ctor; push count; push size; lea this+off; call`): declare the element class with ctor/dtor and the array as a member; stores before that call are member initialisers, stores after are the body. (4) `mov ecx,[global]` before a call means the global is a pointer (`extern T *g; g->M()`), not an object. (5) A tail `jmp` with the arguments untouched on the stack needs the callee to take exactly the caller's parameters; if the compiler caches them in registers it pushes copies instead. (6) Unsolved after many attempts: the `lea eax,[reg+1]` bool result, `ecx` where VC8 picks `eax`/`edx` for a temporary (about a dozen functions), `jmp eax` slot thunks (matched with `__declspec(naked)`).
- 2026-11-05 (Commandos, tiny291-305): (1) A plain `ret` in a function that has stack arguments means `__cdecl`; `ret N` means `__stdcall`/thiscall. Check before blaming the body. (2) A frame with a stack security cookie plus an unexplained 4-byte store (`mov dword [esp],0`) is a struct `{int h; char b[N];}` whose header is zeroed, not a plain `char buf[N]` (frame size and stack offsets differ by 4). (3) `new T(a)` with an EH frame in the original means the constructor is *not* `throw()`; the same call without a frame needs `throw()` on the constructor. (4) A float triple written with `fld/fstp [esp+..]` around pushes lists the elements in memory order of the pushed pointer: derive index from the pushed address, not from store order. (5) Pointer-difference loops (`(p-base)>>2 < n`) get rewritten by VC8 into an index induction variable; the original's `mov ebx,esi` shape did not reproduce from any source form tried (about four near misses).

### Register-allocation tricks (VC8 /O2), found on Commandos
- A `void` call followed by `pop ecx` vs `add esp,4`: the original's `add esp,4` means the call result is live, so make the callee and the caller return `int`.
- Original reloads a field into `ecx` where you get `eax`/`esi`: take the first access through a local pointer (`T *o = f4; o->...`) and the second through the member path, or reassign the local (`o = f4;`) before the second use.
- A tail `jmp` that reuses the argument slot means the callee has the same stack-argument count; pass the arguments straight through.
- Read destructor targets from the FuncInfo unwind map; a base needs a virtual destructor for the derived constructor to get an EH frame.
- Copy a member into a local before a call whose args are computed from it (`void **d = datos;`) to get the original's ecx/edx/eax order (ClassLista::Anyadir).
- Unsolved: a zero kept in a register across a compare (`xor eax,eax; cmp [m],eax`), pointer-difference loops.
- A call whose callee pops the stack (no `add esp` after) and where `ecx` was never loaded is a **thiscall on `this`** (ecx passes through): declare it as a member `N(...)`, not a `__stdcall` free function. Fixes odd register choices (edx instead of ecx) in forwarders.
- A tail `jmp` into a member keeps ecx; when a function reuses its argument slot (`mov [esp+8],-1; jmp`) the callee has the same stack arity.
- An inline accessor like `int N() { return f4 ? f8 - f4 : 0; }` reproduces register layouts that inlined source bodies do not.
- `#include <string.h>` under /O2 makes `strcpy`/`strlen` intrinsics: they expand to the `sub esi,src; mov dl,[eax]; mov [esi+eax],dl; add eax,1; test dl,dl; jnz` loop and the `lea edi,[eax+1]; ...; sub eax,edi` length loop. Do not hand-write those loops, and do not treat them as a pointer-difference mystery.
- `delete[]` of a `char *` compiles to scalar `operator delete`; call the CRT free (a separate address) through an extern when the original calls it.
- COM-style `Release()` shows as `mov ecx,[p]; mov edx,[ecx+8]; push p; call edx`: declare the virtual `__stdcall`.
- `M(F(), F())` pushes the right-hand call first: the first call's result is pushed immediately after it returns.
- **Store scheduling in zero-init constructors: use inline sub-object constructors.** When the original interleaves or groups float/int zero stores in an order the flat body cannot reproduce (fldz first, `xor ecx,ecx` placement, ints before floats), model the runs as small structs with inline constructors (`struct V3 { float x,y,z; V3() { z=0; y=0; x=0; } };`, `struct P { int a,b,c,d; P() { b=0; a=0; c=1; d=1; } };`). The member-init order then drives the schedule. If a sub-object init must sit between body stores, use an inline `Init()` method instead of a constructor and call it in the body at that point.
- **Library code in the image is reproducible from the real headers.** If the original links the same compiler's standard library headers (here VC8 `<string>`, `<fstream>`, `<locale>`, `<stdexcept>`), do not hand-write those functions: write a scratch TU that *uses* the library (`ifstream`, `string` ops, `runtime_error`...) and compile it with the same flags. The emitted COMDATs are byte-identical to the image. Then find them mechanically: for each COMDAT, look for unmatched image addresses whose bytes equal the object's outside relocated fields (`tools/stdscan.py`; allow the image size to exceed the code size, because unwind funclets follow the function), keep only unambiguous hits (unique name, unique address, >=20 bytes), and loop (`tools/stdloop.sh`) so callees found in one pass resolve relocations in the next. Resolve incremental-link `jmp` thunks to the real address before recording a callee. Tiny identical bodies (`ret`, `xor eax,eax; ret`) are ambiguous: skip them until a caller disambiguates.

- `member % N` compiles to `idiv`; copy the member to a local first (`int c = g->cnt; c % 40`) to get VC8's magic-number multiply.
- A struct passed by value stays a tail `jmp` unless it has a user-written copy constructor (then it is built on the stack and called).
- Halving ints is `>> 1`, not `/ 2` (`/ 2` adds the `cdq; sub` sign fix-up).

- Jump-table `switch`: decode the table (`tools/jt.py IMAGE TABLE COUNT FIRST_CASE`) to get case value -> code address; the emitted code order is the *source* order of the case labels, so order the cases by their code address, and grouped targets (`case 0x24: case 0x2b:`) share one body. Handlers that `ret 0xc` with a word second argument are `int f(unsigned short a, unsigned short id, int c)`; `int r = 0; ... r = 1; break; ... return r;` reproduces the hoisted `xor eax,eax`.

- A store followed by a reload of the same member (`mov [esi+14],3; mov eax,[esi+14]`) is reproduced by writing the store through `*(volatile int *)&f14 = 3; return f14;` (a volatile member also reloads, but then schedules the store earlier).

- A local buffer declared as `char data[N]` makes VC8 emit a /GS cookie the original lacks; declare it as N/4 `int` members instead. A `void` function ending in a `void` virtual call becomes a tail `jmp`; if the original calls and returns normally, declare the callee to return `int`.

- A search loop that reloads the array pointer every iteration and jumps from the failed first test to the increment is `do { if (o->At(i)) return i; i++; } while (i < o->n);` with an inline bounds-checked accessor; a `for` loop with the outer test strength-reduces instead.

### Array-new with a constant count (VC8)

`new T[11]` with a literal count compiles to a plain `operator new(44)`. The original's
`mov eax,0xb; mov edx,4; mul edx; seto cl; neg ecx; or ecx,eax; call operator new[]` (unfolded
overflow check) comes from passing the count through an *inline helper's parameter*:
`void Alloc(unsigned n){ f14 = new T[n]; ... } ... Alloc(11);`. Also: hoist a compared member into a
local (`int k = f4;`) when the original loads it before the loop's null test, and keep a result
in a variable (`int r = 0; ... r = 1; return r;`) when the original returns it from a register.
More VC8 shapes that matched: a callback called with `ecx` = object plus one stack arg is a pointer to
member (`typedef void (C::*CB)(int); (a->*cb)(b);`, mangled `P81@AEXH@Z`). A by-value 8-byte struct
argument (`sub esp,8; mov eax,esp; mov [eax],..`) comes from a temporary with a user copy constructor
(`f(S(a,b))`). `delete[] p` of a type with a destructor emits the `__vec_dtor` call (give the class
`~T();` at the destructor's address). A float argument passed with `push ecx; fstp [esp]` is just the
slot for the float, not a stdcall. A COM-style slot call that passes `this` on the stack is a
`virtual ... __stdcall` method. Dead locals the original keeps (`mov [esp+..],0`) are `volatile`.

## Byte-matching idioms (VC8 /O2), learned on Commandos

- Write `p->x` repeatedly instead of hoisting `o = p`; the compiler then reloads into the dead `this` register.
- A by-value struct argument needs a user copy ctor that copies float-wise; a pair copy via an inline-ctor temporary gives loads-then-stores.
- Operand order `key == arr[i].d` picks the `cmp r,m` encoding.
- `int r = 0; if (f == r) r = B(); return r;` gives the tail jmp with zero in eax.
- A ternary store `f = o ? o->g : 1;` gives two return paths.
- An index loop `for (int j=0;j<8;j++)` gives signed `jl`; a pointer-end loop gives `jb`.
- A member array of a class with non-inline ctor and dtor makes the compiler emit the vector-ctor call.
- Thiscall returning a 32-byte object by value: `T o = a->G("KEY");`.
- Config readers (`G` then `M`/`N`/`P`) follow one template and can be generated from disassembly (`tools/genprops.py`).
- When store order differs, hill-climb pair swaps of the store statements until the byte difference is 0 (`tools/reorder_hc.py`).
- Never delete symbol lines by prefix `sed`; filter by exact name (`grep -vF`).
- **eax-vs-ecx intermediates:** if the original chains pointer loads through `ecx` where we use `eax`, an earlier call's return value is probably still live in `eax` (the function returns it on a fall-through path). Make the callee int-returning: `int r = a->F(); T *t = ...; if (t) r = t->F(); return r;`.
- **Constants held in a register:** `int r = 1; if (!A() && !B() && !C()) r = 0; return r;` makes VC8 hoist the 1 into a callee-saved register (`mov edi,1` up front, `mov eax,edi` on the hit path). Preset the result to the common value and overwrite it on the other path. A two-case `switch (b) { case X: f(); break; case Y: g(); break; default: r = 1; } return r;` with `int r = 0;` reproduces `movzx/sub/je/sub/je`.
- **Serializer idiom (stream `m ? W : R`):** the original picks the stream's read or write vtable slot with one merged call (`test m; lea eax,[this+X]; push n; push eax; jz; mov eax,[edx+8]; jmp; mov eax,[edx+4]; call eax`). Reproduce it with an inline *member* of the stream class, `void RW(int m, void *p, int n) { if (m) W(p, n); else R(p, n); }`; a free helper, a macro or an `if/else` at each site does not merge. A `PUSH EBP`/`PUSH EBX` appearing mid-function is a shrink-wrapped callee-saved save, not a stack argument. Pointers the original keeps in `ebp` (`lea ebp,[this+K]` then `[ebp+4]`, `[ebp+0x48]`) need a local pointer in the source (`int *gp = &g; ... gp + 1`). Reloading `this->f1c->f190` after a call means two separate expressions, not a cached local. Delete-then-read of a counted array: `RW(&cnt,4); if (!m) delete[] arr;` (not an `if/else` around the R/W calls).
- **Virtual slot by index:** declare N dummy virtuals so the real one lands at slot N. When the same slot is called with differing argument counts, declare the zero-argument one as the real virtual and call the others through a `__thiscall` function-pointer cast of `(*(void ***)this)[slot]`.
- **Callee is a member of `this` that the caller does not reload `ecx` for:** the argument loads go through `edx` (ecx stays live as `this`). If you wrote a free function and see `ecx` where the original has `edx`, make the callee a member.
- **COM-style interface calls** (`push args; push obj; call [vtbl+N]`, callee pops): declare dummy `virtual void __stdcall dK();` up to slot N and then `virtual int __stdcall Name(args)`.
- **`Sub *t = &s;` as a local** before using a sub-object several times makes VC8 hoist `lea edi,[this+off]` to the prologue (find-or-add helpers on a list member).
- **`int c = a && b; if (c) ...; ...; if (c) ...;`** gives the out-of-line `xor reg,reg` flag block; `if/else c = 1/0` inline gives a `jmp` instead.
- **String literals:** a compared function that pushes a string address needs a symbol line for the literal (`??_C@_..@text?$AA@  0xADDR`, take the mangled name from the generated `.asm`), otherwise the literal looks like a mismatch.
- **Caller-cleaned thiscall** (`this` in ecx, stack args, `add esp,N` after the call) cannot be written: a varargs member passes `this` on the stack and `__thiscall` + `...` is error C2217.
- **`ecx` used after a call that clobbers it**, `mov eax` vs `mov ecx` scratch choice before pushes, and the `lea eax,[reg+1]` bool result survive all flag sweeps (`/Ox /O1 /Os /Ot /Ob2 /G6 /G7`): suspect the original was built with `/GL`.
- The Makefile compiles every `src/*.cpp`, parked files included, so keep them compilable; and `grep -v Zzxa` also removes `Zzxa1` lines (use exact symbol names).

- **Store-then-`sub [mem],reg` per field = inlined struct `operator-=`.** Writing
  `f3c = x; f40 = y; f3c -= f4c; f40 -= f50;` makes VC8 fold into `sub reg,mem`
  plus one store. The original's `mov [m],eax; mov eax,[o]; sub [m],eax` comes from
  a small struct with `Pt &operator-=(const Pt &o){x-=o.x;y-=o.y;return *this;}`
  applied to member sub-structs.
- **Small stores into a local struct or parallel arrays: permute.** When only the
  scheduling of 3-4 independent stores differs, try all orderings (24 for four).
  Cheap and often the match (0x64e750, 0x5d67a0, 0x64d180).
- **Wrong tails from if/else chains: try `switch(msg)`** (0x4b4720, 0x568530).
- **int copy in one case, `fld/fstp` in another: a `union{int i; float f;}` member**
  (0x459fe0).
- **Virtual calls are thiscall.** Declare `virtual void S(int,int,int)` without
  `__stdcall`, and pad the vtable with dummy virtuals to reach the slot (0x581e60).
- **Register assignment follows local declaration order.** Reordering locals
  steers which of esi/edi/ebx a value gets (0x593d50).
- **More than ~4 stores: hill climb.** `tools/hcstores.py NAME` swaps statements
  between `// BEGIN`/`// END` pairwise and keeps improvements (0x5655d0, 0x59d5b0).
- **A function that returns "whatever is in eax":** make it return the pointer and
  reassign it through the call result (`r = r->f; if (r) r = P(r); return r;`, 0x456690).
  A `void` version hoists nothing and misses by a few bytes.
- **Shared return on both paths**: `r = 0; if (g) { r = A(); p = r; if (p) {...} r = p; }
  return r;` steers `mov eax,esi` as the jump target (0x5d6fa0).
- **Still unsolved:** eax/ecx/edx scratch choice before pushes, esi/edi swap, the
  zero-register idiom (`lea eax,[reg+1]`), float argument-slot reuse (0x58acf0).
- **vptr store first in a constructor**: when the original stores the vptr before the
  member zeroing, model the zeroed members as sub-objects with inline constructors;
  a flat `f = 0;` body lets VC8 sink the vptr store below them (Commandos 0x49e880).
- **Derived destructor restoring a middle vtable, then calling the base out of line**:
  the middle class has an inline empty virtual destructor; only the root's is out of
  line (ClassWindowsVentana/Dialogo -> ClassWindowsReceptorMensajes -> ClassReceptorMensajes).
- **`throw()` on a destructor** removes the EH frame when its body calls through a
  function pointer (a C++ function-pointer call may throw; an `extern "C"` call does not).
- **`mov eax,[a]; or eax,[b]; jne`** is `!(a | b)` on two ints (not `!a && !b`, not `__int64`).
- **A pair of stores kept in source order across branches**: an inline `Reset()` on a
  two-field sub-struct (0x5d5a00); flat stores get reordered.
- **`lea edi,[eax+1]` after `lstrlenA`**: keep `n = lstrlenA(s)` and write `n + 1` at each use.
- **A parameter's stack slot reused for a local** (`lea ecx,[esp+0xc]` = the argument slot)
  is VC8's own doing once the parameter lives in a register; declare a normal local.
- **The comparer "learns" unknown relocation targets**, so a wrong import (SendMessageA
  for PostMessageA) can match once and break later; check IAT slots by name.
- **Owners of non-virtual helpers**: many small members are dead code (no caller, no
  data reference). Their class is the one whose vtable-slot functions sit next to them
  (one object file is one contiguous run under incremental linking). Commandos
  `tools/owners.py` combines vtable membership, callers (through the `jmp` thunk table)
  and neighbours.
- **Renaming at scale**: put `// 0xADDR` on each definition and regenerate the symbol
  map from the compiled listing (PROC + first `; Line N`) plus the comparer's `learn`
  lines (Commandos `tools/symsync.py`); never hand-edit mangled names.
- **Deleting generator stubs can lose a unique match.** A one-line stub in a generated
  file may be the only definition at a *derived* override's address (same body as the
  base's, a different address). Diff the matched address sets before and after
  (Commandos `tools/distinct.sh -l`, with `git stash` for the old state) instead of
  trusting the total.
- **Tail-jump wrappers do not show their parameters.** `mov ecx,[ecx+K]; jmp F` forwards
  the caller's stack arguments untouched, so the body says nothing about the parameter
  list; take it from the base class's slot (`ret N`) and forward them (`v->F(a)`).
- **A `thiscall` free-function lookalike**: a placeholder written as `__stdcall f(a,b,c,d)`
  that calls a member through an untouched `ecx` is a 4-argument member (`ret 0x10`);
  rewrite it as one.
- **Vector deleting destructors (`??_E`)** are emitted only where the vtable is (a TU with
  a constructor); a TU with an explicit `delete` emits the scalar `??_G`. The same class
  can have one of each at different addresses.
- **DirectDraw `IDirectDrawSurface` slot 32 is `Unlock`** (`push 0; push surf; call [vtbl+0x80]`).
- **Inherited fields across a real base**: when a member of the base class (by vtable)
  sets a field, move the layout into the base (`class Derived : public Base`) rather
  than casting; nested types then mangle as `Base::Nested`, so regenerate their symbol
  lines.
- **Empty base slot, value-returning overrides**: when the base's slot is a bare `ret N`
  but the overrides return a value (an older compiler allowed a missing `return`), declare
  the slot with the return type and give the base an empty body under
  `#pragma warning(disable: 4716)`; VC8 then emits the bare `ret` (Commandos OrganoMotor
  slot 32). Before reaching for it, check the base really is bare: a stub "with no
  return" may simply have lost its `mov eax,1`.
- **Never run a line-number-based delete twice without rebuilding**: the second pass reads
  stale listings and removes the wrong definitions (Commandos lost 9 matches once;
  `tools/defs.py` now refuses when a listing is older than its source).
- **A pointer stored right before a `sprintf("Estado=...")`** that looks like a vtable may
  be a string literal (the class name): read the bytes at the address before modelling it.
- **eax/ecx scratch after a conditional call**: when the original reloads a stack argument
  into `eax` (ours `ecx`) after `if (x) F();`, load the first argument into a named local
  before the condition (`int id = *(int *)b; if (t.M(id)) F(); Base(b, c);`). Fixed a
  long-parked Commandos function (0x4721d0).
- **Temporaries vs scoped named locals**: a function that builds the same temporary object
  twice (`F(T("x")); if (p) p->G(T("x"));`) may have been written with named locals in
  scopes; the image passes `lea eax,[esp+N]` (the local's address) instead of the
  constructor's returned `this`, and the slots come in source order.
- **Paired stores from two registers** (`xor eax,eax; xor edx,edx; mov [a],eax; mov [b],edx`,
  or `mov eax,K; mov edx,eax` before two stores) are a small struct built by an inline
  constructor and copied: `*(Par *)&f = Par(0x8000, 0x8000);`. The constructor's
  assignment order decides which field gets which register; two pairs filled in opposite
  orders need two struct types.
- **`movzx reg32, byte [p+2]; and reg32, 0xfc`** reads the low byte of a 16-bit field:
  declare the field `unsigned short` (an `unsigned char` field gives `and dl` + `movzx dx`).
- **`switch` labels:** `case 3: f(); break; ... case 5: f(); break;` and `case 3: case 5:`
  lay out differently; a range test `a > 0 && a <= 2` becomes one unsigned compare while
  the image's `jle`/`jg` pair is `switch (a) { case 1: case 2: ... }`.
- **Loop bounds over a global table** compared with `jl` (signed) mean the source cast the
  pointers to int: `(int)p < (int)(tabla + 4)`.
- **`mov ecx,[eax+K]` straight into a call** (no `mov ecx,eax`): keep the intermediate
  object, not the final pointer, in the local (`PantallaJuego *p = G(); p->visor->F();`).
- **Learned addresses of tables can be the end**: when the first relocation to a global
  array is its end bound, the comparer learns `base + size`; check learned data symbols.
- **A build that fails leaves old objects**: count matches only after a clean `make match`.
- **`mov ecx,N; repe cmpsb`** against a string literal is VC8's inline `strcmp(s, "LIT") == 0`
  (N = strlen + 1); `memcmp` gives a dword loop instead.
- **Float constants in `.rdata` (`fld [addr]`) are compiler `__real@` literals**: map them
  by value (`__real@44a56000` = 1323.0f) and order the source assignments so each array
  slot gets the constant the image loads first for it.
- **A base call after an `if`**: when a branch target lands on `mov ecx,esi; call Base`,
  the base call runs on every path; move it out of the `if` (several "almost" matches
  were this).
- **`mov eax,K` followed by the K stored twice or a `jmp` to a shared tail**: VC8 sometimes
  duplicates a short tail where the image jumps to one; if case reordering and `goto`
  don't fix it, park.
- **A loop pointer aimed at a field, reading the struct's other fields at negative offsets
  (`test [esi],1` ... `push [esi-4]`), or two pointers stepping together**: the source indexed
  the array (`for (i = 0; !(a[i].flags & 1); i++) ... a[i].dato`); a `T *p` walk gives a
  pointer to the struct's start instead.
- **Two constant stores to adjacent fields come out in the reverse of source order** in
  some functions: if the image stores +0xdd8 then +0xdd4, try writing them the other way.
- **`fadd DWORD [2^32]` after `fild`** is an unsigned-to-float conversion in a `float`
  expression; a `double` expression uses the QWORD constant.
- **Early `return` inside a nested `if`** (`if (m == X) { if (a != Y) return 1; f = 0; return 1; }`)
  lays out a shared `mov eax,1` the plain `if (a == Y) f = 0; return 1;` does not.
- **Two signed compares per range (`cmp eax,2; jl; cmp eax,6; jg`)**: VC8 folds
  `t >= 2 && t <= 6` into one unsigned compare (`lea esi,[eax-2]; cmp esi,4; ja`) when
  `t` is a local; written on the member (`tipo >= 2 && tipo <= 6`) it keeps both compares.
- **A register used as a known value** (`sub edx,1; jne; cmp [eax+0x10],edx`): the source
  compared with the same variable (`int t = o->tipo - 1; if (t == 0 && o->campo10 == t)`),
  not with the literal.
- **`fld [x]; fld QWORD [K]; fucom st(1)`** (a double constant and an unordered compare)
  is `K != x` with the literal on the left; `x != -1.0` folds to a float constant and `fcom`.
- **A struct argument copied with plain `mov`s and compared from the stack** is a
  by-value struct parameter (`Poner(int id, float v, Posicion p)`); fld/fstp copies of a
  by-value argument come from a user-written copy constructor.
- **A local for a pointer chain before a call** (`ClassMano *m = actor->mano; m->Usar(&x)`)
  swaps which of ecx/edx gets the object and the argument.
- **A duplicated call whose argument is a register known to be 0** (`push eax` after
  `test eax,eax; jne`) where ours pushes `0`: the source had one call after an if/else
  (`if (c && !r) f |= 8; else f &= ~8; Acelerar(r);`) and the compiler duplicated the tail
  late, after constant propagation. An early `Acelerar(r); return;` in the branch lets it
  fold r to 0.
- **A pair passed by value from two adjacent members** (`mov edx,[ecx+0x20]` reloaded after
  comparing it) is a struct copy through a cast (`f(*(Par *)&a1)`), not `Par v; v.a = a1; ...`.
- **An "override" with a different parameter type is a new virtual, not an override**: it
  appends slots and shifts every later one, and nothing complains until a virtual call
  past it lands on the wrong offset (`call [eax+0x70]` where the image has `+0x68`). Keep a
  derived class's redeclarations on the interface's exact signature and cast inside.
- **x87 compares: VC8 loads the left operand first.** `fld [a]; fld [b]; fcompp` comes from
  `a OP b` (and a literal on the left gives a double constant, see above); if ours loads them
  the other way, swap the operands and mirror the operator (`f1 < f3` -> `f3 > f1`).
- **A value-returning slot whose image leaves eax unset**: keep the declared return type and
  write no `return` (C4716 off); `return 0` adds an `xor eax,eax` the image lacks.
- **Local struct for two ints that only exist as arguments** (`sub esp,8` reserved, one field
  spilled, the other passed from eax) is `struct { int x, y; } s; s.x = ...; s.y = ...; f(s.x, s.y)`.
- **A returned temporary kept by pointer in a callee-saved register** (`call Valor; mov edi,eax;
  ... mov ecx,edi; call Cuenta`) is the temporary handed by reference to an inline helper
  (`__forceinline int NumeroODefecto(Bloque &v, int d) { return !v.Cuenta() ? d : (int)v.Numero(); }`
  then `x = NumeroODefecto(b->Valor("K"), 180)`); a named local gives `lea ecx,[esp+N]` per call,
  and `&b->Valor(...)` destroys it at once.
- **Member calls that leave ecx alone look like `__stdcall` calls** (`push ...; call X` with no
  `mov ecx`, `ret N`): before parking such a "stdcall" function, try it as a member calling
  members of `this`.
- **A tail duplicated only in part** (each branch has its own `cmp [x],0; jne; test eax; je;
  store` prefix, then they join): the source called one inline helper from each branch
  (`if (a) return Esperar(c1); return Esperar(c2);`); inlining copies the helper's head into
  both paths and tail merging joins the rest. A plain `int c = a ? c1 : c2; ...` gives one copy.
- **An address comment far from its PROC's first `; Line`** happens when instructions are
  scheduled before the body's first statement; a sync tool should accept the nearest comment
  above unless another definition starts in between.
- **`lea ecx,[esp+N]` before a member call on a returned object** (instead of `mov ecx,eax`)
  is a named local, not a temporary: `{ T v = b->Valor("K"); v.CopiarA(p, n); }`, one block
  per value so each destructor runs right away.
- **Our guessed class name clashing with an RTTI name**: an RTTI name always wins; rename the
  guessed class (and its mangled names in the symbol map) instead of working around it with
  file splits.
- **An inline function the image has both inlined and out of line** (a header constructor):
  keep it `inline` in the header behind a macro (`#define X_INLINE inline` unless defined) and
  let one source file define the macro empty before the include to emit the one copy.
- **A template member defined in a header gets inlined into callers the image calls it from**:
  mark the definition `__declspec(noinline)`; its own bytes do not change.
- **A loop that reloads `*cur` for the call** (`mov eax,[esi]; ... mov ecx,[esi]; call`) calls
  through the iterator (`(*it.cur)->F()`), not through the value it tested.
- **Register order flips with `a = b = x` vs `b = a = x`**: the assignment chain decides which
  register keeps the value and which takes the copy.
- **`new T[n]` calls `operator new` unless `operator new[]` is declared.** VC8 only
  emits a call to `??_U` (and `delete[]` to `??_V`) in files that declare them (`<new>`);
  elsewhere both go to `??2`/`??3`. A CRT-looking allocation that builds an EH frame
  around a base constructor is `new char[...]` with `operator new[]` declared (it can
  throw; `extern "C"` `malloc` cannot under `/EHsc`). Declare `operator delete[]` with
  `throw()` or every destructor that calls it gains an EH frame.
- **A function that leaves an argument in eax on every path** and does its work in other
  registers returns that argument (menu key handlers: `return c;`).
- **Two byte-identical out-of-line copies of an inline destructor** are the implicit
  destructors of two classes (a subclass with nothing to destroy compiles to its base's
  body); map each name to its own copy.
- **A base constructor or destructor inlined in one subclass and called in another**: put
  the bodies in the header behind an opt-out macro and define it in the files whose image
  calls them.
- **`c ? a : 0` compiles branchless** (`setne; sub; and`); the image's `xor eax,eax; cmp;
  jne; mov eax,a` is `int r = 0; if (c) r = a;`.
- **Repeated range tests on a struct (`x >= r.x && x < r.x + r.w && ...`)** allocate
  registers differently from the same tests through an inline helper
  (`int Dentro(int x, int y)`): when hand-written tests are off by registers, try the helper.
- **Two locals the image keeps apart** (frame bigger than ours by one slot) can be a variable
  declared at function scope and assigned later, rather than one declared in the inner block.
- **Object files group a class's functions**: when a member compiles inlined where the image
  calls it (or the reverse), check which class's address range it sits in and put it in that
  file (or a header with an opt-out macro).
- **Sweep the vtables for lone `jmp` slots.** List every unmatched vtable slot whose code is a
  single `jmp` (or `mov ecx,[ecx+N]; jmp`), resolve the target through the thunk table, and
  write each as a one-line override (`return Base::F(args);` or a call to a non-virtual helper).
  Dozens of slots fall out this way; a tail call with no `mov ecx` means the target uses the
  same `this` (a base member, or a sibling helper of the same class).
- **A type whose destructor maps to another class's destructor is that class.** When a
  by-value return type's destructor resolves to an existing class's, rename it everywhere
  (and the mangled names in the symbol map); nothing else needs to change.
- **`fnstcw; or 0xc00; fldcw; fistp qword; fldcw` inline** (instead of a call to
  `__ftol2_sse`) is VC8's conversion of a float to `unsigned int`.
- **A field loaded into a register and tested, then used** (`mov ecx,[x]; test ecx,ecx; je;
  ... call [ecx]`) is a local (`T *v = s->campo; if (v) v->F();`); testing the member
  directly reloads it after the test.
- **Temporaries in conditions:** a temporary created in `a && f().g()` lives to the end of
  the full expression, so VC8 guards its destructor with a flag (`or ebx,2; test bl,1`). When
  the image destroys it right after use without a flag, the test was its own statement
  (`bool hay = false; if (a) hay = f().g() != 0;`).
- **A struct copied through a stack temporary as plain dword moves** (`fstp [tmp]; mov
  edx,[tmp]; mov [dst],edx` per field) is `*p = T(a, b)` with a constructor; assigning the
  fields directly stores them with `fstp [dst]`.
- **All argument temporaries built before any is used** (`Token(1)`, `Token(0)`, then the
  numbers) means they were passed by reference to a `__forceinline` helper; arguments are
  evaluated right to left, the helper's body then runs in its own order.
