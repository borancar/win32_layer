# win32_layer

The Win32 and DirectX surface a late-1990s Windows game calls, implemented
over [SDL3](https://www.libsdl.org) for Linux: no Wine, and no Windows code at
run time. It was written for, and extracted from,
[ArmyMen2](https://github.com/borancar/ArmyMen2) — a source reconstruction of
*Army Men II* (3DO, 1999) — where it carries both the reconstructed game and,
under a small PE loader, the original `ArmyMen2.exe` itself.

It also launches the original `comandos.exe` of *Commandos: Behind Enemy
Lines* (Pyro Studios, 1998; GOG build) through a per-game PE loader
(a Commandos project that is not published yet): it runs the
intro video and the main menu, with 16-bit RGB565 DirectDraw, mouse and
keyboard window messages, and a registry kept in a TOML file. The layer is
shared by both games; each has its own loader.

It is not Wine and does not try to be. It provides what that game uses, as
faithfully as an A/B against the original needs: frame-exact rendering,
deterministic timing on demand, and the same heap answers on every host.

## What is here

| | |
|---|---|
| `src/kernel32.cpp`, `src/kernel32crt.cpp` | files, handles, threads, events, critical sections, heaps, the command line and environment, and the import table a PE loader asks by module and name (`w32l_export_lookup`) |
| `src/user32.cpp` | windows, the message queue, focus, cursor, `MessageBoxA` (to stderr) |
| `src/resource.cpp` | the program image's resources (`FindResource`, `LoadResource`, `LoadString`, `LoadMenu`), read from the PE the loader mapped (`w32l_pe_image`) |
| `src/dialog.cpp`, `src/dlghost.cpp` | dialogs: templates (`DLGTEMPLATE` and `DLGTEMPLATEEX`) built into dialog and control windows, the standard controls with their messages, one SDL window per dialog, see "Dialogs" below |
| `src/gdi32.cpp` | device contexts, palettes (with the 20 Windows static colours under `SYSPAL_STATIC`), fonts drawn with stb_truetype |
| `src/ddraw.cpp` | DirectDraw: 8-bit palettised or 16-bit RGB565 surfaces (`w32l_display_bpp`), palettes, clipper, Blt/BltFast with colour keys, Flip paced to 60 Hz |
| `src/amstream.cpp`, `src/cinepak.cpp` | DirectShow multimedia streaming for a game that plays AVI films through `CLSID_AMMultiMediaStream`: an AVI reader, a Cinepak decoder, MS-ADPCM sound |
| `src/registry.cpp` | the Windows registry, persisted as `registry.toml` in the current directory (tomlc99 in `third_party/`) |
| `src/stubs.cpp` | zero-answering stand-ins for whole modules (`mss32`, `wsock32`) a program imports but whose real implementation is not here |
| `src/dsound.cpp` | DirectSound buffers mixed by one host mixer; winmm's `mmio` and multimedia timers |
| `src/dinput.cpp` | DirectInput keyboard and buffered mouse |
| `src/dplay.cpp` | DirectPlay, enough to answer "no session" |
| `src/sdl.cpp` | the SDL host: window, presentation, input, audio device, clocks, lockstep and replay, and `main()` |
| `src/fixedheap.cpp` | a fixed-address heap, so a run's allocations land at the same addresses every time |
| `src/crt.cpp` | host C-runtime shims with Windows paths translated |
| `include/` | `windows.h`, `ddraw.h`, `dsound.h`, `dinput.h`, `dplay.h`, `mmsystem.h`, ... — declaring exactly what is implemented, so a new dependency is a compile error rather than a silent missing function |

Exclusive full-screen modes are accepted and not enforced: a program that
asks for 640x480 in 8 bits gets a window of that logical size and keeps
running its full-screen code paths. `smackw32.dll` is listed among the
emulated modules so that a program importing it gets zero-answering traps
(no films) rather than a failed load.

## Using it

```sh
make                                   # build/libwin32_layer.a
```

32-bit only (i386): the programs it serves assume 4-byte pointers. It needs an
i686-capable `g++` and SDL3's i686 development files (on Fedora,
`glibc-devel.i686`, `libstdc++-devel.i686`, `SDL3-devel.i686`).

Compile the application against this layer's headers and link the library:

```sh
g++ -m32 -isystem include -include callconv.h -c app.cpp
g++ -m32 app.o build/libwin32_layer.a -lSDL3 -lpthread -lm
```

The application provides:

- **`WinMain`**, declared in `include/windows.h`. The layer's `main()` sets up
  SDL and calls it.
- **`w32l_app_info`** (`src/platform.h`): its name (window title and SDL app
  name), version, reverse-DNS identifier, and the Windows path
  `GetModuleFileNameA` answers. The layer's own definition is weak and names a
  generic program.

Optional hooks are function pointers in `src/platform.h` the application may
set: `w32l_host_frame_hook`, `w32l_host_hotkey`, `w32l_host_cursor_set`/`_query`,
`w32l_host_lua`, `w32l_thread_attach`. Defining `W32L_DEVTOOLS` when compiling
the layer builds a development variant: F5 and F9 go to `w32l_host_hotkey`
instead of the program, and `W32L_AUDIO_DUMP` works.

## Dialogs

`CreateDialogParam`, `DialogBoxParam` (and the `Indirect` and wide forms), `EndDialog`, `GetDlgItem`,
`Get/SetDlgItemText/Int`, `SendDlgItemMessage`, `CheckDlgButton`, `CheckRadioButton`, `IsDialogMessage`,
`EnableWindow`, `ShowWindow`, `DestroyWindow`, `Get/SetWindowLong` (`DWL_MSGRESULT`, `DWL_USER`,
`GWL_USERDATA`), `GetWindowRect`/`MoveWindow` and the rest of the window calls work on dialogs and on their
controls. A program's resources come from its own PE image: a loader sets `w32l_pe_image` (platform.h) to
where it mapped the image (headers included) and `FindResource(NULL, name, RT_DIALOG)`, `LoadString` and
`LoadMenu` read its `.rsrc`. A program that sets none finds no resources and its dialogs fail as before.

What a dialog is:

- Windows `HWND`s of a dialog or a control are small integers (0x10000 and up), looked up in a table, so a stale
  one finds nothing. The program's own window keeps its handle; `user32.cpp` hands a dialog's or control's call
  over to `dialog.cpp`.
- The standard controls are real: `BUTTON` (push, default, check, radio, group box, auto variants), `EDIT`
  (single and multi line, `ES_NUMBER`, read only, password, selection, clipboard), `STATIC` (text, frames, etched
  lines), `LISTBOX` (single, multiple and extended selection, sort, item data), `COMBOBOX` (simple, drop-down,
  drop-down list) and `SCROLLBAR`, each with the `BM_`, `EM_`, `LB_`, `CB_`, `SBM_` and `WM_*TEXT` messages a
  game sends, and with the notifications to the parent (`BN_CLICKED`, `EN_CHANGE`, `EN_SETFOCUS`,
  `LBN_SELCHANGE`, `LBN_DBLCLK`, `CBN_SELCHANGE`, `CBN_DROPDOWN`, `WM_VSCROLL`, ...). Other classes (a tree view,
  a track bar) are a labelled box, or the procedure of a class the program registered.
- The dialog procedure follows DLGPROC's rules: `WM_INITDIALOG` with the parameter, `WM_COMMAND`, `WM_CLOSE`
  (undone by default: `DefDlgProc` sends `IDCANCEL`), `WM_DESTROY`; a `TRUE` answer returns `DWL_MSGRESULT`.
  `DialogBoxParam` runs its own loop over the program's queue until `EndDialog`.
- Every top-level dialog is one SDL window (`dlghost.cpp`) drawn by the layer in classic Windows 9x chrome with
  SDL's 8x8 debug font (Latin-1 letters with accents are drawn as their base letter), at 9 pixels per 4 dialog
  units across and 12 per 8 down, once a pump when something changed. The game's window beside it keeps
  working; input to a dialog's window is posted to the program's queue addressed to the control, and reaches it
  through `DispatchMessage`. Tab, Enter and Esc act as `IsDialogMessage` does. Without a window of the program's
  own, the dialogs still paint and take events.
- Headless (`SDL_VIDEODRIVER=dummy` or `offscreen`) the dialogs are drawn into their own canvas: set
  `W32L_DIALOGDUMP` to see them.

Replay verbs for dialogs (`W32L_REPLAY`, and the step socket; the frame number is the pump):

| verb | effect |
|---|---|
| `dlgclick ID` | press and release the left button at the middle of control `ID` of the newest dialog that has one |
| `dlgat X Y` | the same at client coordinates of the newest dialog |
| `dlgtype ID TEXT` | click control `ID`, then type `TEXT` (`_` is a space) |
| `dlgkey NAME` | a key to the newest dialog: `ENTER TAB ESC BACK DELETE LEFT UP RIGHT DOWN HOME END PAGEUP PAGEDOWN SPACE F4` |
| `dlgsel ID INDEX` | select item `INDEX` of a list or combo box, with its notification |
| `dlgclose` | the window's close button |
| `dlgdump PREFIX` | write every visible dialog now as `PREFIX` + `dlgNN-NAME.ppm` |

`tests/dialogs.cpp` (`make test`) builds a dialog from a template in memory, drives it with all of the above
and checks the answers; `make test IMAGE=comandos.exe DIALOGS="D_DEBUG_INFO ..."` maps a PE and dumps its
resource dialogs to `tests/out/`.

## Switches

All read from the environment.

| variable | effect |
|---|---|
| `W32L_SCALE=N` | integer window scale (default: the largest that fits) |
| `W32L_RESIZABLE=1` | a free-size, letterboxed window instead of a fixed-size one |
| `W32L_VSYNC=1` | also wait for the display's vertical blank on present |
| `W32L_FPS=N` | the rate Flip paces to (default 60; 0 turns pacing off) |
| `W32L_NOSOUND=1` | open no audio device; `DirectSoundCreate` answers `DSERR_NODRIVER` |
| `W32L_AUDIO_DUMP=FILE` | write the mixed audio as raw stereo float (`W32L_DEVTOOLS` builds) |
| `W32L_FONT_NARROW`, `W32L_FONT_BLACK`, `W32L_FONT_DEFAULT` | TTF files to use instead of the built-in choices; `W32L_FONT_NORECORD` |
| `W32L_DRIVE_X=PATH` | where Windows drive `X:` is on the host |
| `W32L_FIXED_HEAP=0` | turn the fixed-address heap off; `W32L_DEV_NOREUSE`, `W32L_DEV_POISON`, `W32L_DEV_CANARY` check it |
| `W32L_LOCKSTEP=1` | deterministic run: clocks advance a fixed step per pump (`W32L_LOCKSTEP_MS`, default 1000/60); `W32L_LOCKSTEP_PACE=1` keeps real-time pacing |
| `W32L_REPLAY=FILE` | scripted input, applied by pump number |
| `W32L_STEP=SOCKET` | wait for a coordinator before every pump (two programs stepped side by side) |
| `W32L_FRAMELOG=FILE` | a hash of every presented frame; `W32L_FRAMEDUMP_AT=N` with `W32L_FRAMEDUMP_TO=FILE` writes frame N as PPM |
| `W32L_PLATFORM_QUIET=1`, `W32L_PLATFORM_DEBUG=1` | less or more of the layer's own stderr diagnostics |
| `W32L_DIALOGDUMP=DIR` | write every visible dialog as `DIR/dlgNN-NAME.ppm` (NAME: its resource name or title) when first drawn and again after it changed, every `W32L_DIALOGDUMP_EVERY` pumps (default 60), and at exit |
| `W32L_DIALOG_MODAL_PUMPS=N` | close a modal dialog (`DialogBoxParam`) with `IDCANCEL` after N pumps, so a headless run cannot wait for input for ever |
| `W32L_BLT_TRACE=1`, `W32L_TRACE_DI=1` | trace blits or DirectInput traffic, within pumps `W32L_TRACE_FROM`..`W32L_TRACE_TO` |

## License

Licensed under the GNU GPL v2 or (at your option) any later version. See
[`LICENSE`](LICENSE) for the version 2 text.
`src/stb/stb_truetype.h` is Sean Barrett's, under its own public-domain/MIT
terms (see the end of that file).

## Third-party code

`third_party/tomlc99` (MIT, Cktan) parses the registry file. Its license is in
that directory.
