---
name: windows-game-reconstruction
description: Reverse-engineer a 1990s Win32 (PE32 i386) game and reconstruct it for Linux without Wine. Use when running an original Windows game executable on Linux through a custom PE loader and the win32_layer (Win32/DirectX over SDL3), when binding a PE's import table to host implementations, when building a hybrid (original code + reconstructed code) build as a correctness oracle, or when porting a Windows game piece by piece. Living document: extended as each stage is reached.
---

# Reconstructing a Windows game on Linux

This skill is the method distilled from **ArmyMen2** (`~/git/retrofoundry/ArmyMen2`),
from which `~/git/win32_layer` was extracted. It is applied next to
**Commandos: Behind Enemy Lines** (`~/git/retrofoundry/Commandos`).
It is a living document: when a stage teaches something, edit it here
and record the change in `CHANGELOG.md` beside this file. Sibling skill for DOS games: `dos-game-reconstruction`.

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
- **Prune `STATUS.md` regularly; it must stay short (about 100 lines).** It
  holds only the current state: the goal, the numbers (matched, ported, hooked,
  still original), the rules in force, how to work, the known defects and the
  next steps. Do not append a log line per session or per batch: replace the
  stale numbers, delete what is done, and move findings to `docs/`. The old
  text stays in git history. A status file that grows without bound becomes a
  bottleneck: every new agent reads it first, and on Commandos it reached
  285 KB (about 70k tokens) and used up agent context before any work started.
  Prune it whenever it passes about 150 lines, after every milestone and
  before briefing agents, and brief agents with a short `docs/PORTING.md`
  (rules, tooling, how to derive a function) instead of telling them to read
  the status or this skill whole.

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

## Verifying a port: side by side, a Lua control socket, the Win32 boundary

Learned from the Commandos port goal (2026-11-10). The test of a port is not a
recording of the original; it is **the original (hybrid) and the port running
side by side, live, on the same input**, compared as they go.

- **Side by side, not recorded.** Do not build oracle files of the original's
  traces or frames to diff later. Start the hybrid (the original over the layer)
  and the port together and compare them every step; the first difference is the
  lead, and both programs are still alive and inspectable on it. The layer
  already provides the lockstep: `W32L_LOCKSTEP=1` (deterministic clocks, a fixed
  step per pump) and `W32L_STEP=<unix socket>` (each program waits for a
  coordinator before every pump and reports `pump N`, each presented frame's
  hash and the host events it applied; the coordinator answers with replay
  actions and `step`). `W32L_FRAMELOG` (a hash per frame) is the cheap check,
  `last FILE` / `W32L_FRAMEDUMP_*` writes the frames that differ. A coordinator
  of this shape is `Popcorn/sidebyside.py` (emulator vs port) -- the same flow.
- **Lua scripting over a control socket instead of command-line scripted input.**
  Drive and inspect the game with Lua lines sent over the control/step socket,
  executed on the game thread at a pump boundary (`w32l_host_lua`, the `lua`
  verb), so an injection lands on the same pump in both programs and they stay
  frame-exact. The environment worth exposing: peek/poke guest memory at image
  addresses, key/click/move injection, frame hash and dump, the pump number.
  (A `W32L_REPLAY` file or a command-line script is only good for hover and
  screenshots; scripted clicks did not register in Commandos, real events and the
  socket do.) A loader sets `w32l_host_lua`; the retail layer has none, so a game
  that needs it brings a (32-bit) Lua in its loader.
- **Send input as Win32 messages from Lua, not as X/SDL host events.** Do not
  synthesize X events or SDL events to drive the game (an xdotool-style script or
  a fake host pointer is another source of divergence, and a headless run has no
  real window to receive them). Have Lua post the window messages and set the
  input state the game reads, at the pump boundary, through the same entry points
  the layer's host event handlers use: `PostMessage`-style `WM_MOUSEMOVE`,
  `WM_LBUTTONDOWN`/`UP`, `WM_KEYDOWN`/`UP`, `WM_CHAR`, `WM_MOUSEWHEEL` (client
  coordinates, with the wParam/lParam the real message carries), plus the
  DirectInput keyboard/mouse state and `GetCursorPos`/`GetKeyState` answers for a
  game that polls instead of using the queue. Both programs get the identical
  message on the identical pump, so a click lands the same way in the hybrid and
  in the port; Lua can also read back the queue and the cursor to confirm it was
  delivered. (Commandos: the layer posts `WM_MOUSEMOVE` and the button messages;
  a scripted click from the replay file did not register where a real one did,
  which is the reason to send the whole sequence, move then button down and up
  on later pumps, from Lua.)
