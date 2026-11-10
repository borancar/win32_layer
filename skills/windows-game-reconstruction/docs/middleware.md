# Third-party providers and licensing

Referenced from `SKILL.md`, stage 1.

- **Miles (`mss32`)**: OpenMiles (`https://github.com/maci0/openmiles`) is the real
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
