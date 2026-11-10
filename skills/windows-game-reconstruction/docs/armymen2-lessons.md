# Lessons from ArmyMen2 (the reference port, `retro-foundry-games/ArmyMen2`)

Referenced from `SKILL.md`, later stages.

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
