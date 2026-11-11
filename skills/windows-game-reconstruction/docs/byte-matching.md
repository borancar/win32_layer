# Byte-matching ideas, with what survived

Source shapes and methods that brought a reconstruction closer to the bytes the
original compiler emitted, collected from project history. Each entry carries the
compiler it was measured on and whether it lasted.

## Read this first

This header is the part to read every time. Do not read the file whole.

**Tags.** Every entry names the compiler it was measured on:

| tag | compiler |
|---|---|
| `[vc6]` | Visual C++ 6.0, build 12.00.8168, `/O2 /GX` |
| `[vc8]` | Visual C++ 2005 SP1, 14.00.50727.762, `/O2 /Gy /GR /EHsc` |
| `[vc8 rtm]` | Visual C++ 2005 RTM, 14.00.50727.42; not rechecked on SP1 |
| `[vc6, vc8]` | several tags in one bracket: measured on each, same behaviour |

**How to look something up**

1. Find the section for your symptom in the table below, or grep for the
   instruction pattern (`grep -n -i 'setz\|lea eax,\[' byte-matching.md`).
2. Read the entries whose tag includes your compiler first.
3. If none fits, read the other compilers' entries for the same symptom and
   try them as hypotheses. An idea from another compiler often carries over,
   and sometimes inverts.
4. When another compiler's idea matches on yours, add your tag to that entry's
   bracket (`[vc6]` becomes `[vc6, vc8]`). When it fails or inverts, add one
   line under the entry: your tag, "opposite" or "no effect", and what
   happened. For a compiler with no tag yet, add a row to the table above and
   its build and flags to *Tags, statuses and figures*.

**The rules** (each is a reversal recorded in section 1):

1. Fix the compiler identity first: build, flags and service pack. Try every
   revision that stamps the same Rich build before tuning a near-miss family.
2. A lower number proves nothing about meaning. Check every respelling for
   every input, and reject one that emits an instruction the image lacks.
3. When a function stalls, re-derive it from the image (block order, exits,
   frame) before reading the old source again.
4. After any structural or compiler fix, ablate every hack in the function and
   keep only those whose removal costs.
5. Declare no wall before typed structs, real classes, the frame layout, a
   from-scratch derivation and the other compiler revisions have been tried.
6. Write whole-body asm only where the original source was assembly.
7. Fix what counts as the game's code before counting; leave library code out.
8. Name every address before comparing it. Never let the scorer learn a name.
9. Count distinct addresses, after a clean build, and diff the address sets
   around every cleanup.
10. Before respelling a 2 to 4 byte miss, re-read `ret N`, each argument's slot,
    the store order and the constants' bytes.
11. A search only proposes: read every edit it applies.
12. Keep a behaviour fix that raises the number.
13. Use number wins last, mark them, and never count them as recovered source.

**Where to look**

| symptom | section |
|---|---|
| blocks, jumps, shared tails, `switch`, loop shape differ | 3 |
| load or store order, a local, the frame size differ | 4 |
| field width, signedness, struct copies, parameters differ | 5 |
| calls, inlining, EH frames, constructors, destructors differ | 6 |
| registers only | 7, then `vc6-regalloc.md` or `vc8-regalloc.md` |
| unsure of the calling convention or what a prologue means | 8, and 13 for VC8 |
| about to add a `volatile`, a dead read or naked asm | 9 |
| about to run a search or a flag sweep | 10 |
| an unrelated function moved after a header or file change | 11 |
| about to call something unsolvable | 12 |
| `/GS` cookies, `/GL`, static functions, service packs | 13 |

**Reading an entry.** The tag names the compiler; **held**,
**number win**, **reverted/superseded**, **no effect** say what became of the
idea; *(match)* and *(record)* say how far it was checked. Definitions follow.

## Tags, statuses and figures

**Register allocation has its own files**, one per compiler, beside this one:
[`vc6-regalloc.md`](vc6-regalloc.md) (the measured VC6 model, summarised in
section 7) and [`vc8-regalloc.md`](vc8-regalloc.md) (VC8 observations, no model
yet). Read the one for your compiler when a diff is registers only.

