# VC8 register allocation: observations from Commandos

No model has been measured for VC8, unlike VC6 (`byte-matching.md`, section 7,
and ArmyMen2's `docs/vc6-regalloc.md`). This file holds single observations of
what moved a register choice under `/O2`, moved here from `SKILL.md` in the
order they were found. None has a survival status yet; give one a status by
checking it against the tree and recording it in `byte-matching.md`.

Tags as in `byte-matching.md`: `[vc8]` is Visual C++ 2005 SP1
(14.00.50727.762); `[vc8 rtm]` is an observation made on the RTM compiler
(14.00.50727.42) and not rechecked on SP1, which matters most here: the RTM
"register quirks" were largely the wrong service pack (`byte-matching.md`,
section 1).

## What is known not to move it

- `[vc8]` **Known negative.** on SP1 a temporary's register for `F("X", g->campo)` (`eax` in
  the image, `edx` or `ecx` in ours: 0x512640, 0x4e3930, 0x42c970) moves with
  no field type, local, inline getter, flag or `/GL`. Cause unknown.

## Tricks, in the order found

- `[vc8]` A `void` call followed by `pop ecx` vs `add esp,4`: the original's `add esp,4` means the
  call result is live, so make the callee and the caller return `int`.
- `[vc8]` Original reloads a field into `ecx` where you get `eax`/`esi`: take the first access
  through a local pointer (`T *o = f4; o->...`) and the second through the member path, or reassign
  the local (`o = f4;`) before the second use.
- `[vc8]` A tail `jmp` that reuses the argument slot means the callee has the same stack-argument
  count; pass the arguments straight through.
- `[vc8]` Read destructor targets from the FuncInfo unwind map; a base needs a virtual destructor
  for the derived constructor to get an EH frame.
- `[vc8]` Copy a member into a local before a call whose args are computed from it (`void **d =
  datos;`) to get the original's ecx/edx/eax order (ClassLista::Anyadir).
- `[vc8 rtm]` (Superseded: the kept zero and the pointer-difference loops; see sections 3, 6 and
  12.)
- `[vc8]` A call whose callee pops the stack (no `add esp` after) and where `ecx` was never loaded
  is a **thiscall on `this`** (ecx passes through): declare it as a member `N(...)`, not a
  `__stdcall` free function. Fixes odd register choices (edx instead of ecx) in forwarders.
- `[vc8]` A tail `jmp` into a member keeps ecx; when a function reuses its argument slot (`mov
  [esp+8],-1; jmp`) the callee has the same stack arity.
- `[vc8]` An inline accessor like `int N() { return f4 ? f8 - f4 : 0; }` reproduces register layouts
  that inlined source bodies do not.
- `[vc8]` `#include <string.h>` under /O2 makes `strcpy`/`strlen` intrinsics: they expand to the
  `sub esi,src; mov dl,[eax]; mov [esi+eax],dl; add eax,1; test dl,dl; jnz` loop and the `lea
  edi,[eax+1]; ...; sub eax,edi` length loop. Do not hand-write those loops, and do not treat them
  as a pointer-difference mystery.
- `[vc8]` `delete[]` of a `char *` compiles to scalar `operator delete`; call the CRT free (a
  separate address) through an extern when the original calls it.
- `[vc8]` COM-style `Release()` shows as `mov ecx,[p]; mov edx,[ecx+8]; push p; call edx`: declare
  the virtual `__stdcall`.
- `[vc8]` `M(F(), F())` pushes the right-hand call first: the first call's result is pushed
  immediately after it returns.
- `[vc8]` **Store scheduling in zero-init constructors: use inline sub-object constructors.** When
  the original interleaves or groups float/int zero stores in an order the flat body cannot
  reproduce (fldz first, `xor ecx,ecx` placement, ints before floats), model the runs as small
  structs with inline constructors (`struct V3 { float x,y,z; V3() { z=0; y=0; x=0; } };`, `struct P
  { int a,b,c,d; P() { b=0; a=0; c=1; d=1; } };`). The member-init order then drives the schedule.
  If a sub-object init must sit between body stores, use an inline `Init()` method instead of a
  constructor and call it in the body at that point.
- `[vc8 rtm]` (Reverted by the owner: matching library code; see section 1, "Library code counted as
  matched".)
- `[vc8]` `member % N` compiles to `idiv`; copy the member to a local first (`int c = g->cnt; c %
  40`) to get VC8's magic-number multiply.
- `[vc8]` A struct passed by value stays a tail `jmp` unless it has a user-written copy constructor
  (then it is built on the stack and called).
- `[vc8]` Halving ints is `>> 1`, not `/ 2` (`/ 2` adds the `cdq; sub` sign fix-up).
- `[vc8]` Jump-table `switch`: decode the table (`tools/jt.py IMAGE TABLE COUNT FIRST_CASE`) to get
  case value -> code address; the emitted code order is the *source* order of the case labels, so
  order the cases by their code address, and grouped targets (`case 0x24: case 0x2b:`) share one
  body. Handlers that `ret 0xc` with a word second argument are `int f(unsigned short a, unsigned
  short id, int c)`; `int r = 0; ... r = 1; break; ... return r;` reproduces the hoisted `xor
  eax,eax`.
- `[vc8]` A store followed by a reload of the same member (`mov [esi+14],3; mov eax,[esi+14]`) is
  reproduced by writing the store through `*(volatile int *)&f14 = 3; return f14;` (a volatile
  member also reloads, but then schedules the store earlier).
- `[vc8]` A local buffer declared as `char data[N]` makes VC8 emit a /GS cookie the original lacks;
  declare it as N/4 `int` members instead. A `void` function ending in a `void` virtual call becomes
  a tail `jmp`; if the original calls and returns normally, declare the callee to return `int`.
- `[vc8]` A search loop that reloads the array pointer every iteration and jumps from the failed
  first test to the increment is `do { if (o->At(i)) return i; i++; } while (i < o->n);` with an
  inline bounds-checked accessor; a `for` loop with the outer test strength-reduces instead.
