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