**Tags.** `[vc6]` is Visual C++ 6.0, compiler build 12.00.8168, `/O2 /GX`, on
ArmyMen2 (https://github.com/borancar/ArmyMen2), which went from 399 to 1,639 of 1,646
functions byte-identical.

`[vc8]` is Visual C++ 2005 SP1, `cl` 14.00.50727.762 (`c1xx`/`c2` of the same
build, `link` 8.00.50727.762), `/O2 /Gy /GR /EHsc` with the default `/GS`, on
Commandos: Behind Enemy Lines (https://github.com/retro-foundry-games/CommandosBEL).
A handful of functions need `/GL` and `link /ltcg`
(section 13). At first the project compiled with the RTM compiler
(14.00.50727.42), so every idiom found before the switch to SP1 was found on RTM; the
first entry of section 1 is what that cost.

`[vc8 rtm]` marks an observation made on that RTM compiler and not rechecked on
SP1. A function matched on RTM also matches on SP1 (all 9,684 did, by the
record), so idioms proven by a match carry `[vc8]`; what carries `[vc8 rtm]` is
anything that says a shape could not be reproduced, was parked or is unsolved,
because on RTM that was sometimes the compiler (section 1).

A tag can name several compilers: `[vc6, vc8]` means the idea was measured on
both and behaves the same.

Entries under a "Folded `[vc8]`" sub-heading are the Commandos record with
duplicates merged and function names dropped: general statements, **held**
*(record)* unless they say otherwise, none rechecked one by one. The entries
above them in each section were checked as their marks say and are the
authority where the two overlap. `vc8-regalloc.md` is still raw.

Add new compilers here under their own tag. An idea measured on one compiler is
a hypothesis on another.

**Status** says what happened to the idea afterwards:

- **held**: still in the tree with the function byte-identical, and no record
  of it being replaced. `[vc6]` checked against ArmyMen2 `20c901fe`. `[vc8]`
  checked against Commandos `83ef99f8`: the spelling read in the source and the
  function identical in a `make match` of that tree. Two weaker marks:
  *(match)* means the function is identical there but the spelling was not
  read; *(record)* means the entry is taken from the log, `STATUS.md` or
  `docs/` and not rechecked.
- **number win**: lowers the distance and is still in the tree, but is not
  source the original author would have written. Listed for removal once the
  binary matches. Use last, mark it in the source, never count it as recovered
  source.
- **reverted** / **superseded**: gave a better number and was later taken out,
  because it was wrong, because a better source shape replaced it, or because
  the project owner ruled it out. These are the local optima; read section 1
  before trusting any score.
- **no effect**: tried broadly and moved nothing. Recorded so it is not
  re-ground.

The `[vc6]` figures `A -> B` are the per-function edit distance of the relinked
bytes against the image (0 is identical; a replaced byte costs 2).

The `[vc8]` figures are what Commandos' `make match` prints: `N identical, M
different`, where N counts symbol names whose object bytes equal the image
(relocations masked, their targets checked against `symbols.tsv`), and, for one
function, `K bytes differ`. Parked near misses (`~` in `symbols.tsv`) are in
neither number. The record's last figure is 10,082 identical, 0 different (SP1).
The check for this file had only the RTM compiler: 10,036 identical, 46
different, the 46 being SP1-only matches by the record (the sum is 10,082); the
`/GL` check did not link there, so its 6 identical, 8 parked are *(record)*.

## 1. Local optima: wins that did not last

Read this first. Every item lowered the number when it landed.

- **A respelling that is not equal for every input.** `[vc6]` **reverted.**
  RowUpdate's clamp `cl <= 0 ? 0 : cl` was respelled
  `(uint32_t)cl <= (uint32_t)0 ? 0 : cl` for 16 bytes (154 -> 138). That is true
  only for `cl == 0`, so a rect hanging off the left edge went unclamped. It
  shipped for a day and was found by reading, not by any test. The signed clamp
  went back and the function stays at 154. *Rule: for every unsigned cast,
  negation or difference test, check the variable's whole domain. The byte
  metric cannot see wrong semantics.*
  `[vc8]` none recorded; the tree was not audited for it.
- **A different instruction that scores better.** `[vc6]` **reverted.** LoadMap's
  TRIG size check written `size - (uint32_t)plane` scored 2 where the plain
  `size != plane` scores 356, because the `sub` supplies one scratch-register
  rotation step the image gets some other way. The image has a `cmp`. The owner
  ruled the `sub` form out: a spelling that emits an instruction the original
  does not have is not the original's source, whatever the total says. The same
  class (`a - b` for `a != b`, `(-x) ? a : b` for `x != 0`) was removed from
  OptionsDefaults, where the image has `test` and the respelling left a `neg`;
  an impossible pair (section 9) took its place there (4 -> 0).
- **Months of single-lever tuning on the wrong layout.** `[vc6]` **superseded.**
  ScrollMapCache sat at 138 with a hoisted `px`, a volatile temporary and
  reordered clamps, each of which had lowered the number. Writing the function
  again from the image's disassembly *before reading the old source* gave 0: the
  image's block order was structurally different (the full repaint is the last
  block, reached by three forward jumps; `abs()` at each use). The accumulated
  levers were all discarded. ObjRemap went 90 -> 0 the same way on a second
  re-derivation, after hundreds of local edits had not moved it.
  *Rule: when a function has stalled, re-derive block order and exits from the
  jump targets, then locals from the frame layout, then compare with the old
  source. It pays when the control-flow layout is off; it did not beat the
  current source where the residue was register colour (RowUpdate, FindPath).*
- **Hacks that go stale.** `[vc6]` **reverted.** Impossible-condition pairs and
  dead reads (section 9) were added to steer registers; later source fixes made
  several of them useless or harmful. Dropping them *improved* the functions:
  BlockWeightRoute 26 -> 16, SightScan 76 -> 52 (three dead reads), ObjRemap
  124 -> 108, and FindPath now carries none. *Rule: after any structural fix,
  ablate every hack in the function and keep only those whose removal costs.*
  `[vc8]` same, and not yet done: see "Workarounds for the wrong compiler" below.
- **"The ceiling."** `[vc6]` **reverted.** At 1,507 of 1,645 the remaining 138
  were triaged as "register allocation and loop rotation the source cannot
  steer" and declared the practical limit. The cause was stub structs reached
  through `base + OFFSET` casts (section 5); typed members took the count to
  1,639. A band of 5 to 13-byte near misses written off as "the allocator wall"
  mostly closed too. *Rule: do not declare a wall until typed structs, real
  classes, the frame layout and a from-scratch derivation have been tried.*
  `[vc8]` same class, different cause: the wall was the service pack (below).
- **A narrower type that scores well.** `[vc6]` **reverted.** Narrowing a
  32-bit uid or a field (`approach` holds 2 for a roach) lowered HudSquadUpdate's
  printed distance. Rejected: the type is wrong, and the gain was a jump table
  shifting (section 10).
- **Search-tool proposals that change meaning.** `[vc6]` **reverted.** A
  statement-move search proposed RowUpdate 164 -> 152 by moving `spr = r->sprite`
  below the statements that read `spr`; an operand-swap sweep had a precedence
  bug (`b > 0` out of `a - b > 0`); a cast-dropping sweep produced "wins" that
  truncated values; an unfiltered store-swap pass proposed exchanging
  `delta = AngleDelta(...)` with `delta = abs(delta)`. Scoring with jump targets
  masked rewarded truncated functions. *Rule: a search only proposes. Read every
  accepted edit for a dependency or a domain change before applying it.*
  `[vc8]` no meaning-changing proposal recorded; the search's wins were number
  wins (below).
- **A behaviour fix that raises the number is still right.** `[vc6]` **held.**
  UpdateTrooperAction 380 -> 464 and DepthInsert 460 -> 468 when real bugs were
  fixed (a held weapon never dropped; an append linking a node to itself).
  DepthInsert later reached 0 and UpdateTrooperAction stands at 8. A higher
  distance with the image's control flow beats a lower one without it.
- **Register-only residue over a lower number.** `[vc6]` **held.** ScriptPad was
  moved from 10 to 16 on purpose: the 16-byte form differs from the image only in
  register choice, the 10-byte form had a store in the wrong place. Check with a
  diff that masks registers and turns jump targets into instruction indices.
- **Claims that were wrong.** `[vc6, vc8]` **reverted.** "A two-byte difference is a
  GOG-patched jump" (the reference already had the patches undone); "declaration
  order decides register order" (section 11); two diagnoses from a second model
  that named the wrong source block. Verify a claim against the bytes before it
  goes into a document.

### `[vc8]` Commandos

Every item raised the count or closed a question when it landed.

- **"The compiler's choice" that was the wrong service pack.** `[vc8]`
  **reverted.** Under RTM (.42) whole families were parked as allocator quirks
  no source could steer: a 1 made by `lea eax,[zero+1]` (`Alcanzado` of
  missions 21 to 26, 0x474d80..0x475870, 14 bytes each), a switch's zeroed
  register pushed or stored (0x532360, 22 bytes; 0x46bb30, 76), `jb` closing
  an unsigned table scan where RTM emits `jl` (0x5b3d20, 0x5ed7f0, 0x5b3c60,
  0x5db9a0, 1 byte each). At 4,419 matched, 26 of 56 parks were this family; 22
  flag combinations on 0x512640 and `/GL` moved none of them. The Rich header
  says build 50727 for RTM and SP1 alike. With SP1 (.762): 9,684 -> 9,724, 40
  parks identical with their plain source, no RTM match lost (two respelled)
  *(record; the byte counts are the RTM build of `83ef99f8`)*. *Rule: when a
  family of near misses resists every spelling and flag, list every compiler
  revision that stamps the same Rich build and try each before tuning further.*
- **Workarounds for the wrong compiler, never swept.** `[vc8]` **number win
  (suspect).** What was written to dodge RTM (result variables added only to
  hold a zero, swapped operands, `volatile` locals, the notes on 609 parks) is
  still in the tree. `STATUS.md` says to re-test the plain forms against SP1; no
  commit in the log does it. *Rule: after a compiler or flag correction, ablate
  every hack and re-derive every park note before any new tuning.*
- **Whole-body asm for compiler output.** `[vc8]` **superseded.** Five
  stream-sentry constructors and destructors (0x5867b0, 0x5867e0, 0x586ea0,
  0x586ef0, 0x586fb0) and 0x586f20, 0x586f60, 0x5b1030 were matched as naked
  `__asm` (count 9,672). All eight are C++ now and identical (virtual-base
  member access for the sentries). 0x586d00's asm was deleted and its C++ is
  parked at 18 bytes: one match given up on purpose. 0x6100a0 was byte-matched
  as inline asm, then replaced by C++ that is not identical (379 instructions
  against 385) and is parked. *Rule (owner): a body is whole-body
  asm only if the original source was assembly; an asm copy of compiler output
  is copied bytes, not source.*
- **The overcorrection: real assembly rewritten as C++.** `[vc8]` **reverted.**
  The same audit rewrote the 18 hand-assembled blit routines as C++: 10,049 ->
  10,030. They were restored as `__asm` (10,048; 10,049 with 0x64a500). The
  evidence that they were assembly: a C frame around an `__asm` block (`push
  ebp; mov ebp,esp`, exactly the registers the body touches), `ebp` reused as a
  data pointer, loop counters in globals, a stray `movsw`, assembler alignment
  padding, and `/GL` callers that rely on `edx` surviving the call. *Rule:
  decide asm or compiler output from the frame and the loops before choosing
  the language; then leave real assembly alone.*
- **Library code counted as matched.** `[vc8]` **reverted** (owner). The VC8
  `<string>`, `<locale>` and iostream instances in the image were matched by
  compiling probes with the real headers and scanning for equal bytes
  (`stdscan.py`); 9,547 -> 9,401 when they were removed, 9,300 after the
  stand-in classes the generators had given library code went too. The method
  works; the scope was wrong. *Rule: fix what counts as the game's code before
  counting, and name library ranges so generators skip them.*
- **"Unwritable" calling conventions.** `[vc8]` **superseded.** Functions
  taking an argument in `eax` were written off ("only naked asm, which is not
  reconstruction"); they are `static __declspec(noinline)` functions (0x441b80,
  0x48a6f0, 0x48a710, 0x526d60, identical). A thiscall-looking function with a
  plain `ret` was "a dead end, park at once", and arguments in `edi`/`esi`/`ebx`
  were then filed under the static form too: 0x5f7520 written that way was
  removed. They are `/GL` code and match under `/GL` + `/ltcg` (0x5f7520,
  0x5f7640, 0x5f7670, 0x4b10b0) *(record)*.
- **"Suspect `/GL`."** `[vc8]` **reverted.** The `eax`-against-`ecx` scratch
  choice at 0x4721d0 (2 bytes; unmoved by `/Ox /O1 /Os /Ot /Ob2 /G6 /G7
  /arch:SSE`) was blamed on whole-program allocation. It was a missing local:
  `int id = *(int *)b;` before the condition (identical). Measured later, `/GL`
  changes none of the scratch families (the `jb` scan, the `edx` tail call,
  0x512640).
- **Parks that were misreads.** `[vc8]` **superseded.** 0x480350 sat as a
  store-order near miss; its `ret 8` shows two arguments, and it matched once
  rewritten. Three `InfoEstado` parked as "rep movs count set up before the
  switch" were the `strcpy` intrinsic once per case. The "zero" at 0x6dd5bc was
  the literal 180.0f. Two serializers' parked approximations were simply wrong
  *(record)*. *Rule: before respelling a 2 to 4 byte miss, re-read `ret N`,
  each argument's slot, the field order of the stores and the constant's bytes.*
- **A comparer that learns.** `[vc8]` **held as a hazard.** `bytematch.py`
  records any unknown relocation target as a new symbol, so a wrong import
  (`SendMessageA` for `PostMessageA`), a wrong CRT name or a table's end bound
  "matched" once and broke later; the first `/GL` comparer masked addresses and
  the strict one at once caught a misnamed global *(record)*. *Rule: an address
  must be named before it is compared; never let the scorer supply the name.*
- **Counting names, not addresses.** `[vc8]` **held as a hazard.** "4,565
  identical" was 4,282 distinct addresses once counted that way; `83ef99f8`
  under RTM is 10,036 names on 9,512 addresses. Aliases, duplicate definitions
  and scratch copies inflate the first number, and deleting a "duplicate" can
  lose the only definition of an address (37 such files checked: removing them
  loses 36). *Rule: report distinct addresses, and diff the address sets before
  and after any cleanup.*
- **Matches on a wrong class layout.** `[vc8]` **reverted.** `SimBichoBase`
  redeclared two virtuals with other parameter types; they became new slots,
  the class's small members still matched, and every virtual call past slot 23
  was off by 8 until one such caller was written. *Rule: a derived
  redeclaration keeps the interface's exact signature; a matching leaf says
  nothing about the vtable.*
- **"No candidates left."** `[vc8]` **reverted.** At 9,586 the work list built
  from `symbols.tsv` was empty. About 2,200 function starts had no symbol at
  all, and a function only declared `extern` counted as done. Lists rebuilt from
  the comparer's `ok` lines gave 9,589 -> 9,640 in the next passes *(record)*.
- **Search wins that are number wins.** `[vc8]` **number win.** The variant
  search's applied matches for 0x4ea6a0 and 0x4803b0 are `volatile float` on a
  local; both still carry the note that the image spills differently. Its
  `int`/`unsigned` rewrite can change meaning; no case is recorded, none was
  audited.

## 2. Method

- **Score the relinked bytes, nothing softer.** `[vc6]` **held.** Compile every
  unit with the original compiler, place each function at its original address,
  resolve every relocation to the address the original linker wrote, compare to
  the image with nothing about reconstructed targets wildcarded. Masked "shape"
  scores hide a swapped global (`H*W` against `W*H`), a wrong jump target and a
  wrong register; they are for reading a diff, never for scoring one.
  `[vc8]` weaker here: objects are compared unlinked, relocations masked and
  their targets checked by name, and the checker learns unnamed targets
  (section 1).
- **A one-second screen plus a full confirm.** `[vc6]` **held.** Save the symbol
  tables from one full relink; a screen then compiles one unit, resolves it the
  same way and prints `FN old -> new`. Confirm every win through a full relink:
  an edit to a shared header or text shared with a sibling moves other functions.
- **Distances are lumpy.** `[vc6]` **held.** One misplaced block or one byte of
  length shifts every later displacement: a single line moved functions from 770
  to 170. Pick work by what the diff says, not by size.
- **Judge length first.** `[vc6]` **held.** With several known single-hunk fixes,
  each alone changes the length and scores worse; test all subsets and take the
  one whose length equals the image's (FindPath 66 -> 54 from three edits that
  scored 98, 100 and 132 alone).
- **Jump tables flatter the metric.** `[vc6]` **held.** A table after the `ret`
  holds absolute label addresses, so a 4-byte length change moves about 70 bytes
  of table. Score length-changing edits against the full image, and check that a
  residue is not table words decoded as instructions.
- **Behaviour first.** `[vc6]` **held.** Where the image's control flow disagrees
  with the source, that is a reconstruction bug. Read large diffs for stores the
  image has and ours lacks, back-edges landing after a load, and guards whose
  jumps land outside ours. This found more than a dozen real bugs. Reproduce the
  original's defects (a null dereference, a lock never released); do not repair
  them.
- **Check every pushed argument's source.** `[vc6]` **held.** `push imm` against
  `push reg` in a masked diff was a wrong argument (MakePlacedUnit 84 -> 5).
  Diff with absolute addresses kept and name each global: a named address with
  no swap partner is a wrong global (three bugs found).
- **Re-derive which source lines a hunk belongs to.** `[vc6]` **held.** A hunk
  diagnosed as a register choice in one `case` was in another; fixing the right
  one took ScriptParseActionRecon 218 -> 50.
- **A scratch clone must reproduce the relink first.** `[vc6]` **held.** Compile
  a cloned function with the relink's own defines and path, or its registers
  differ for unrelated reasons. A clone's differing-line count is not a proxy
  for the relink distance (47 -> 27 lines in a clone was 82 -> 98 in the relink).
- **One compiling job at a time**, and refresh the saved symbols after changing
  any signature: MSVC mangles parameter and return types, so a stale table
  resolves calls to a stub and mis-scores callers. `[vc6]` **held.**

- **Park with a marker and the exact difference.** `[vc8]` **held.** `~name` in
  `symbols.tsv` keeps a near miss out of both counts while its address still
  validates callers; `// 0xADDR ~ (why)` sits on the definition. 1,186 `~`
  lines and 1,164 markers at `83ef99f8` (functional ports included).
- **Fixed-shape functions by generator, before any hand work.** `[vc8]` **held**
  *(record)*. Bodies decided by their bytes (getters, setters, forwarders,
  deleting destructors, vptr-store constructors, template factories) are
  emitted from byte patterns and kept only when identical: 491 on the first
  build of `gentrivial.py`, 82 of 84 factories, about 1,000 accessors in one
  sweep; about a third of all matches.
- **Decompiler candidates, the compiler as judge.** `[vc8]` **held** *(record)*.
  Kuna's C with struct synthesis off, rewritten mechanically and kept only when
  identical: about 390 of 1,900 candidates, 267 of 701 under 32 bytes, nothing
  past 200 bytes, 0 of 264 on a later rerun.
- **Group the unmatched by masked bytes.** `[vc8]` **held** *(record)*. With
  calls and absolute addresses zeroed, a group of two or more is one source
  shape; one match pays for all (about 60 exact duplicates needed only a second
  name).
- **Write for behaviour first, then compare.** `[vc8]` **held** *(record)*. Of
  the functions ported from the disassembly for function only and parked
  unread, 23 were already identical and 9 more matched after edits: 10,050 ->
  10,082.
- **Count after a clean build, and diff the address sets after any deletion.**
  `[vc8]` **held.** A failed compile leaves the old object and its match. A
  prefix `sed` on the symbol table deleted another file's lines three times, a
  line-number delete run twice on stale listings lost 9 matches *(record)*.

### Method, folded `[vc8]`

Folded from the project record, duplicates merged: **held** *(record)* unless
an item says otherwise.

- `[vc8]` Run a generator over the unmatched addresses plus the ones it
  already produced; otherwise its output file loses earlier matches.
- `[vc8]` Sweep the whole image for bodies fixed by their bytes, not only
  vtable slots: with no `/OPT:REF` every out-of-line copy of an inline member
  is kept, called or not. Sweep lone-`jmp` slots in primary and secondary
  vtables. Hash function bytes (call targets made absolute) to find exact
  duplicates of matched functions; each needs only a second name.
- `[vc8]` Give every literal a symbol line: a pushed string address
  (`??_C@...`), a float constant (`__real@...`, by value), an import slot
  (`__imp__Name@N`, declared `__declspec(dllimport) __stdcall`). An unnamed
  literal reads as a mismatch.
- `[vc8]` Write float literals with the image's exact bits (nine significant
  digits); read the constant from the image, never guess it.
- `[vc8]` Do not trust the decompiler's extent: it drops float tails and
  invents arguments. Take the argument count from `ret N`, and check for a
  `je` that leaves past the first `ret` (a cold tail laid out after it).
- `[vc8]` Compare instruction bytes, not listing text, when one displacement
  differs: it is a struct off by a word or a local of another size.
- `[vc8]` Score only the function under test in a hill climb; a second
  differing function in the file corrupts the score.

## 3. Control flow and block layout

- **Write each site out; remove hand-made shared labels.** `[vc6]` **held.** The
  compiler merges identical tails itself, so a hand-merged tail is a different
  layout. ScriptParseActionRecon 1498 -> 218, AiTrooperStep 484 -> 134.
- **Which copy of a merged tail survives.** `[vc6]` **held.** VC6 keeps the copy
  in the arm that comes later in the source. One label on the first copy and one
  `goto` to it from any other arm makes the first copy the survivor.
- **A goto target that will not stay put goes after the function's last
  statement.** `[vc6]` **held.** Labels inside a `switch` kept VC6 from sharing
  blocks; placed after the final `return` they landed where the image has them
  (SelectFirePose 300 -> 240, RegionFindPath).
- **A labelled block stays where it is only if no later `goto` targets it.**
  `[vc6]` **held.** Otherwise it moves after its last predecessor.
  FlowRecvMessage 540 -> 18.
- **A forward `goto` pulls its target up.** `[vc6]` **held.** `if (x) goto L;`
  lays `L`'s block straight after the test. Written `if (!x) {...} else {L}` the
  block stays in source position (SightScan 1324 -> 1048). The same pull is
  useful when the image has a return duplicated inline:
  `if (changed) goto cool; return;` (Step3Input 96 -> 54).
- **`for (;;)` and `while (1)` are different loops.** `[vc6]` **held.**
  `for (;;) { if (c) break; ... }` is rotated (test copied to the bottom);
  `while (1)` tests at the top and jumps back. RecvThreadProc 122 -> 16.
- **`inc; cmp; jge exit; jmp top`** is `while (1) { ...; i++; if (i >= N) return; }`
  with the increment its own statement. `[vc6]` **held.**
- **Success path as the `if` body; positive nesting when the trivial case
  reaches the shared tail.** `[vc6]` **held.** Reversing a condition changes which
  side falls through.
- **Guards in the order the image compares them, combined into one `||`.**
  `[vc6]` **held.** CommSend 26 -> 0; FindPath 772 -> 172 with the budget test
  inside the loop.
- **`else break` nesting decides where the loop exit lands.** `[vc6]` **held.**
  ScriptPad 496 -> 280.
- **Every `continue` as `goto next` with one label at the loop end.** `[vc6]`
  **held** (SightScan 528 -> 466; that function still differs for another reason).
- **Switch shapes.** `[vc6, vc8]` **held.** Separate identical arms give a plain jump
  table; shared labels give a range test or a byte table. `sub/je/dec/je` is
  separate `case X: return 1;` arms. A table slot pointing at the default is an
  explicit `case N: return 0;`. Arm layout follows source order, the table index
  the case value. `mov eax,K; push eax` in every arm is a local set per arm and
  one call after the switch, which VC6 copies into each arm.
- **A dead `cmp` with no branch after it** is two identical if/else arms the
  compiler merged. `[vc6]` **held.**
- **Small shared tails reached by `goto` get duplicated; do the work in each
  branch and `break`** to get one block (VC6 cross-jumps identical tails).
  `[vc6]` **held.**
- **`xor r,r; test; jle; mov r,X` is `x = 0; if (c) x = X;`**; a ternary gives
  `mov; jmp; xor`. `sub 3; neg; sbb; and K; add B` is the two-constant ternary
  `v == 3 ? B : B + K`. `[vc6, vc8]` **held.**

- **One result variable, one exit.** `[vc8]` **held.** A zero or one kept in a
  register and returned is `int r = 0; ... if (e) r = e->F(); return r;`; early
  `return`s let VC8 fold the constant. `Tabla557240::Aplicar` (0x5572c0),
  `ClassSubInterfaceMultiple::Tiene` (0x5e1a30), `CWinEditAngle::Desplazamiento`
  (0x4ca620, preset 1). Found on RTM, still identical. It does not close every
  kept-zero case (section 12).
- **A materialised `== 0` that is then tested** (`xor eax,eax; cmp [m],eax;
  setz al; test eax,eax; jz`), listed unsolved, is the result variable again:
  `int b = (x == 0); if (b) b = F(); return b;`. `[vc8]` **held.**
  `WinEditorMisiones::PuedeCerrar`/`PuedeSalir` (0x4ade00, 0x4ade50),
  `CaracterUnico` (0x64ee60).
- **Write identical arms out twice** when the image has `cmp; jmp L` with the
  conditional jump at `L`: VC8 cross-jumps them itself; a ternary or flag gives
  `setne`. `[vc8]` **held.** `ClassAcCasa::Marcar` (0x4ef9e0).
- **`sub eax,K; je` chains are a `switch`**, also with one case
  (`mov eax,[x]; sub eax,1; jne`). `[vc8]` **held** *(match)* (0x4b4720,
  0x568530).
- **A 0 or 1 argument pushed through a branch** is `if (c) f(1); else f(0);`,
  two calls the compiler merges; a ternary gives `setcc`. `[vc8]` **held.**
  `Lista4926f0::Marcar` (0x4926f0).
- **Two branches that store the same field** match only as one conditional
  expression. `[vc8]` **held** *(match)* (`DiferenciaAngulo` 0x4eea40,
  0x62b670).
- **Loop close.** `[vc8]` **held** *(record)*. An `int` index loop over a table
  ends `jl`, a pointer loop `jb`; `(int)p < (int)end` forces `jl` on pointers.
  An unsigned index against a constant bound ends `jb` only on SP1 (0x5b3d20);
  on RTM no spelling gave it.
- **Falling off the end.** `[vc8]` **held** *(record)*. A value-returning slot
  whose image leaves `eax` unset is written with no `return` (C4716 off);
  `return 0` adds an `xor`.

### Folded `[vc8]`

Folded from the project record, duplicates merged: **held** *(record)* unless
an item says otherwise.

- `[vc8]` **Result variable, all its forms.** A constant kept in a register
  across calls or a loop and returned: declare it before the work, assign it
  only on the path that changes it, return it once (`int r = 0; for (...) if
  (hit) { r = 1; break; } return r;`). Preset it to the common value. Where the
  loop compares a field with that register, compare with the variable
  (`e->hecho != r`). A literal `return 0`, or an `r` assigned on every path,
  folds to an immediate.
- `[vc8]` **Fall-through is the success path.** `je Lend ... Lend: xor eax,eax;
  ret` is `if (c) { work; return 1; } return 0;`, not an early `if (!c) return
  0;`. A failure block laid out last is the `else` of the first test. A miss
  and a null case sharing one `xor eax,eax` is `if (p) { ... return p; } return
  0;`.
- `[vc8]` **Two conditional jumps to one out-of-line block** after the epilogue
  is a single `else`: `if (a && (i = F()) != -1) {...} else G();`.
- `[vc8]` **One ternary store** (`f = o ? o->g : 1;`, `*out = n ? v : 0;`, `x =
  c ? Call() : 0;`) shares the store and the return; `if/else` stores do not.
  The reverse: `test; mov ecx,K; je; mov ecx,eax` is a default overwritten (`q
  = ""; if (p) q = p;`), not a ternary.
- `[vc8]` **Flags.** `int c = a && b; if (c) ...; if (c) ...;` gives the
  out-of-line `xor reg,reg` block. `test byte [m],0x60; jnz` with a branch is
  `(b & 0x20) || (b & 0x40)`; `(b & 0x60) != 0` gives `neg/sbb`. `mov; or;
  jne` on two ints is `!(a | b)`. `setcc al` on return is a `bool` local;
  `neg/sbb/add 1` is `int x = !cond`; returning the comparison directly is
  `sbb; add 1`.
- `[vc8]` **Compare spelling picks the jump.** Unsigned `x > 0` is `jbe`, plain
  truth `je`; `cmp r,1; ja` needs an `unsigned`; `> 0` not `>= 1` when no 1 is
  kept; `test r,r; jns` is `x & 0x80000000`, not `x < 0`; `cmp r,m` wants the
  key on the left (`key == a[i].d`).
- `[vc8]` **`switch` details.** Grouped labels and separate identical arms lay
  out differently. A `jle`/`jg` pair is `switch` cases where a range test
  folds to one unsigned compare; a range test on a member (not a local) keeps
  both compares. Bytes equal but `$LN` labels differing means the
  case-to-block mapping is wrong: decode the table.
- `[vc8]` **Tails.** A tail duplicated only in part is one inline helper called
  from each branch. A duplicated call pushing a register known to be 0 is one
  call after the `if/else`. A branch landing on a base call means the base
  call is outside the `if`. Two paths ending in one call: `else goto label;`
  into the block that has it. An early `return` inside a nested `if` lays out
  the shared `mov eax,1`.
- `[vc8]` **Loop forms.** A search that reloads the array pointer each
  iteration and enters at the increment is `do { if (o->At(i)) return i; i++; }
  while (i < o->n);` with an inline checked accessor. A back edge on the flags
  of `add ecx,4` is `while (p && *p)`. A top test with a `jl` bottom branch is
  `do { ...; e++; i++; } while (e < end);`. A straight run of identical blocks
  is a short constant `for` VC8 unrolled; write the loop. A loop with a call
  in it is not unrolled.
- `[vc8]` **Table search with inline `strcmp`.** If `return 0` precedes the
  found block, do the found work after the loop (`if (i < N) {...}`); two
  identical found-returns are one `goto` target after the `return 0`.
- `[vc8 rtm]` A short tail the image jumps to and VC8 duplicates did not move
  with case order or `goto`.
- `[vc8]` **Loop shapes with two exits.** Read at the top, compare at the
  bottom: `do { r = Read(); if (bad) return 1; } while (strcmp(...) != 0);`
  (0x62a710). A copy loop that stops on a separator or the terminator:
  `do {...} while (c != '\\' && c); if (!c) fin = 1; else p[k-1] = 0;`; a
  `goto`, or a `for (;;)` with two `break`s, gives a peeled or rotated loop
  instead (0x62ae30). **Held.**

## 4. Variables, loads and stores

- **Loads never move above stores; stores do get reordered.** `[vc6]` **held.**
  VC6 does no alias analysis between a store through a pointer and a later load,
  so the image's load-before-store order is the source's statement order. One
  exception: a local whose address was passed to a call is read before a heap
  store only when declared in an inner block.
- **A register reloaded mid-function means the source reassigns the variable.**
  `[vc6]` **held.** ListUpdate 5 -> 0.
- **A value read at the top of an arm and used after calls is a local**, not a
  re-read; fields reused across a call are scalar locals. `[vc6]` **held.**
- **The loop's own variable, not a copy of it.** `[vc6]` **held.** SightScan
  774 -> 528.
- **Separate the walk pointer from the pointer used after the loop.** `[vc6]`
  **held.** One variable for both gives one web with the loop's weight; a
  distinct loop local assigned to the shared one only on a match gives two.
  DepthInsert 322 -> 0.
- **A bare declaration plus an assignment right before first use**, where an
  initialiser at the declaration orders the webs differently. `[vc6]` **held.**
- **A parameter used through a local copy is not always propagated away.**
  `[vc6, vc8]` **held.** `T *o = obj;` changed base/index order and where a parameter
  is reloaded after an inline `strcpy` (ReceivePlayerMsg 2 -> 0). The reverse
  also holds: renaming the parameter and dropping the copy lost two matches,
  which came back with the copy.
- **Dead argument slots are spill slots.** `[vc6, vc8]` **held.** VC6 parks byte or
  address-taken locals in a parameter's home once the parameter lives in a
  register. `mov [esp+N],cl; mov edx,[esp+N]; and edx,0xff` is a `uint8_t`
  local, not a parameter.
- **Frame size is evidence.** `[vc6]` **held.** A different `sub esp` is a missing
  or extra local. Map every `[esp+N]` to an entry-relative slot (subtract pushes
  and saved registers) and name the image's slots before changing anything.
  LoadMap 502 -> 135 from the frame alone: a buffer's true size, two locals that
  were one, one that was two.
- **Stack arrays.** `[vc6, vc8]` **held.** Function-scope arrays never share storage;
  block-scope and inline-local arrays overlap at the lowest free offset. A frame
  too small for one buffer per name means the original reused a buffer
  (CheckMapRules 9 -> 0). Array size moves slot order (HudEdgePaint `text[4]` to
  `text[3]`, 37 -> 2).
- **Runs of `mov [esp+N], imm`** are a local array initialised from constants;
  declaration order decides the order of the store groups. `[vc6]` **held.**
- **Chained assignment.** `[vc6, vc8]` **held.** One load stored to two fields is
  `a = b = src;` (inner first). `a = b = c = 0` stores right to left, so write
  separate statements when the image stores in declaration order.
- **Commutative operands.** `[vc6]` **held, fragile.** VC6 loads `A + B` as `B`
  first, so source order is often the reverse of the loads. Where both operands
  are globals the order can instead follow the compiler's symbol table
  (section 11).
- **Word-sized locals cost a `movsx`**; a local the image keeps on the stack
  while ours takes a register may want a narrower type (ObjAfterMove's height as
  `int8_t`, 58 -> 30). `[vc6]` **held.**
- **Write the expression straight into the field when the image compares the
  computed value**; `if (!g && (sel = f->x) >= 0)` loads the field after the
  global test. `[vc6]` **held.**

- **Compare with the variable, not the literal.** `[vc8]` **held.** A register
  reused as a known value (`sub edx,1; jne; cmp [eax+0x10],edx`) is
  `int t = o->tipo - 1; if (t == 0 && o->campo10 == t ...)`. `MotorAliado::s09`
  (0x4468f0), first filed as a compiler quirk.
- **The loop bound is the member, not a copy of it**, when the image re-tests
  the count after the loop. `[vc8]` **held.** `ClassMenu::Cambiar` (0x5a1320).
  The same idea did not move 0x623900.
- **A reference to the element anchors the loop pointer** on a later field
  (`T &e = t[i];`); `p++` or `t[i].f` anchor elsewhere. `[vc8]` **held.**
  `Tabla583d30::Vaciar` (0x583d30).
- **Store order: read it as structure first, then permute.** `[vc8]` **held.**
  Stores that precede the body, or sit between the vptr store and a
  registration call, come from member sub-objects with inline constructors, in
  declaration order (`Datos612b20` 0x612b20: `TresCero`, `UnCero` members and a
  two-line body). Only then try orders: all 24 for four stores, a pairwise hill
  climb (`hcstores.py`) for more (0x5655d0, 0x59d5b0 *(match)*). Brute force
  ruled order out for several parks; those were something else.
  Symptoms that name the structure: fields written through one base register
  (`lea edi,[esi+0x68]` then `[edi+0x4c]`) are a sub-object with its own inline
  constructor and the body writes the rest through a pointer to it (0x4fda00,
  0x4e8470); a store of 0 then of 1 to the same field, both kept, is a member
  constructor's dead store under a body that overwrites it (0x4e3980); one
  instruction (`lea`, `push`) sitting between two store pairs means the block
  is two members (`Lugar { y=0; x=0; }`, `Tamanyo { ancho=1; alto=1; }`,
  0x642140, one match). `[vc8]` **held.**
- **A float stored and reloaded before a compare.** `[vc8]` **number win.**
  `volatile float` reproduces the `fstp/fld` pair and the frame size
  (`Radio4364e0::Radio` 0x4364e0, 0x4ea6a0, 0x4803b0). A plain `float` local
  gives the round trip in other functions *(record)*, so try that first.

### Folded `[vc8]`

Folded from the project record, duplicates merged: **held** *(record)* unless
an item says otherwise.

- `[vc8]` **Local or member path decides the reload.** Loaded, tested, then
  used from the register: a local (`T *v = s->campo; if (v) v->F();`). Reloaded
  after the test or after a call: write the member path again, no local. A
  pointer the image keeps in a callee-saved register (`lea ebp,[this+K]`) is a
  local pointer (`Sub *t = &s;`). For a call through a chain, keep the
  intermediate object in the local, not the final pointer. Reading an element
  twice (`t[i]->a`, `t[i]->b`) and through a local order registers differently:
  try both.
- `[vc8]` **Indexing or walking.** A loop pointer aimed at a field, other
  fields at negative offsets, or two pointers stepping together: the source
  indexed the array (`a[i].f`). A pointer to the element's start: a `T *p`
  walk. A reference (`T &e = t[i];`) is a third form.
- `[vc8]` **Stores.** Two constant stores to adjacent fields can come out in
  reverse of source order. A dead store to an address-taken local array stays
  in the image: write every store, in the image's order. Three members and an
  array of three schedule differently.
- `[vc8]` **Frame.** Size buffers until `sub esp` agrees; a frame larger than
  its visible locals has an unused gap, reproduced by one struct holding the
  neighbours and the gap as used members. Two ints that exist only as
  arguments with one spilled are a local struct. A variable declared at
  function scope and assigned later keeps a slot an inner-block one shares.
- `[vc8]` **Float spills.** `fstp [esp]; fld [esp]` before a call or compare is
  a `float` local holding the value (`float s = x*x + y*y; (float)sqrt(s)`);
  one expression keeps it in `st0`. A conditional that is stored and reloaded
  is `float t = c ? 0.0f : f(); float v = t;`. A compare of an expression
  against zero stays in the condition. Where a plain local will not spill, see
  section 9.
- `[vc8]` **x87 compares follow the written form.** Which value is loaded
  first, `fcom` or `fcompp`, and `test ah,0x41` against `test ah,5` all change
  between `a < b`, `b > a`, `!(a >= b)` and `b <= a`: try all four. A literal
  on the left gives a double constant; `f < 0` with an int zero gives `fldz`;
  `fcomp [const]` right after a call is a `float`-returning callee against a
  literal; a named global gives `fld; fcompp`; a `const float` global is
  loaded once and reused.
- `[vc8]` **Conversions.** `fadd [2^32]` after `fild` is unsigned to float
  (dword constant in a `float` expression, qword in a `double`). Inline
  `fnstcw; or 0xc00; fldcw; fistp` is float to `unsigned`. A double constant
  that is a widened float is `(double)0.0015f`.
- `[vc8]` **Integer shapes.** Halving is `>> 1` (`/ 2` adds `cdq; sub`).
  `member % N` is `idiv`; through a local copy it is the magic multiply.
  `shr 2; add r,r; add r,r` is unsigned `x / 4 * 4`. `shr r,16; cmp r,K` is
  `(unsigned short)((unsigned)x >> 16) == K`. `i++` inside call arguments
  orders `imul`/`add`.
- `[vc8]` **String shapes.** `char buf[N] = "LIT";` is the literal's copy plus
  a clear of the rest. A literal copied in word moves is a struct assignment
  (`*(T9 *)buf = *(const T9 *)"..."`) or the `strcat` intrinsic. `mov ecx,N;
  repe cmpsb` is `strcmp(s, "LIT") == 0`; two pointers give the `sbb eax,eax;
  sbb eax,-1` loop; `memcmp` gives neither. `memmove` is a call, `memcpy`
  inline. `strcpy(dst, "")` as a byte loop copies from the shared empty
  literal: name it. Write accents as octal escapes. `lea r,[eax+1]` after a
  length call is `n + 1` written at each use. Force a real call with `#pragma
  function`.
- `[vc8]` **A zero stored after an early return is declared there.** A local
  initialised to 0 that the image stores only after the first `if (...) return`
  block is declared at that point, not at the top; at the top ours schedules
  the `mov [esp+x],0` into the early-return block (0x59bad0). **Held.**
- `[vc8]` **An indexed store right after zeroing the index blocks folding.**
  `n = 0; a[n] = -28;` keeps the later increments of `n` as separate
  instructions where `n = 0; ...; n = 1;` folds them into a constant
  (0x646d10). **Held.**

## 5. Structures, types and parameters

- **A stubborn mismatch is usually a stub struct.** `[vc6]` **held.** Typed
  member access (`obj->army`) and pointer arithmetic on a byte pointer
  (`*(int8_t *)(t + 0x10)`) compile to different register allocation, scheduling
  and branch folding at the same offset. Flesh the struct out to its real layout
  and use members. The count had been declared stuck at 1,507; it resumed
  climbing from the first typed struct and reached 1,639 with this and the
  other levers here.
- **A member's signedness follows its reads.** `[vc6]` **held.** If every read is
  unsigned, declare it unsigned; a mismatch turns `ja`/`jbe` into `jg`/`jle`. A
  field read signed in one place stays signed and the unsigned reads take a cast.
- **Typed pointer subtraction is not a byte divide.** `[vc6]` **held.** `p - base`
  on structs is VC6's unsigned exact divide (`mul`/`shr`); the image's
  `bytes / 76` is a signed `imul` magic. Keep the byte-distance form there.
- **Retyping a pointer silently scales old arithmetic.** `[vc6]` **held.**
  `p + OFFSET` on a now-typed `p` scales by `sizeof` and still compiles. Scan
  every touched unit for pointer-plus-integer on record types; an old half-word
  view's doubled index (`where[i * 2]`) survives that scan and only the relink
  catches it.
- **Narrow parameters show at the call site.** `[vc6]` **held.** A byte register
  pushed with no `and 0xff` means the callee's parameter is `uint8_t`; a byte
  local's whole dword slot pushed raw means the same. `mov reg, dword [x+2];
  push` is an `int16_t` lvalue passed as a field, not a composed dword.
- **Halves read from an argument slot** (`movsx r, word [esp+N+2]`) mean the
  parameter is a point struct. **`mov ecx,eax; push eax; push ecx`** is one value
  passed as two by-value struct arguments. `[vc6]` **held.**
- **A by-value struct parameter changes the caller's allocation.** `[vc6]`
  **held (as a caution).** Retyping one packed-point parameter from `uint32_t` to
  a point struct cost a caller its match; restoring the scalar restored it. Most
  such retypes were neutral. Where a caller stops matching, the original took a
  scalar there.
- **A parameter may not be what its prototype says.** `[vc6]` **held.**
  EncodeGlyph's first parameter is the byte cursor (`uint8_t *buf`, 164 -> 38). A
  pointer walked straight from `[esp+N]` is the parameter used as the cursor.
- **Lone scalars and elements load at different widths.** `[vc6]` **held.** A
  lone `short`/`char` global is read with a word/byte load; an element of a
  larger object with a dword load. VC6 also reloads a global after any pointer
  store. Where one function reads a global at its own width, give the relink a
  view declared beside that function, not in a shared header (section 11).
- **Aggregates alias as one object.** `[vc6]` **held.** If the build lumps several
  globals into one struct symbol, any two of them alias, which changes store
  scheduling around string intrinsics.
- **`memset` shapes.** `[vc6]` **held.** A zeroing `for` loop gives `rep stosd`
  with the destination set up first; `memset` sets it up last; `memset` of a
  small struct is inline dword stores through a copy of the pointer. A plain
  byte-store loop becomes a `memset` expansion unless the row address is computed
  before the inner guard (BoxAction 88 -> 0). Copy loops do not become `rep movs`.
- **A walking pointer anchors on one field.** `[vc6]` **held.** In a
  strength-reduced loop over an array of structs the induction pointer points at
  one field: the last one read in a straight-line body, the first after a
  leading test, the last again when every read goes through a held record
  pointer. Advancing the array pointer itself (`for (...; i++, rows++)`) anchors
  on the row's start (ParseScenarioPart 82 -> 48). An element pointer local moves
  the anchor too (DrawEffectLayer 616 -> 584 by indexing at each use).
- **Index spelling picks the addressing form.** `[vc6]` **held.**
  `g[x & 0xFFFF]` compiles `[idx+table]`; a variable index `[table+idx]`. A signed
  `jl` loop close is an index loop storing into a typed member array.

- **Small operators on member pairs.** `[vc8]` **held.** Per-field
  store-then-`sub [mem],reg`, or two ints updated through one base, is an
  inline `operator-=`/`operator+=` on a sub-struct (`ClassVolumen::Desplazar`
  0x6287f0); unrolled `mov` pairs instead of `rep movsd` are an inline
  memberwise `operator=` (0x587b00 *(match)*).
- **By-value structs and their copy constructors.** `[vc8]` **held**
  *(record)*. `sub esp,N; mov eax,esp; mov [eax],..` with loads before stores
  needs a user copy constructor; `fld/fstp` copies need one that copies as
  floats; without one the fields are pushed. A temporary copied through integer
  registers is the implicit copy.
- **`class` or `struct` is in the image.** `[vc8]` **held** *(record)*. RTTI
  names are `.?AV` or `.?AU`; a wrong keyword fails the type descriptor compare
  and silently drops the function from the comparer (the mangled name differs).
- **An "override" with another parameter type is a new slot.** `[vc8]` **held.**
  See section 1, "Matches on a wrong class layout".

### Folded `[vc8]`

Folded from the project record, duplicates merged: **held** *(record)* unless
an item says otherwise.

- `[vc8]` **Field types from the loads.** `movzx r32, byte [p+2]; and r32,K`
  is the low byte of an `unsigned short` field. A word load from the middle
  of a field is a fixed-point value read through a `short *`. An `int` copy in
  one case and `fld/fstp` in another is a `union { int i; float f; }`. A
  32-bit load of a flag parameter is `int`, not `bool`. `movzx` on return is
  an `unsigned` result. Records of equal size behind several names are one
  struct.
- `[vc8]` **Struct copies say how the struct was written.** Plain dword moves
  for a float struct: the implicit copy. `fld/fstp`: a user copy that copies
  floats. A copy through a stack temporary: `T t = o->pos; dest = t;` or `*p =
  T(a, b)` with a constructor. One load reused for two fields without the
  compared value: a struct assignment (`tam = *t;`). Paired stores from two
  registers: a small struct built by an inline constructor and copied
  (`*(Par *)&f = Par(a, b)`), the constructor's order choosing the registers.
  A pair passed from two adjacent members reloaded after a compare:
  `f(*(Par *)&a1)`.
- `[vc8]` **Scalars or a struct by value.** When a function of `(x, y, w, h)`
  differs only in register use, try the pairs as by-value structs. A by-value
  struct stays a tail `jmp` unless it has a user copy constructor.
- `[vc8]` **A field the base's member sets belongs to the base.** Move the
  layout into a real base class, not a cast. A callee taking `const T *const
  &` that is handed the address of the caller's own parameter means that
  parameter is by value.

## 6. Calls, inlines and C++

- **A helper with no address of its own is inline in the original.** `[vc6]`
  **held.** If callers test its result, make it return a 1/0 flag and let the
  caller index the data: returning a pointer leaves a null test the image lacks.
- **An inline wrapper reverses argument setup order.** `[vc6]` **held.** Its
  parameters become temporaries evaluated right to left: a bare `memcpy` sets up
  the destination first, `static inline void Copy(d, s, n) { memcpy(d, s, n); }`
  sets it up last, as four loaders needed.
- **An inline thiscall forwarder is not the member call.** `[vc6]` **held.**
  `Wrapper(obj, a)` over `obj->Method(a)` blocked push sharing between a failure
  arm and the next call; the direct member call matched. Parenthesise the object
  expression when scripting such a sweep: `(T *)v + OFF` scales.
- **An inline over field pointers is not an inline over the record.** `[vc6]`
  **held.** Passing `&out->state, &out->field` lost matches that a body naming
  the members kept.
- **Arguments pushed before an inner call** mean the call sits inside the
  argument expression. `[vc6]` **held.**
- **The same statement in parallel arms gets rotating scratch registers.**
  `[vc6]` **held.** If the image has the same register in two arms, the source
  had one statement after the if/else that the compiler tail-duplicated.
- **No unwind frame inside `extern "C"`.** `[vc6, vc8]` **held.** VC6 emits no
  `push -1; push handler; fs:[0]` frame in a C-linkage function and treats
  C-linkage callees as non-throwing. A function whose original has a frame must
  have C++ linkage; guard header `extern "C"` blocks so VC6 does not see them.
- **Callee definition order.** `[vc6, vc8]` **held.** VC6 infers "cannot throw" only
  for callees defined earlier in the unit, so keep definitions in original
  address order.
- **An EH state store around a constructor call is a real `new T(...)`**, not an
  allocation plus placement construction (MpPanelScreen 339 -> 260). Stores
  before the vptr store are member initialisers. `[vc6]` **held.**
- **Write the classes.** `[vc6, vc8]` **held.** Constructor/destructor pairs become
  `struct X : Base` with a mem-initialiser base and `~X()`; the compiler then
  writes each vptr store itself, and a destructor's comes first, before the
  unwind state, where no statement can go. A class with no destructor deriving
  through `: Base(n)` needs `typedef Base Base;` to avoid C2437.
- **Real virtual methods keep the bytes.** `[vc6]` **held.** Giving the classes
  the image's virtuals in slot order, with overrides forwarding to the
  reconstructed members, changed no compared byte in 1,646 functions; the
  compiler's `??_7Class@@6B@` is mapped to the image's vtable address.
  Spelling the call sites `w->Paint(r)` compiled the same call but tipped an
  unrelated register tie in another function (0 -> 28), so the sites keep a
  pointer-to-member reach into the vtable by slot index (a single-inheritance
  pointer-to-member is one code pointer, and it is the only way to spell a
  thiscall through a pointer in VC6).
- **`delete g` of a global.** `[vc6]` **held, number win in spirit.** The image
  copies the pointer for the free before the null test and calls the destructor
  on a second read of the global. Every single-value spelling compiles to one
  callee-saved web. `m = g; if (g) { g->~T(); operator delete(m); }` matches
  (MovieStart 10 -> 0). Found by editing the front end's IL until the code
  matched, then looking for the spelling.
- **Two case arms calling one address** are two identical source functions
  folded by the linker (`/OPT:ICF`). Identical empty virtuals fold with unrelated
  stubs of the same bytes. `[vc6]` **held.**
  `[vc8]` opposite: no folding in this image (incremental link). Identical
  bodies each keep an address and each needs its own name
  (`~ClassLineaVariable` at 0x5a67f0 and 0x5a6b10, both identical).
- **Enumerators compile exactly like literals.** `[vc6]` **held.** An out-of-range
  comparison constant spelled as an `enum` silences a host-compiler warning with
  no byte change; the same constant in range, or passed through an inline helper,
  changes the bytes.

- **`ecx` passing through is a member call on `this`.** `[vc8]` **held**
  *(record)*. Argument loads through `edx`/`eax` with `ecx` untouched before a
  callee-pops call: declare the callee a member, not a `__stdcall` free
  function. The commonest early near miss (six functions at once).
- **One inline member for a merged read-or-write call.** `[vc8]` **held.** The
  serializers' `test m; ... jz; mov eax,[edx+8]; jmp; mov eax,[edx+4]; call
  eax` comes from one inline helper (`ES(guardar, f, p, n)`); `if/else` at each
  site does not merge. `ClassAcPisada::Serializar` (0x51f0f0) and about 30
  siblings matched in a row.
- **String work is the intrinsic.** `[vc8]` **superseded** the "pointer
  difference loop" mystery. `strcpy`, `strlen`, `strcat`, `strcmp` under
  `#pragma intrinsic` (or `<string.h>` at `/O2`) expand to the loops that hand
  code could not reproduce *(record)*.
- **A function-local `static T x = f();`** gives the guard bit, the EH frame
  and the state numbers for free. `[vc8]` **held.** `Contador497780::Bajar`
  (0x497780); 0x587990, 0x5878b0 *(match)*. A guard bit and state that belong
  to another function mean that function was inlined.
- **A local filled by a call, EH state set after it,** is a function returning
  the object by value, not a default-constructed local. `[vc8]` **held**
  *(match)* (`DialogoOrgano4a5a30::Borrar` 0x4a5a30).
- **One object file per class.** `[vc8]` **held** *(record)*. Destructors sit in
  their own unit; merging two units changed a destructor (inlining). Where the
  image has a function both inline and out of line, keep it in the header
  behind an opt-out macro.

### Folded `[vc8]`

Folded from the project record, duplicates merged: **held** *(record)* unless
an item says otherwise.

- `[vc8]` **Tail calls.** A tail `jmp` needs the callee to take exactly the
  caller's stack arguments, untouched; cached in registers they are pushed
  and called. A `void` function ending in a `void` call tail-jumps: where the
  image calls and returns, the callee returns `int`. A wrapper that
  tail-jumps shows no parameters; take them from the base slot's `ret N`.
- `[vc8]` **Virtual calls.** They are thiscall; reach slot N with dummy
  virtuals. `push obj; call [vtbl+N]`, callee pops, is `virtual R __stdcall`.
  One slot called with different argument counts: declare the shortest and
  call the others through a `__thiscall` function-pointer cast. `ecx` = object
  plus stack arguments through a variable is a pointer to member. A real
  dispatch on `this` inside a constructor is `T *q = this; q->slot();` (VC8
  devirtualises `this->slot()`).
- `[vc8]` **Argument evaluation.** `M(F(), G())` runs the right-hand call
  first and pushes its result at once. Pushes hoisted above an earlier,
  unrelated call are the compiler's own. All temporaries built before any is
  used means they went by reference to a `__forceinline` helper.
- `[vc8]` **Inline helpers show as repeated code.** A bounds check repeated
  right after a guard is an inline checked accessor called twice. Range tests
  off by registers, a clamp that keeps both values on the x87 stack, a small
  accessor (`return f4 ? f8 - f4 : 0;`): write the helper, not its body. An
  inlined copy can need a different source shape than the standalone copy;
  give the callers their own helper.
- `[vc8]` **Call or inline is decided by where the callee is defined.** Mark a
  callee `__declspec(noinline)` or move it to another file when the image
  calls it; force it (`__forceinline`, class body visible) when the image
  expands it; a template member defined in a header needs `noinline` too.
- `[vc8]` **Temporaries.** `lea ecx,[esp+N]` before a member call on a returned
  object is a named local, one block per value; `mov ecx,eax` is the
  temporary itself, chained (`a.F(i).G()`) or passed on (`&(T &)b->F("K")`). A
  temporary in `a && f().g()` lives to the end of the expression and gets a
  destructor flag; if the image destroys it at once, the test was its own
  statement.
- `[vc8]` **Array new.** A literal count folds to plain `operator new(size)`.
  The unfolded `mul; seto; neg; or` guard comes from an `unsigned` count
  variable or an inline helper's parameter. `operator new[]`/`delete[]` are
  called only where declared (`<new>`); elsewhere the scalar ones.
  `delete[]` of a `char *` is the scalar delete; of a type with a destructor
  the vector-destructor call.
- `[vc8]` **Constructors.** Stores before the vptr store are a base
  constructor's or member initialisers; a member array of a class with a
  constructor and destructor emits the vector-constructor call; `: Base(p)`
  stores the base's field before the vptr; `p->T::T();` constructs in place
  with no null test; `new T(args)` with an inline constructor has its own
  shape that hand assignment does not give. An EH frame around `new T(a)`
  means the constructor is not `throw()`.
- `[vc8]` **Destructors.** Implicit: a bare `jmp` to the base's. User-declared
  empty: resets the vptr. Storing the base's vtable, not its own: the class's
  empty destructor with the base's inline one folded in. A middle class
  restored before an out-of-line root call has an inline empty virtual
  destructor. `??_G` (scalar) is emitted where `delete` is used, `??_E`
  (vector) where the vtable is. A "deleting destructor" with no vptr store is
  a hand-written member. A frame-less body that sets its vtable and calls the
  base destructor is an explicit `this->Base::~Base()`.
- `[vc8]` **Templates.** Factories are real templates (`return new T(a, b)`);
  explicit instantiation of the class emits every member, of one member that
  COMDAT alone. Two identical out-of-line copies of an inline destructor are
  two classes.
- `[vc8]` **Globals.** `mov ecx,[global]` before a call is a pointer global
  (`g->M()`), not an object. A function-local static object needs its guard
  word mapped as well as the object.
- `[vc8 rtm]` A frame-less destructor that frees a field and calls its base was
  reached only with raw field stores in a plain struct; a member with its own
  destructor adds a frame. Treat as a number win until rechecked.
- `[vc8]` **Read the unwind map to find the members.** A funclet
  `mov ecx,[ebp-0x10]; add ecx,N; jmp dtor` names a member at +N that has that
  destructor: declare it, and put its out-of-line constructor in the
  member-initialiser list (the call then sits between the vptr store and the
  body, 0x51c840; an initialiser with an argument, 0x520230). A `new T` with no
  EH state around it in the image means `T()` is `throw()`, and the image's
  `push 0xNN` before `operator new` is `T`'s real size, so give `T` that size
  (0x60cfd0). **Held**; extends the entry on reading destructor targets in
  `vc8-regalloc.md`.
- `[vc8]` **A later argument pushed before a call nested in another
  argument.** The image pushes `[x+4]` first and then calls: read that argument
  into a local before the call expression (0x5a89f0). **Held.**
- `[vc8]` **A list walk written with the project's iterator class gives the
  image's walk.** `RecorridoLista` (lista.h) reproduces the iterator shape and
  its inlined insertion where a hand-written pointer loop does not (0x619630,
  0x6199f0, 0x59b110); other list walkers matched this way are likely
  candidates when a loop differs only in register or block placement. **Held.**
- `[vc8]` **Inline budget.** VC8 inlined four uses of a small helper but not a
  fifth: `__forceinline` on the helper when the image has it inline in every
  caller (0x6412f0). A function-local static's guard stays where the source
  declares the static, so move the declaration to the image's position. **Held.**

## 7. Register allocation: the measured model

`[vc6]` Full detail, with the experiment behind each rule and the traces of the
code generator's passes, is in [`vc6-regalloc.md`](vc6-regalloc.md), beside this
file. `[vc8]` The
Commandos observations are in [`vc8-regalloc.md`](vc8-regalloc.md), beside this
file; no VC8 model exists yet. The VC6 parts that decide most stuck functions:

- **Two layers.** `[vc6]` **held.** Global allocation colours *webs* (live
  ranges; `z = g(z)` is two webs, a plain copy `T c = x;` usually none). Local
  assignment gives short-lived temporaries a scratch register from a rotation.
- **Priority is a weighted use count: each read or write counts `2^depth`** of
  loop nesting. **Uses are counted before common subexpressions are merged**, so
  writing `a + b` twice gives its operands two uses though the sum is computed
  once (DiedEffectB 204 -> 38). `[vc6]` **held.**
- **Ties go to the web that appears first in the instruction stream.** Arguments
  are pushed right to left, so the last parameter of a call wins a tie. `[vc6]`
  **held.**
- **Callee-saved registers go `esi`, `edi`, `ebx`, `ebp` in priority order**; a
  fifth web live across a call spills, a parameter by being read from its slot
  at each use. Webs not live across a call prefer `eax`, `ecx`, `edx`, with `eax`
  held back for the return value. `[vc6]` **held.**
- **Temporaries rotate `eax -> ecx -> edx`, skipping busy ones, straight through
  calls and branches.** One extra or missing temporary early shifts every later
  choice. A diff whose scratch registers are consistently one out from some
  point has a temporary more or fewer before it. Two locals copied out for an
  `if` cost a temporary. `[vc6]` **held.**
- **The rotation follows source order where layout and source disagree.**
  `[vc6]` **held.** DepthResort matched once its arms were in the image's order
  in the text, with identical layout before and after.
- **Byte operations need `al`/`bl`/`cl`/`dl`.** `setne` prefers the register of
  a dying operand; if that sits in `esi`/`edi`/`ebp` the result goes elsewhere
  and later choices shift. `[vc6]` **held.**
- **`or byte ptr [mem], K` means register pressure**, not a spelling: VC6 emits
  load/or/store whenever a register is free. The original kept one more value
  alive than ours. Look for a variable we folded away. `[vc6]` **held.**
- **A lazy `push ebp` mid-function** means the image never used `ebp` before
  that point. `[vc6]` **held.**
- **Classify before editing.** `[vc6]` **held.** Callee-saved registers swapped
  between two values is a priority difference (count uses). Scratch registers
  rotated from some point is a temporary count difference. Fix the
  register-masked shape first; bytes often follow.
- **Find invisible temporaries by ablation.** `[vc6]` **held.** A local that
  never reaches the output still shifts the rotation. Delete statements one at a
  time and watch the one differing instruction (PointerPickRepair matched by
  dropping a private copy that held a uid).

- `[vc8]` **No model measured.** None of the rules above was tested on VC8.
  The Commandos observations, all single cases without a status, are in
  `vc8-regalloc.md` beside this file.

## 8. Reading the image

Tells that say what the source was, before any compile:

- A plain `ret` with stack arguments is `__cdecl`; `ret N` is `__stdcall` or
  thiscall. `[vc6, vc8]` **held.**
- The frame size and each slot's offset name the locals and their sizes
  (section 4). `[vc6]` **held.**
- A buffer's size follows from the gap to the next slot. `[vc6]` **held.**
- The order of loads relative to stores is the statement order. `[vc6]` **held.**
- Decode which stack slot each call argument comes from before theorising about
  a call; misread slots and misread branch targets each caused behaviour bugs.
  Read the target address in the original before "fixing" behaviour from a
  branch. `[vc6]` **held.**
- The front end's intermediate files show which temporaries exist before the
  code generator merges them (`VC6_KEEPTMP`, `VC6_IL_OVERRIDE` in ArmyMen2's
  host). Editing the IL until the code matches gives the target shape. `[vc6]`
  **held.**

- `[vc8]` `push ebp; mov ebp,esp; and esp,-8` in an ordinary function is a
  local aligned to 8, not a flag: `struct __declspec(align(8)) { T *p; }` filled
  at the top (0x5690b0, and its `__fastcall` twin 0x5aa0f0). **held.**
- `[vc8]` The Rich header gives the major build only; it cannot name the
  service pack. **held.**
- `[vc8]` Hand assembly shows as a C frame around a body no compiler schedules
  (section 1). Check before transcribing. **held.**

### Folded `[vc8]`

Folded from the project record, duplicates merged: **held** *(record)* unless
an item says otherwise.

- `[vc8]` **The return type is in the epilogue.** A null path ending `xor
  eax,eax; ret`, a register preset to 0 that survives to `ret`, a `mov eax,1`
  after the side effects, or `add esp,4` (not `pop ecx`) after a call whose
  result looked unused: the function returns `int`. An argument left in `eax`
  on every path is returned. `eax` holding `this` from the first instruction
  to the end is a constructor or `return this`. A merged epilogue with no
  zero on the null path is `void`.
- `[vc8]` **A hidden first argument that ends up in `eax`** is a struct
  returned by value; with `ret 4` and no `ecx` use the function is
  `__stdcall`. `ret N` with `ecx` never read is `__stdcall`, or a member whose
  `this` is dead (they compile alike).
- `[vc8]` **Not arguments.** A `push ebp`/`push ebx` mid-function is a
  shrink-wrapped save. `push ecx; fstp [esp]` is a float argument's slot. A
  callback pushed as a plain address, no thunk, is a function with internal
  linkage.
- `[vc8]` **Read exact targets and offsets before theorising.** A `je` landing
  on `mov eax,1` is a return-1 branch. Count which `[esp+N]` each compare
  reads before naming parameters; swapped parameters show as one byte in a
  prologue load. A guard bit and EH state that belong to another function
  mean that function was inlined here.
- `[vc8]` **Check what an address holds before modelling it.** A "vtable"
  stored before a `sprintf` may be a string; a "global float" may be a literal;
  the empty string and shared CRT entry points sit at fixed addresses.
- `[vc8]` **Inline `__asm` for routines that were assembly.** Write only the
  body and let the compiler emit the frame; `test r1,r2` operands come out
  swapped; absolute data must be named globals; use `__declspec(naked)` only
  when the original pushes a register the frame would also save; reproduce
  assembler padding with `_emit`.

## 9. Number wins: levers that work and are not source

Each is behaviour-neutral, marked "a number win" in the source, and listed in
ArmyMen2's `docs/numberwins.md` (40 markers in 12 files at `20c901fe`). They
stand in for uses the original source had and we have not found (debug code
compiled out, a stripped macro, an inlined helper). Trade each for a real shape
once the binary matches.

- **Impossible pairs**, `|| (v == 1000 && v == 1001)`. `[vc6]` **number win.**
  They fold away and add pre-CSE uses of `v`, which reorders callee-saved
  registers, or supply one rotation step. Rules measured: use distinct constants
  per pair (identical pairs merge); keep the pair in a condition that is really
  evaluated; a single `&&` chain is inert; they fold to nothing on a frame local
  or a global and emit real code on a byte field behind a register-held pointer
  unless nested as `(A && !P) || P2`; the constants must stay out of the type's
  range (in-range ones change the code). Colouring is not monotone: extra uses of
  one variable pushed a different one down.
- **Dead volatile reads**, `(void)*(volatile T *)&x;`, and a volatile read into
  a temporary followed by a plain use. `[vc6]` **number win.** A dead
  non-volatile read or `(void)(a - b)` is deleted before it can matter.
- **Self-referential ternaries**, `p = c ? &o->pos : &o->pos;`, to give a pointer
  two reaching definitions. `[vc6]` **number win.**
- **A pointer rebuilt from an integer expression with a runtime read** loses its
  provenance, which keeps a store ordered before a reload. `[vc6]` **number win.**
- **A local that picks a symbol-table tie** (section 11). `[vc6]` **number win,
  fragile.**
- **Difference and negation tests** (`a - b` for `a != b`, `-x` for `x != 0`).
  `[vc6]` **reverted** wherever the image has `cmp`/`test` (section 1).

`[vc8]` Commandos has no marker convention for these; they were found by grep.

- **`volatile` locals.** `[vc8]` **number win.** `volatile int t = 0;` for a
  dead zero stored to a slot (`ClassEngineJuego::ActivarObjetoC4` 0x594500,
  0x556670), `volatile float` for a spill (section 4), a store through
  `*(volatile int *)&campo` to force a reload. 24 files at `83ef99f8`. The dead
  zero may be a real exception-state marker; nobody has shown which.
- **Naked `jmp eax` slot forwarders.** `[vc8]` **number win.** `mov eax,[ecx];
  mov eax,[eax+K]; jmp eax` (VC8 picks `edx`) is naked asm at 0x45a220,
  0x60f600, 0x639700 and identical; the same shape written as C++ is parked at
  0x607ab0 and 0x621bc0. One family, two treatments; the parked one is honest.
- **Naked adjustor thunks.** `[vc8]` **number win.** 247 `add/sub ecx,N; jmp`
  thunks are generated asm until the classes get their real multiple-
  inheritance bases, which make the compiler emit them.
- **A dead `__declspec(align(64))` local** for an `and esp,-64` prologue.
  `[vc8]` **number win, and not a match:** `OrganoMotor::InfoEstado` (0x45a320)
  is still parked 5 bytes off. The real aligned object is unknown.
- **Harness scaffolding.** `[vc8]` **number win.** An exported caller that
  keeps a `static` function alive, a throwaway helper under `#pragma
  inline_depth(0)` that emits an inline function's out-of-line copy, a stand-in
  second caller that stops `/GL` folding a global into a helper. None is source;
  each stands for a caller the original had elsewhere.
- **Patched bytes.** `[vc8]` **number win.** `EsIdiomaAleman` (0x594840) is
  naked asm because the retail image has its jumps overwritten with `nop`s.

## 10. Searches, and what never paid

- **Adjacent statement swaps**, keeping the best that worsens nothing, repeated.
  `[vc6]` **held.** Store order is the lever it finds: eleven functions closer in
  two passes, three of 49 on a later run. Filter for independence and read every
  winner.
- **Copy substitution** (use a copy's source instead of the copy, or the
  reverse) was the only mechanical family that produced matches late
  (1,552 -> 1,555). `[vc6, vc8]` **held.**
- **Aliases into re-reads and back**; a walked alias into a retyped parameter;
  `for (;;)` against `while (1)`; label moves between identical `return E;`
  statements. Each found a few. `[vc6]` **held.**
- **Annealing over the number-win set** found seven wins when seeded with a
  hand-built source closer in register-blind shape but worse in bytes, and
  nothing in fifteen runs without such a seed. `[vc6]` **held.**
- **A second model driving the screen** on one named function, forbidden to touch
  the tree except through the screen, produced the largest single wins by trying
  many variants. Confirm with a full relink and read the diff for behaviour.
  `[vc6]` **held.**
- **Declaration-order permutation**: changed nothing in any of 85 functions.
  `[vc6]` **no effect.** (Declaration order does decide the order of constant
  store groups for local arrays, section 4.)
  `[vc8]` opposite in places: `unsigned i = 0;` then `ColumnaLista *p = tabla;`
  as separate statements gave `PrepararColumnas` (0x4c6790) its register
  setup order, and local order steered `esi`/`edi`/`ebx` in 0x593d50 *(match)*.
  The three-counter loops (`Hoja5c0e50`) moved with no order.
- **Compiler-flag sweeps**: no per-file flag beat `/O2 /GX`; `/Oa` and `/Ow`
  explode the diffs. `[vc6]` **no effect.**
  `[vc8]` same for flags (below); the lever was the service pack.
- **Retyping each integer local across the fixed widths**: 17 closer, none
  matched. **Introducing a parameter copy everywhere**: 3 closer, none matched.
  `[vc6]` **no effect.**
- **decomp-permuter**: found one match in two minutes and then cost more memory
  than it returned; dropped for single-process sweeps. `[vc6]` **superseded.**

- **Mechanical variant search** (`variants.py`: adjacent statement swaps,
  mirrored and negated comparisons, `volatile` on a local, `int`/`unsigned`,
  `x == 0`/`!x`; beam of 3 rounds, one function scored). `[vc8]` **held, with
  number wins among its results.** 6 of 390 parks in five minutes, at least 4
  more in later runs *(record)*. Read what it applied (section 1).
- **Statement-order brute force.** `[vc8]` **held** *(record)*. Cheap for up to
  four stores; a negative result is information (the park is not an order
  problem).
- **Flag sweeps.** `[vc8]` **no effect.** 22 combinations on 0x512640; `/Ox /O1
  /Os /Ot /Ob2 /G6 /G7 /arch:SSE` on 0x4721d0; `/Os` and `/O1` break what `/O2`
  matches. `#pragma optimize("s", on)` does produce a kept zero register but
  changes the calls and the frame around it (0x4261e0).
- **`/GL` as an explanation for scratch-register families.** `[vc8]` **no
  effect.** It explains register arguments and nothing else (section 13).

## 11. Declarations and headers move code

`[vc6]` **held, and a standing hazard.**

- A type declared earlier in the unit can change a function that never names
  it. One struct added to a widely included header swapped two registers in an
  unrelated function; a typedef placed before a function cost it 2 bytes and
  nothing placed after it.
- Where two globals are added, which is loaded first can follow the compiler's
  symbol table. Removing 51 `extern` declarations from a shared header flipped
  one function from 0 to 22 with its source untouched; restoring 51, 25 or 10
  dummy declarations restored it, 50, 2 or 1 did not. An anonymous `enum` added
  to a shared header flipped a second function on the same tie. No spelling held
  in every header state; a local for one operand inverts the dependency (0 with
  the current header, 22 with the old one).
- So: a change that only adds or removes declarations in a shared header is not
  free. Relink and compare every function. Put per-function views and typedefs
  beside the function that needs them.

`[vc8]` **held** *(record)*, in a different form: units here are small and
mostly header-less, so the lever is what else is defined in the unit.

- A callee defined in the same file changes its callers (inlined, or `ecx`
  known to survive). Adding a definition beside its callers broke a matched
  caller.
- Placeholder class names reused across small files collide; the compile is
  clean and one COMDAT is silently dropped.
- Merging header-less files needs a shared header first; each merge is
  followed by a full `make match`.

- `[vc8]` Renaming a placeholder class to its RTTI name turns on the vtable
  compare for it: placeholder virtual bodies must go, and each slot's name
  must map to the image's slot target.
- `[vc8]` An RTTI name always wins over a guessed one; rename the guess, do not
  split files around the clash.

## 12. Known not to exist, and still open

- No zero-code rotation consumer; no register hint without code; a dead pair on
  a local or register operand folds in the front end. `[vc6]`
- **Identical return blocks where the image keeps the earlier copy** while arms
  stay in source order: solved only by the single-`goto` and label-after-return
  forms (section 3); ShotHitsObj-style cases where a label changes the first
  test stayed open until other changes closed them. `[vc6]`
- **Residues traced into the code generator with no source lever found:** a
  block-layout decision made by jump threading (SightScan, 26), a reload forced
  by a consumed-store rule (RowUpdate, 154), one missing rotation step
  (LoadMap, 356). Tracing the passes in the compiler DLL under a debugger
  explained each and found no spelling. `[vc6]`
- **Register-only residues accepted as such:** FindPath (32), ScriptPad (16).
  `[vc6]`
- `[vc8]` **Open on SP1**, no source lever found: `add eax,-1` for a read-only
  `x - 1` (0x552d80, 0x552ee0; `switch (x - 1)` and a stored-back `n--` do give
  it); the `eax` temporary of `vc8-regalloc.md`; `jmp eax` slot forwarders (0x607ab0,
  0x621bc0); a zero or one kept in a register that SP1 did not close (0x45b910,
  0x5de170, 0x4261e0, 0x5d6d80 at 66 bytes); frame slot placement (0x4f7240, 2
  bytes; 0x63d460); a tail `jmp` lost to cached arguments (0x62fcf0); a static
  function whose object stays in `ecx` (0x5d48e0, 38 bytes). Cause unknown for
  each.
- `[vc8]` **Open:** the image's `__ehfuncinfo` has one more unwind entry than
  the source declares (a 6th entry with no action, `maxState` 6 for 5 function
  statics) while the code is identical to the byte, 1 byte differing at +4
  (0x6095b0, 0x607410, 0x607840, 0x606e10, 0x6060e0). No source shape found.
- `[vc8]` **Open:** a zero `V3` temporary whose copies the image sinks past
  later stores, or two zero temporaries sharing one stack slot where ours uses
  two (0x5102a0, 0x6214c0); no source shape reproduced either.
- `[vc8]` **Solved after being listed unsolved in the early record:** the `lea
  eax,[reg+1]` result, `jb` against `jl`, the switch's zero pushed as a
  register (all SP1); the materialised `== 0` (section 3); pointer-difference
  string loops (intrinsics, section 6); the float argument-slot reuse of
  0x58acf0 (plain source once the owner class had its layout: `T *e =
  sonidos[canal]; if (e) return e->VolumenSonido(); return 0.0f;`, identical);
  `eax` and other register arguments (static functions and `/GL`, section 13).

- `[vc8 rtm]` **Parked on RTM and not retried on SP1:** a flag store scheduled
  after a call's argument pushes; `xor reg,reg` for a stored zero where ours
  uses an immediate; a loop's list and count registers swapped; a
  full-buffer branch laid out after the epilogue; three induction variables
  of one stride that the image keeps apart and VC8 merges; a list walk with
  three stack slots where ours has four; a thiscall with a caller-cleaned
  stack (later shown to be `/GL`, section 13).

## 13. VC8 only

- **The service pack is part of the compiler identity.** `[vc8]` **held**
  *(record)*. RTM and SP1 both stamp 50727. SP1's patch is an EXE around an MSP
  around a cab of whole PE files; swap `c1.dll c1xx.dll c2.dll cl.exe link.exe`
  into a copy of the RTM tree. One SP1-only behaviour to dodge: after `int a =
  0;` a later `int b = 5;` becomes `lea b,[a+5]`; declare the non-zero constant
  first (`PonerBando`, `Texto`).
- **`/GS` cookies follow the local's type.** `[vc8]` **held** *(match)*. A
  `char` array, or a struct of them, gets a cookie; a local array of shorts
  adds a pad. Where the image has neither, the buffer was not declared as
  chars: an `int` array seen through a `char *` (`Marco61c8b0::Pintar`
  0x61c8b0). Taking the address of a pointer parameter makes `/GS` copy it; a
  variadic declaration removes the copy *(record)*.
- **Static functions take arguments in registers.** `[vc8]` **held.** With
  internal linkage and no inlining VC8 passes the first argument in `eax`, then
  `ecx`, and still emits a standalone copy: `static __declspec(noinline)`
  (`Insertar48a6f0` 0x48a6f0, 0x48a710, 0x526d60, 0x5cf640, the four copies of
  0x441b80). Declared parameter order picks the registers. Whether the original
  functions were `static` or `/GL` output of the same shape is not settled.
- **Other register arguments are `/GL` + `link /ltcg`.** `[vc8]` **held**
  *(record)*. Pointer arguments go in callee-saved registers, scalars stay on
  the stack; a global every caller passes is folded into the callee. Checked
  after linking, per operand, with every address named (`glcheck.py`): 6
  identical, 8 parked. `/GL` objects cannot be compared as COFF.
- **Register-argument entries the source cannot express** (other registers
  with a caller-cleaned stack) get a small asm adapter over a C++ body.
  `[vc8]` **number win** *(record)*; not counted as matched.
- **EH states are members.** `[vc8]` **held** *(record)*. Under `/EHsc` the
  frame and its state numbers come from members with destructors, by-value
  temporaries and function-local statics; write those and the frame follows.
  A frame-less constructor over a base with a destructor is still open.
- **Incremental-link thunks.** `[vc8]` **held** *(record)*. Callers reach most
  functions through a `jmp` table; map a name to the thunk where callers use
  the thunk, and give a body reached both ways a second name.
- **Checked iterators and the rest of the C++ library:** not measured; library
  code is out of scope by owner decision (section 1).
- **`__fastcall`.** `[vc8]` **held.** A function that reads `edx` (or `ecx`
  and `edx`) before writing it and ends with a plain `ret` takes its first two
  integer arguments there; declare the unused `ecx` parameter.
- **`/GL` source shapes.** `[vc8]` **held** *(record)*. Index the table as
  `g[i].field` at each use (a local `&g[i]` changes register roles); a test
  compiled as `mov eax,[arg]; sub eax,1; jnz` is a one-case `switch`; parameter
  order of a static decides which of `ecx`/`edx` holds which pointer.
- **Unused-but-emitted copies.** `[vc8]` **held** *(record)*. With no
  `/OPT:REF` and no COMDAT folding the image keeps every out-of-line inline
  member and every identical copy. Emit a wanted copy from another object
  file; once emitted in a file, VC8 stops inlining it there.
