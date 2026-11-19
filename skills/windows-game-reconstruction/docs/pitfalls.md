# Known pitfalls of the hybrid layer (stage 1)

Things that cost time while loading an original Win32 executable over
`win32_layer`. Referenced from `SKILL.md`, stage 1. Add new ones under the game
that found them.

## From ArmyMen2

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

## Found booting a second game (Commandos)

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

## From "imports bound" to the first menu (Commandos)

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

## Dialogs and resources (Commandos)

A game's editor and debug dialogs are `CreateDialogParam` / `DialogBoxParam` over its own `RT_DIALOG`
templates. The layer (`README.md`, "Dialogs") runs them; what to do in a port:

- **Tell the layer where the image is.** `w32l_pe_image = <mapped image, headers included>` in the loader (and in
  a native build that copies the exe's data sections: the `.rsrc` section must be among them). `FindResource`,
  `LoadString`, `LoadMenu` and every dialog read the resource directory from that memory; with it unset
  `CreateDialogParam` logs `no such RT_DIALOG resource` and answers NULL.
- **Survey the templates before writing the controls.** Walk the resource directory (type 5) in a script: the
  classes in use (ordinals 0x80..0x85, a few custom ones), the style bits, and the font. Commandos' 60 dialogs use
  only the six standard classes (plus one tree view and one track bar), so no common-controls library was needed.
  Grep the reconstruction's `SendMessage`/`SendDlgItemMessage` constants (`LB_ADDSTRING` 0x180, `CB_SETCURSEL`
  0x14E, `BM_SETCHECK`, ...) to see which messages matter.
- **A dialog is a window you can look at.** Dumping each dialog to a PPM (`W32L_DIALOGDUMP`) shows at once whether
  the constructor and its `WM_INITDIALOG` handler ran and filled the controls (a half-run constructor shows empty
  lists and default texts). Drive it with the replay verbs `dlgclick`, `dlgtype`, `dlgkey`, `dlgdump`.
- **Order matters for input.** Give a clicked control the focus when the click is generated, not when the
  posted `WM_LBUTTONDOWN` is dispatched: keys typed right after go to the focus at post time.
- **Command handlers are vtable slots.** In the native build the first click can end in `no native code for
  0x408bbb thunk to 0x...`: the dialog's own `WM_COMMAND` handler (a slot of the editor class) has no native code
  yet. The dialog itself, its controls and the constructor do not need it.

## The original's libm is not glibc's (Commandos, 2026-11)

A 1990s msvcrt's `sin`, `cos`, `tan`, `atan`, `asin`, `acos`, `pow` and `log10` are Intel table-driven SSE2 routines (an entry
tests a CPU-feature byte and jumps to them); `atan2` is x87 `fpatan`. glibc's results differ in the last bit for 0.1 to 0.6
percent of inputs (atan2: one in 4,500 calls with x < 0), and a game whose positions or headings follow them diverges from the
original in a lockstep comparison after a few hundred frames. `sqrt`, `floor`, `ceil` and `fmod` are exact. What to do: find the
CRT entries by their float-library shape, measure the rate with a random-input differential test (call the original address in
the hybrid, the port in the native build, compare the returned bits), then carry the original's routines over: generate their
instruction streams from the exe at build time (tables from its `.rdata`), send the error and out-of-range paths to glibc, and
write the x87 ones as a stand-in (`fpatan`, `pi - r` in extended precision, round to double at the end). A float conversion
hides most differences, so measure with double consumers. Without the original at build time glibc is the fallback and the build
is no longer bit-exact. Details and the measured rates: Commandos `docs/14-libm.md`.

## A name with no address and no definition links, then aborts when the code runs

A native build that stubs unresolved names (a trampoline that aborts when called) hides every missing destructor: the compiler's
scalar deleting destructor (`??_G`) calls `??1Class` by name, the image's own function is the body behind `??_G` (or is inlined into
it), and nothing in the source or the symbol table ties the two. It shows up at exit or at the first `delete`. Make an unresolved
game name a build error (exempt only import entries and an allowlist file whose lines say why the name is never called), and resolve
each one from the image: alias the name to the function `??_G` calls, define the body where the image inlines it, or fix a declaration
that invented a virtual destructor (read slot 0 of the vtable from the exe; it is often a message handler or an accessor). Do not
exempt `??1` and `??_` names to quiet the list.

## One name, several layouts: per-file class views meet at the native link
Byte matching compiles every source on its own, so each file declares the view of a class it needs (a stand-in with the fields it
touches, a full local class for a constructor match). The native link merges same-named symbols: one constructor, one destructor,
one `??_G` for all views. Two checks, both cheap, after every round that adds a local class with a real constructor:
- **Functions**: list every symbol defined by more than one object and compare the bodies (the first in link order wins among weak
  copies, so run it in link order and in any other order: a result that depends on the order is a bug waiting for a new file name).
  The usual offender is a constructor view that leaves the virtual destructor implicit: its `??_G` calls the base's destructor and
  skips the class's own. Declaring `virtual ~T();` in the view is byte-neutral for the constructor.
- **Types**: dump the record layouts per source (`clang -Xclang -fdump-record-layouts`) and compare them per name. Most differences
  are harmless partial views, so rank by what can overflow: read the image's `push SIZE; call operator new` in front of each
  `call constructor` (a base constructor inlined into a derived one reports the derived size: take the smallest) and flag a plain
  `new T` whose view is smaller than that, and a *ported* constructor whose view is larger. Two different types that merely share a
  name (a path point and a route point) get distinct names. The image's sizes are also a matching aid: a view larger than the
  allocation has a wrong tail.