- **The Win32 boundary is ours: use it from both Lua and gdb.** Every call the
  game makes to the platform goes through the layer, so it is the one place to
  stop, inspect and inject without touching the game's code: log or break at the
  import thunks (`CMD_TRACE`-style thunks in the loader, with the return
  address), read the arguments and the stack there, and take a **backtrace** by
  scanning the stack for return addresses (an optimised image has no frame
  pointer chain: keep words that point into `.text` just after a `call`). From
  Lua, the same points are hooks (`on_call(name, args)`); from gdb, break on the
  thunk (`break *thunk`, `commands`, `x/32wx $esp`) on either program, the
  hybrid or the port, at the same pump. The boundary tells which functions the
  game uses on a path (callers of the calls it makes), what a port must call in
  the same order, and where the two programs first diverge.
- **Hot patch functions in the hybrid with the reconstructed versions.** The
  hybrid keeps the original image mapped; write a `jmp` at an original function's
  entry to the reconstructed one and compare frames side by side with the
  untouched hybrid. The memory must match between the two: every global,
  vtable and string the reconstruction owns has to sit at the ORIGINAL address
  (a linker script, or absolute symbols generated from `symbols.tsv`), and
  externs to the original's data are plain absolute addresses into the
  still-mapped image. Watch the ABI when the reconstruction is built with a
  host compiler: member functions are `thiscall` (this in ecx) and need the
  attribute; MSVC vtable/destructor layout differs from Itanium, so a class with
  virtuals must store the original vtable address itself. Start with non-virtual,
  non-constructor functions and widen from there.
- **The proven shape (ArmyMen2, `retro-foundry-games/ArmyMen2`): no Wine, the hybrid is the host.**
  Replace functions piecewise and check each against real gameplay; keep the
  reconstruction's code high (`-Wl,-Ttext=0x00700000`) and carry the game's DATA at
  its own original addresses with a generated linker script (`place.ld`,
  `origchunks.S`, `origgap.S`), so patched and unpatched code and Lua `peek`/`poke`
  see the same memory; drive and inspect through the control socket and a Lua
  console (`peek8/16/32`, `poke8/16/32`, `peekstr`, `call(addr, ...)`, `sym(name)`;
  `docs/lua.md`, `src/inject/luaconsole.c`); compare with `tools/sidebyside.py` +
  `docs/lockstep.md`; audit conventions with `tools/checkabi.py` (ecx read plus
  `ret N` is thiscall) and never reorder a vtable the original indexes. Any
  injection harness that needed Wine there is, with the hybrid, a patch module the
  loader applies.
- **The loader and DLL approach, no Wine.** The reconstruction is built as a hook DLL
  with the ORIGINAL compiler and linker (hosted natively on Linux, `vc8host`/`vc8link`
  style), so its code has the original's ABI, vtable layout and calling conventions by
  construction. Our own loader maps the original EXE (the hybrid), then maps the hook
  DLL into the same process: apply its base relocations, bind its imports (the layer
  for Win32, and the loader for a few host entry points), run its `DllMain`/install,
  which writes a `jmp` at each replaced original function's entry. This is what
  ArmyMen2's `am2hook.dll` did under Wine; with the hybrid there is nothing to
  install Wine for. The DLL reaches the original's data by absolute address (the EXE
  stays mapped), owns no duplicate of it, and its constructors store the ORIGINAL
  vtable addresses for classes the unpatched code also uses. A host (gcc) port comes
  later from the same sources; the DLL proves function by function that the sources
  and the data map are right, with frames compared side by side against the untouched
  hybrid.
- **A port must not lean on the original executable.** It carries its own
  structures and globals; assets are data. Port existing reconstructed
  functions only (no invented functions); keep the byte-match baseline
  (`make match`) from falling while porting.

## Lessons from ArmyMen2 (the reference port, `retro-foundry-games/ArmyMen2`)

The first game taken all the way (every function reconstructed; ~99.6% byte-identical
under the original VC6; the native port frame-exact against the original on 16 of 16
recorded sessions). Read its `STATUS.md`, `CLAUDE.md`, `docs/lockstep.md`,
`docs/verification.md`, `docs/lua.md`, `docs/01-harness.md`, `docs/reconstruction-lessons.md`.

