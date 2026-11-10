# Running the original compiler (Commandos: VC8 `cl.exe`)

Referenced from `SKILL.md`, later stages. The byte-match loop that uses it is in `SKILL.md`.

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
