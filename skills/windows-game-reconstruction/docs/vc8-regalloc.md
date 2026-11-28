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

- `[vc8]` **Known negative.** On SP1 a temporary's register for `F("X", g->campo)` (`eax` in
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

## The scratch rotation and live-range split, measured (rounds K10, G2)

`[vc8]` The `eax -> ecx -> edx` rotation of VC6 (`vc6-regalloc.md`) holds on VC8 with these clauses, each predicted before its micro-experiment ran (four of four right); the model is **partial**: it does not explain callee-saved colouring, frame allocation, spill reload order or EH states. Applied in `byte-matching.md`, section 13, last subsections.

- **Shared multi-use values do not follow the one-use rotation.** An index used only for addressing allocates separate index and store-source temporaries and advances the cycle twice; the same value as both index and store source takes ONE register and the next choice does not advance. Repeat a shared sum directly in a clamp conditional (`w = a + b >= 100 ? 100 : a + b;`) when the image keeps its addend in its own scratch register. **held** *(0x525100)*.
- **Two independent arm stores and a joined local are different objects**: direct stores in two exclusive arms consume one rotation step per arm at the join; a local assigned in both arms and stored once after the join shares a web and does not advance the cycle.
- **Values promoted to callee-saved registers across calls do not advance the cycle; shared base-pointer temporaries do** (a temporary caching a parameter used as a common member-access base consumes a rotating register). This is not a universal lever: a base-pointer alias did not transfer to a constant-record-copy context.
- **Fixed-register instructions**: `div`/`mod` force `eax`/`edx` without advancing; a variable shift advances once for the shifted value and not for `cl`; a materialised `setcc` counts the compared input and the boolean result.
- **For a destructive `add`, a dying call-result scratch is preferred over a callee-saved operand** (`add eax,edi`); there is no general destination toggle when both operands already sit in the desired registers (the forms that flip it also change the spill webs: **no effect** on `add ebx,edx` vs `add edx,ebx` ties).
- **A post-switch deferred-action flag sets the rotation phase** for the cases after the last tail-duplicated one (`byte-matching.md`, 'Scratch-register phase').
- **Live-range splitting through a branch-local shadow variable** (`short *c = col; ... col = c;`, skipping the store on the unchanged arm) lowers `col`'s outer priority below `texto`'s, so `texto` gets `ebp` and `col` is homed in a stack slot (0x630d00, 148 -> 57 lines). Computing `x` and `y` into scalars before an `int c1 = ...; Punto p(x, y);` keeps `x` in `ecx` across `c1` (a constructor delays the stack stores and frees `ecx`). **held.**
- **Persistent constants race by use count**: the image keeps a constant (EH state 5, or a zero) in a callee-saved register chosen by the number of uses of each; one use more or fewer in our source flips the choice. Guard-mask setups (`mov ebx,2` once, against 1, 4, 0x20 rematerialised) are the same race. A real extra call flips it at the cost of instructions: **partial**, none closed.

## The persistent-constant choice, the `this` register and the 8-aligned frame, measured

**held** *(DeepSeek D3 micro-experiments with the exact `/O2 /Gy /GR /EHsc` harness)*. [vc8]

