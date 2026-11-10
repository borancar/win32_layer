# Verifying a port: side by side, a Lua control socket, the Win32 boundary

Referenced from `SKILL.md`, later stages.

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