- **Four builds, each answering a different question.** *Native* (no original code at
  run time); *hybrid* (the original EXE over the same layer, no Wine: the behavioural
  oracle, and it exercises the layer too -- its first run found two layer defects);
  *injected harness* (a DLL that patches the reconstruction into the original one
  function at a time; Wine there, a loader patch module when you have a hybrid);
  *original-compiler relink* (the original compiler hosted natively, every function laid
  at its original address, every relocation resolved as the original linker would, bytes
  compared with the image; a runnable image from the real linker at other addresses).
- **Piecewise, never big-bang.** The original keeps running while its functions are
  replaced one at a time, each checked against real play before the next; once the
  Win32/DirectX boundary is closed, the same sources link standalone, then natively.
- **Differential testing in the game's own process.** A self-check calls each
  reconstruction and the original it replaces side by side, with the same pointers and
  globals, BEFORE the patch is installed (a patch overwrites the first bytes with a jmp
  and leaves no trampoline, so that moment is the whole chance). Offline: emulate the
  original (Unicorn), record input->output vectors, replay them on the C++. Where the
  input space is small, enumerate it (an oracle) instead of sampling. **Check the test
  by mutating a function and watching it fail** (swap `min` for `max`); a harness that
  fed every pointer argument the same bytes (periodic fill vs pointer stride) passed
  for a long time.
- **Take every signature from the CALL SITES**, not the body: a body shows the arguments
  it uses, only a caller shows how many (`ret N`). Audit conventions mechanically
  (`checkabi`: ecx read + `ret N` = thiscall, ecx+edx = fastcall; `xor ecx,ecx` and `or
  ecx,-1` read nothing). Ask "which record is this, HERE" for every offset; a field
  pointer named as a table base is the commonest mistake; a field nothing writes is dead
  code; print the whole literal pool at once; an exit noted is not an exit reproduced.
- **Layouts the original sees are the original's** (vtable slot order, structs the image
  carries, import ordinals): "nothing indexes it today" is a claim about today's callers.
  Use the real SDK headers, never restate Windows/DirectX structures. Fixed-width
  `<stdint.h>` types everywhere in reconstructed code; no shorthand `#define`s for reads
  (write the expression at each use); declare a global as the TYPE the original reads.
- **A static analysis suite plus a drift check** (`make check`): tools that need no game,
  generated files recomputed and compared, and a claims checker that recomputes the
  numbers quoted in the docs so prose cannot go stale. One launch target with variables
  (`ID`, `TRACE`, `ARGS`), runs independent of each other (ports, logs, screenshots
  derived from `ID`).
- **Keep the game believing what it believed.** It asked for exclusive full-screen
  640x480 8-bit; the platform answers success and gives a window, because those code
  paths (a full palette, the full-screen blit) are what every comparison exercises.
  Platform pieces with no host equivalent are recorded from the original once (fonts
  read out of the original under Wine into a reference file, replayed by the layer).
  Differences that are by design are listed (films play in the port, trap in the hybrid).
- **Debugging a frame difference:** a difference inside the pointer's box is history
  before it is rendering (move the cursor away on both sides); look for what both sides
  have in common before blaming the renderer. Both programs stay alive at the first
  differing frame for inspection (the side-by-side coordinator).
- **Provenance.** If a leaked source for a sibling game exists, take names and struct
  layouts only and derive every piece of logic from the disassembly; never copy it into
  the repo or quote it.

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

### Byte-matching ideas, and which of them lasted

`docs/byte-matching.md` (beside this file) is the catalogue of source shapes,
methods and searches that moved a byte match, each tagged with the compiler it
was measured on (`[vc6]` from ArmyMen2, `[vc8]` and `[vc8 rtm]` from Commandos)
and with what became of it: **held**, **number win**, **reverted/superseded**
or **no effect**. Read its first section, *Local optima*, before trusting a
score: a respelling that was not equal for every input, an instruction the
original does not have, hacks that went stale, and a "ceiling" that was a
missing struct all lowered the number first.

Every matching idiom lives there, none here: the measured VC6 register model
is its section 7 (long form, with the experiments: `docs/vc6-regalloc.md`), the
VC8 register observations are `docs/vc8-regalloc.md`, and
the raw VC8 record (found in order, no status yet) is its section 14. When a stage teaches a matching idea, add it there, not here:

- Tag it with the compiler (`[vc6]`, `[vc8]`, ...). An idea from one compiler
  is a hypothesis on another.
- Give the function and the distance before and after.
- Before recording a win from history, check whether it was later reverted or
  replaced (search the log for the function name, "drop", "restore", "back to",
  and distances that went up), and record that status.
- When a stalled function is rewritten from the image and the old levers are
  discarded, go back and mark those levers superseded.