- **A constant is kept in a callee-saved register only at four or more uses across calls** (first measurement; the threshold depends on the context: 'The held-constant table' below replaces this figure). At two or three uses it is an immediate store each time (`mov [state],5`); from four it is hoisted (`mov bl,5`, or `xor bl,bl` for zero) and reused. Zero is not special at equal counts. Two constants used in sequential regions share one register; two constants with interleaved uses are held by neither.
- **A held constant is reused everywhere, which extends its live range and can lock the register.** `push ebx` (1 byte) replaces `push 0` (2 bytes), so a zero held for the EH-state stores of one region is reused as a `Token(0)` index argument in the next region and blocks the register from holding the other constant. This, not a bare use count, decides which constant wins (zero against 5 in a function with two blocks of state stores: the shape that holds zero is 'the discarded read in one block only'; symmetric blocks, or the dead read in the other block, hold 5). Use it to predict which value must be held, and look for an immediate where ours has a register push.
- **The callee-saved allocation order is fixed: esi, edi, ebx, ebp.** `this` (ecx on entry) takes the first free one (esi) unless another value is allocated before it. `this` in ebp or ebx in the image means some constant or value out-prioritised it: the `this` choice is the persistent-constant choice applied to `this`. Constants 0 and -1 used once are pushed as immediates and take no register.
- **`and esp,-8` does not come from a plain `double` or `__int64` local.** MSVC x86 gives those alignment 4, so the frame is `sub esp,N` with no alignment. The alignment comes from `__declspec(align(8))`, `__m64`, or, as a secondary trigger, from 8-byte QWORD temporaries spilled by high x87 pressure in long float-matrix expressions. For a port whose image has a plain frame while ours is aligned, change the spelling of the projection expression (fewer `(double)` casts or different temporaries), not the declared types of the locals. The full trigger list is 'The frame-kind tables' in `byte-matching.md` (section 13, last subsection): there the pressure trigger was NOT reproduced (integer spill pressure, a `fistp` qword and two doubles live across a call all keep a plain frame), so treat an aligned frame without `align(8)`, `__m64`, `_alloca` or `setjmp` as open.
- **These rules predict which value the image holds; they are not a source lever.** On three ports the image obeyed the four-use threshold while ours chose differently because the `this`/object-pointer register dominated, and no spelling (dead locals, copies, re-reads, reorders) reproduced the allocation; QWORD spills in an aligned frame stay under named float temporaries or `(float)` casts. Use them to find the wrong shape instead (byte-matching.md, section 13). A tie between two callee-saved registers (a pure ebx/ebp swap) is the exception: an explicit `if (n > 0)` before the loop flips it for free. **partial.**

## Rotation phase and held values inside a `switch` (rounds B110-B121, K24, Q1-Q2, D5)

`[vc8]` Observations from full ports, not micro-experiments; the structural rules they belong to are in `byte-matching.md`, section 13, 'Block layout, wrong views and storage shapes'.

- **`x = a ? a : y;` against `if (a) x = a; else x = y;` shifts the scratch rotation phase of the calls that follow**: the same stores, the registers of the next calls one step apart. **held** *(0x4dcdc0, 95 -> 39 inside the chain that matched it)*.
- **Constant registers can depend on ONE case.** If the image holds constants in callee-saved registers (1 in `ebx`, -1 in `ebp`) that ours writes as immediates, look for a value the image keeps in a callee-saved register only inside one case: binding a record to a reference before a call across which it stays live took a register there and gave the image's constant registers for the whole function. **partial** *(0x4dea90)*.
- **One result variable doubles as the zero register**: in a message handler written with `int r = 0; ... r = 1; break; ... return r;` the image keeps `r` in a callee-saved register and uses it wherever a zero is pushed or stored; which of `r` and the node pointer takes `edi` or `ebp` stayed swapped in two ports. **partial** *(0x4985a0, 0x493320)*.
- **A named pointer against a re-read decides who holds the register**: a pointer the image keeps in `eax` across four stores is the already-existing named local, not the owning member re-read at each store (0x4d4590, 235 -> 224); a named OWNER pointer for a receiver fixed the last receiver sequence of a function (0x5107f0, 4 -> 0). The opposite case, a global the image reloads at each use, is a re-read at each use. **held.**
- **`noalias` on a callee that reads into `&local` keeps a value computed from that local in its callee-saved register across the next call**; without it the value passes through `eax` first (0x49bd10, 9 -> 0). **held.**
- **The shape of ONE loop decides the persistent constants of the whole function.** An indexed loop (`for (k < 9) new (t + 0x7c + k*0xdc) T`) against the same loop as a running-pointer down-counter changed which registers hold 0, 1, -1 and -2 across a separate, unrolled table loop (`esi`, `ebx`, `eax`, `edx` in the image). When explicit blocks have the right shapes and every held constant is in the wrong register, rewrite the sibling loops before touching the blocks. **partial** *(0x603da0, 1329 -> 220)*.
- **A forced-inline helper's return value keeps its own register after the expansions join**: an extra `mov eax,ecx` at the join means the helper returns the value; an out-parameter or shared local has no such copy. **held** *(0x5467c0)*.
- **Ties with no source lever found**: a single zero store that gets its own `xor reg,reg` where ours reuses a held zero, or the reverse (nine spellings on three ports); `add reg,reg` commutation; the operand order of a loop-top `cmp`; the jump-table index in `ecx` against `edx` when `this` stays in `ecx` across the dispatch. **no effect.**

## The held-constant table (round K31)

`[vc8]` **held** *(a matrix of 1,568 generated functions plus 57 focused controls, compiled with the exact `/O2 /Gy /GR /EHsc` harness)*. 'Held' means the constant is set once in a register (`xor reg,reg`, `mov reg,K`) and reused; 'immediate' means each use encodes it (`mov [m],0`, `push 0`). The flat 'four uses across calls' of the earlier section is one cell of this table.

| # | context | immediate up to | held from |
|---|---|---|---|
| 1 | straight-line stores to members, no call between them | 2 uses | 3 uses, in the caller-saved `eax` |
| 2 | stores ACROSS CALLS to an OFFSET-ZERO member (`[reg]`) | 4 uses | 5 uses, callee-saved |
| 3 | stores across calls to NONZERO-offset members (pointer and enum destinations included) | 3 uses | 4 uses, callee-saved |
| 3a | row 3 with even ONE offset-zero store among them | 4 uses | 5 uses |
| 5 | uses only as CALL ARGUMENTS or COMPARISONS | 8 uses (the sweep's limit) | never seen |

Rules around the table, numbered as measured:

- **(4) Spelling is inert.** The literal `0`, `int z = 0;` and `const int z = 0;` compile IDENTICALLY over every swept cell; `NULL` equals a pointer zero; an enum spelling stored into an integer member changes nothing. A named zero is not a lever, in either direction.
- **(5) A named local does not promote an argument-only or compare-only zero**: it stays immediate (row 5).
- **(6) First-use order.** Three stores FOLLOWED by a zero argument create a held zero that the argument then reuses (`push reg`); the argument FIRST, or only two stores before it, stays immediate even when more stores follow. So the same use count gives two results depending on what the first three uses are.
- **(7) Two competing constants.** In SEQUENTIAL regions they share one register in source order (also at three uses each); ALTERNATING three-use regions hold neither; at four or five uses each the SECOND constant wins the register. Zero, 1 and -1 behave alike.
- **(8) High register pressure** (four genuinely live pointers plus `this`): no callee-saved constant at all; immediates across calls, and a fresh caller-saved setup (`xor eax,eax`) for each adjacent run of stores.
- **(9) Dead-store elimination invalidates the textual count.** Repeated stores to the SAME member count only when they survive (a call between them); count the stores of the final code, not of the source.
- **(10) Loops differ from straight-line code.** A loop whose body is one zero store becomes `rep stosd`; larger bodies hold the register, or re-emit `xor eax,eax` at the back edge under pressure. Do not carry a straight-line count into a loop.

Reading it against a port: count the image's uses by kind (rows 1-5), check which was first (6), and compare with ours; a differing choice at equal counts is pressure (8) or a dead store one side keeps (9), which is a statement about the objects and stores of the function, not about how the zero is spelled. On the ports tried, pointer-hoisting forms, loop-local zeros, type changes and inline clears all canonicalised to the same code: the table predicts the image and has not yet closed a port. **partial** as a lever.

## The register-role table (rounds Q30, Q31)

`[vc8]` **held** *(a matrix of 1,570 generated functions (pairs, triples, pressure) plus 64 targeted and disambiguation controls, exact `/O2 /Gy /GR /EHsc` harness; each use of a value is a store to its own global sink, so the store's source operand names the holder: a callee-saved register is held, an immediate is immediate, a scratch register is a reload)*. The held-constant table says WHETHER a value gets a callee-saved register; this one says WHICH. The allocation order is fixed (`esi`, `edi`, `ebx`, `ebp`); the values are sorted by an effective weight and take the registers in that order:

`W(value) = surviving uses across calls + bonus`

| value kind | bonus |
|---|---|
| an INDEPENDENT memory-load local defined at function entry (`int m = q[2];`, `int m = g;`: its base is not itself a held value) | +2 |
| `this` (the thiscall receiver: no home slot) | +1 |
| a parameter (it has a home slot) | 0 |
| a compute-local defined from a held value (`m = n + 13`) | 0 |
| a memory-load local whose BASE is a held value (`m = q[2]` with `q` held, `m = this->q` with `this` held) | 0 |
| a memory-load local defined LATE (after the first call) | 0 |
| a constant | 0 |

- **Why the entry load wins**: an independent load is scheduled at entry BEFORE the parameter home loads; a load whose base is held is scheduled after its base and loses the bonus. So `this` needs one use fewer than a parameter to take `esi`, and an entry-load local two fewer.
- **Ties.** Equal weights: the earlier first use or definition wins (for two entry loads that is DECLARATION order: swapping the declarations swaps `esi` and `edi`). Two constants at equal weight: the SECOND wins (rule 7 of the held-constant table). An identity copy (`m = n`) COALESCES: both names share one register and no second holder appears, so a named copy is not a lever.
- **Promotion thresholds.** Parameters, locals and `this` are held from TWO uses across a call (one use stays in a scratch register); constants only from four, and only when the uses are stores (arguments and compares stay immediate: row 5 of the held-constant table).
- **Loops.** The induction variable takes top priority (`esi`), ahead of `this`, parameters and entry loads: loop-weighted uses dominate straight-line ones. A constant used once per iteration stays immediate.
- **Reading it against a port**: a value in `esi` ahead of `this` is an entry load or a loop counter; `this` in `ebp` means three values out-weigh it. The dials, in order of reach: (1) promote a value by making it an independent memory-load local at function entry; (2) demote it by computing it from a held value, loading it through a held base, or defining it after the first call; (3) add or remove SURVIVING stores of it; (4) swap the declarations of two entry loads; (5) give a pointer loop weight if it must beat the counter.
- **What it cannot reach**: a permutation between values of identical weight and kind (the allocator's internal schedule order: a uniform `this`/parameter rename stayed at 88 lines under every copy form, because the copies coalesce); a zero-SOURCE tie (one store taking a call result already known to be zero where the image takes the held zero); cross-jump hosting, x87 order and frame-cell packing. On six role-swap ports it named the cause each time and closed none: the entry-load bonus is a lever only where the residue IS an entry-load against compute-local difference. **partial** as a lever.
- **The dials are real and the cheap ports sit below them.** A detector over 330 parked rows (the first definition of each callee-saved register in image and ours) found 23 with the SAME held-value set in a different assignment, 138 with a different set and 122 identical. On the opened ones every dial was inert or catastrophic, because two-line residues are knife-edge webs: swapping the declarations of two entry loads 2 -> 186 lines, declaring a member pointer early so that it takes the image's base register 2 -> 106 (and `(&a->m)->f` folds back to the direct offset: inert), reversing a copy's direction (`p = start` against `start = p`) 4 -> 37, moving a pointer's definition into its `switch` case 8 -> 25 (the image reserves the register in the prologue and schedules the `lea` late: a push/definition split no source expresses). Frame cells swapped between two locals follow liveness and size, not declaration order (inert). The +2 entry-load bonus held under every dial (the local kept `esi`). These decisions belong to the allocator's internal schedule, the zero-source choice, the pointer bias, frame liveness and cross-jump hosting, all below the weight layer: classify a role residue with the detector, and open it only when the weights of the table actually differ. **no effect** *(about sixteen ports)*.

## Shape rules of the cheap band (round Q27)

`[vc8]` **held** *(micro, exact harness)*; **predictive, not levers**: on fifteen ports of two to thirty lines each rule explained the residue and no source form moved it. Use them to classify a cheap residue as a wall before spending variants on it.

- **Load order of a compare's operands (SH2).** Both operands promoted to registers: through NAMED locals they load in DECLARATION order whatever the written direction (`int t = s->a, u = s->b; if (u > t)` loads `a` then `b` and only flips the `cmp`); as DIRECT member operands they load in WRITTEN order, left first (`if (s->b > s->a)` loads `b` first). A named count local forces a register compare (`unsigned m = n; i < m`); without it the compare is `cmp reg,[mem]`; a copied index folds away. Index-first WITH a register compare was reachable by neither.
- **Reload order after a call is the PUSH order** (right-to-left arguments), not the declaration order: `f(b, a)` loads `a` first. A commutative add of two members loads the same operand first in either spelling; a subtraction follows the source.
- **Scratch rotation (SH1).** Single-use loads feeding one call rotate `eax`, `ecx`, `edx` in push order, and the rotation PERSISTS across separate calls (`f(a); f(b); f(c);` uses the three in turn); a value used twice through a named local takes ONE register; a trivial inline identity (`Id(x) { return x; }`) FOLDS and does not shift the phase (a phase shift needs a non-foldable inline, a real setter or getter); a store between two loads reschedules them.
- **Cross-jump hosting (SH3).** The shared exit tail is hosted at the LAST identical epilogue in every standard form (if/else chain, early returns, `switch`, uniform epilogues, the big arm moved last). An image whose exits jump into the SUFFIX of the FIRST epilogue (`pop esi; ret` inside `pop edi; pop ebx; pop esi; ret`) is a layout wall; the flag form and a `goto` into the wanted arm are inert or worse. A shared `goto fin` merges the epilogue COUNT (three to two on one port), never the hosting position.

## Whole-function levers found in ports (rounds K29-K40, Q3-Q42)

`[vc8]` Observations from full ports; the rules they belong to are in `byte-matching.md`, section 13, last subsection.

- **A cached owner-pointer local against a re-read at each use reallocates the whole function**: with the re-read (`#define A ((Derived *)owner)`) the image's `this` register and held-zero register both appeared before any other fix. Only for member reads the image reloads across calls; a re-read of a parameter cast is forwarded and inert. **partial** *(three ports)*.
- **A named preset local for a ternary argument** (`int v = B; if (c) v = A; f(v);` against `f(c ? A : B)`) flipped the callee-saved roles of `this` and the result variable for the whole function, besides giving a push per arm. **partial.**
- **A loop bound loaded at function scope** moves the allocation enough to close a 4-byte frame gap; the declaration alone, with the load later, does not.
- **An owner pointer declared and loaded at FUNCTION scope flips `this` between two callee-saved registers** (`edi` to `esi` on one port, together with the sparse `switch`): it is an entry-load local, the +2 row of the register-role table.
- **A down-counter loop** (`for (i = n; i > 0; i--, p++)`) gives a `sub reg,1` latch and its register; on a port whose image counts up it is worse.
- **Store order follows declaration birth order** for independent stores: reorder the declarations, not the statements (statement reorders cascade through the register web).
- **Declaration interleaving** (a computed float local declared just before its first use, not up front) restores the image's just-in-time computation order and its frame cells; the same offsets as inline cast arguments shrink the frame too far, a named shared product over-CSEs, a reordered sum diverges the x87 schedule (`byte-matching.md`, 'Declaration birth order'). A lever of projection webs with named offsets only: in a deep x87 web the declaration order IS the schedule and one moved declaration re-times it; in register-role ports it is inert. **partial** *(two ports)*.
- **A frame larger than the image's usually means the image keeps a POINTER in a register** (or computes it inline into a register it reuses) where ours spills a separate local; next an index-against-pointer loop form or a frame kind. It is rarely a declaration to move.
- **The hidden-return pointer in `eax` after a record-returning call**: an inline `this` accessor on the temporary (`V *d = r->F(v).P();`) makes the following loads `[eax+N]`; a named result makes them frame-relative. Whether `&v` of a named result is then pushed as `eax` or re-derived (`lea ecx,[v]; push ecx`) follows a PHASE over consecutive sites: six independent named-local blocks give `lea, eax, lea, lea, eax, lea`; declaring several results before their calls shifts it by one site; one reused named local gives `lea` everywhere. A one-site micro-test always shows `lea`: test with several sites. Passing the temporary itself does not touch the unwind states (an earlier note here said it deletes one: that diff was the rotation cascade below) (`byte-matching.md`, 'By-value records').
- **Several record-returning calls as reference arguments of ONE inline call** are evaluated right to left, their returned pointers stay in registers through the member calls, and the temporaries are destroyed after all uses: the register-held form of a scope-end-destroyed temporary, valid for a second compiler.
- **A direct temporary changes the scratch-rotation phase of every call after it.** The rotation persists across calls (rule SH1), so one site rewritten from a named record to a direct temporary can match its own instructions and move the registers of all later sites (a count that became exact with the diff 19 lines worse). The sign of each site depends on which other sites are rewritten: levers are superadditive, a site rejected on one phase pays on another, and a recorded verdict can flip after unrelated header changes. Search subsets mechanically and re-test after each adoption (`byte-matching.md`, 'Method: what the diff cannot see').
- **A named count local plus a preset result** (`unsigned m = p->n; f = 0; if (i < m) f = p->data[i];`) is the bounds-checked accessor form that gives the register compare of rule SH2; ternary and if/else forms do not, and a named element temporary adds a frame cell.
- **A nested block that ends early shortens a local's lifetime interval, and the frame-cell allocator then pairs its cell with a later local** (`{ char name[32]; Read(name); kind = Classify(name); }` closed before the rest of the loop body): one surplus 32-byte cell gone, 98 -> 6 lines. Declaration order, scope level and initialisation form were inert: cells are shared by disjoint LIFETIMES, so shape the interval (`byte-matching.md`, 'A static custom convention, lifetime intervals and switch order'). **held** *(match)*.
- **A same-file `static` callee gets a compiler-chosen convention**: two arguments in `ecx` and `edx`, the rest on the stack, CALLER cleanup, a plain `ret`. Homes follow the formal order (last pointer formal `ecx`, second-last `edx`, first on the stack), and also the callee body's own allocation: while the body is far from the image, a formal reorder helps one caller and hurts another. **held** *(match)*.
- **Member construction is a scheduling input**: a pointer stored through an inline holder member precedes a following `fldz`, a raw pointer member's store follows it, but only when members with throwing inline constructors come after; with none the two forms are identical. A record initialised inside a `new` constructor argument (`new T(p.Init(k))`) has its stores after the allocation call.
- **An unused inline helper in the translation unit, or an edit to a neighbouring function, can move a port's allocation by a line**: re-measure the file's other parked ports after every adoption.
- **A direct temporary against a named local for a by-value record argument** removes the local's lifetime from the schedule (81 -> 37 on one port); new named record locals enlarge the frame.
- **Reading a `this` member into a local BEFORE the first call on a parameter receiver** (`idx = start; v->Visible(...)` against the read after the call) shifts the callee-saved allocation slightly: three lines on one port, the instruction counts from four apart to one; the cyclic permutation of `this` and two parameters stayed.
- **A named pointer to a record before copying it** (`T *d = o->m; T v = *d;`) frees one scratch-rotation step.
- **x87 operand order is a schedule, not a spelling**: aggregate and global operands load in descending address order, scalar stack homes ascending, a commutative sum's terms in load order, a subtraction in source order (`byte-matching.md`, rules X1-X6).
- **Ties these rounds confirmed with no source lever**: index-first against count-first load order before a register compare (a named count local forces count-first, removing it forces a memory compare; reorders cascade 2 -> 20 lines); the load direction of an iterator's constructor; `this` in one callee-saved register against another under a uniform rename; a zero the image rematerialises (`push 0`) where ours holds one and spills `this`; the image comparing a member with an immediate (`cmp [this+off],0`) where ours tests against a held zero, which shows up as a 4-byte frame surplus AND a memory null test (one wall, the held-zero race, not two); the addressing fold `sub reg,4` tied to the register web (routing the store through a named pointer cascades through the birth order, 2 -> 110 lines); `y * pitch` with a global multiplicand in a pixel loop (operand order, a local copy and a multiply-only inline helper all load the global first; a whole pixel-writing helper flips it and loses elsewhere, in all six parameter orders). **no effect.**
