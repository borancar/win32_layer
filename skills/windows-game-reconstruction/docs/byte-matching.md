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
  both compares, and so does a byte read again through the pointer in each
  test (`*p >= '0' && *p <= '9'`; a `char` local folds to `sub; cmp; ja`). Bytes equal but `$LN` labels differing means the
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
- `[vc8]` **A result assigned over another keeps its branch.** `test eax,eax; jne L; pop; ret; L: mov eax,edi;
  pop; ret` after two calls is `int q = b.F(); if (!q) r = q; return r;`. `if (!q) return 0;`, `return q ? r : q`
  and `if (q) q = r` all fold to `neg eax; sbb eax,eax; and eax,edi`. An entry test `cmp [m],0; jbe` in front of a
  loop whose body starts with the exit test is a `for` over an unsigned bound, not `if (n) do ... while`.
  **held** *(match)*.
- `[vc8]` **A float compared with 0 or 1 has the constant loaded first.** VC8 turns `v < 1.0f` into `fld1; fcom
  [v]; test ah,0x41; jne` (1 > v) and `v > 1.0f` into `fld1; fcom [v]; test ah,5; jp` (1 < v), and likewise with
  `fldz`: write the variable first and read the image's test as the mirrored one. Writing the constant first
  (`1.0f > v`) gives `fld [v]; fcomp QWORD PTR [1.0]`. A clamp whose arms end in `fstp [r]; fld [r]` assigns the
  result variable (`if (r < 0.0f) r = 0.0f; else if (r > 1.0f) r = 1.0f; return r;`), early returns of the
  constants do not; a port's `!(a <= b)` for `a > b` shows as `jnp`/`jp` against `jne` and differs for a NaN.
  A flag set by `cmp [m],0; jne set` whose set path stores 1 is one expression (`m = m || a > x || b > y;`), not
  `if (!m) m = ...`. **held** *(match)*.
- `[vc8]` **One copy of a store that two paths need, out of line behind a return and ending in a backwards
  `jmp` into another arm's tail, with the success block last**: the outcomes are an `else if` chain and the
  common tail (an error code) is *written in two arms*: `if (own != id && id) { err = E; } else if (!id)
  { c->n = b; err = E; } else { c->ok = 1; return 1; }` and the function's `return 0`. VC8 merges the two
  error tails and threads the second test; every form with the error written once (success tested first, a
  flag, a result variable, `goto`) puts the success first or duplicates the store (0x49cc40, 137 -> 42 -> 0
  bytes over about forty-five arrangements). When a merged tail appears in the image, try duplicating it in
  the source rather than factoring it. **held** *(match)*.
- `[vc8]` **A null exit with its own `xor eax,eax` while the other arm still ends in `test eax,eax; setne`**:
  the pointer comes from `new T(args)` with an inline constructor. The new-expression's null test plus the
  source's `if (p) {...}` and one `return p != 0;`: VC8 threads the null arm to the return (folded to 0) and
  leaves the test where the pointer is a join. Constructor stores written inside the port's own `if (p)` give
  one test and one return path (0x4a8ad0). **held** *(match)*.



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

- `[vc8]` **An answer slot handed to a callee is a local of its own, not the parameter assigned again.** A port
  that zeroes an incoming parameter and passes its address (`s = 0; f(&pt, &s);`) has a frame one word smaller
  than the image; a local (`T *answer = 0; f(&pt, &answer);`) matched at once (Commandos 0x4e7170, 372 bytes;
  0x55d920). Both directions are a regex (`paramlocal` below). **held** *(2 matches)*.
- `[vc8]` **Where a zero register first appears is where its variable is declared.** `xor esi,esi` behind the
  second of three `operator new` calls was a result variable declared between the second and third `new`
  (`int ok = 0;`, set to 1 in the success block, one `return ok;`). Declared at the top the zero loads in front of
  the first call; with two `return`s two unrelated pointers exchange callee-saved registers, which read as
  "register allocation" for eight rounds (0x49ebe0). **held** *(match)*.
- `[vc8]` **A result variable declared first is `mov eax,1`; declared last it is `lea eax,[zero+1]`.** With the
  refusals as `r = 0; goto fin;` and an element copied as a struct (`v[i] = *x;`: the first word through the indexed
  address, then one `lea` for the rest; member assignments take the `lea` first) (0x50e230). **held** *(match)*.

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
  that is a widened float is `(double)0.0015f`. The destination's type picks
  the form: `unsigned short`/`unsigned char` is the inline sequence with
  `fistp DWORD` (word or byte read from the slot), `unsigned` the inline
  `fistp QWORD`, `short`/`signed char`/`int` call `__ftol2_sse`. The effective
  destination counts (an `unsigned char` cast stored into a `signed char`
  array calls), and `(unsigned)(int)f` or `(unsigned)((int)f - 1) > K` folds to
  the unsigned form: keep a signed local and write the range out. For a
  second compiler guard the unsigned forms (`(unsigned)(long long)f`): a
  negative or huge value is undefined there and the low word here.
- `[vc8]` **Sweep by library callee.** Per unmatched function, count the
  image's calls to each CRT/library address and the inline idioms (`rep movs`,
  `rep stos`, `repne scas`, `fistp`, `fldcw`) against the candidate's. A
  differing count names a class of fixes at once (conversion signedness, an
  intrinsic the image calls, a helper the image has in place) and finds
  off-by-one table bases when the neighbouring `lea` is read.
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
- `[vc8]` **The `lea eax,[reg+1]` result: a counter, not a flag.** Declare
  `int r = 0;` before the work (at the top when the image's `xor` is early),
  then `if (cond) r++; return r;`. Neither `r = 1` nor `r += (x != 0)` gives
  it. This closed 0x45b910, 0x4a3e10, 0x4305b0 and 0x4305e0. **Held.**
- `[vc8]` **Pair temporaries with an inline constructor.** `p = ParCero();` or
  `de = ParEnteros(o->b, o->a);` gives two `xor` zero registers, both loads
  before both stores, or a constant copied with `mov edx,eax`; the
  constructor's store order picks which store comes first. **Held.**
- `[vc8]` **Member array with an inline element constructor.** `lea
  esi,[zero+N-1] ... sub esi,1; jns` is the construction of a member array whose
  element has an inline constructor. A by-value argument taken from a local
  copy (`T t = f->tam; F(t);`) loads both words before `sub esp` when `T` has a
  user copy constructor. **Held.**
- `[vc8]` **Result variables on shared exits.** A zero kept in a callee-saved
  register as the return: `int r = 0; if (o) r = o->F(); return r;`. A global
  set on two tail-duplicated exits through `eax`: a result variable, then
  `g = r; return r;`. `-1` returned through the shared `ret`: `if (i != -1)
  i = F(); return i;`. A call result stored and reloaded before use is a named
  float local. **Held.**
- `[vc8]` **A zero register that is not a result variable.** Three sources,
  told apart by where the `xor` sits. *Behind a `call operator new`* (`xor
  ecx,ecx; cmp eax,ecx; je`, later null tests as `cmp r,ecx`, even in a `void`
  function): the class has a constructor that zeroes its members and the
  function assigns them all right after; the dead stores go, their register
  stays. *In front of four plain stores that the next statements overwrite*
  (`xor eax,eax; mov [o],eax; ...` kept, ours has dropped them): the
  constructor is `memset(this, 0, sizeof(*this))`, expanded after the
  dead-store pass. *At entry, the register then reused for a flag*: the flag
  is the result variable (`r = !(f & 0xc); if (m) r = r && ...; else r = r &&
  ...; return r;`), not a second local. A result that ends a loop sits in the
  loop condition (`i < n && !r`), not in an `if (r) return r;` at the body's
  top. **Held.**
- `[vc8]` **An out word read back from the argument's own home slot** with
  the argument kept in a callee-saved register: the call is inside an inline
  helper that takes the argument by value and returns the out word; the
  inlinee's local lands in the dead parameter slot. A copy of the argument
  (`int t = a; F(t, &a)`) goes through `eax`. **Held.**
- `[vc8]` **A call in front of the pushes of the call it feeds** (`call
  GetDlgItem; push 0; push 0; push msg; push eax`): its result is a local;
  nested in the argument list the earlier pushes come first. A pointer stored
  as `xor eax,eax; mov [m],eax` in the empty arm: one conditional expression
  (`Set(n ? new T[n] : 0)`); an `if`/`else` of two calls stores an immediate.
  **Held.**
- `[vc8]` **`delete[] p` and `new T[n]` call the scalar operators in a file
  that does not declare `operator delete[]` and `operator new[]`.** Declare
  both wherever the image calls the array thunks; where a matched function's
  image calls the scalar one, the source says `delete p`. The per-function
  byte count does not see a call to the wrong operator when relocations are
  masked: audit by call target. With the array operator declared (it may
  throw) an EH state store appears in front of the call. **Held.**
- `[vc8]` **Local pointer copy before a member call.** `mov eax,[p+N]; lea
  ecx,[eax+0x44]` comes from `q = p->x; q->m()`; the direct `p->x->m()` gives
  `mov ecx; add ecx`. Testing a member just stored: `quien = p->quien; if
  (quien)`. A fallback that reuses a loaded register: re-read the member chain
  at each use instead of a local copy. **Held.**
- `[vc8]` **Float copies and conversions.** A float vector copied with
  `fld`/`fstp` is a user `operator=` plus `d = V3(...)`. A conditional float
  adjustment with no rounded intermediate: a ternary straight into the
  destination. Float to unsigned (`fnstcw ... fistp`) also forces the aligned
  frame (`push ebp; and esp,-8`). `fabs` on a float uses the `<math.h>` float
  overload. **Held.**
- `[vc8]` **Where stores after an EH-state store belong.** Stores that follow a
  member's EH-state store belong to the containing constructor's body; a
  count-down `jns` loop after them is a body loop, not member-array
  construction (0x54b210). A three-float local with only two fields set gives
  12-byte slots, and `p->z += 0.0` reproduces a constant z (0x550140). A float
  zero local keeps `0*0` from being folded (0x591940). `fadd dword 2^32` means
  an unsigned-to-float conversion. **Held.**
- `[vc8]` **Aligned frame from a union.** A `LARGE_INTEGER`-like union local
  gives the `and esp,-8` frame; a plain `__int64` does not (0x647540). A dead
  `strcpy` into an unused local buffer keeps a read loop and the /GS frame
  (0x577cd0). **Held.**
- `[vc8]` **Identify the class from the vtable.** An image vtable's RTTI name
  and its non-pure slots name the class a constructor really belongs to
  (0x58ade0 is `ClassElemIntroScript`'s constructor, not a base's). A typed
  global array explains `[esi+GLOBAL]` byte-offset indexing next to a separate
  index register (0x5c0b50). A zero struct temporary assigned to a global
  struct gives the `fldz`/stack temporary/dword-copy shape (0x5d87b0). A field
  read inside the first test of an `||` chain is read there, not hoisted into a
  local. **Held.**
- `[vc8]` **Compare and string forms.** `strcmp` gives `repe cmpsb` with length
  + 1, `memcmp` does not; `int c = 0; c = F(v, c);` moves the `xor` before the
  call; `x < lim` selects `test 0x41`; `d < 0 ? -a : a` is the negate-select.
  **Held.**
- `[vc8]` **Source order is recoverable from statics and EH states.** Function
  statics' guard bits and EH state numbers are assigned in source order, so they
  give the order of switch cases and if/else branches. A member read into a
  named local before the test gives `mov eax,[m]; test eax,eax` instead of
  `cmp [m],0`; a pointer member re-tested after a ternary or `&&` chain means
  the member is read in each expression (a local copy lets VC8 thread the
  second test away). A signed bound test survives merging when written negated
  on a signed local. An `int` flag, not `bool`. **Held.**
- `[vc8]` **A zero vector temporary assigned last.** A zero `V3` temporary
  whose z store is sunk past later stores: write the assignment after those
  stores (may unlock the "zero V3 temporaries" open family in section 12:
  0x5102a0, 0x433d80, 0x4efb60). A user `operator=` that copies floats gives an
  `fld`/`fstp` struct copy instead of dword moves; an inline function returning
  the vector difference by value plus that `operator=` gives the vector-difference
  `fld`/`fstp` copy. A three-float array gives the 12-byte slot and the x,y,z
  frame order. An argument slot reused for a result: reassign the argument
  (`n /= cnt`). **Held.**
- `[vc8]` **RTTI differences and real sizes.** When the image's `??_R3` differs
  at +8 it is the base-class count: declare the bases under their RTTI names;
  a vtable relocation that resolves to the base's slot means the class
  overrides it. A large local's real size and its constructor/destructor come
  from the frame gaps and the EH states (a reader that looked like 0x40 bytes
  was a 0xf44-byte class). A box whose stores come in two separated pairs is
  two pair types with inline constructors. **Held.**
- `[vc8]` **Evaluation order and result locals.** An argument assigned inside a
  later argument (`new T(j, p, (p = g->c)->Rect(), "X")`; VC8 evaluates right to
  left) gives "read once, used as `this`, pushed after". A pointer-typed result
  local gives `cmp reg, zero-reg` instead of `test` after a call. An int
  constant assigned before a zero V3 temporary gets its register load hoisted
  above the temporary. A flag computed inside a constructor argument, not in a
  named local. A function returning through `else r = 1;` into the shared tail.
  **Held.**
- `[vc8]` **Constructors that store several vtables by hand.** Write the real
  class with its bases and member initialisers, and declare every override found
  by comparing the class's vtable slots with the base's (following thunks); any
  undeclared override shows up as a vtable relocation mismatch, and the
  `??_E...@W7AE` adjustor then matches at its own address. This matched the
  `Sim*` constructors (0x45e2a0, 0x45e090, 0x461070, 0x45fc60, 0x45c870) and
  should work for other hand-written-vtable constructors. Within a constructor,
  its own virtuals are called through a copy of `this` to stay virtual.
  **Held.**
- `[vc8]` **Nested-block locals and typed parameters.** A local initialised from
  a parameter that the image keeps in the parameter's own slot (`mov eax,[arg];
  test; mov [arg],eax`, frame 4 bytes smaller) is declared in a nested block
  starting after the parameter's last use. An `int`-typed pointer parameter
  costs an extra frame slot: give it its real pointer type. An array-of-members
  loop stepping by 1 byte means the element type is empty: give it its real
  size. A member address computed before a `new` that takes it: read the value
  into a local before the new-expression. **Held.**
- `[vc8]` **x87 operand order through typed inline helpers.** Squared length
  with x loaded first (a 12-byte difference slot): pass a three-float
  difference struct by value to an inline helper written `d.y*d.y + d.x*d.x`;
  a float array, named floats, or a reference or pointer parameter keep y
  first (five functions). Add into a field in the image's operand order: an
  inline helper taking the destination struct by pointer and the addend by
  const reference (`p->x = p->x + v.x; p->y = v.y + p->y;`) with typed two- or
  three-float structs; VC8 follows the written order inside the helper, but a
  member function (`this` as the pointer) or a by-value addend does not work
  (five functions). A dot product against a typed normal through a
  by-reference helper puts the x term first. `fld z; <sum>; fcompp; test ah,1;
  je` is `!(k->z <= Helper(...))`. **Held.**
- `[vc8]` **Small forms.** Two truncations into an `int q[2]` local converted
  later in the other order (q[1] first). A by-value 32-byte name through the CRT
  `strncpy` (0x651a70): an int-word name type whose constructor only calls
  `strncpy`, with the `[0x1f] = 0` statement written after the next load. A
  local stored only after its zero test: reassign the same value inside the
  block. A symbols line at a `jmp` thunk compares the thunk: name the function
  body. **Held.**
- `[vc8]` **Constructor on a real base.** Write the real base class with an
  inline constructor and a virtual destructor, and the derived fields as
  members with inline constructors; a class defined locally in the
  constructor's own file is enough when the shared header's layout is wrong
  (0x47cb50, 0x481c30, 0x485c40, 0x5a00d0, 0x64bcb0). Declare every vtable
  slot so the hook DLL gets a correct vtable, and give unnamed slots `~` alias
  lines. A guarded block that constructs objects at fixed addresses with one
  guard bit is a function-static array built from temporaries; the zero vector
  temporary's store order selects which constructor to use. **Held.**
- `[vc8]` **Virtuals and scheduling barriers in constructors.** A virtual call
  the image keeps inside a constructor: VC8 devirtualises calls on `this`, so
  call through a pointer copy (`PonerBanderas(0)` in 0x64bcb0). A vptr store
  kept after a run of field stores: write the long zero run as a loop over the
  member array, which acts as a scheduling barrier. A port that calls a function
  conditionally where the image always calls it: check where the `je` lands
  (0x627560 was a real behaviour bug). **Held.**
- `[vc8]` **Checking a shared header change.** Rebuild every dependent file
  with the old and new header and diff the asm: 234 files in about 11 seconds
  for `lista.h`; only the two instantiations that should change did.
- `[vc8]` **x87 zero tests and comparisons by constant side.** For a zero test
  the constant on the left (`0.0f == x`) gives `fld x; fldz; fucom st(1)`;
  `x == 0.0f` gives `fldz; fcom [mem]`. `s > 0.0f` gives `fldz; fcomp [s]`
  while `0.0f < s` gives `fld s; fcomp [0.0 as a double]`; `xmax >= x` against
  `!(xmax < x)` and `x >= x0 - r` select different forms. Integer: `if/else`
  producing 1/0 instead of `setle`. Commutative operands written in the order
  opposite to the image's (`campo24 + o[e]`, `pos + ini`). **Held.**
- `[vc8]` **Aliasing, not statement order.** A block of int stores scheduled in
  another order, or a field rematerialised in place, follows from naming the
  same address through a different expression (`&c->origen` against a pointer
  `o`): it is the alias analysis. Naming the member instead of a pointer that
  already holds it changes scheduling. Declaring a local after a store fixes a
  read the compiler hoisted. A path with no `return`. **Held.**
- `[vc8]` **Constructor hints.** A thiscall whose first instruction moves
  `this` into eax and keeps it to the end is a constructor (it returns `this`).
  An argument object built straight in the outgoing argument area means its
  class has a non-trivial copy constructor: declaring one (not defining it)
  reproduces this, and a member-initialiser copy constructor gives
  float-by-float copies. Real constructors for `Construir()` bodies on a base
  with list members need the list constructor specialised and a complete
  element type (otherwise the list destructor loses its virtual delete).
  Switch cases are laid out in reverse source order. A callee that ignores ecx
  is still a member function. **Held.**
- `[vc8]` **Opaque field stores block scheduling.** A store through a `char[]`
  field or `*(int *)((char *)this + k)` cannot be scheduled past later loads; a
  typed field, or a typed local overlay struct, lets it sink (0x637230). A
  `.rdata` "global" loaded with `fld` and hoisted above integer stores is a
  float literal; a named float local is stored and reloaded instead. Reusing a
  dead parameter (`pos %= periodo`) or a parameter-derived local puts the value
  in the argument slot the image uses (0x62e570). **Held.**
- `[vc8]` **A defensive initialiser is one store too many.** A port's `int x = 0` in front of a call that takes
  `&x` shows as a single extra `mov [x],0`; when the callee writes every out parameter the image has none. Read the
  callee first: the zero rule for slots the image really reads uninitialised is a different case. The statement
  whose `fstp` comes after two integer stores to globals is written in front of them; an unsigned converted with
  `fadd DWORD PTR [2^32]` was cast to `float`, with the qword constant to `double`. **held** *(match)*.

- `[vc8]` **A frame 8 bytes larger than the image: locals that belong in block scopes.** Two small structs of
  disjoint blocks (a point for two calls, a colour for a later one) share a slot only when each is declared in
  its own block; at function scope each keeps its own (0x20 against 0x18). An integer store that the image has
  one statement later (`mov [m],1` behind the `fld` of the next statement) was written after that statement:
  move it down before trying anything else. A named float that the image keeps in a dead argument's slot is the
  compiler's packing (first fit over the dead homes); when it picks another slot than ours no spelling moved it.
  **held** *(match)*.
- `[vc8]` **A commutative x87 sum is not evaluated in source order; a `double` local fixes it.** `(px - cx) * dx +
  (py - cy) * dy` came out `y` product first in one function and `x` first in its twin, whichever way the sum, the
  products and the declarations were written. VC8 orders the two operands by where their member loads first
  appear in the function's layout: exchanging two statements in the *other* `switch` case (the one laid out first)
  flipped this one. The first product in a `double` local (`double a = (px - cx) * dx; float t = (float)(a + (py -
  cy) * dy);`) stays on the x87 stack, costs no store, and the sum takes the local first; two double locals are
  ordered like the plain sum again, and a split into two float statements adds a rounding store (Commandos
  0x44dbe0, 0x451de0). A single product of two memory operands (`s * y`) is ordered by the same key and no local
  moved it (0x5d4100, 0x59e740: 138 spellings). The aligned listing shows nothing for an operand exchange when
  it replaces numbers: rank such ports by differing bytes. **held** *(match)*.
- `[vc8]` **A rounding store in the "wrong" dead argument slot: assign the parameter itself.** Two block locals
  with disjoint lives share one dead home (first fit), so a second clamp's `fstp [slot]; fld [slot]` landed in the
  first argument's slot where the image uses the second's. The image's variable is the parameter, reassigned
  after its old value was copied to a `double`: `double v = vel; m[0] = (float)v; if (hi < v) m[0] = hi; else {
  vel = -lo; if (vel > v) m[0] = vel; }`. A `float` copy is stored and reloaded; function-scope locals, declaration
  orders and an inline clamp keep the shared slot (0x45bd70, 2 bytes with no instruction differing). This
  supersedes "no spelling moved it" above for the case where the wanted slot is a parameter's. **held** *(match)*.


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
- `[vc8]` **A frame larger than ours by less than a slot's rounding, every instruction equal, is an array
  size.** Subtract the cookie and the known locals from `sub esp`: 0x271c minus 4 and an 8-byte point is
  10000, two `char [5000]`; the distances between the `lea`s had given 5016 and 4980, which only bound the
  first (0x58a760). A frame smaller by whole slots with scalars stored and read back in a dead parameter's
  slot and in slots that are taken again is something else and stayed open (0x4582e0: twenty-five forms of
  three float differences copied into a vector). **held** *(match)* / **open**.
- `[vc8]` **`fcomp DWORD PTR [k]` against `fld g; fcompp` with the test's mask changed**: the constant is a
  literal in the source; a port that declared the `.rdata` float as an `extern float` gets it loaded and the
  compare turned round. `fld1; fcomp [x]; test ah,5; jp` is `x > 1.0f` (not `!(x <= 1.0f)`, not `1.0f < x`).
  **held.**

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

- `[vc8]` **An inline helper's arguments are loaded left to right.** `Volumen(script, channel)` loads the script
  first, `Volumen(channel, script)` the channel; the parameter order of an invented inline helper is read off the
  load order in the image (0x58e6e0, 3 bytes). **held** *(match)*.
- `[vc8]` **Equal code, still different: the RTTI of a constructed class.** A comparison that follows relocations
  into `??_R0`/`??_R3` fails on an invented class name once the code is equal. Read the type descriptor's name at
  the reported address and declare the real class with its real bases (0x49ebe0: an invented link class was
  `NodoEjecucion : NodoListaEnlazada<...>`). **held** *(match)*.
- `[vc8]` **Folding a duplicate.** A member that never reads `this` compiles to the bytes of the `__stdcall` free
  function with the same arguments, and a virtual called from its own class's constructor is a direct call: a
  matched twin's body can go under the port's member name and the twin's name be dropped (0x63e8f0, 0x4c6350).
  **held**.
- `[vc8]` **A two-word pair assigned as a struct.** Both words loaded in front of the stores (`mov ecx,[p+4];
  mov eax,[p]; mov [o+0x1c],ecx; mov [o+0x2c],ecx; mov [o+0x18],eax; mov [o+0x28],eax`) is a struct assignment,
  never member-wise code: inline setters with by-value ints, a by-value pair, a reference and a pointer to the
  local all forward the local's words to their uses, one at a time. The order tells the form: a chained
  `B = A = par` loads the high word first and stores `A.y, B.y, A.x, B.x`; `par = Par(a, b)` (a constructor's
  temporary into a local) loads `b` first and stores `y, x`, finished before the next statement begins;
  `*(Par *)&m = Par(a, b)` through pointers loads `a` first and stores in member order. The second word loaded
  in front of the first test's jump, and a member read back from memory behind the stores, is a constructor's
  temporary made in front of the test (`Par t(par.x, par.y); if (t.x > 0 && t.y > 0) { B = A = t; ... }`)
  (Commandos 0x5ce8a0, 0x565f00, 0x56b9e0). **held** *(match)*.
- `[vc8]` **A by-value struct built in the argument's slot** (`sub esp,N; mov esi,esp; ...strncpy into esi...`
  and no local of that size in the frame) is the struct's converting constructor called by the argument
  (`t->Buscar(texto)` with `Nombre(const char *)`); a named local is built in the frame and copied with
  `rep movs`. A name struct of ints (`int w[8]`) takes no security cookie, one of chars does (0x5ce8a0,
  0x565f00). **held** *(match)*.
- `[vc8]` **Frame slots follow scopes.** Function-level locals take their slots by first use, whatever the
  declaration order; block-scoped locals come behind them, siblings sharing. A local whose slot lies between two
  function-level ones in ours and behind them in the image is declared in each arm instead of once at the top
  (0x5ce5e0: `float a` in both arms of the mode test). **held** *(match)*.
- `[vc8]` **A library stream object on the stack** whose destruction is written out at the end (`call ~base;
  mov [f+N],vtable; call ios_base dtor`) under an EH frame and with no cookie is a stand-in class of ints with
  an inline destructor doing exactly that; a `char[]` stand-in with the calls at the end of a block has the
  cookie and no EH state (0x5cdaa0, 0x5cdbf0). **held** *(match)*.
- `[vc8]` **Case blocks lie in source order**, and each block's single-use loads take the next scratch register
  in text order: `case 2`, `case 1`, `case 0` behind a `sub eax,1` chain is how the source lists them
  (0x5ecbd0). An object loaded first and its member last around the argument pushes (`mov eax,[this+0x74]; lea;
  push; lea; push; mov ecx,[eax]`) is an inline forwarding member of the object; a repeated register
  (`mov eax,[g]; mov eax,[eax+8]`) an inline getter; a table loaded in front of a by-value argument's copy a
  getter's return value. **held** *(match)*.
- `[vc8]` **Rewriting a callee can change a matched caller.** A function made smaller (an inline constructor, a
  `switch`) started to be inlined into a matched function of the same file; it belongs in the file of its
  address neighbours. Run the whole comparison after rewriting anything another function of the file calls
  (0x5cd8d0). **held**.
- `[vc8]` `neg cl; sbb ecx,ecx; neg ecx; ...; xor edx,edx; test ecx,ecx; setne dl` (a flag made twice) is an
  inline `bool` function returning `r != 0` of an `int r = (a & b) != 0`, used in an int sum (0x586df0).
  **held** *(match)*.

- **A `__stdcall` helper called with an object in `ecx` is a `thiscall` member.** `[vc8]`
  **held (7 matches on this alone).** A member that never reads `this` has the
  same bytes as a `__stdcall` free function, so the callee matches either way;
  the *caller* does not. Signs in the caller: `this` stays in `ecx` up to the
  call where ours copies it away and reuses `ecx`, a global goes into `edx`
  while `ecx` is free, or `mov ecx,[m]` sits right in front of the call (the
  helper belongs to that member's class; a port that calls it as a free
  function passes no object, a real bug outside the original compiler). A
  "free" function that never touches `ecx` is a member itself. List every
  `__stdcall` name whose address (or thunk target) also has a `thiscall` name
  in the symbol table, then the ports that call one.
- **Two memory read-modify-writes on one flag word (`or [m],1; and [m],~2`).**
  `[vc8]` Bitfields and masks merge through a register; inline helpers taking
  the word's *address* (`Pon(unsigned *p, unsigned m) { *p |= m; }`) keep the
  two. Inline members of the record merge again.
- **Block locals of equal size in different `switch` cases share a slot and let
  the cases' tails merge.** `[vc8]` When the image keeps the tails apart (each
  case loads the callee's object before `push N`, one shared call sequence),
  give the buffers the sizes that are actually passed (0xe, 0x12), not the
  frame's rounded ones: same frame, no merge.
- **Count and destination of a string copy set up in front of a `switch`.**
  `[vc8]` Every case copies its own literal (`strcpy(buf, "...")` per case);
  VC8 merges the copies and hoists the setup. One pointer chosen in the switch
  and a single copy sets up behind it.
- **Two failure exits laid out in the other order.** `[vc8]` Nested `if`s
  and one `&&` condition compute the same thing and lay the exits out
  differently (0x5bcae0: `if (n < 10 && o->Probar(x))` puts the outer test's
  exit first). Likewise `r = n++; t[r] = o;` loads, bumps through `lea`, then
  stores, where `t[n++] = o` stores and then `add [n],1`.
- **`cdq; mov ecx,K; idiv ecx` for `/K` and `%K`.** `[vc8]` The divisor is a
  local (`int n = 20; q / n; q % n`); the literal gives two magic multiplies.

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
- `[vc8]` **Control-flow and member-path shapes that moved the allocator.**
  A virtual slot loaded before the branch (`cmp [this+f],0; mov ecx,eax`):
  write the member path at each use (`if (panel->visible) panel->Mostrar(0);
  else panel->Mostrar(1);`), not a local pointer. A shared `ret` returning the
  tested value: nest the rest under the inverse test (`if (e != 0xc) {...}
  return e;`) and re-read members instead of a local copy. A list walk to the
  tail: `while (h && h->siguiente) h = h->siguiente;`. **Held.**
- `[vc8]` **The extra x87 rounding around a root.** `fstp/fld` around
  `__CIsqrt` plus the local's own store is the `<math.h>` float `sqrt(float)`
  overload, or an inline `float f(float v) { return (float)sqrt((double)v); }`
  called on the expression (0x62d8d0). **Held.**
- `[vc8]` **More constructor and loop shapes.** An int store before the third
  store of a zero vector: write the int assignment before the zero-`V3`
  temporary. A count stored after a member array's construction loop: the count
  and the array are one member struct whose inline constructor sets the count.
  Base-class stores before the derived vtable store: an `inline` definition of
  the base constructor in the caller's file. A character tested from a register
  while copying: keep the dead buffer copy (`*d++ = c`) in the source; the
  optimiser drops the store but keeps the register load. **Held.**
- `[vc8]` **Tail merges and temporaries.** A call reached by `push; jmp` from
  each branch means the call is written in each branch. The `new` temporary
  shares its slot with a local declared in a later nested block (frame 8
  instead of 0xc, 0x562ce0). A test written as an inline `int` member puts the
  failure block inline. `operator new` plus placement or `->CtorXXX()` should be
  a plain `new T(args)` with a symbols line for the real constructor at its
  thunk (0x5776e0, 0x466e20, 0x4568d0). **Held.**
- `[vc8]` **Unwind entries with no action are an inlined callee's dead
  statics.** The image's `__ehfuncinfo` has more entries than the source
  declares when a callee with function statics is inlined and constant
  arguments remove the code that uses them: the statics still take EH states
  (dead ones come after the caller's, live ones at their source position). Put
  the callee's definition in the same file, above the caller, and call it with
  the constants (`Cambiar(1, 0)`); a sibling call with other constants stays out
  of line. This closed 0x54e650 (inlining 0x54c720 with kind 3) and the window
  `Mensaje` family (0x6095b0, 0x607410, 0x607840, 0x606e10, 0x6060e0, 0x606970).
  A temporary whose destructor is `throw()` has no `state = -1` store before the
  destructor call. **Held.**
- `[vc8]` **More small shapes.** `d >>= 1;` as its own statement moves the
  `sar` up to the merge point; `ActorSarg *yo = actor; m.quien = yo;` changes
  which scratch register holds the value. With a named float `d`, `d < 4.0f`
  loads the literal first; `4.0f > d` loads `d` first but against a qword
  constant; `(float)(expr) < 4.0f` gives `fld d; fcomp dword [4.0]`; an int zero
  gives `fldz`. An inline `mov cl,[eax]; mov [edx+eax],cl` copy loop is
  `strcpy`. A count read once and decremented at the top of the loop is `for (k
  = n; k != 0; p++) { k--; ... }`. A hidden return pointer plus an unused zeroed
  flag slot means the function returns the class by value. **Held.**
- `[vc8]` **Array new and devirtualised locals.** A function-local `new T[n]`
  reproduces SP1's count-overflow check (`mul; seto; neg; or`): do not
  hand-write the size computation. A virtual of a local object that shows up
  inlined was devirtualised by the compiler; defining it `inline` in the file
  reproduces that. **Held.**
- `[vc8]` **Hypothesis, partly reproduced:** a member load repeated after float
  stores may mean the stores went to memory the compiler treats as escaped, such
  as a reused address-taken argument slot (0x583640). Unconfirmed.
- `[vc8]` **Local objects with destructors, and float parameters.** A local
  destroyed through an EH state: declare the destructor at the image's address
  with its symbols line. An "int" parameter loaded with `fld dword` is a float
  (also a correctness check for ports: 0x480f60). An int-word name type with
  two inline strncpy constructors explains no /GS cookie. `if (m > x)` written
  member-on-the-left gives `fld x; fcomp [m]; test ah,5; jp`. An inline
  sub-object method (`datos.Reset()`) sets the loop pointer's anchor field.
  **Held.**
- `[vc8]` **Copies and template constructors.** A copy loop compiles to
  `rep movsd`; `memcpy` of a large size and struct assignment call the library.
  An inlined template constructor that differs from the out-of-line copy: an
  explicit specialisation of the constructor (`template <> inline
  ListaObjetos<T>::ListaObjetos(...)`) with `<new>` for `operator new[]`. A
  dead EH state between statics is an inlined callee called with constant
  arguments: move the function next to that callee's definition. **Held.**
- `[vc8]` **COM chains and x87 forms.** A COM call chain with `jl` to one
  failure block and callee-saved registers that `&&` does not reproduce: write
  it as an `hr` chain (`long hr = ...; if (hr >= 0) hr = ...`). `fld [x]; fmul
  st,st` means the expression is written twice (`a->r * a->r`); a named local
  stores and reloads it. An x87 compare whose left operand is loaded first
  (`p.z > Altura(...) + 1.0`) gives `fld z; ...; fcompp; test ah,5; jp`.
- `[vc8]` **Partial answer to the x87 operand-order family.** A dot product
  where ours always multiplied the y term first matched with an inline helper
  taking a typed struct pointer and float by-value coordinates, fields on the
  left (`s->a * x + s->b * y + s->c`); the same expression through `float *pl`
  (`pl[0] * x`) keeps the wrong order. Worth retrying on 0x529440, 0x5091e0,
  0x5b6480, 0x461790 and 0x45d540. **Held** on one function.
- `[vc8]` **Forwarding constructors and inlined callees.** An inline
  forwarding constructor works out its arguments before the pushes: declare
  the wrapper constructor (not inline) and point it at the base's address with a
  `~` line (0x632dc0 contexts). A callee the image inlines while the port calls
  it: a `static inline` copy of its body in the caller's file (0x44d3e0 inlines
  `PonerEstado(0, 0)`). x87 bound tests with the parameter kept on the stack:
  `if (x < xmin || x > xmax) return 0;`; early returns also move the
  callee-saved pushes below the early exits (0x610870). A vptr store that falls
  between two runs of field stores: the later run as a member array cleared by a
  loop, or the earlier run as member sub-objects (0x5006b0). **Held.**
- **Constructors written as functions fall in one compile once they are real
  classes; read the class off the constructor.** The order of the stores is the
  class: base constructor call, vtable store, then members in declaration order
  (a member initialised from an argument in the initialiser list sits before the
  constructor call of the member declared after it), then the body. An EH state
  store marks each base or member that has a destructor; the unwind funclets in
  front of the handler (`mov ecx,[ebp-N]; add ecx,off; jmp dtor`) give the
  member offsets and destructor addresses. A state that is never stored means
  nothing can throw after that member (a C function such as `sscanf` does not
  count), not that the member has no destructor. A run of member stores placed
  after the last state store and before body stores of lower offsets is body
  text, not a member with a constructor. Give the base and member functions the
  addresses they already have under new names (second symbol lines) and keep
  the old function name as a plain line when other files still call it
  (0x623210, 0x5676d0, 0x58e3b0, 0x5cfbe0, 0x586910). **Held, five in a row.**
- `call DWORD PTR [vtable]` (the vtable's absolute address) in a constructor or
  destructor: a virtual call on `this` written inside an inline member that the
  constructor or destructor calls. Written directly in the destructor the
  compiler binds it statically (`call func`); through the inline member the
  call stays virtual and the optimiser folds the vptr it has just stored
  (0x5862b0). An inline `~Base() {}` shows as a bare vtable store on the normal
  path while the unwind funclet jumps to its out-of-line copy.
- `lea eax,[arg]; lea ecx,[member]; push eax; call` (the argument computed
  before the receiver, pushed after it): the call sits in an inline
  constructor or wrapper of the member (`Nombre(const char *s) { Poner(s); }`),
  whose parameter is a temporary first. A direct call pushes the argument
  before loading `ecx` (0x5cfbe0).
- `new T[n]` with `mul`/`seto`/`neg`/`or` and a call of the vector constructor
  iterator taking a constructor address: `T` has a constructor and no
  destructor; the out-of-line constructor copies it points at are the inline
  constructors of the element types (0x586910). When giving an existing struct
  a constructor would change other functions of the file, derive an element
  type with the constructor and allocate that.
- A local class that declares a virtual destructor makes the compiler emit the
  deleting destructor beside the vtable. Compare it with the image's: when the
  image's version has the base destructor inline, define the base's destructor
  inline in the class too (0x58b1d0, 0x49a940), or the emitted one differs.
- Trap: a source copied into the tree with its old modification time (rsync
  `-a`) is older than an object built from another version of it; make keeps
  the stale object and the function looks absent or identical. Touch what you
  copy or delete the object.
- `[vc8]` *(match, 17 constructors in one round)* **A functional port of a
  constructor that writes its zero stores through `this` casts is a class
  without its members.** Read the image between the base constructor's call
  and the first body call: stores behind the vptr store are member
  constructors, in declaration order. A run that goes back to a lower address
  after a higher one (`+0x74, +0x78, +0x44, +0x40, +0x64.., then +0x28, +0x2c,
  +0x30`) is a member whose own constructor body runs after its nested
  member's: declare the outer struct with the inner one as its last member
  and give each its inline constructor with the stores in the image's order.
  Three floats stored high to low are a vector member (`{ z = 0; y = 0; x = 0; }`),
  `[m+4]` before `[m]` a pair. The same sub-object then shows in the body:
  later writes to it go through the member, and its inline methods give the
  `mov ecx,[m]; test; je; mov [ecx+8],1; mov [m+8],1` blocks.
- `[vc8]` The global that takes `*(int *)this` reads the vptr back
  (`mov ecx,[esi]; mov [g],ecx`) when member constructors stored between the
  vptr store and the read, and is an immediate when nothing did. A port that
  folds it has the stores in the body.
- `[vc8]` `mov byte [esp+N],1` behind a member constructor's call is an EH
  state: the member's class declares a destructor. Take the destructor's
  address from the unwind funclet (`mov ecx,[ebp-0x10]; add ecx,OFF; jmp
  dtor`) and name it; a member built through a `Construir()` helper or a
  placement `new` has no state.
- `[vc8]` Three loads into registers and three stores from a stack temporary
  (`fst [t]; mov edx,[t]; fst [t+4]; ...`) in a constructor body is the member
  assigned a temporary (`c68 = Punto(0.0f, 0.0f, 0.0f)`), not three stores.
- `[vc8]` `xor eax,eax` and eight dword stores, three times over, is three
  `memset(p, 0, 0x20)` with the intrinsic: one `memset` of 0x40 calls the
  function.
- `[vc8]` A pointer loaded in front of a run of stores and used behind them
  (`mov eax,[esi+0x1c]` ... `lea ecx,[eax+0x44]; call`) is a local declared
  there; written inside the call expression it is loaded at the call
  (`add ecx,0x44`).
- `[vc8]` `push dtor; push ctor; push n; push size; push p; call` in a
  constructor without an EH frame is a member array of a class with a
  constructor and a destructor (`eh vector constructor iterator`); with only
  a constructor declared the compiler writes the loop inline.
- `[vc8]` *(match, 12 constructors)* **Where the EH state store and the vptr
  read-back fall tells initialiser list, member constructor and body apart.**
  The state store in front of the derived vtable store with scalar zero stores
  behind it: the scalars are in the initialiser list (`c28(0), c38(-1)`), not
  in the body (the state store sinks behind body stores) and not in a member
  struct. A vptr read-back (`mov ecx,[esi]`) between two groups of zero stores:
  the group in front is a member with a constructor, the group behind is the
  body's first statements, even though the global is written later.
- `[vc8]` *(match)* A group of sub-object stores split by the EH state store,
  or a field stored behind fields that follow it in memory: the sub-object has
  a nested member with its own constructor (`struct { int a; Par p; int c, d;
  S() { a = 0; d = 0; } }` gives `p.b, p.a, [state], a, d`); the same stores in
  one constructor body come out in another order.
- `[vc8]` *(match)* Field stores in front of the derived vtable store, plus an
  unwind funclet that passes `this` unchanged to a destructor: an inline base
  constructor (its vtable store is dead) with a base destructor; the funclet's
  target names the base. With no stores in front of the vtable the base has
  only the destructor.
- `[vc8]` *(match)* `push size; add reg,OFF; call operator new`: the
  constructor argument was computed into a local in front of the `new`
  expression; written inside it the add comes behind the null test.
- `[vc8]` An inline constructor of a class with virtuals makes the compiler
  emit the class's deleting destructor in that file; if the image's has the
  destructor body inline, define it inline in the class there too, or the
  emitted copy differs from the matched one.
- `[vc8]` Not usable: a wrapper struct around a member to move its address
  load. The constructor matches, and the unwind funclet and the class
  destructor then call the wrapper's implicit destructor (`__forceinline` does
  not remove it).
- When a port is `void` and the image's tail is `cmp [g1],0; je; cmp [g2],0;
  je; mov eax,1; ...; xor eax,eax`, the function returns `g1 && g2`: fix the
  return type (and the mangled name) before matching.
- Read the port against the image's *slots*, not kuna's variable names, when a
  frame slot is reused: `fstp [esp+0x20]` in front of `add esp,4` is not the
  slot `[esp+0x20]` read behind it (a wedge angle taken from the wrong
  variable was a logic bug in a functional port).
- Trap: an edit that leaves the file uncompilable keeps the previous object,
  and the comparison shows the old code. Make the loop print compiler errors.
- `[vc8]` **Look at the call's target before its arguments.** A port that pushes a value where the image pushes
  an address may be calling the wrong neighbour: the image called a 16-byte wrapper (`mov eax,[esp+8]; mov
  eax,[eax]; mov [esp+8],eax; jmp f`) in front of the function the port named. That wrapper matches with the
  dereferenced argument as a local (`int c = *p; return f(a, c);`); `f(a, *p)` takes `edx`. An `xor edx,edx` the
  image lacks before a call means the callee is a `__thiscall` member, not `__fastcall` with a dummy argument.
  **held** *(match)*.

- `[vc8]` **Unlink stores in front of the vtable load are a `delete`.** `mov [n+8],0; mov [n+4],0; mov edx,[n];
  mov eax,[edx]; push 1; mov ecx,n; call eax` is `n->ant = 0; n->sig = 0; delete n;` through a virtual
  destructor; a named virtual `n->Borrar(1)` lets the stores sink behind `mov ecx,n`. `mov eax,[eax]; lea
  ecx,[eax+N]` (ours `mov ecx,[eax]; add ecx,N`) is a pointer returned by an inline accessor with the callee a
  member at +N (`O()->angulo.F()`). A null-tested pointer whose two fields are written with one test, the second
  through a reload, is one inline setter of both (`void Pon(int x, int y) { if (m) { m->a = x; m->b = y; } }`)
  called in each arm; two setters test twice. Three dword moves into a local and float pushes from it for a
  by-value argument are an assignment to a declared local (`P p; p = *src; f(p, ...)`) of a class whose copy
  constructor copies floats. **held** *(match)*.

- `[vc8]` **A temporary passed by `lea; push` instead of the constructor's `eax`: a named local in its own block.**
  `call T::T; lea ecx,[tmp]; push ecx` with the frame smaller than ours by the object's size is `{ T b(t);
  r.Poner("K", b); }`: the block scope lets `b` share the temporaries' slot. A function-scope `b` keeps its own
  slot; `r.Poner("K", T(t))` pushes `eax`; an inline function returning `T` gets the frame and swaps two
  registers. Read the char buffer's size off `sub esp` only after the scopes are right. A callee whose result is
  destroyed by a direct destructor call under an EH state returns the class by value (declare the same address
  under that prototype), not a pointer to a raw buffer. The idiom sweeps by regex over sibling writers. **held**
  *(match)*.

- `[vc8]` **A port class with the vtable set by address is usually an overloaded constructor of the real class.**
  Vptr store, member stores, then the EH state store (ours: state first) means the members' constructors ran
  before the body. `fldz; fst [m+8]; fst [m+4]; fst [m]` right after the vptr store is a member whose default
  constructor zeroes z, y, x; an assigned temporary goes through dword moves. Count the arguments at `ret N`
  before reading a "variant" as a wrapper around the other constructor: the image repeats the body. An EH frame
  without `__$ArrayPad$` means the local with the destructor holds no char array (`int c[15]`). **held**
  *(match)*.

- `[vc8]` **A member reloaded right after it was stored (`fst [m1]; fstp [m2]; fld [m1]`) came in by reference.**
  An inline setter with a `const float &` parameter bound to the member, storing through another pointer first,
  forces the reload; plain member reads are forwarded. A constant kept on the x87 stack across uses (`fld QWORD
  PTR [k]; fmul st(1),st`) is a literal, not an `extern const double`. **held** *(match)*.

- `[vc8]` **By-value struct arguments built through `mov eax,esp`** (`sub esp,N; mov eax,esp; fstp [eax]; ...`, or
  word moves for shorts) are copy-constructor calls; giving the struct a copy constructor also moved the rest of
  the loop to the image's registers. Struct stores that fall after the pushes are a temporary constructed in the
  argument list (`&Color(0, 0x80, 0)`; a ternary of two gives two slots). A call with `ecx` loaded and no `add
  esp` afterwards is a `__thiscall` member (`ret N`), not a register-argument static. **held** *(match)*.


- **`[vc8]` An argument loaded between `cmp [obj+N],0` and its `je`: the guarded virtual send is an inline member of
  the object.** `cmp [actor+0x208],0; mov edx,[this+0x20]; je; mov ecx,[actor+0x208]` with the vtable in `eax` and
  the id in `edx` is `void Motor(int a, int b, float c, int d) { if (motor) motor->Mensaje(a, b, c, d); }` called on
  the actor: the inline's parameters are evaluated in front of its test. Ports that write `if (p->m) p->m->F(...)`
  get the same pair with the registers exchanged, and their notes say "register roles". Three shapes tell the source
  apart by where the first argument loads: between compare and branch (the inline alone), behind the branch and in
  front of the member's load (the test written out with the inline inside it), behind the member's load (the member
  spelled twice). One byte pattern over the image lists every candidate; twelve ports matched in one sweep.
  **held** *(match)*.
- **`[vc8]` `mov r,[p]; lea ecx,[r+N]; call F` where ours has `mov ecx,[p]; add ecx,N`: the sub-object's method is
  called through an inline member of its owner** (`float Angulo() { return orientacion.Angulo(); }`). The inline's
  `this` is a temporary in the scratch cycle. Writing `p->sub.F()` out, a getter returning the sub-object's address
  and one returning a reference all give the `add`. The same rule with `N = 0` is `mov eax,[m]; ...; mov ecx,eax`
  for a callee wrapped by an inline member of the pointed object. **held** *(match)*.
- **`[vc8]` Identical tails merged differently from the image hide from an aligned listing.** Three `F(); break;`
  tails, two merged: when ours merges another pair the instructions line up one to one and only short against near
  jump encodings differ. Compare jump targets by instruction index. A label and a `goto` reproduce a given merge
  when no structure does. **held** *(match, one function)*.
- **`[vc8]` Small x87 and constant-register tells.** `fadd st,st; fstp [t]; fld [t]` is `(float)(v * 2.0)`; a
  default kept on the x87 stack through a compare (`fld K; fcom st(1); fstp st(1)`) is one ternary initialiser, not
  a default followed by an `if`; a constant in a callee-saved register shared by a member store, a guard mask and
  the EH state comes from the store written in each arm instead of once behind the `if`/`else`; `and r,1` behind a
  `sete` in a loop is `res &= cond`. **held** *(match)*.
- `[vc8]` **A virtual call with a call among its arguments reads the vptr in front of that call.** `m->Send(id, 0,
  d.Angle(), c)` gives `mov ebp,[ebx]` before `call Angle` and `mov edx,[ebp+12]` after it, in every form: the
  member spelled out, pointer and argument locals, inline senders of either object by value, by reference and
  forced, getters (16 sender forms x 6 argument forms in a scratch file). The read moves behind the call only when
  the argument's call comes from an inline expansion (`float B() { return A(); }`), which VC8 hoists in front of
  the whole call with a stored float temporary. An image that pushes the last argument, calls, and only then
  reads `mov eax,[ebx]; mov eax,[eax+0xc]` (Commandos 0x430200 and four twins, the only such sites in the image)
  is still **open**: do not spend sender spellings on it. A float held across a call as `fstp DWORD; call; fsubr
  DWORD` is likewise not a plain spill (VC8 spills x87 intermediates as QWORD): it is a by-value float parameter
  of an inline, whose arguments are evaluated first to last with the last one substituted (0x5b4c50, order of the
  two calls not reproduced). A load through a pointer that the image replaces by a register copy (`mov eax,edi`
  for a second `*pp`) and ours reloads: look for stores into an address-taken local between the two reads; they
  kill the reuse (0x447cf0, open). **record**.

- `[vc8]` **Two to four values all loaded (or all computed) before the first is stored: constructor arguments.**
  `call f1; mov edi,eax; call f2; add [m1],edi; add [m2],eax` where ours adds the first result before the second
  call; `mov ecx,[eax+0x4c]; mov eax,[eax+0x48]; mov [c+0x20],ecx; mov [c+0x24],eax` (the last value in the
  dying base's register) where ours takes a third register and another order; register sums stored afterwards
  where ours folds `add [mem],reg`. The values are the arguments of a pair's or a box's inline constructor:
  `Par d((int)(nx - px), (int)(ny - py)); x += d.a; y += d.b;`, `c->size = Size(f->w, f->h);`,
  `r.p = Point(dx + r.p.x, dy + r.p.y);`, `*r = Box(q->x, q->y, q->w, q->h);`. Plain locals, member-wise
  assignments and inline setters with the same parameters do not do it (they are scheduled statement by
  statement). A by-value pair whose words are both loaded before `sub esp,8` has a copy constructor that itself
  assigns such a temporary (`Par(const Par &o) { *this = Par(o.a, o.b); }`). A zero held in a callee-saved
  register for the later stores often arrives with the fix (the register is saved anyway). Seven Commandos ports
  whose notes said "register only" or "store scheduling" (0x47c250, 0x47dd20, 0x588cf0, 0x4f8df0, 0x581fc0,
  0x563550; 0x64e6c0 through an inline over the pair's address, `Shrink(&w, a, b)`). **held** *(match)*.
- `[vc8]` **An argument loaded into the register of the pointer it is read through (`mov ecx,[ecx+0x5c]; push
  ecx`) where ours takes `eax`:** the call is the body of an inline that takes the value as a parameter
  (`static inline char *F(int v) { return g.Set(v); }`, `F(member)`); an inline getter called on a pointer
  parameter and a by-value struct local do the same, the member spelled out, a pointer local and a reference
  parameter do not. Only for a base that arrived in a register (`this`, a `__fastcall` argument): for a base
  loaded from a global (`mov eax,[g]; mov eax,[eax+8]; push eax`) 29 inline, getter and local forms all give two
  registers, **open** (Commandos 0x581d70, 0x5944c0, 0x5cf530, 0x512640, 0x4e3930). The inverse case, ours
  `mov ecx,[g]; mov ecx,[ecx+N]` against the image's `mov eax,[g]; ...; mov ecx,[eax+N]`, is an inline getter of
  the global's class (`g->Names()->Index(n)`, 0x4dbb40). **held** *(match)*.
- `[vc8]` **A pointer member loaded in front of the argument to its right** (`mov eax,[this+4]; mov ecx,[a]`
  where ours loads and pushes `a` first): the by-value struct argument is read through an inline getter that
  returns a reference, `Point &Pos() { return obj->pos; }`, `Line(Pos(), a, o->pos, ...)` (0x5b8530). A getter
  of the object pointer, a pointer local and a reference local give other orders. **held** *(match)*.
- `[vc8]` **A float local with a slot of its own (`push ecx` at the top) where ours reuses a dead float
  parameter's slot:** the floats are one struct passed by value; VC8 reuses a dead scalar parameter's home and
  never a by-value aggregate's (0x428b40; the result was a struct returned by value there: same pushes and
  `ret N` in every caller). **held** *(match)*.
- `[vc8]` **The result's stores strictly in front of the first read of `this`, ours with the member load hoisted
  between them:** the function writes through a pointer parameter, which may alias `this`
  (`Box *Fig::Get(Box *r) { r->y = 0; ... }`); a local returned by value is known not to alias and its loads are
  scheduled early (0x581fc0). **held** *(match)*.
- `[vc8]` **One byte differs, an `esp` displacement behind a call, every instruction equal:** a callee in between
  pops bytes the source thinks belong to another call. `c->Put(img.Find(), name)` and `c->Put(img.Find(name))`
  push the same bytes in the same order when `name` is a by-value struct; only the compiler's idea of `esp`
  after `Find` differs (the image reloads a spilled vptr at `[esp+0x10]`, ours at `[esp+0x30]`). Check the
  callee's `ret N` before anything else: the port's signature was wrong and would have broken under any other
  compiler (0x588cf0). **held** *(match)*.
- `[vc8]` **A tail `jmp` to the last call with the arguments left in place while other parameters sit in
  callee-saved registers** (0x6455b0): VC8 tail-calls a `__cdecl` function with identical arguments only while
  no parameter is enregistered (13 shapes: inline halves, block copies, an early return, a returned result).
  **open**.

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

- `[vc8]` **New regex transforms over every port still pay** (Commandos round 109C, 1550 ports, seven identical
  at once, 176 closer): `retgoto` (every early `return X;` becomes `{ r = X; goto fin; }` in front of one `return
  r;`, all at once and one at a time, with a new result variable or the one the last `return` names: 4 matches),
  `inlvar` (a local initialised from a call-free expression and never assigned again, spelled out at every use: 1
  match, a function earlier filed as unreachable), `paramlocal` (an assigned parameter becomes a local of its own,
  and the reverse: 2 matches). `vretgoto` (`return;` to `goto fin;`), `retbreak` and `decltop` (declarations
  lifted to the top) matched nothing. Apply only exact matches: `inlvar` can change behaviour, and equal bytes
  are the check. Read the nearest non-exact results by hand afterwards: three more matches came from one more
  change on top of a sweep step. **held.**
- `[vc8]` **Twenty-two more transforms, three runs, eight exact and nine by hand** (Commandos round 111C, 1514
  ports). What matched: `declmove` (a constant-initialised local moved down, statement by statement, to its
  first use: 3), `inlone` (a cached value spelled out again at ONE use, the local kept for the rest: 2),
  `elseret` (a tail `if/else` as an early return, and the reverse: 1), `initelse` (`T v = K; if (c) v = x;` as
  `T v; if (c) ... else v = K;`: 2 as a step), `unnest` (a trailing `if (c) { body }` as `if (!c) return;`: 1 as
  a step). What only pointed: `voidint` (a void function returning a constant, the symbol renamed for the
  trial), `noret` (the last `return K;` dropped), `uns` (`int` local as `unsigned`), `declmove2` (any
  declaration moved past simple statements), `sumtmp` (a call term of a sum in a local in front). What matched
  nothing: operand exchange of `+` and `*`, `float`/`double` locals, casts dropped, literal suffixes,
  `char`/`int` locals, block scopes, ternary/if, chained assignments, an inline helper's parameters exchanged.
  A result can score closer for the wrong reason (reading an uninitialised local, returning a value the image
  does not): read it, then find what the image really does there. **held.**
- `[vc8]` **A cached array element spelled out once more at a later use exchanges the order in which a loop's
  two strength-reduced pointers are formed** (`o = l->v[i]; ... f(l->v[i])`: the two `lea`s in front of the
  loop flip; why is not measured), and the same lever makes VC8 recompute a difference instead of spilling it.
  **held** (three functions).
- `[vc8]` **A result that is reloaded from the stack behind every call, in a frame one word larger, is a member
  of a local struct whose other member's address goes to a callee.** `volatile` and a pointer to the result
  give other code. **held.**
- `[vc8]` **`fld [p+K+4]; add p,K`: read the float through the sub-object's pointer** (`d = p + K; q = d[1]`).
  VC8 folds the read to the old base and advances the dead base in place; reading `p + K + 4` directly keeps
  both pointers live in two registers. **held** (three constructors).
- `[vc8]` **A non-void function none of whose exits sets `eax`**: no `return` at all needs
  `#pragma warning(disable: 4716)` (C4716 is an error by default); with one real `return` elsewhere it is only
  C4715. An uninitialised `int r; return r;` gets a slot and a load instead. **held.**
- `[vc8]` **Refusal as the fall-through and the call as the taken arm behind a compare chain**: the chain sets a
  flag that is tested once (an inline predicate), `if (!c) return 0; return f();`. **held.**
- `[vc8]` **A sum whose call term is added last**: that term is a local of its own in front; no order of the
  one expression gives it. **held.**
- `[vc8]` **The result variable as a mechanical transform** (`rvar`): parse the body into statements, turn
  every `return K;` into `r = K;`, move what follows a returning `if` into its `else`, bail out on a return
  inside a loop or a `switch` and on code that would be duplicated; emit the result uninitialised and
  initialised to each constant returned (dropping those assignments), the constant assignment last or first
  in its arm, guards as written and negated: sixteen spellings. Four exact results over 1396 ports and part
  of 89 closer ones, where the `goto` form of the same idea had given none. With it: adjacent declarations
  exchanged, a member or dereferenced argument named in front of its call (one exact), the last
  `if/return/return` inverted, a pointer local replaced by the advanced parameter. A greedy run that mixes
  these with literal, compare and flag transforms found nothing the single transforms had not; of the round's
  six matches two were exact sweep results and one a sweep lead walked by hand. **held.**
- `[vc8]` **Scan the ports for hand-written string loops** (`while (*p) p++`, `do { c = *s++; *d++ = c; }
  while (c)`): each is `strlen` or `strcpy` (section 6). Twelve ports had them after 114 rounds; two matched
  at once with the intrinsic (one needed the buffer sizes, one the result variable), three had their
  differences elsewhere. The inline `strcpy` increments by whatever register holds a 1 (`add ecx,eax` with
  the result variable's 1). **held.**
- **A merge into another agent's dirty working file must go through a temporary copy.** `git merge-file` on
  the working file itself writes conflict markers into it when the other agent's uncommitted edit touches the
  same line (a rename of a symbol the change removes): merge into a copy, stop on conflict, resolve by hand.
- **Helper traps.** A replace-by-address helper that picks "the first file with a `// ADDR` comment" lands in a
  stub comment of another file; one that ends a function at "`}` followed by a blank line" swallows what follows
  a port with an inline helper in front; a helper that copies stdin to a scratch file must not be fed that file.
  Diff the own copy against its base after every helper call.

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
  0x621bc0); a zero or one kept in a register that SP1 did not close (0x5de170, 0x4261e0, 0x5d6d80 at 66 bytes); frame slot placement (0x4f7240, 2
  bytes; 0x63d460); a tail `jmp` lost to cached arguments (0x62fcf0); a static
  function whose object stays in `ecx` (0x5d48e0, 38 bytes). Cause unknown for
  each.
- `[vc8]` **Open:** VC8 picks the x87 operand order independently of the source
  order for adds and multiplies of plain locals and fields; the helper trick of
  section 5 only works when the destination is reached through a parameter
  pointer (0x438e70 block 1, 0x52c4a0, 0x53bf90).
- `[vc8]` **Open:** a load compared in memory and then reloaded in the next
  block (`cmp [x],0; je; mov eax,[x]`) that no source form tried reproduces
  (0x624140, 0x5a15e0).
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

- **A `.rdata` literal moves the vptr store.** `[vc8]` **held** *(match)*. A
  constructor whose member takes a float argument: with a global the vptr store
  comes early, with the literal it sinks to just before the member's call, as
  in the image (0x528570, 0x4228d0).
- **`??_R3` numBaseClasses off by one is a wrong base chain.** `[vc8]` **held**
  *(match)*. Read the real hierarchy from the image's `??_R2` array.
- **Inline asm `pusha`/`popa` assemble as the 16-bit forms.** `[vc8]` **held.**
  Write `pushad`/`popad`.
- **A function mixing C++ and `__asm` has a frame the source cannot steer.**
  `[vc8]` **held.** VC8 places the asm-referenced locals regardless of names,
  scope or declaration order; if the frame differs, the original was whole asm:
  use the naked form (evidence in the project's asm doc).
- **Still open:** the image returns the argument where ours folds the constant
  0 on an empty or null path (0x49cbd0, 0x536f50); taking the parameter's
  address does not stop the fold.

- **An unwind state with no action is a dead static in an inlined callee.**
  `[vc8]` **held** *(match)*. Reproduce it with a `static __forceinline`
  helper `if (x) { static int id = F(); ... }` called with a constant 0.
- **Singletons with one EH state per `new`:** write each as `new T` with T's
  constructor; if the code uses the pointer register, give T an inline
  constructor that calls the out-of-line init. `[vc8]` **held** *(match)*.
- **Inline virtuals emitted out of line** (e.g. `ClassDC2D::s06`, 0x563d00) mean
  the original header defined them in the class; calling them on a local object
  inlines them. VC emits the vtable and `??_G` in every file with an inline
  constructor: harmless for the comparison.
- **No value on some paths, one shared epilogue:** omit the `return` and reach
  one `fin:;` label. A second `return x;` at the end of the if block lays a
  branch's exit last and shrink-wraps the pushes. `[vc8]` **held** *(match)*.
- **`new T[n]` keeps the overflow check only for a non-literal count.**
  `[vc8]` **held.** Write `int n = 1; new T[n];` with `<new>` included.
- **An unrolled run of word copies** is one assignment per word; a loop stays a
  loop. `[vc8]` **held.**

- **Separate cases with identical bodies stay separate jump-table targets:**
  write them separately, in the image's layout order. `[vc8]` **held** *(match)*.
- **A by-value point constructor plus an inline two-pointer sum helper**
  reproduces a "store first, reload second" pair. `[vc8]` **held.**
- **A new ternary local gets a frame slot** that reassigning an existing
  variable does not share; a `{}` scope on a second count lets it share the
  first `new` temporary's slot. `[vc8]` **held.**
- **Win32 descriptor structs with `char` padding get a `/GS` cookie:** pad with
  ints, exact size. `[vc8]` **held** *(match)*.
- **The image's `setcc` is the negation of the source condition**
  (`x <= K ? a : b` gives `setg`). `[vc8]` **held.**
- **x87 add operand order follows the whole expression shape**, not source
  order; `a*b + c*d + e` can flip when unrelated code nearby changes: re-test
  after other fixes. `[vc8]` **held.**
- **A caller-saved register (edx) live across a call proves `/GL`.** Move such a
  function to the gl list.
- **"N bytes differ" is inflated by a frame-size or length shift:** fix `sub esp`
  first.
- **A port that cannot match often hides a functional bug:** an empty-class
  member array (size 1), or an output buffer smaller than what the callee writes.
  Look for these first.
- **Hook builder:** `extern int a, b;  // 0x.., 0x..` gives both names the first
  address; declare one global per line.

- **After a port fix that changes the frame, re-run the scripted variants:**
  the frame size shifts every esp offset, so earlier byte counts mislead.
- **Check symbols for a second line at the same address** (a plain caller name
  next to the `~` port) before matching; bytematch skips such functions.
- **Store order wrong:** try the order inside an inline setter's body; the
  argument order and local copies do not move it. An inline accessor on the
  middle struct (`hijo ? hijo->alto : 0`) gives an eax-chained null test; a flag
  stored as 1 at the top and cleared by a branch is `int f = 1; if (!(a > b))
  f = 0;`. `[vc8]` **held.**

- **Walk of the game's own list type: use its iterator first.** `[vc8]` **held**
  *(match)*. When the image tests a member list's own address (`lea ebp,[esi+0x1d0];
  test ebp,ebp; je`), write the loop with the project's iterator shape
  (`if (m) { it.cur = it.beg = m->datos; } while (it.Ok(m->cuenta)) { ...; it.cur++; }`);
  a hand-written pointer loop hoists the first load instead.
- **A value kept in a register and reloaded from its frame slot after an inline
  string intrinsic** is a register variable with a home slot; if ours lives only
  in memory the cause is elsewhere in the function.

- **A second `~` line with the same name blocks a match** (the comparator keys
  by name, and a thunk line for the same name counts). A plain line with another
  name at the same address does not; those are references from other objects
  and must stay. Delete the extra same-name `~` line when the body matches.
- **Odd dead-slot reuse means struct-by-value parameters** (structs of shorts
  read only as words); the caller's declaration often has the right signature.
- **`lea [eax+sym-k]` is indexing from `n - 1` on the next symbol:** write
  `&base[n - 1]`; the plain `&base[n]` gives `add`. `[vc8]` **held** *(match)*.
- **A single `int r = 0;` set in every branch with one `return r`** keeps a zero
  register; VC8 does not propagate constants along paths, so an early `return 1`
  folds it. `[vc8]` **held** *(match)*.
- **Debug-text functions hide behaviour bugs:** swapped sprintf arguments,
  uninitialised record fields, accents dropped from strings (`"s\xed"`).
  Compare against the image's literal bytes.
- **In an `and esp,-N` frame** a large `char` array aligns to 16 and shifts
  every local; the original was `char[0x100]` plus a 4-byte local, not
  `char[0x104]`.
- **A count loop** `for (int i = n; i > 0; i--, d++)` keeps the counter in the
  dead pointer parameter's slot; a last choice test is `else if (e == 5) n = A;
  else n = B;`.

- **Commutative x87 product operand order follows how the operand is addressed**
  (array index vs struct field through a pointer), not the source order: read
  the vector through a three-float struct pointer when every term order fails.
  `[vc8]` **held** *(match)*.
- **VC8 duplicates small return tails itself.** When two copies of a tail differ
  only in SIB order or register, write one shared `return` after the if/else.
  `[vc8]` **held** *(match)*.
- **`add reg,-imm` where `sub` is expected** means the subtraction is shared with
  a compare or sign test: write the test on the expression (`fotograma - 1 >= 0`),
  not on a local. `[vc8]` **held** *(match)*.
- **A vtable tail call jumps through eax only when its value is returned through
  a typed function pointer;** otherwise it goes through edx. `[vc8]` **held.**
- **Iterator structs:** VC8 keeps a three-word struct in memory and scalarises a
  two-word one; choosing which three words it holds (list, cursor, start)
  reproduces the image's frame. `[vc8]` **held** *(match)*.
- **Check that a recorded "near miss" is really in the tree** before chasing it.

- **Audit `ret N` for every port once.** `[vc8]` **held** *(match)*. A wrong
  arity or calling convention is a byte mismatch and a functional stack bug
  (constructor 0x5b4f80 took two radii, 0x4bb230 four arguments, 0x63d460 and
  0x5ecf00 were `__stdcall`). `{0, N}` in our listing against `{N}` in the image
  is not a mismatch: the extra `ret 0` belongs to EH stubs.
- **By-value struct parameter vs separate floats:** same stack layout, different
  x87 scheduling; the caller's `sub esp,N` followed by field-by-field `fld/fstp`
  copies is the sign. Only the mangled name changes. `[vc8]` **held** *(match)*.
- **Prefer the natural loop to a hand-rotated one.** VC8 rotates and strength-reduces
  itself. Not always a plain `for`: try the `while` whose condition order mirrors
  the image's tests (`while (!r && n--) {...}` keeps the result re-test), 2 of 6
  hand-rotated ports matched. `[vc8]` **held** *(match)*.
- **A count compared in memory on every pass:** make the data a member
  sub-object with inline accessors and read through the member at each use; a
  local pointer to the list keeps the count in a register. `[vc8]` **held** *(match)*.
- **A search whose empty-list exit and miss share `xor eax,eax`:** initialise the
  result to a sentinel (`d = -1`) and test once after the loop so VC8 merges the
  exits. `[vc8]` **held** *(match)*.

- **A `~` comment's diagnosis can be wrong.** A "scheduling" near miss was a
  functional bug (the image calls `Soltar()` on both paths, the port only on
  one): read the image's branch targets before tuning order (0x4cc350).
- **A value in eax on every exit of a void port means the function returns it;**
  that fixes the register choice for the whole function. `[vc8]` **held** *(match)*.
- **VC8 folds a constant return through an early `return K` but not through a
  variable cleared in an if:** `int r = 1; if (c) r = 0; return r;` keeps the 1 in
  a callee-saved register across a call; `{ r = i; break; }` keeps the zero in eax.
  `[vc8]` **held** *(match)*.
- **Nested flag test shape:** `if (a) { if (!b && !c) v = 0; } else if (!c) v = 0;`
  instead of `!(a&&b)&&!c`; one fix matched three ports sharing the tail.
  Grep other `~` ports for the same tail code. `[vc8]` **held** *(match)*.
- **A store through the same base as a later load lets VC8 hoist the load above
  it;** a separate pointer stops that. `[vc8]` **held** *(match)*.
- **Bounds-checked accessor in a do-while** (`do { if (t->Usado(i)) return i; }
  while (++i < n);`) gives the image's entry `jae` into the increment.
- **Named element pointer** (`int *s = p[i]; r = (s[0]==3)|(s[0]==1);`) puts the
  `setcc` result in eax.
- **Resolve an unexpected thunk call in a near miss:** bytes under relocations are
  masked, a wrong call target only shows as a note under the DIFF line. The real
  callee's type also explained 12 extra frame bytes (0x4b4620).
- **Comparison tooling:** filter the image's `nop`/`lea r,[r+0]` padding before
  side-by-side diffs, or later lines look shifted.

- **After a functional-porting phase, run bytematch with the `~` marks stripped:**
  seven ports were already byte-identical and only needed their marks removed.
- **Ports written for another build target often emulate constructors** (`operator
  new` plus a member call, placement new). Restore `new T()` with a declared
  constructor (add its name to the symbols file): the EH frame follows in one step.
- **A `sub eax,K; je` chain is a `switch`;** VC8 chooses the arm order itself, an
  if/else-if chain cannot reproduce it. `[vc8]` **held** *(match)*.
- **Matching beside another agent:** keep an origin snapshot per file (`.orig`) and
  check `HEAD:path` against it before committing; `git log` since a base is not enough.
- **A caller that calls members without reloading ecx, or passes arguments in
  eax/ecx to a static function, is a `/GL` case** (candidates for the gl list with
  their callees).
- **An `int` local prevents narrowed word loads; `if (x) {body}` instead of an
  early return gives `mov eax,[m]; test eax,eax`;** access through repeated casts
  rather than a typed local changes the `fadd` operand order. `[vc8]` **held** *(match)*.

- **Out-parameter locals: declare `int n;` without an initialiser.** `[vc8]` **held**
  *(match)*. A local the callee fills (a block size) reuses the dead parameter's
  stack slot as in the image; `int n = 0;` gives it a slot of its own. (Safe when
  the callee always writes it before any read.)
- **Small `short`/`char` array locals bring a `/GS` cookie the image may not
  have; a struct avoids it.** Per-case colour blocks: a small pair struct
  declared inside each `case` reproduces one 12-byte block per case.
- **A float triple copied from ints:** a local struct assigned whole
  (`V3 v = {(float)x, (float)y, (float)z};`, then the clears, then
  `*(V3 *)&r->x = v;`). `[vc8]` **held** *(match)*.

- **Ports written for another build target: look for structural causes first.**
  Emulated constructors and destructors (`Crear()` plus a hand-stored vtable
  becomes a real constructor with a base initialiser and a declared base
  destructor; callers use `new Sub(...)`, each subclass its own size, or VC8 merges
  the two `push size`), a hand-made release call where the image has an EH state
  (declare the destructor), `operator new` for `new T[]`, `memcpy` for CRT
  `strncpy` (`call 0x651a70` with dst, src, n), wrong buffer sizes. Most of one
  round's 16 matches came from these.
- **Gaps in the image's frame layout** (between the last local and the cookie, or
  shared slots) reveal wrong array sizes and missing `{ }` block scopes: give each
  use of a shared slot its own block scope.
- **A static I/O helper may need `__forceinline` and access to members** (a
  template over the owner) so the image's member re-reads come back.
- **Rect built late:** an inline `Rect(a, b, c, d)` constructor gives "last
  argument's call first, stores after"; `T *t = &a[i]; t->F(...)` moves the address
  computation between the pushes; a 12-byte float block is copied into a local,
  passed, and copied back whole. `[vc8]` **held** *(match)*.

- **Port-specific workarounds look like scheduling diffs.** Uninitialised
  out-parameters, and calls the image routes through an interface pointer
  (`((ISavRW *)&r)->Leer` instead of `r.Leer`, which compiles to a direct call),
  are typical of ports written for another target: check for them before tuning
  scheduling. An out-parameter the image keeps in a callee-saved register: leave it
  uninitialised and copy it into a local after the call; otherwise its escaped
  address makes VC8 re-read the slot at every use. `[vc8]` **held** *(match)*.
- **A first-byte store before a hand copy loop is a literal `nombre[0] = 0;`.**
- **`__forceinline` on helpers that take function pointers** recovers the
  image's inlined shape when the original wrote the code inline; values read
  inside each arm must stay inside the arms.

- **After each batch of new functional ports, rerun bytematch with the `~` marks
  stripped and compare its `ok` names against the `~` lines (`comm`):** 13 of one
  batch were already identical and only needed their marks removed.
- **Writer called through its vtable** (`mov eax,[w]; mov edx,[eax+8]; call edx`):
  call it through an interface pointer (`((ISavRW *)&w)->Escribir(...)`, not
  `w.Escribir`, which compiles to a direct call); one substitution matched four
  savers. `r.Poner(key, Bloque(x))` where the image passes `lea eax,[slot]`
  rather than the constructor's returned eax needs `{ Bloque b(x); r.Poner("KEY", b); }`;
  if the argument comes from `push eax` after the constructor keep the temporary.
  Apply such mechanical substitutions call by call, not to a whole family.
  `[vc8]` **held** *(match)*.

- **Games that read heap they never set** (a walk counter, a mode field): a CRT
  that sees Windows 9x (`GetVersion`) uses its small-block heap, which hands back
  old bytes; the same CRT on NT uses `HeapAlloc`. A reference build and a native
  port must use the same deterministic zero-filled heap (the layer's fixed heap, with
  the loader reporting NT/XP), or behaviour (a soldier patrolling vs walking)
  diverges for reasons that are not in the code.

- **"0 bytes differ" but still DIFF:** an inline constructor or destructor emitted
  in the object and referenced through a thunk is compared there. Give it a symbols
  line at its real address; that fixes it and adds a match. `[vc8]` **held** *(match)*.
- **Check the `push N` size before every `new` against the port's struct size:**
  it caught a real heap overflow (0x310 vs 0x3d0 per mission).
- **A caller that sets ecx right before calling a port declared `__stdcall`** means
  the callee is a member function.
- **VC8's float overloads in `<math.h>`** (`sqrt(float)` -> `sqrtf` -> `(float)sqrt`)
  leave the image's double rounding and stop tail merging; an explicit double `sqrt`
  gets tail-merged. `[vc8]` **held** *(match)*.
- **An inline `fnstcw/fistp qword` conversion** where neighbours call `__ftol2_sse`
  means the target is `(unsigned)`.
- **Savers: `delete[]`/`new T[n]` not `operator delete`/`operator new(n*size)`;**
  a count the image gets by calling a tiny function must be called each time; an
  update through a pointer (`unsigned *b = &banderas; *b |= 4;`) keeps the image's
  load order.
- **Debug text/log functions, savers and loaders hide behaviour bugs** (swapped
  arguments, wrong initial index -1 vs 0, wrong allocation sizes): a failed match
  is a prompt to re-read the original.

- **A slot whose result no caller reads falls off the end with no `return`** (the
  original function was `void`). If the image never sets eax on any return path of
  a port that returns a pointer, declare it `void`; functions that set eax to 0/1
  at their exits return `int ok`. In a function with an EH frame VC8 makes a
  missing return error C4716: check the try output for compiler errors, since
  fast-loop tools keep showing the old asm when a compile fails.
- **A by-value struct built with `sub esp,N; mov eax,esp; mov [eax],...`** means a
  user-written copy constructor.
- **VC8 hoists a constant used many times into a callee-saved register;** turning
  `!= k` tests into a `switch` (the `sub eax,k` form) removes it.
- **`mov ecx,this` before a call to a function that ignores ecx** means the original
  called it as a member: declare a small struct with the method plus a symbols alias.
- **An address-taken value computed in branches and stored once is a register
  temporary copied into the addressed local.** A `.rdata` float the image `fcomp`s
  directly is a literal in the source (an extern global gives `fld; fcompp`).
- **Dead parameter slots are reused for block-scoped locals of disjoint paths:**
  put each path's locals in its own block.
- **ofstream emulation:** an inline destructor on the stream type gives the EH
  state; `int c[0x22]` not `char c[0x88]` avoids the `/GS` cookie; the stream test is
  an inline `operator void *` returning `(state & 6) ? 0 : ios`.

- **VC8 canonicalises commutative x87 operand order;** source order does not decide
  it. Named `double` sub-results and compound assignment (`x *= f`, `d += v`) do:
  naming one product as a `double` local forces it first. `[vc8]` **held** *(match)*.
- **Check the symbols file for declared-only names pointing mid-function:** a line at
  an address that is not a thunk, not a call target and not after `int3` padding makes
  another build call into the middle of another function silently (four such lines
  were found and fixed).
- **Multichar constants:** a byte-match diff on a `push imm32` can be a reversed
  `'ABCD'` spelling (`'OTLA'` vs `'ALTO'`), which is also a behaviour bug.
- **A call with no `mov ecx` reload after another member call means `/GL`:** stop early.
- **Verify from `git archive HEAD` (or a clean checkout):** the live tree picks up a
  concurrent agent's uncommitted files and can show false differences.
- **`cmp 3; jge` is `< 3`** (this form also keeps a later `>= 2` test that `<= 2`
  folds away); a float result stored and reloaded before each `ret` is one result
  variable assigned in both arms; a `char[32]` name passed by value adds a `/GS`
  cookie (use `int w[8]` with an accessor); a flag materialised at an if/else join is a
  `__forceinline` helper returning it.

- **A `~` line at a function's incremental-link thunk makes bytematch skip the real
  line even when that line is plain.** Scan for names that have both a thunk line
  and a real line.
- **A newly matched function that references a data symbol with no symbols line:**
  bytematch's "learn" lines (address plus offset, e.g. `g_items+4`) can set that
  symbol's address in another build that uses learned addresses (a table landed at
  `+4` and crashed the start-up). Add an explicit line for every data symbol such a
  function touches and rerun the other build's replays after each matching batch.
- **Mechanical sweeps are cheap:** converting emulated placement-new into plain
  `new` across all ports, and replacing `.rdata` float/double globals with literals,
  each gave a match and moved near misses closer.
- **Spilled values as separate scalars, not an array** (the frame slot order follows);
  early returns rewritten as nested tests share one `return 0` epilogue; copy a
  parameter into a local when the image holds it in a callee-saved register across the
  first call; `(double)1.2f` for a double constant that is a widened float;
  `0.0f == p.x` (constant on the left) for `fld x; fldz; fucom`. `[vc8]` **held**.

- **A load moved above a store into an output pointer in a member function** means
  the output is the hidden return slot of a struct returned by value.
- **Two adjacent float/int parameters whose slots the image reuses for spills or
  locals are one by-value struct argument;** VC8 packs locals into a dead by-value
  struct argument but not into dead scalar argument slots. `[vc8]` **held** *(match)*.
- **dx/dy squared-length order:** put the difference in a 12-byte struct passed by value to
  an inline square (`static inline float Cuad(Dif d) { return d.y*d.y + d.x*d.x; }`);
  it matched four functions after 21 operand-order variants failed. `[vc8]` **held** *(match)*.
- **x87 product order:** name the first product as a widened double
  (`double t = (double)b1 * y;`) or widen the whole sum. `[vc8]` **held** *(match)*.
- **Check `~` ports against byte-identical twins at the same address in the symbols
  file:** the twin gives the exact source shape and exposes signature or
  calling-convention errors (9 matches this way).
- **`!(x >= 0.0f)` gives `fldz; fcomp; test ah,0x41; jnp`;** a call that leaves
  `ecx = this` untouched is a member call on `this`; storing a 64-bit member through a
  pointer makes a later `return` re-read memory instead of folding a constant.
- **A consistent displacement difference in otherwise equal aligned instructions is a
  struct layout bug** (a port with a struct four bytes short read the wrong fields).

- **A store through a named local pointer is a scheduling barrier in VC8 and keeps
  member stores in source order.** When VC8 reorders member stores whatever the source
  order, write them through a pointer local to the member (`float *f = &c8; f[3] = 0;
  ...`, `int *k = &x; *k = v;`); an inline helper (`*Pt(x) = v`) does not work, the
  pointer must be a local of the function. Sweep it mechanically on single
  `lhs = literal;` statements: 15 matches. `[vc8]` **held** *(match)*.
- **Sweep "cached local -> member re-read" mechanically too** where the image reads
  memory again. `[vc8]` **held** *(match)*.
- **Find symbols lines at the wrong address (or at a thunk) by searching the image for
  each DIFF function's object bytes with relocations wildcarded** (`elsewhere.py`):
  two lines pointed at the wrong function.
- **A full `make match` can catch a wrong callee that a single-file check missed;**
  it shows as "0 bytes differ" on a relocation. A port calling the projecting
  variant where the image calls the plain store (0x55aaa0 vs 0x55aa20) is also a
  behaviour bug.

- **A faithful decompiler (kuna with the stripped-exe fixes) is the best source of the original's structure:**
  expression nesting and operand order, local types, do-while index loops, statement order and widened-float
  constants (`0.800000011920929` is `(double)0.8f`). Rewriting a near-miss port to follow its output produced 21
  matches in one round, and diffing its tested masks, returned variables and dropped call results against the port
  found 4 real behaviour bugs in about 30 functions (wrong flag mask 0x100 vs 0x1000, a returned `visto` instead of
  `ok`, a discarded smoothing result). `[vc8]` **held** *(match)*.
- **`<math.h>` float overloads (`sqrt(float)`, `fabs(float)`, `cos(float)`) leave a fingerprint:** a double
  `fstp t; fld t; fstp t; fld t` round trip after the x87 call that no hand-written inline reproduces.
- **A temporary that seems to sit in a dead argument slot usually comes from an inline function's by-value
  parameter,** not from a scalar local reused by the compiler.
- **A bytematch "N bytes differ" count can mislead:** relocated constants are excluded, so a "0 bytes differ" DIFF
  can be a wrong `.rdata` constant.
- **Switch:** `sub eax,0; je; sub eax,1; jne` is a switch with `case 0` and `default` alike; `test ah,5; jp` with
  the left operand loaded first means writing the other operand on the left.

- **VC8 places compiler temporaries (an inline function's by-value parameter or return value, `$T`) into dead
  argument slots, but never a named local.** A decompiler that prints `aN = ...` for a float stored to an argument
  slot therefore tells you either the original reassigns that parameter, or the value is the parameter/return
  temporary of a `__forceinline` helper (`Mandar(actor, id, d.Angulo(), dato)`; a `Cuad(a, b)` squared length).
  `[vc8]` **held** *(match)*.

- **A native-only behaviour bug on a path of byte-identical functions: suspect the native CRT/runtime stand-ins
  first** (text-mode `_read` turning CR LF into LF, ignored open flags). A save-name editor whose logic matched the
  image byte for byte still lost the first letter of the NEXT slot's name because the stand-in `read()` kept the CR
  and the (identical) line reader moved on by 2. Typing replays only ever covered slot 0: prepare the user directory
  (index plus slot files) and dump what the game reads back.

- **Translation-unit visibility is a matching lever.** A callee whose body the compiler has seen in the same file
  changes its callers even without `/GL`, with `__declspec(noinline)`, and when the definition comes after the
  caller: x87 evaluation order, register choice and argument schedule move, and small callees get inlined where the
  image calls them. Sweep it mechanically: for each differing function compile its file with every other external
  definition removed (members) or reduced to a prototype (free functions) and report what becomes identical or
  closer (`alone.py`); then move the callee into a file of its own. Two direct matches and nine large drops in one
  pass. `[vc8]` **held** *(match)*.
- **Locals of a `__forceinline` helper die at its end and their slots are reused; an address-taken local of the
  function itself never shares its slot.** A frame that holds only one vector, with a later QWORD spill over its
  first two members (a decompiler prints `CONCAT44`), is the vector living in a helper that returns the scalar
  (`static __forceinline double Rumbo(V *t, O *o) { V d; ...; return d.Largo(); }`). The helper's parameters also
  schedule both pointer loads before the first `fld`. `[vc8]` **held** *(match)*.
- **A floating-point constant as a literal and as an `extern double` are scheduled differently:** `g + (a + 180.0)`
  with the literal loads the member first; the same expression with an extern constant keeps the other order. Try
  both spellings before permuting the expression.
- **A one-word struct passed by value is not packed like a scalar argument.** With a trailing argument as a
  one-word by-value struct (or union) the compiler reuses the dead argument slots for temporaries differently than
  with an `int`/`float` there: the instructions can match while the slot assignment does not. Which arguments are
  structs is part of the signature to search.
- **`/fp:precise`: a float call result subtracted from a second call's result.** `call A; fstp DWORD [a]; call B;
  fsubr DWORD [a]` (the first value kept as a float in its slot) only comes from naming the second result as a
  double before the subtraction: `float a = A(); double b = B(); a = a - b;`. Every `a - B()` form spills a QWORD
  temporary, and `/fp:fast` gives the DWORD spill but drops rounding stores the image keeps. It holds only while
  the next callee's body is not visible in the file (first bullet). Seven matches. `[vc8]` **held** *(match)*.

- **Dynamic initialisers and their exit stubs match as plain functions, in bulk.** `mov ecx,OBJ; call ctor; push STUB;
  call atexit` is `g.T::T(args); atexit(Stub);` with the explicit constructor call (a placement `new` plus a second
  call brings an EH frame and a cookie), and the stub `mov ecx,OBJ; jmp dtor` is `g.T::~T()` called qualified. Porting
  the registration with the stubs (instead of leaving it out as "exit behaviour") gave 71 identical functions in one
  round, half of them new. `[vc8]` **held** *(match)*.
- **Group functions by image code with immediates and displacements wildcarded** (`twins.py`): twin groups multiply
  the reward of one match, and families of generated code (initialisers, serialisers) show up at once. A template
  instantiation compiles like the hand-written copy of its body, so folding twins into a template only pays where
  the image's shape comes from an inline member of the shared class (a non-folded `new T*[n]` with the count as an
  inline function's parameter, class temporaries built in the argument area).

- **Take a large differing port apart by reading three things before permuting anything** (Commandos round 57, one
  1859-byte port down to 19 bytes): (1) the *slot layout*: VC8 packs locals with disjoint lifetimes into shared
  slots, largest first, so adjacent float pairs that later hold int pairs are two-member structs, not scalars (compare
  the listing's `_x$ = -N` lines with the image's offsets; scalars leave a bigger frame); (2) the *x87 stack order*
  with a diff that keeps `st(N)` (a diff that wildcards numbers reports such lines equal): the order loop-carried
  floats are preloaded is the order of their first assignment in the IR, so an image that stores x, y but preloads
  y first built the pair through an inline constructor with float parameters (arguments evaluated right to left,
  stores in member order); (3) *values reloaded from argument slots* in mid-function: they are fresh parameters of a
  `__forceinline` helper (a loop with its count and pointer, a setter whose by-value floats sit in dead argument
  slots while a `const float &` first parameter is stored directly). `[vc8]` **held** *(19 of 1859 bytes left)*.
- **The first product of an x87 sum is the one named as a `double` local** (`double t = m[1] * y; r = (int)(t +
  m[0] * x + m[2] * z);`), one local per expression; casts inside the expression and the written operand order do
  not move it. A member read through a `float *` to the struct (`pl[0]`) and through its name (`s->pa`) commute
  differently inside a product. `[vc8]` **held**.
- **A single-use `(int)` conversion is forward-substituted into its use and reorders the conversion calls;** results
  stored in small structs (`a.x = (int)...`) keep the calls in source order. The emission order of several
  single-use integer temporaries is not their source order: sweep the statement permutations (120 for five).
- **Flip float comparisons mechanically before anything else** (Commandos round 58: five matches, 73 ports closer).
  The operand written on the left is loaded first, which fixes the test mask (`test ah,5; jp` against `test ah,0x41;
  jne`) and which value stays on the x87 stack: `fld A; fld B; fcom st(1); fstp st(1)` with B used afterwards is
  `A > B`, not `B < A`. `x < y` to `y > x` is behaviour-neutral, so a script can flip every comparison of every
  differing port one at a time and keep what lowers the byte difference; restrict the pattern to whole operands
  between `(`/`&&`/`||` and `)`/`&&`/`||` so that `a < b + 1` is never touched. `[vc8]` **held** *(match)*.
- **A stand-in `extern float` for a `.rdata` constant is a guess that shows in the compare.** `fcomp DWORD PTR [K]`
  against a float on the stack is the float literal; `fld DWORD PTR [K]; fcomp st(i)` is an `extern float`;
  `fcomp QWORD PTR [K]` with a float-valued double is the float literal against a double expression. Swap the
  stand-ins of one file for literals, compare the file's differing list, and keep per function what helped.
  `[vc8]` **held** *(match)*.
- **`mov reg,ecx` at entry and `mov ecx,reg` before calls to helpers that never read `ecx`:** the function and those
  helpers are members of one class, not free `__stdcall` functions. **One result variable** (`int r = 0; ... r = 1;
  ... return r;`) gives the zero register reused for `return 0` and for zero arguments. **A tail shared by two
  branches in the image was written in both branches;** written once after the if/else it is duplicated.
  `[vc8]` **held** *(match)*.
- **`fld st(i); fmul [m]` where ours gives `fld [m]; fmul st(i)`:** read the struct members through a float pointer
  (`float *k = &box->k11; x * k[1] - y * k[0]`), or, inside an inline helper, name every product as a double in the
  image's stack order with the first one compound (`double t = x; t *= m[0];`). `[vc8]` **held** *(match)*.
- **A caller-saved register live across a call is NOT `/GL`: the callee was compiled earlier in the same file**
  (Commandos round 59: seven ports matched that six rounds had filed as whole-program code). At plain `/O2` VC8
  records which registers each function it has already compiled leaves untouched, and its later callers in that
  translation unit rely on it: no `mov ecx,esi` before the second member call, `this` never saved, a pointer kept in
  `ecx`/`edx` across the call (`call dtor; push ecx; call delete`), `mov eax,[ecx]; jmp edx` after a call. A port
  that sits in a file of its own, or reaches the callee through a stand-in declaration under another name, cannot
  match whatever its source says. Fix the file, not the function: put the caller after the real callee, in the
  callee's class. The callee itself need not be byte-identical. The same placement also explains "our port repeats
  three stores the image seems to have inlined": it is the neighbour called once and inlined once. To find
  candidates, scan the image of every unmatched function for `ecx`/`edx` read after a `call` without a write in
  between (treat `push ecx` as a slot reservation and stop at branch targets). Arguments passed in registers to a
  non-exported function remain `/GL`. `[vc8]` **held** *(match)*.
- **A float literal used by several comparisons is loaded once and stays on the x87 stack across pointer stores;**
  an `extern float`/`extern const float` stand-in or a local copy is reloaded or spilled. Write the literal at
  every use. While a wrong stand-in is in place a comparison sweep converges on the wrong operand order, so redo
  the neighbouring comparisons after replacing it. `[vc8]` **held** *(match)*.
- **"The image re-reads a parameter from its stack slot" where every source form keeps it in a register or folds
  it: the variable read is an unset local, homed by VC8 in that dead argument slot.** `int r; if (p) r = p->F(...);
  if (!r ...)` reads the last argument's slot on the path that skips the call; `int r; if (z != 0x47) r = 1;
  if (r == 1) ...` reloads `z` and compares it with 1 after `cmp eax,0x47`. Keep the image's value for other
  compilers behind a compiler guard. `[vc8]` **held** *(match)*.
- **A parameter the port passes on may be the wrong one.** `mov ecx,[esp+N]` between pushes: recount the offset
  with the pushes already made; one port passed its first parameter where the image passes the unnamed second one
  (a behaviour bug hidden behind "register allocation"). `[vc8]` **held** *(match)*.
- **Open:** the order in which dead x87 registers are popped at a merge point (`fstp st(1)` repeated against
  `fstp st(2); fstp st(0)`), the last 27 bytes of one port.
- **A port hundreds of bytes off with the right logic: the source named the whole member path in every condition
  and never cached it** (Commandos round 60: 352, 682 and 260 bytes to zero). Signs in the image: the table entry is
  tested with `cmp [base+idx],0` before the `lea` of its address, the element's first word is reloaded after each
  store through `this`, and a later call argument recomputes the whole chain. Write one `&&` chain
  (`tabla && n && n > i && tabla[i].p && tabla[i].n > paso`) and spell `tabla[i].p[paso].campo` at every use; the
  compiler's own CSE then produces the image's registers. A port with `T *tr = &tabla[i]; int paso = ...;` locals
  cannot be nudged there. `[vc8]` **held** *(match)*.
- **`add reg,8` on an object pointer, two `fsub`/`fstp` pairs, then `fld a; fld b; fld st(1); fmulp st(2),st;
  fmul st(0),st; faddp`:** an inline vector `operator-` (temporary built by a three-float constructor) and an inline
  squared-length member called on the result. Scalar `dx`/`dy` locals and float-taking helpers give other product
  orders. Which temporary lands in a dead argument slot follows creation order: naming one component first inside
  the operator (`float a = x - o.x; return V(a, y - o.y, z - o.z);`) moves it. `[vc8]` **held** *(match)*.
- **Two store/reload pairs after `__CIsqrt` or `fabs`:** the C++ `<math.h>` float overload (`l = sqrt(l)` with a
  float), not `(float)sqrt((double)l)`. `[vc8]` **held** *(match)*.
- **Array or struct for three float locals shows in the code:** `v[0] *= k; v[1] *= k;` on an array gives
  `fld k; fld st(0); fmul [v0]`, the same on struct members loads the member first in every written form. The
  order the zeros were assigned (`x = y = z = 0.0f` stores z first) also fixes the order of later sums of products
  over those elements, regardless of how the sum is written. `[vc8]` **held** *(match)*.
- **An `__asm` body is a visible callee too.** A non-naked function whose body is an `__asm` block tells the
  compiler which registers survive; a caller compiled after it in the same file keeps a pointer in `edx` across the
  call, while calls to `naked` routines reload. Port the caller into the assembly file, after the routine, as a
  member of the same class when the image loads `ecx` before each call. clang rejects naked member functions: give
  the member's mangled name to a free naked function with an asm label under a compiler guard. `[vc8]` **held**
  *(match)*.
- **A recursive call goes through the function's incremental-link thunk in the image;** compared as an in-function
  label it differs by itself. Declare a second name at the thunk's address and call that. `[vc8]` **held** *(match)*.
- **Jump-table blocks in address order are the `case` labels in source order;** a flag that is zero on the false
  path, with the false block after the `ret`, is the condition itself (`int f = a && b; if (f) {...}`), not an
  `else f = 0`. `[vc8]` **held** *(match)*.
- **Register-keep scans have false positives:** `or ecx,-1` and `lea ecx,[ecx+0]` after a call are a write and a
  pad, not reads.
- **A block at the end of a reader that repeats a small list routine is that routine inlined**, not a call: scan
  the unmatched ports for the routine's signature instructions and give them an inline copy with the stores in
  the image's order (Commandos: the sorted subscriber insert at the end of three `Leer` functions). `[vc8]`
  **held** *(438 to 15 bytes)*.
- **`push dtor; push ctor; push n; push size; call` after an overflow-checked allocation is `new T[n]`** with a
  declared constructor and destructor, never a hand-written cookie and loop; `call f; mov ecx,eax; call g` with no
  `lea` of a named object is a temporary (`v.Token(i).Numero()`). clang refuses a temporary for a non-const
  reference parameter: name it under a compiler guard. `[vc8]` **held**.
- **Locals of class type are reused late in a function** (a point zeroed again, a box refilled, all values
  computed in registers before any store): `c = Caja(...); p = Punto();` through inline constructors; a
  constructor written `x = y = 0` stores y first. A pointer stored into the slot of a dead address-taken local was
  assigned there in the source; the compiler never packs such slots itself. `[vc8]` **inferred** *(closer, not
  matched)*.
- **`x << 1` and `x * 2` are different expressions to VC8's common-subexpression pass:** a global written
  `paso << 1` is not shared with `short *` steps by the same count; `paso * 2` is merged with them and homed in a
  stack temporary. `[vc8]` **held** *(loop bodies matched)*.
- **Check which tree the native generator reads its symbol table from** before verifying a workspace tree that
  adds symbol names: a generator that derives the path from its own location needs a copy inside the workspace.
  A missing name shows only at run time, as an abort on first call.
- **The same call in both arms has its common argument pushes hoisted above the test** (`fld y; sub esp,8;
  test; fstp [esp+4]; ...; je <call>` with the pushes repeated once more for the fall-back path): write the
  early return with the full call (`if (!k) return F(x, y); ...; return F(x, y);`), not one shared call after
  the `if`. `[vc8]` **held** *(match)*.
- **A shared failure block laid out before the success return:** give every failing test its own
  `{ error = E; return 0; }`; the identical blocks are merged and the merged one lands where the last was
  written. One `goto` label or one `&&` chain puts it after the success return. `[vc8]` **held** *(match)*.
- **A result kept in its register across an `if` that proved it zero** (no `xor eax,eax` on the way out, the
  saves pushed only inside the `if`): one result variable assigned in the nested `if`
  (`if (d == 0) { if (c) d = F(); } return d;`). A `return d;` inside the block is turned into the constant.
  `[vc8]` **held** *(match)*.
- **`mov eax,K; push eax` in front of a call, with tests the compiler should have folded repeated:** the
  argument is an inlined function's return value (`G(Decidir())`). The inlined copy may need other statement
  forms than the standalone function compiled from the same source would (an early `if (e == K) return e;`
  against a wrapping `if (e != K) { ... }`): keep an inline twin. `[vc8]` **held** *(match)*.
- **One level of a recursive function inlined, then `mov [esp+4],arg; mov ecx,next; jmp <self>`:** VC8 turns
  plain tail recursion into a loop; the image's shape comes from the function calling an inline twin that
  calls the function back. `[vc8]` **held** *(match)*.
- **`fstp tmp; fld tmp` on every return path of an inlined float function and once more at the store:** two
  nested inline functions returning float (the outer one `if (e) return e->Inner(); return 0.0f;`).
  `[vc8]` **held** *(closer)*.
- **A cleanup call skipped by the failure branch** (the `je` lands after its `add esp`): the call is inside the
  `if` in the source. Read branch targets against the stack cleanups before blaming scheduling; the port had
  a behaviour bug here (the working directory restored when it had never been changed).
- **A loop that re-tests the count at its top on every pass and jumps back on a constant bound**
  (`cmp i,[n]; jge out; ...; add i,1; cmp i,K; jl top`): the condition is `i < K && i < n`, the constant first.
  The first test folds away on entry and the loop is rotated on the constant. `i < n` with a `break` on the
  constant inside gives the mirror image. One port had read this shape as "count above zero, then all K"; the
  image stops at the count. `[vc8]` **held** *(three matches)*.
- **A zero result held in a callee-saved register across a tail-jumped call:** `int r = 0; ... if (d) r = F(d);
  ... return r;`; `return F(d);` in the block folds the zero into `xor eax,eax` at the other exit.
  `[vc8]` **held** *(closer)*.
- **A frame-compare replay can be load-sensitive:** a run made while another build was using the machine
  differed from the reference in hundreds of frames for an unchanged tree, and matched when repeated alone.
  Repeat a differing run before bisecting.
- **Library inlines keep the library's spelling:** a string's small-buffer choice evaluated twice
  (`cmp cap,0x10; jb` before each use) matched only as `0x10 <= cap ? heap : inline` in both places of
  `if (p < Myptr() || Myptr() + size <= p) return false; else return true;` (the VC8 `_Inside` over `_Myptr`);
  `cap < 0x10 ? inline : heap` compiles 2 bytes shorter. When a hand-written port mirrors a CRT or STL inline,
  copy the header's operand order from the toolchain's own include directory. `[vc8]` **held** *(match)*.
- **Apply a family's recipe whole, not one idiom at a time:** a script reader 396 bytes off matched in a
  single edit once every known idiom of its siblings went in together (values as temporaries, `strlen` for
  the hand loop, the unsigned conversion written inside the call it feeds, the list insert as an inline
  member). Scores of the idioms applied singly would each have looked like "no change". `[vc8]` **held**
  *(match)*.
- **The x87 test mask is the source spelling, and the NaN behaviour:** with `d` on the stack and `X` on top,
  `fcomp; test ah,1; je fail` is `!(d <= X)`, `test ah,0x41; jnp fail` is `!(d >= X)`, `test ah,5; jp` is
  `d > X`. A port written from the "obvious" comparison can be right for numbers and wrong for NaN; take the
  spelling from the mask. `[vc8]` **held** *(closer)*.
- **`volatile` is a probe, never a fix:** when the image reloads one float from its slot at every join and
  yours carries it on the x87 stack, compile once with that local `volatile`. If most of the diff collapses,
  one variable's home is the whole problem (look for a twin that writes it through an out pointer); if not,
  stop chasing it. Do not commit the `volatile`. `[vc8]` **held** *(diagnosis; cause not reproduced)*.
- **A decompiler that prints ONE variable assigned on every path is reporting a fact about the slot:** a port
  that split it into `lim`, `d2`, `z` lost the value one path leaves behind (the height was computed from the
  wrong quantity for polygons). Diff the port's data flow against the decompiler's variable, not only its
  control flow. `[vc8]` **held** *(behaviour bug found)*.
- **Sweep the one-result-variable form over handlers:** a handler whose image zeroes a callee-saved register
  first and returns it on the handled exit (`xor ebx,ebx` ... `mov eax,ebx`) is `int r = 0; ...
  if (handled) { ... } else { r = Base(...); } return r;` with `r` declared before the statics. A script that
  rewrites every port ending `return 0; } return X;` this way matched 3 of 20 at once. `[vc8]` **held**
  *(three matches)*.
- **Read a small vector class off the way its three floats are copied.** `fld [a]; fstp [b]` three times is a
  user-written copy constructor (by-value arguments built in place with `mov eax,esp; fstp [eax]`, returned
  objects); three `mov` through integer registers is the default `operator=`. Differences stored into
  temporaries and then copied float by float are a subtraction that returns through the three-float
  constructor (`return V(x - o.x, ...)`: the temporaries are the constructor's parameters); differences stored
  straight into the result are the named-result form (`V r; r.x = ...; return r;`). One file can use both.
  Give the port ONE class with exactly those members instead of member-wise float code: three ports matched
  and four came close in one file once the class existed. `[vc8]` **held** *(four matches)*.
- **Three scalar float parameters that are later read or written as a block** (`lea eax,[esp+arg];
  mov ecx,[eax]`) are one object passed by value; change the signature (and the symbol name) and pass the
  object from every caller. A caller became identical from that alone. A class with a user copy constructor
  is passed by invisible reference under the Itanium ABI: declare the same members in every file that calls
  it, or the native build disagrees with itself. `[vc8]` **held**.
- **A hand-made "variant without X" helper in a port is often the real callee inlined with a constant.** When
  the image shows the callee's whole body after a by-value copy of the argument, call the real function
  (`F(0, pos)`), define it earlier in the file and delete the helper. `[vc8]` **held**.
- **Count the null unwind entries:** each one is a separate inlined call of a callee with a dead static. Two
  entries mean the source called it twice (`if (!A) P(1); else if (!B) P(1); else {...}`), not once behind
  `||`; the count also fixes the block layout. `[vc8]` **held** *(match)*.
- **A caller inlines a callee the image calls, though both are in one file:** the callee's port is not yet the
  image's function (size or EH shape differs). Match the callee first; do not fight the caller. `[vc8]`
  **held** *(diagnosis)*.
- **A message handler that returns 1 after its base call keeps that 1 in a callee-saved register across the
  call (`mov edi,1; call Base; mov eax,edi`): one result variable for the base call.** Declare it with the
  handled flag (`int hecho = 0, base = 0;`), make the last named message an ordinary block
  (`if (!hecho && id != -1 && id == idX) { hecho = 1; ... }`), then `if (!hecho) { base = 1; Base(id, a, b); }
  return base;`. `Base(...); return 1;` gives `call; mov eax,1`, and usually swaps the registers of the flag
  and the id through the whole function. A sweep that rewrites that tail mechanically over every handler
  matched twelve functions in one pass and brought ten more closer. `[vc8]` **held** *(12 matches)*.
- **A literal argument lets VC8 inline a large callee whose switch folds away; the same constant through a
  local keeps the call.** The image calls a 700-byte state function with `push 10`; written `F(10, 0, 0)` VC8
  inlines it (the dead statics show as extra unwind states), written `int e = 10; F(e, 0, 0);` it calls. The
  inliner decides on the tree before constant propagation. `[vc8]` **held** *(match)*.
- **A switch whose cases each end `push 0; push 0; push N; jmp <one call>` calls the function in every case**
  (`case 4: ...; if (t <= 0) Cambiar(5, 0, 0); break;`); a shared `siguiente` variable and one call after
  the switch gives `mov reg,N`. Cases that push the same arguments and jump to one `mov ecx,esi; call` are
  the same call written per case (`case 9: ...; F(p); break; case 7: ...; F(p); break;`), and a case with its
  own epilogue ends in `return`. The blocks in address order are the cases in source order. `[vc8]` **held**.
- **A constant `push` for the next call that sits after only some of the stores in front of it:** the stores
  above the push came from a counted loop that VC8 unrolled (`for (i = 0; i < 3; i++) v[i] = 0;`), or from an
  inlined member; plain statements let the push rise above all of them. `[vc8]` **held** *(match)*.
- **Two conversions stored side by side and read back with `fild` (`mov [a],eax` ... `fild [a]; mov [a+4],eax`):**
  an int pair (`int p[2]; p[0] = (int)(...); p[1] = (int)(...); v[0] = (float)p[0];`); two scalar ints are
  reordered by VC8. `[vc8]` **held** *(match)*.
- **Loop indexing `[reg-0xc]` off one member pointer and `[idx+global]` for every table** is the member arrays
  named in full each time (`cuenta[i]`, `indice[i]` as members); `int *cuenta = ...` locals become separate
  induction pointers and pointer differences. `[vc8]` **held** *(match)*.
- **A port whose code bytes are all equal but whose unwind table differs** (the comparer reports zero differing
  bytes and a moved `__unwindtable` label) lacks a null unwind entry: some run of plain stores in it is a
  callee with a dead static, inlined. Call an inline copy of that callee. `[vc8]` **held** *(match)*.
- **Two locals zeroed at entry whose stack slots are swapped (displacement bytes only):** declare them in the
  other order. Check this before anything else on a 2-byte difference. `[vc8]` **held** *(2 matches)*.
- **Ports of one family repeat one mistake.** Two handlers called the debug-info member on the message's
  argument instead of `this`; the byte diff showed it as `mov ecx,<arg>` against `mov ecx,<this>`. When a
  register difference sits on a `mov ecx` before a call, read it as a possible wrong receiver first, then grep
  the other ports for the same spelling. **held** *(2 behaviour bugs)*.
- **Two loads, then two stores, higher source offset first: a two-int struct returned by value.**
  `mov edx,[m+0x4c]; mov ecx,[m+0x48]; mov [o+0x20],edx; mov [o+0x24],ecx` is
  `Par t = m->Tam(); o.a = t.a; o.b = t.b;` with an inline `Par Tam() const { Par p; p.a = c4c; p.b = c48;
  return p; }`. Two plain assignments reload between the stores; two int locals, a two-argument setter and
  inline accessors all load the lower offset first. `[vc8]` **held** *(3 matches, one left since three rounds)*.
- **A by-value argument's callee object: a local when it is loaded before the copy.** `mov ecx,[g]` ...
  `mov eax,[ecx+8]; sub esp,N; rep movs; mov ecx,eax; call` is `T *d = g->tabla; d->Buscar(n);`; the chained
  `g->tabla->Buscar(n)` loads the object after the copy. `[vc8]` **held** *(3 matches)*.
- **A receiver that is a member's address: name it in full for every step.** `mov edx,[this+0x8c];
  lea esi,[this+0x8c]` (vtable read through the member's place, address formed afterwards) comes from
  `((R *)(p + 0x8c))->A(i); ((R *)(p + 0x8c))->B();`; a local for the receiver, or one inline member that
  does both steps, forms the address first. `[vc8]` **held** *(2 matches)*.
- **An EH state that skips a number between two statics is an inlined callee with a dead static, and the
  callee may be the class's own state setter with a literal.** `Estado(7)` inlined left one store and the
  dead static of another case; the port had written the store by hand. The sign is a handful of differing
  bytes that are all EH state immediates. `[vc8]` **held** *(1 match, 2 more calls found)*.
- **Variant runners must show compile errors.** A failed compile leaves the previous object, so every
  variant scores the same and looks like "the compiler folds every spelling". Print the compiler's error
  lines in the scoring loop, and score by the byte count, not by a count of differing listing lines.
  **held** *(two rounds of variants read as "unchanged" before it was seen)*.
- **A handler port that never reaches the base call is a behaviour bug the byte diff shows as a missing
  tail.** The image's `mov edi,1; call base; mov eax,edi` block had no counterpart: the port answered 0 to
  everything. When a scanner lists a function for a missing sequence, check first whether the port has the
  call at all. **held** *(1 behaviour bug)*.
- **Scan for functions whose original has an EH frame and whose port has none (and the reverse).** The
  original's first bytes (`6a ff 68`) against an `__ehhandler$` label in the rebuilt listing: a port without
  the frame lacks a destructor (a helper class used as a plain object), a `new` written as a call, or a real
  constructor. One helper class with a one-store destructor accounted for a whole family of paint routines.
  `[vc8]` **held** *(6 matches from one scan)*.
- **Field loads in front of an inlined callee's test (`mov ecx,[p+0x14]; mov edx,[p+0x38]; ...; test
  [obj+4],1; je`)** are locals declared in the order of the loads and passed to an inline member that holds
  the test. Written inside the call the test comes first and the address arithmetic sinks into the branch;
  wrapped in a helper function (even `__forceinline`) the surrounding stores move. Each repeated block needs
  its own locals. `[vc8]` **held** *(6 matches)*.
- **A dead 4-byte stack slot beside a spilled int** (the frame 4 bytes larger, nothing else different) is a
  two-int struct returned by value with one member kept in a register. `[vc8]` **held** *(match)*.
- **A load between the stores of two constant pairs** is two small objects with constructors and a local
  declared between them; one array or a temporary in the call puts the load after all the stores. `[vc8]`
  **held** *(match)*.
- **Constructor stores that come before the EH state store, in an odd order (`[+0x80]=0; [+0x7c]=0;
  [+0x84]=1; [+0x88]=1`)** are member objects with inline constructors in a real class, not statements in
  the body. If the shared header's base class is declared too long for those members, declare a local base
  of the right length under a second symbol name for the base's constructor and destructor, with the same
  virtual list. A class with a virtual destructor defined elsewhere needs it declared, or the implicit one
  is emitted and compared. `[vc8]` **held** *(2 matches)*.
- **The null unwind entry is often the class's own flag setter.** `flag = 0;` at the end of a routine or
  `flag = 1;` in a constructor was `Set(0, 0)` / `Set(1, 0)` of a setter whose other branch holds a static;
  dump the original's unwind map to see a state with no action and no store. `[vc8]` **held** *(8 matches)*.
- **A table lookup whose result lands in the table pointer's own register (`mov ecx,[ecx+0x28];
  mov ecx,[ecx+edx*4]`)** is a nested conditional expression (`v < 100 ? (idx[v] < n ? tab[idx[v]] : 0)
  : 0`), not `if (...) return ...; return 0;`: a dozen other spellings put it in the index's register.
  When the original has an out-of-line copy of the same lookup that already matches, copy its text into
  the inline. `[vc8]` **held** *(2 matches, 3 bytes off before)*.
- **Several inlined blocks in one function filling a small array in source order** (constant stores first,
  each loaded word stored at once): one array at function scope, assigned element by element in every
  block. An initialised array per block shares the slot but lets the stores sink below the later loads.
  The frame size tells whether there is one array or two. `[vc8]` **held** *(match)*.
- **Where a local's initialisation sits decides jump threading.** `if (x) A(); else B();` followed by
  `if (x)` is split into the two branches only when nothing lies between; `int k = 0;` declared after it
  leaves one merged indirect call plus `test; mov [k],0; je`, declared before it the store moves up and
  the call is duplicated per branch. `[vc8]` **held** *(3 matches)*.
- **A shared helper taking a maker function hides an EH frame.** If sibling functions each have the frame
  of a `new` and differ only in the class made, write the body once as a macro over the `new` expression;
  a helper with a function pointer has no frame at the call site. `[vc8]` **held** *(6 matches)*.
- **A member's store that sits after the argument pushes of a call made on the stored object** (`call
  ctor; ...pushes...; mov [member],eax; call`): the call is made through the member read back after the
  store (`T *a = new T(...); member = a; member->Set(x, y, z);`). Calling on the local, or assigning
  inside the receiver expression, puts the store in front of the pushes. `[vc8]` **held** *(2 matches)*.
- **Trap: a variant loop that prints only the tail of the compare** drops the `DIFF` line of the early
  functions of a long file, and they look matched. Grep the whole output for the symbol, and run the full
  match before recording anything as identical. *(one false match caught)*.
- **A field loaded and stored straight back (`mov ecx,[esi+0x18]; mov [esi+0x18],ecx`) beside a real
  update of its neighbour: the two are one pair struct copied by value.** `Par p; p.x = e->x; p.y = e->y
  + n; *(Par *)&e->x = p; *(Par *)&e->x2 = p;` keeps the self-store and gives the image's order (both
  loads, then the four stores); scalar locals (`int x = e->x; e->x = x;`) lose the store as dead. The same
  for a size pair set from two locals (`Par q; q.x = w; q.y = h;` copied to two places: the stores come
  out `y, y2, x, x2` around the next load). `[vc8]` **held** *(1 match, 1 function aligned)*.
- **A counted loop with `sub reg,1` at its top, entered by `test; je` and closed by `test reg,reg; jne`:**
  `unsigned k = n; while (k) { k--; ... }`. `for (k = n; k > 0; k--)` puts the decrement at the bottom
  and closes with `ja`. `[vc8]` **held** *(1 match)*.
- **`sub eax,1; cmp eax,7; ja; mov eax,[eax*4+table]` is the index in an unsigned local** (`unsigned i =
  level - 1; if (i <= 7) ... table[i]`); the cast inside the condition with `table[level - 1]` gives
  `lea ecx,[eax-1]` and `table[eax*4-4]`. A table loop closing with `cmp esi,0x40; jb` on a scaled index
  is `for (unsigned i = 0; i < 16; i++)` (the `int` index walks a pointer and closes with `jl`). `[vc8]`
  **held** *(1 match, 1 function to 2 bytes)*.
- **`mov ecx,esi` in front of a call whose callee was matched as a `__stdcall` free function: the callee
  is a member that never touches `this`.** Both compile to the same callee bytes; only the callers tell.
  Call it through a local struct member with a second symbols line for the address, and leave the matched
  definition alone. `[vc8]` *(1 caller, 24 bytes closer)*.
- **Frame slot order is not declaration order, and one more slot reshuffles the others.** Moving,
  renaming or re-scoping locals leaves the layout alone; adding or removing a variable that owns a slot
  changes the order of the same-size scalars (`c2, a, b` became `b, c2, zz, a`). The copy of `this` kept
  for the unwind funclet in a function with a GS buffer (`_this$GSCopy$`) sits directly under the buffers;
  a spilled value shares its slot only when no other slot is free for it (a second address-taken local
  took the spill instead). An image whose `this` copy is the lowest slot, shared with other locals, is
  still open. `[vc8]` *(observations, no match)*.
- **Trap: an experiment compiled in a second file leaves its object behind**, and the next full match
  reports that file's matched function as different. Delete the object of every file touched only for an
  experiment before the full run. *(one false alarm)*.
- **Float literals in a port copied from a decompiler as raw words (`*(unsigned *)&p.x = 0x45764000`) never
  match an image that loads them (`fld DWORD PTR const; fstp [slot]`).** The image built the place as a
  class temporary with a float constructor in the argument (`l.Poner(&V3(3940.0f, 2136.0f, 57.0f))`);
  word stores and aggregates give `mov reg,imm; mov [slot],reg`. `[vc8]` *(3 functions in one edit)*.
- **A float compared with a literal: the literal as the right operand is loaded as a `float` constant,
  as the left operand as a `double`** (`v <= 9.0f` gives `fld DWORD PTR`, `9.0f >= v` gives
  `fld QWORD PTR`; one opcode byte). `[vc8]` *(1 function, the last byte)*.
- **Function statics a porter wrote as file statics with hand-made guard bits: write the real `static`
  locals.** The guard's load, the `or` and the stores schedule differently by hand. No EH frame in the
  image although an initialiser calls a function means the callee is `throw()`; a callee that can throw
  brings the frame, one state per static and funclets that clear the guard bit. `[vc8]` *(1 function,
  196 bytes)*.
- **Arithmetic a porter spelled out step by step from the disassembly is worth writing back as the
  operator**: `rand() % 64` for `and eax,0x8000003f; jns; dec; or; inc` (the hand version moved the
  schedule of everything after it, 160 bytes), `(float)fabs(a - b)` for a compare with zero and a
  negation, `(int)(100.0 < x ? 100.0 : x)` for two conversions, `p <= 0 ? 0 : p` for `setle; sub 1; and`.
  `[vc8]` *(3 functions)*.
- **A file-static helper used several times is called through a private register convention, not
  inlined**; `__forceinline` (or a member defined in its class) gives the image's inline copies.
  `[vc8]` *(1 function)*.
- **A vector difference used once as a temporary (`(a - b).Angulo()`) spills the constructor's arguments
  and then copies them into the object** (three `fstp [t]`, then three `fld [t]; fstp [obj]`); a named
  result or a named local has only the first three. And `mov reg,[this+m]; lea ecx,[reg+off]; call` (the
  object kept in a register for a call on its sub-object) is an inline member of the object doing the
  call. `[vc8]` *(2 functions)*.
- **An argument of a later parameter loaded before an earlier parameter's temporary is built, with the
  receiver's null test after both: the send is an inline member taking all the arguments**, and a block
  the image repeats with its own function static each time (different guard bits) was written out in each
  place, not shared through a helper. `[vc8]` *(1 function, 602 to 22 bytes)*.
- **Trap: a generator that reads addresses from declaration comments loses a callee when a one-line
  struct is spread over several lines.** Keep each address comment on the line of its declaration; the
  list of unresolved names of a native build shows the loss, the byte compare does not.
  *(one unresolved name caught before the commit)*.
- **A member read passed as an argument is a local in front of the call when the image loads the other
  arguments first** (`mov ecx,[idX]; mov eax,[this+actor]; mov eax,[eax+4]; push 0; push 0; push ecx`):
  `int n = actor->numero; g->Enviar(-1, n, idX, 0, 0);`. In the argument list the compiler loads the
  object first and the other value into `eax` (a shorter encoding, every later byte shifted). Two such
  arguments are two locals in the order the image loads them; in a loop with a call between the reads
  and the pushes they take callee-saved registers and push the loop counter and `this` out of theirs. A
  sweep that hoists the member-read arguments of every call and keeps what gets closer found six
  identical functions in one pass. `[vc8]` *(6 functions, one more from 298 to 1 byte)*.
- **A message handler that answers 0 at once in one state (`xor eax,eax; cmp [this+state],N; je end`)
  holds its whole body under the test with the result declared outside** (`int base = 0; if (estado != 4)
  { int hecho = 0; ... } return base;`); `if (estado == 4) return 0;` in front swaps the registers of the
  flag and the id. And `mov eax,[base]; test eax,eax; je end; call Base` is `if (base) base = Base(...);
  return base;`. `[vc8]` *(2 functions)*.
- **`call f; fstp [x]; fld const; fcomp [x]` (a returned float stored before a compare against a literal)
  is the value passed through an inline function's `float` parameter**; a local compares `st(0)`
  directly. `[vc8]` *(1 function)*.
- **`if (p) p->x = 1; m = 1;` beside a call of a member of the same sub-object is that member's body
  inline**: declare an inline twin on the struct; the same statements on the owner's members schedule the
  pointer test first. `[vc8]` *(1 function, 22 to 0 bytes)*.
- **The last differing byte is sometimes a behaviour bug of the port, not codegen**: a one-byte jump
  displacement was a call the port had put inside an `if` (the image runs it either way), and an inlined
  callee showed a parameter named like a member hiding it (`id`). Read the last bytes as logic first.
  *(2 bugs)*.
- **Count the function statics the decompiler shows against the port's.** A lookup macro used twice
  gives two statics with the same string and two guard bits (`id == idA || id == idB` in the condition
  and a fresh `static int idA2 = Lookup("A")` inside the block); the port that reuses the first one is
  right in behaviour and off in every later guard bit and register. A scan that counts the lookup calls
  of each port against the decompiled function lists the candidates (inline helpers and macros show up
  too: read before rewriting). A shared helper holding a static is the same finding from the other side:
  when the image's guard bit belongs to the caller's own guard word, the loop was written out in each
  function. `[vc8]` *(3 functions)*.
- **`if (p->organ) p->organ->Msg(a, b, c, d);` with an argument loaded in front of the null test, and
  `mov eax,[this+p]; lea ecx,[eax+N]; call` for `p->member.F()`, are inline helpers** (`static inline
  void Send(P *p, ...) { if (p->organ) p->organ->Msg(...); }`, `static inline float Rumbo(P *p) { return
  p->member.F(); }`). Both are regular enough to sweep: a script that rewrites each occurrence through a
  generated template helper and keeps what gets closer matched four functions in one pass and brought
  three more within reach. `[vc8]` *(7 functions)*.
- **A `switch` whose compare tree is `cmp x,M; jg; je; cmp x,L; jne` has four cases**, one of them
  assigning the default value (three cases compile to a linear chain). And a frame exactly 16 bytes
  larger with only the EH record and cookie displaced is a buffer declared 16 bytes larger.
  `[vc8]` *(1 function, 307 to 0 bytes)*.
- **Temporaries chained in one expression (`v.Token(0).Valor("X").CopiarA(buf, n);`) alternate their two
  slots and EH states between statements**; named locals in a block give the same calls with other slots.
  `[vc8]` *(1 function, 425 to 0 bytes)*.
- **Two float locals that never share a slot with the temporaries around them are the first members of a
  three-float struct local** (the third unused); and `fld x; fld y; fmul st,st; fld st(1); fmulp st(2)`
  is `double a = v.x, b = v.y; ... b * b + a * a` (both loaded, then the products), whatever operand
  order the single expression is given. A counter that only feeds `fild` is the loop variable itself
  kept in memory (`for (int i = 0; i < 8; i++) f((float)(i * 45.0))`), not a second variable.
  `[vc8]` *(2 functions)*.
- **A `case` that falls into the next one in the port but has the next case's statements repeated in the
  image was written twice with a `break`**; the fall-through shares the code and moves every later
  block. `[vc8]` *(1 function)*.
- **Tool: compare frame displacements, not only instructions.** A side-by-side that normalises
  `[esp+N]` hides slot swaps; resolve the listing's `_x$ = -N` equates and print the lines whose
  displacement differs once the instruction streams agree. That is where struct locals, buffer sizes and
  store order show. *(4 functions finished from their last 2 to 33 bytes)*.
- **Two small locals can decide the register allocation of a whole function.** A 1400-byte handler kept
  its handled flag in a register and reloaded the argument everywhere; with the two message arguments of
  its sends read into locals in the image's load order (`int para = pa[1]; int de = ac->id;`) the flag
  moved to the stack and the argument into a callee-saved register as in the image: 1077 to 152 differing
  bytes from four lines. Run the cheap local sweeps on large functions too, not only on near-misses, and
  include non-member arguments (`((int *)a)[1]`) and both orders. `[vc8]` *(2 functions)*.
- **A handler's result variable carries the answer it gives when it handled the message**: `mov edi,1`
  at entry and `xor edi,edi; call Base; mov eax,edi` at the end is `int base = 1, hecho = 0; ...; if
  (!hecho) { base = 0; Base(...); } return base;` (declared in that order: the other order derives the 1
  from the zero register with `lea`); `mov edi,1; call Base; mov eax,edi` is the same with 0 and 1
  swapped. Sweep the tail forms, the nested state guard and the argument locals together: each alone
  often shows no gain. `[vc8]` *(5 functions)*.
- **Check the argument index of every parameter use against the decompiler.** A port read the payload
  pointer from the third parameter where the image reads the second (`[esp+0x28]` against `[esp+0x2c]`
  after four pushes); the callers leave the third 0, so the port compared against a null pointer on a
  path no replay reached. The decompiler's `a2`/`a3` names make this a one-line check. *(1 bug)*.
- **A callee the image inlines with constant arguments sits in the caller's translation unit, in front
  of it; it is not a header inline.** The sign: the inlined copies use the same function-static (same
  guard word and bit) as the out-of-line copy, and callers passing non-constant arguments still `call`
  it. Define the callee as an ordinary member in the same `.cpp` ahead of its callers and write plain
  calls: the compiler takes the same per-call decision as the original. An `inline` definition in a
  header gives a COMDAT static that may alias other stores, and the schedule around the static's load
  differs (15 bytes in every copy); a hand-made `static inline` twin has a guard of its own. So the
  split of the port into files matters: functions of one original source file that inline each other
  must share a file. Scan for it by comparing, per port, the callees our build calls with the calls
  in the image (a callee we call more often than the image does is inlined there). `[vc8]`
  *(3 functions at once, 2 more after merging files)*.
- **Cases with identical bodies written one by one are not the same as stacked labels.** `case 0: x =
  s; break; case 2: x = s; break; case 3: x = s; break;` is merged by the compiler into one block that
  jumps over the next case; `case 0: case 2: case 3:` falls into the code after the switch and moves
  the remaining case to the end of the function. A test of the switch value after the switch (`if (e
  != 1)`) stays as a dead `cmp; je` on the default path and is threaded away on the case paths: do
  not replace it with a flag set in the case. `[vc8]` *(3 functions)*.
- **`cmp x,3; jle out; cmp x,5; jg out` where the compiler would normally emit `lea r,[x-4]; cmp r,1;
  ja` is a range test written through a conditional**: `(x > 3 ? x <= 5 : 0)`. `x > 3 && x <= 5`, an
  inline predicate and a nested `if` are all merged into the unsigned range compare. `[vc8]` *(2
  functions)*.
- A one-case `switch` gives `mov eax,x; sub eax,2; jne` in front of code that still compares `x` with
  2 later; `if (x == 2)` gives `cmp x,2`. `cond ? f(A) : f(B)` as two pushes on two paths with one
  call is `if (c) f(A); else f(B);`; the conditional expression computes the argument with `sete`
  and `lea`. `[vc8]` *(3 functions)*.
- A value rounded to `float` twice in a row and then spilled as a QWORD across a call
  (`call __CIsqrt; fstp [s]; fld [s]; fstp [s]; fld [s]; fst QWORD [tv]`) is a `float` result kept
  in a `double` local and narrowed again at its use: `double s = (float)sqrt(d2); float t =
  (float)s;`. No inline wrapper, float parameter or second float variable produces the second
  store. `[vc8]` *(1 function, three siblings carry the same lines)*.
- x87 compares, read by load order: the operand loaded first is the left one; `test ah,0x41; jne`
  (skip) is `<`, `test ah,5; jp` (skip) is `>`. `fld K; fcomp [x]; test ah,5; jp` is `x > K` with a
  float literal; the mirrored `K < x` loads `x` and compares with a double constant. A port with
  the right logic and the operands swapped differs in every one of these. `[vc8]` *(2 functions)*.
- A decompiler that shows a float parameter assigned several unrelated values is showing stack
  colouring: VC8 puts temporaries and block locals into a dead parameter's home slot. Write
  separate locals. `[vc8]`
- When a re-read `*p` comes out as a reload where the image copies a register (`mov eax,edi`),
  look for stores into an address-taken local between the two reads: they kill the common
  subexpression. Removing the local in a scratch build confirms it. `[vc8]` *(open case)*.

- Two branches that each end in the same float test and share the block behind it (`fabs; fstp;
  fld; jmp L` / `fabs; fstp; fld; L: fcomp`) are a flag set under one condition in each branch
  (`if (b < 0 && a > 0 && (float)fabs(b) < 90.0f) fin = 1;`) and `if (fin) { ... }` behind them.
  The compiler threads the flag away and merges the two compares, not the rounding in front of
  them. The block written twice stays twice; a variable with a default that fails the test is
  materialised. `[vc8]` *(2 functions)*.
- A call result subtracted from a QWORD spill without a rounding store, with `lea ecx,[r+N]` for
  the receiver, is the call written inside the expression through an inline that returns the
  sub-object's address (`Orient(pp)->Angulo()`). A float-returning inline getter rounds its
  result into a temporary and is evaluated first. `[vc8]` *(2 functions)*.
- The order in which the two products of `(px - cx) * dx + (py - cy) * dy` are evaluated does not
  follow the written order: it follows the value numbers of the subtractions, which another
  block of the function may have defined first (`ex = px - cx; ey = py - cy;` in a sibling
  `case`). Swap those statements in a scratch build to prove the dependency before trying
  forms of the expression. `[vc8]` *(open: the image has the reverse in two functions)*.
- A sibling function with a matched twin is a behaviour check: one class's "go to point" slot
  skipped a repeated order when the motor stood (`&& stopped`), while the image and the two
  matched twins of other classes skip it when the motor is under way (`cmp [this+N],0; je end`).
  The port had copied the condition of a neighbouring slot whose test has the other sense. Read
  the branch sense of every flag test against the disassembly, not against the neighbour.
- A functional port written for a native build carries shapes that come from its origin, not from
  the image; check for them before any idiom search: private inline copies of small game
  functions the image calls, a decompiler's operand order in float compares (`!(K <= t)` for
  `t < K`, `m < t` for `t > m`: the decompiler puts the operand the image loaded first on the
  left), and safer buffer sizes. One port lost 300 of 356 differing instructions that way.
  `[vc8]` *(1 function)*.
- Compare the stride of every array the port allocates (`mul K`, `add reg,K`, the element size
  passed to the vector constructor) with `sizeof` of the port's element type: a port whose struct
  had every used field at the right offset was 4 bytes short, so the unported readers of the
  array stepped past its elements. The struct's own comment gave the right size.
- Align the two listings (difflib over normalised instructions) before comparing them; a
  line-by-line comparison reports everything behind the first inserted instruction, which hides
  that three of four `case` bodies already agree.
- `[vc8]` Under an EH frame, block locals of sibling scopes and the `$T` slots of by-value class
  arguments share frame slots, but not every non-interfering pair does: of two 8-byte structs in
  sibling `case` blocks one shared a 12-byte struct's slot and the other took its own, whatever
  the nesting, the order or the place of the function statics; without the EH frame all three
  shared. Function-level declarations share nothing. Measure the slot map of a scratch build
  (the `name$N = -off` lines of the listing) before rewriting for a frame size. *(open: the
  image shares all three)*.
- `[vc8]` `test ecx,ecx; je` in front of an inline `rep stos` fill is an unsigned counter
  (`for (unsigned n = w * h; n; n--)`); the signed `n > 0` gives `jle`. *(match)*
- `[vc8]` **Rank the functional ports by differing instructions after alignment, not by
  differing bytes.** A port with one wrong idiom near its top differs in a hundred bytes and
  in three instructions. Ports that were never opened (their comment says only "functional
  port") fall fastest: nineteen of twenty-three opened in one round, each on one of the forms
  below. *(match)*
- `[vc8]` Two stack slots swapped between two out-arguments, with every later read at the
  slot the image uses, is not a slot question: the port passes the two addresses in the other
  order. Read which slot is tested as what before trying declaration orders. *(match, and a
  behaviour bug)*
- `[vc8]` A by-value class argument with a user copy constructor is built in place
  (`sub esp,N; mov eax,esp; fstp [eax]...`). In a function with an EH frame the caller also
  stores `esp` into a `$T` slot, unless the call is made by a callee that was inlined (the
  small member that forwards the argument): write the forwarding call, not its body.
  The same three floats passed as three `float` parameters are pushed without `mov eax,esp`,
  and stored by the callee as floats where the struct assignment moves ints. *(match)*
- `[vc8]` Stores into three members in front of the derived vtable store are the base
  constructor's (inlined, its own vtable store dead): call it in the initialiser list.
  *(match)*
- `[vc8]` `movzx eax,WORD [arg]; sub eax,2; jne` is `switch ((unsigned short)arg) { case 2: }`;
  the `if` gives `cmp WORD [arg],2`. `movzx eax,ax; push eax` is the `(unsigned short)` cast
  (`MAKEINTRESOURCE`); `& 0xffff` gives `and eax,65535`. *(match)*
- `[vc8]` `js; cmp eax,N; jae` is `k >= 0 && (unsigned)k < N` (an index checked against an
  unsigned size); `k <= N-1` gives one unsigned compare. *(match)*
- `[vc8]` `je; push A; jmp; push B; push ...; call` (the pushes inside the branches) is the
  call written in both arms of an `if`/`else`; a conditional expression as the argument gives
  `mov eax,A; jne; mov eax,B; push eax`. *(match)*
- `[vc8]` A parameter loaded into a callee-saved register in front of the first call although
  it is first used in a loop behind it: a local copy declared at the top (`int py = y;`).
  Compound assignment to the parameters themselves (`x += rect[0]`) gives
  `mov ecx,[x]; add ecx,[member]`; a new local gives `mov ecx,[member]; add ecx,[x]`. *(match)*
- `[vc8]` A value kept in a callee-saved register across a call and added to its member only
  after a second value is computed: the two values are the elements of one two-int local
  (`int d[2]`), added afterwards; `add [m+4],eax` in front of the next statement's first load
  is the add written through a pointer to the member pair (`int *q = &caja.x; q[0] += d[0];
  q[1] += d[1];`). *(match)*
- `[vc8]` `mov ecx,[p+0x194]; add p,0x194; test ecx,ecx` (the address of a member made after
  its first field is read) is an inlined member function of the sub-object: a pointer to the
  sub-object as a local (`Nodo *s = &a->nodo; if (s->otro) s->otro->f = 1; s->f = 1;`), the
  sub-object declared as a struct member in place of loose fields. Scan the image for the
  `add reg,OFF` to find the other functions with the same inline. *(match)*
- `[vc8]` Three dwords copied as ints and the fourth as a float, with the destination pointer
  reloaded for it: a three-float struct assignment and one float member (`{V3 v; float w;}`),
  not a four-float struct. A zero vector moved through a stack temporary as ints is the
  assignment of a constructor temporary (`e->v = V3(0, 0, 0)`). *(match)*
- `[vc8]` `fld [member]; fstp [t]; fld [t]` on a plain float copy that the compiler would
  normally forward: the local's address is taken (`float h = rumbo; float *ph = &h;` and the
  value read through `*ph`); a float-returning inline, a float parameter of an inline and an
  out-parameter inline are all forwarded. *(match)*
- `[vc8]` `fld x; fadd st(0),st(0)` is `x * 2.0f`; `x + x` loads twice or duplicates with
  `fld st(0)`. *(match)*
- `[vc8]` `mov ecx,eax; test ecx,ecx; je; ...div...; mov ecx,edx; L: mov [m],ecx` is
  `if (d) d /= 50; m = d;`; the conditional expression materialises the zero. *(match)*
- `[vc8]` A commutative float sum whose products come out in an order no spelling in the caller
  produces: the sum is the body of an inline member (`float Dot(const V &o) const { return y * o.y
  + x * o.x + z * o.z; }`); inside the inline the written order survives. Two sums over the same
  loaded operands: the first sum's written order fixes the load order, the second keeps its own.
  *(match)*
- `[vc8]` A register one off for a pushed address or a call's `this` (`lea eax` against `lea
  edx`): give the intermediate pointer of the chain its own local (`M *m = g->menus; l =
  m->lista;`), or the pointer argument a local in front of the call. *(match)*
- `[vc8]` `add eax,-1; mov [m],eax` with a later `cmp [m],reg` re-reading memory, where the port
  keeps a register copy: the counter is used through a pointer local (`int *n = &field; if (*n >
  0) { --*n; if (*n == 0)`). *(match)*
- `[vc8]` One argument pushed before the call that computes the next argument: only that
  argument is a local, the others are expressions inside the call. *(match)*
- `[vc8]` A callee followed by `[ecx+k]` reads and a second call with no `mov ecx,...`: the
  linker knew the callee keeps `ecx`; not reachable from per-file source, skip it. *(triage)*
- Ranking tools that drop "addresses already in the docs" by substring also drop an address
  quoted as a range marker ("the pool from 0x41e7d0 on"); match on entry headings only. *(tool)*
- `[vc8]` A resource-setup sequence the image repeats with `[this+k]` re-read after every call
  was written out with the members named each time (or a macro); a helper taking the member
  by reference keeps it in a register. Match one instance, then grep the idiom's tail
  (`x->w = f->w; x->h = f->h`) across the other ports: one recipe gave six matches. *(match)*
- `[vc8]` Two fields copied from another object, both loaded before either store: either a
  pair temporary assigned whole (`*(Par *)&t->w = Par(f->w, f->h)`, first argument loaded
  first) or two locals declared in reverse (`int h = f->h, w = f->w; t->w = w; t->h = h;`):
  the load order says which. *(match)*
- `[vc8]` Member destructor call, state store, base destructor call and `operator delete` in a
  function whose port calls one destructor: the class's implicit destructor is inline; declare
  the base and the member with their own destructors and `__forceinline ~T() {}`. *(match)*
- `[vc8]` Default constructor and an init call inside one `new` null test, the init's arguments
  loaded before the constructor call: a derived helper class whose inline constructor calls the
  init, `new Derived(a, b, c)`. *(match)*
- `[vc8]` `mov ecx,this` in front of a call you declared `__stdcall`: the callee is a member
  that ignores `this`. Callee-clean both ways, so the callee already matches; only a caller
  shows it. Rename the symbol, change both declarations. *(triage)*
- `[vc8]` A store the image reads back from memory (`fstp [esi]; fadd [esi]`) while yours
  forwards the value through a temporary: write the store and the read through two different
  struct types over the same address; same-type or pointer-local accesses forward. *(match)*
- `[vc8]` The `new` temporary shares a slot with a local array only when the array lives in a
  nested block; a frame 4 bytes larger than the image's is the hint. Sum the frame
  (locals + cookie + temporaries) to get array sizes instead of trusting the port. *(match)*
- `[vc8]` `fldz` hoisted in front of a branch and reused by a later compare: both compares have
  the literal on the right (`d > 0.0f`); literal-left forms compare against a QWORD constant.
  `fabs` needs the intrinsic (`#pragma intrinsic(fabs)`), `fabsf` is a call. *(match)*
- The match harness must print compiler errors: a wrapper that greps for the symbol's line
  hides them and you compare the previous object. *(tool)*
- `[vc8]` Float code full of stack slots the port does not have (`fstp [t]; mov r,[t]` runs,
  a 12-byte frame) is the game's vector class, not scalar code: rewrite the port with one
  small class (`V3(x,y,z)`, a copy constructor, `operator+`, `operator+=`, `operator-`) and
  the plausible original statement (`V3 v(cos*r, sin*r, 0.0f); v += pos; v.z += alto;`,
  `f->p = a + b;`). Ports hundreds of bytes off matched on the first or second build. Which
  operand is the receiver decides the load order of `operator+` (`actor + offset` against
  `offset + actor`); spelling the sum inside the operator the other way changes nothing. *(match)*
- `[vc8]` One zero temporary copied to several places (`fst [t]; fst [t+4]; fst [t+8]` once,
  then four groups of three `mov`s) is a chained struct assignment
  (`c->a = c->b = c->c = c->d = V3(0.0f, 0.0f, 0.0f);`), and the scalar stores between them a
  chained float assignment (`c->fa = c->fb = c->fd = 0.0f;`). A loop over the floats gives
  `rep stos` or plain float stores. *(match)*
- `[vc8]` A missing `mov [state],-1` (after a function static, or after `new T[n]`) with
  everything else equal: the calls behind it are known not to throw. Declare the callee
  `throw()` (the member called last in a message handler; `void __cdecl operator delete(void *)
  throw();` at the top of the file when the only call left is the delete). Per file: the same
  declaration breaks functions of other files that keep the store. *(match)*
- `[vc8]` `call Token; mov ecx,eax; call Numero` (the returned object used through `eax`)
  is the unnamed temporary (`x = v.Token(0).Numero();`); a named local gives `lea ecx,[t]`.
  A discarded first call of the same expression in the image is a statement of its own. *(match)*
- `[vc8]` A store order across an inlined setter that no statement order gives
  (`mov eax,[dur]; mov [estado],6; mov [cuenta],eax`): the setter is one `__forceinline`
  member and its sub-object notice (`if (otro) otro->avisar = 1; avisar = 1;`) another
  `__forceinline` member inside it; VC8 leaves small members outline on its own. *(match)*
- A pushed constructor address (vector constructor iterator) is compared at the thunk unless
  the inline constructor has its own symbol line at the real address: "0 bytes differ" with a
  DIFF means a note on a referenced COMDAT, print the notes. *(tool)*
- Before any float idiom hunt, grep the tree for the same statement shape in other unmatched
  ports: one zeroing idiom matched four functions in three files. *(method)*
- `[vc8]` A tail jump to the last call (`add esp,N; jmp f`) although the address of a local
  was passed to an earlier call: the local lives in a nested block that closes in front of the
  last call (`{ V4 s; Punto(&s); *d += s.v; } Asentar();`). At function scope VC8 emits
  call and return. *(match)*
- `[vc8]` Inside an inlined `operator+=` one component whose load order no member spelling
  flips (`fld [s+8]; fadd [m+8]` where the others load the member first): write that component
  through a pointer to the first member (`float *a = &x; a[2] = o.z + a[2];`). *(match)*
- `[vc8]` `fld t; fcomp ds:K` in the image against `fld K; fcompp` in yours: the constant is a
  literal in the original (`< 0.001f`), not an `extern const float`. *(match)*
- `[vc8]` `a < b ? a : b` / `a > b ? a : b` on ints compile to one canonical sequence
  whatever the spelling in the function (ternary, `if/else`, a swap, any statement order).
  When two callee-saved registers are swapped against the image around such a pair, write it
  through forced-inline functions (`static __forceinline int Min(int a, int b)`, `Max`): the
  inliner's copies change the allocation order. *(match)*
- `[vc8]` A class temporary passed by reference whose address is reloaded behind its
  constructor (`call ctor; lea eax,[t]; push eax`) and whose slot the next temporary reuses is
  a named local in its own block (`{ Block b(text); r.Join(b); }`); the unnamed temporary
  pushes the constructor's `eax` and keeps a slot to itself. In an `if/else` whose arms meet at
  one call, the arm with `push eax` passes a callee's result and the arm with `lea` a named
  local. *(match)*
- `[vc8]` A function result rounded twice (`call fmod; fstp [t]; fld [t]; fstp [t]`) with a
  QWORD constant operand: the `float` overload of the `<math.h>` function (`fmod(a, 360.0f)`,
  `sqrt(f)`), an inline `(float)f((double)x)` whose result the assignment rounds again. Write
  `fmodf`/`sqrtf` if another compiler builds the same source: clang calls the overload
  ambiguous under the VC8 headers, and the bytes are the same. *(match)*
- `[vc8]` Three floats copied into slots and reloaded for a constructor, x and y adjacent and
  z elsewhere: a two-float local and a float, addresses taken (`float xy[2], z; ...;
  float *px = xy, *pz = &z; V3 p(px[0], px[1], *pz);`). Three address-taken floats give the
  same instructions with the slots in another order; no declaration order moves them. *(match)*
- `[vc8]` `mov [m],0` inside the null test of an inlined `delete`: `if (m) { delete m; m = 0; }`.
  Unconditional `delete m; m = 0;` stores on both paths. Do not sweep it: both forms occur.
  *(match)*
- `[vc8]` A commutative float sum whose operand order follows another block (two `switch`
  cases sharing sub-expressions): removing the shared sub-expression from the other case flips
  the order, so the order comes from value numbers, not from the spelling. No fix known; stop
  permuting spellings. *(negative)*
- `[vc8]` Three component sums of a vector add with the operands of one of them exchanged
  (`fld [d]; fadd [s]`, then `fld [s+4]; fadd [d+4]`): the vector's `operator+=` is a loop
  (`float *a = &x; const float *b = &o.x; for (int i = 0; i < 3; i++) a[i] += b[i];`), which
  VC8 unrolls without one shared operand order. Member-wise bodies tuned per line (`x = x +
  o.x; y = o.y + y;`) are the sign that the loop was meant; one loop then serves every caller.
  `__forceinline` where a caller with other calls leaves it out of line. *(match)*
- `[vc8]` The order of a commutative x87 operation on two memory operands follows the
  operands' other occurrences in the function, counted lexically (a call or a store in
  between changes nothing): an operand read anywhere else goes second; with one use each the
  higher offset goes first, and at equal offsets a pointer loaded from a member before a
  pointer parameter, parameters in declaration order, a parameter before a global, `this`
  last. Occurrences are compared before addresses are folded, so `p[1]` with `p = &s->e` is
  not `s->f`. A product that loads the "wrong" operand behind a copy of that operand
  (`f = s->f; g = s->f * k;` gives `fld k; fmul [s->f]`) wants the copy spelled through a
  pointer to the neighbouring member (`const float *p = &s->e; e = p[0]; f = p[1];`). Change
  the other occurrence, not the product. *(match)*
- `[vc8]` A zeroed local vector stored highest word first: `v[2] = 0; v[1] = 0; v[0] = 0;` or
  the chain `v[0] = v[1] = v[2] = 0`; an aggregate initialiser stores upwards. The two
  spellings store alike and change the operand order of later sums over `v`. *(match)*
- A variant runner must print the port's own DIFF line or fail: one that cuts the build
  output (`tail -400` over a large file) reports every variant as identical. After a sweep
  that restores the source, delete the objects it built; `make` will not. *(tool)*
- Compare an own copy's base with HEAD in the same command that commits it. Another agent's
  commit between a check and the commit is taken back silently when the own file is copied
  over the tree. *(process)*
- A native-build generator that reads a callee's address from the comment on a declaration
  line may bind it to an overload on the next line (a default constructor's address taken for
  the inline three-argument constructor): leave the comment off when the name resolves through
  the symbol table, and read the generator's duplicate report after every batch. *(tool)*
- `[vc8]` One record in two frame slots, one per arm of an `if`: the record is declared and
  filled inside each arm. A single record in front of the `if` gives one slot. *(match)*
- `[vc8]` A small struct stored in the middle of a call's argument pushes is a constructed
  temporary bound to a `const &` parameter (`F(a, Color(255, 255, 0), 0)`); a by-value point
  copied `fld/fstp` into the argument area has a user copy constructor. The callee's mangled
  name changes with the reference: check its other callers first. *(match)*
- `[vc8]` Two arms laid out in the other order whatever the spelling of the condition
  (negation, `continue`, swapped `else`): the condition is an inline predicate that returns
  the negated form (`int Expired() { return !alive || age + 5 >= limit; }`). *(match)*
- `[vc8]` `cdq; mov ecx,K; idiv ecx` for `/ K` and `% K` of one value: the dividend is a
  member assigned just before (`a = x >> 13 & 63; m = a % 20 + 1; a = a / 20;`); a local
  dividend gives the magic multiply. *(held)*
- `[vc8]` A member constructed with literal arguments whose `this` is computed in front of
  the argument pushes, with an unwind funclet that calls a one-`jmp` thunk: the member's
  class is a wrapper derived from the constructed class with an inline forwarding
  constructor. Its implicit destructor is a COMDAT the image keeps out of line; give it its
  own symbol-table line or the comparison reports a difference of zero bytes. *(match)*
- `[vc8]` Constructors: N consecutive `mov [m+4k],reg` from the function's zero register are
  N statements; a counted loop over the same members takes a fresh `xor eax,eax`. A zero
  vector member re-assigned in the body through stack slots is `v = Vec(0.0f, 0.0f, 0.0f);`.
  *(match)*
- `[vc8]` `mul edx; seto cl; neg ecx; or ecx,eax; push ecx; call` is `new T[n]` with a
  variable count (the overflow test), not `malloc(n * sizeof(T))`. *(match)*
- `[vc8]` A record built in a frame slot in member order and copied whole (`rep movs`) is a
  local constructed with an initialiser list and assigned (`*p = Rec(a, b, c);`); members the
  constructor leaves unset are copied as stack residue, so the port must clear them itself.
  *(match)*
- A callee that takes `ecx` plus stack arguments the caller cleans is a whole-program
  (`/GL`) static with a private convention: its callers cannot match from plain sources.
  *(negative)*
- Before renaming a mangled name, grep every source and the native build's reports for the
  old name with a tool that does not expand globs (a zsh `--include=*.cpp` pattern silently
  matched nothing): one missed user aborts the native build's first replay. Inline members
  added to a one-line struct take over the address comment at the end of that line: put them
  on their own lines. *(tool)*
- `[vc8]` Under `/GS` a pointer parameter is copied into a register (or a slot below the
  buffers) at the top of any function that has a cookie; an `int` is read in place. An
  argument that the image loads at entry and the port loads at its use is a pointer the port
  declared `int`. *(match)*
- `[vc8]` A dynamic initialiser that ends in `push fn; call atexit; pop ecx` matches as a
  plain function that registers a one-line stub (`void F() { g.~T(); }`); call the
  constructor by name, placement `new` on a class with a destructor adds an EH frame. The
  stub and the destructor each need their own symbol line. *(match)*
- `[vc8]` An unwind funclet that destroys "a member" starting in front of an array, while
  the body inlines the array's destructor loop, means the array and the word before it are
  one wrapper member with an implicit destructor. *(match)*
- `[vc8]` A walk that tests `(p - first) < count` with `first` and the list in frame slots
  is an iterator object with an inline test; written in the loop the test is
  strength-reduced to a counter. *(match)*
- `[vc8]` A hand-written "dtor" function with an EH frame, descending states and member
  destructor calls at its end is the real destructor: `delete`/`delete[]` statements in the
  body, the members as classes with declared destructors. *(match)*
- `[vc8]` A constructor called between a call's argument pushes whose result is pushed is
  `&T(...)` (the address of a temporary), not a named local. *(match)*
- `[vc8]` `while (n--)` and `for (i = n; i; i--)` over a pointer walk allocate the counter
  and the pointer to opposite registers and decrement on opposite sides of the call.
  *(match)*
- A source file the native build leaves out (skeleton or parked files) must not hold a
  function another native object calls: the program builds and aborts when the call is
  reached, here only at exit. Check which files the native Makefile filters before placing
  a new function. *(tool)*
- Never start a second build-and-replay run while the first is still replaying: they share
  the binary and the output directories, and both results are void. *(tool)*
- `[vc8]` The `/GS` pointer-parameter copy is sweepable: list every unmatched function whose
  image stores the cookie in a frame slot and copies a stack argument into a callee-saved
  register in front of the first call, compute the argument's index from the frame size and
  the pushes so far, and compare with the declared type. Six matches from one such list
  (Commandos round 87); "the image loads the id early" comments in older ports were this.
  Three arguments all copied at the top only mean the function uses them often.
- `[vc8]` A destructor whose vtable store comes in front of the EH state store is a real
  virtual destructor; write it in a class that carries the RTTI name and the base chain,
  because the comparison follows the vtable's relocations into `??_R0` (the name) and
  `??_R3` (the base count). The compiler then emits the scalar deleting destructor from the
  same file: drop the hand-written one. A member destructor that is called through a pushed
  address (vector destructor iterator) or an unwind funclet needs a symbol line of its own
  for any build that links by name, even where the byte comparison tolerates the unknown.
- `[vc8]` A vector deleting destructor (`??_E`) is emitted wherever `delete[]` meets a class
  with a destructor, and its layout depends on the unit's declarations; if the class name
  is one the symbol table knows, the copy from the new unit is compared and fails. Use a
  unit-local element name with an alias line.
- `[vc8]` An inline constructor shared through a header can exist in two source spellings in
  one binary (Commandos' list: the step stored third in one class, last in another, and the
  array from `operator new[]` in some units and `operator new` in others). Read the store
  order per call site and name the class by the destructor in the unwind funclet.
- `[vc8]` VC8 inlines a small member into every caller in the same unit regardless of
  definition order. A caller that calls it while a sibling inlines it lives in another
  unit: split the file rather than mark the member `noinline`.
- `[vc8]` A function whose first stack argument is the receiver of a constructor call and
  comes back in `eax`, with a zero store to a frame slot in front, returns a class by
  value; ports written from a decompiler listing show it as an extra pointer parameter.
- The native generator binds an address comment at the end of a struct line to an inline
  constructor on that line: keep `// 0xADDR` off lines that define inline members.
- `[vc8]` A destructor that starts in a lower EH state than its members give (two members
  with destructors, one unwind funclet, `mov [state],0` at the top), or calls a member's
  destructor with no state store in front: nothing can throw while the dropped state is
  current. With `delete a; delete b;` as the body, `operator delete` and `operator delete[]`
  declared `throw()` at the top of the file remove the last member's state. Sweeping the two
  declarations over every file is cheap and shows where they hurt. *(match)*
- `[vc8]` A function static whose guard takes EH state 1 or 2 with no other static in the
  function: a callee that has a static of its own is inlined in front of it and the constant
  argument folded the static's branch away (`modo = 2; espera = 20;` was `PonerModo(2)`). The
  bytes are equal except the state numbers. Comparing the image's `__ehfuncinfo` state count
  with ours lists every function of this shape, and the ones that need real members,
  constructors or `new` expressions. *(match)*
- `[vc8]` `mov cl,[eax]; mov [edx+eax],cl; add eax,1; test cl,cl; jne` with `lea edx,[dst];
  sub edx,eax` in front is the `strcpy` intrinsic (`#pragma intrinsic(strcpy)`), not a written
  loop; no loop spelling gives the delta-indexed store. *(match)*
- `[vc8]` A flat "destroy" port of a class with a vtable converts in one pass when each owned
  part is written as `delete`: a null test, a destructor call and a free is `delete p` with a
  declared destructor; `call [vt+4](1)` is `delete p` through a virtual destructor in that
  slot; the vector destructor iterator with a constant count is `delete p` on a struct with an
  array member; written-out `delete`s of a class with a base and a member take one state each.
  A member destructor called with no state store is declared `throw()`. *(match)*
- `[vc8]` `operator new`, a null test and an init-function call written out by a port is a
  `new` expression with a declared constructor; the EH state around the constructor comes
  with it. In its arguments, a small struct built in place is `&T(a, b, c)` with an inline
  constructor, and an int read as a float through an inline getter gives `fild; fstp tmp;
  fld tmp`. *(match)*
- `[vc8]` The receiver of a member call loaded in front of the argument pushes (`mov ecx,[g];
  mov ecx,[ecx+N]; add eax,8; push eax`) is an inline getter for the receiver together with
  the argument held in a pointer local declared first; either alone gives another order.
  *(match)*
- `[vc8]` Several function statics that each carry an EH state in the image all call a
  throwing initialiser; a port that declared a `throw()` twin of the callee to drop states
  also loses the immediate-operand guard tests. *(match)*
- `[vc8]` **The EH state count as a file-boundary detector.** When the image's function has
  N more unwind states than the port and N is a multiple of the number of function statics
  in the class's state setter (`PonerModo`, `Cambiar`, `Estado`...), each multiple is one
  inlined call of that setter with a constant argument: replace the port's hand-written
  stores (`estado = 5; avisos cleared; ...`) by the call. The setter is inlined only when
  its body is in the same translation unit, so a port kept in another file (or one that
  calls a file-local reduced copy of the setter) has to move next to the real setter; the
  image's guard variable address tells which function the statics belong to (search the
  disassembly for the other users of the guard). A reduced copy is also a behaviour risk:
  it drops the arms the first caller did not need. Two inlined copies with the same
  constant are tail-merged (one arm pushes the arguments and jumps into the other's
  stores): write both calls in an `if / else if` chain, not with early returns. *(match)*
- `[vc8]` A float store followed by a re-read of the same member with no value forwarding
  (`fstp [m]; ...; fld [p]; fadd [m]`), and the object pointer loaded after the store:
  the store and the read go through different pointer expressions (`float *q = v; q[0] =
  cos(a) * 100.0;` then `((Vec *)v)->x += p[0];`). One expression for both lets the
  compiler keep the value in a rounding temporary; the member written directly lets it
  hoist the unrelated pointer load above the store. *(match)*
- `[vc8]` A function static array of a class with a constructor and a destructor
  (`static T tabla[10] = { T(...), ... };`) takes one EH state per element, registers a
  compiler-made `??__F<name>@...@YAXXZ` stub with `atexit` (the stub pushes the destructor,
  the count and the size for the vector destructor iterator) and needs no hand-written
  guard; a second static array with an inline constructor in front of it takes the guard's
  first bit and no state. *(match)*
- `[vc8]` Sweep by library callee: count the calls to the vector constructor iterator
  (`??_L`) and the vector destructor iterator (`??_M`) per function in the image and in
  the port. A port that passes the iterator's arguments by hand, with a hand-made size and
  count word, is an array `new` expression: `p = new T[n];` gives the `mul`/`seto`/`neg`/`or`
  size, the `add 4`/`setb` cookie, the count store, the null test and the EH state. A class
  with a constructor and no destructor gives the four-argument `??_H` call and no count
  word. `delete[] p` on a class with a virtual destructor gives `if (p) { if (p[-1])
  p->vtbl[0](3); else operator delete[](p - 1); }`. *(match, two functions)*
- `[vc8]` `push dtor; push count; push size; push p; call ??_M; push p-4; call delete`
  inside the class's own destructor is `delete[] p;` with the vector deleting destructor
  (`??_E`) inlined. VC8 inlines it there only when a `delete[]` of the class was parsed
  before the destructor's definition; when the destructor's own `delete[]` is the first
  use, the unit calls `??_E` out of line. An uncalled inline member in the class body
  (`void Borrar() { delete[] p; }`) is enough and emits only the `??_E` COMDAT. *(match)*
- `[vc8]` `mov eax,[o+k]; test eax,eax; mov [this+k],eax; je` is an assignment inside the
  condition (`if ((tabla = o.tabla) != 0 && o.flag)`); as a separate statement the source
  is read again after the store. `movzx eax, WORD PTR [p+k]; cmp ax, 8` where the port
  has `cmp WORD PTR [p+k], 8` is an inline getter returning `unsigned short`. *(match)*
- `[vc8]` Two inlined `memcmp`s, each ending in its own sign tail (`test esi,esi; mov
  eax,1; jg; or eax,-1`) and then the epilogue, are stored to one result variable that
  starts with the fall-back value and is returned as it is. A further sign conversion in
  the source makes the compiler merge the two compares into one loop with a variable
  size. *(6 bytes from a match)*
- `[vc8]` **A dynamic initialiser of a table is matched by defining the table**: `T g[N] =
  { T(...), T(...) };` at file scope compiles to `??__Eg@@YAXXZ` (give it the address of
  the image's initialiser in the symbol map) and, when `T` has a destructor, to the
  `atexit` call at its end with the stub `??__Fg@@YAXXZ` (`push dtor; push N; push size;
  push g; call ??_M; ret`). The EH state stored after each element's constructor and the
  funclets `mov ecx,elem; jmp dtor` are the array's partial destruction; a plain function
  with placement `new` cannot produce them. A native (non-MSVC) build of the same source
  emits its own `.init_array` entry for such a global: if the port's runtime already walks
  the image's initialiser table, drop that section from the object and export the `??__E`
  symbol instead, or the table is constructed twice and in another order. *(match, nine
  initialisers, the largest 11859 bytes)*
- `[vc8]` A table whose elements have a destructor (the `??__F` stub is registered) while
  the initialiser has no EH frame: the element's constructor is declared `throw()`.
  *(match)*
- `[vc8]` `sub esp,8; mov eax,esp; mov [tmp],esp; ...; mov [eax+4],v; mov [eax],v` inside
  an argument list is a by-value argument of a class that has a user copy constructor,
  built in place; without the copy constructor the class goes as plain pushes. The order
  of the member stores tells which constructor made each argument (a default constructor
  that zeroes `b` then `a` against a two-argument one that stores `a` then `b`). *(match)*
- `[vc8]` Aligned byte loops `mov cl,[esi+src]; mov [esi+dst],cl; add esi,eax; cmp cl,bl;
  jne` with 1 and 0 held in registers are `strcpy` inside an inline constructor that is
  called with literals; `strcpy` on a literal written directly is a dword copy and a
  hand-written loop loses the alignment. A call that sits between the member stores of an
  inlined constructor is made in the constructor body (`m = name ? Lookup(name) : dflt;`),
  not in the argument list: an argument is evaluated before the body, keeps the function's
  constants live across the call and moves them to callee-saved registers everywhere.
  *(match, 7502 bytes)*
- `[vc8]` An initialiser that is only `push stub; call atexit; pop ecx; ret` is a global
  whose constructor folded into static data and whose destructor did not; the stub is the
  inlined destructor (`mov [g],base_vtable; ret` for an empty virtual destructor of a
  derived class). *(match, five pairs)*
- `[vc8]` An unwind funclet `mov ecx,[this]; jmp dtor` with no `add ecx,k` destroys a base
  class. If the port models those fields as a member, the body is byte-identical and only
  the funclet differs (the comparison reports zero differing bytes with relocation
  mismatches): make the class derive from the type whose destructor the funclet calls.
  *(match)*
- `[vc8]` A member kept in a register across a call (`mov edi,[esi+0x24]` at the top,
  `cmp edi,5` after the call) means the callee is defined in the same translation unit and
  stores nothing: VC8 uses the effect summary of functions it compiled in the same file.
  With the callee in another file the member is read again. Move the callee into the
  caller's file. *(match, 768 bytes)*
- `[vc8]` Whether a callee is inlined at one site depends on what is inlined into the
  callee first: a state setter was inlined only at the constant that folds its switch to
  one small case, and only once its own callee lived in the same file and was inlined into
  it. `inline` changes nothing, `__forceinline` inlines every site. A port file named
  after one address is a split of the original file; sweep for game callees the image
  inlines and ours calls, and for wrong callees at the same time (image-only against
  ours-only callee sets: one real logic bug found that way). *(match)*
- `[vc8]` Virtual calls on a stream that push their arguments before reading the vtable,
  next to calls that read it first, come from an inline switched by a constant mode
  (`inline void Ser(int modo, void *p, IStream *s) { if (modo) s->Write(p, n); else
  s->Read(p, n); }`); a plain inline, a member of the element or a cast all read the vtable
  first. *(match, two functions)*
- `[vc8]` An argument read before the null test of the receiver (`cmp [obj+k],0; mov
  edx,[this+0x20]; je`) is an inline forwarder `void F(a, b, c, d) { if (m) m->f(a, b, c,
  d); }`: arguments are evaluated in front of the inlined body. *(match)*
- `[vc8]` A small function the image calls at every site, from callers that come after it
  in the same file, and whose `this` is not reloaded between two such calls: the callee
  was compiled in front of the caller and never inlined. `__declspec(noinline)` on the
  definition (after checking that no matched function of the file inlines it) plus the
  branches as a `switch` in the image's block order matched a port that inlined the first
  site of each callee. A sweep of "callee of the port's own file that the image calls more
  often than ours" lists the candidates. *(match)*
- **Audit functional ports with set-difference sweeps, not by reading them.** For each
  port compare a set from the image's function with the same set from ours and list the
  port only when both sides have something the other lacks: direct callees (resolve
  linker thunks on BOTH sides, or every lookup under a thunk name is a false line),
  member offsets `[reg+N]`, compare immediates, string literals, float constants. One
  round found four logic bugs this way (a heading read through the vector-angle function
  instead of the stored-angle getter at seven sites, a member one word off twice, a table
  walked for 130 of its 134 records) and a symbol line at another function's address.
  Noise: `lea r,[x+N]` against `add r,N`, an advanced base register, inlining on one
  side. Also scan names that embed an address for lines listed at another one.
  *(four behaviour fixes)*
- **Compare immediates per instruction class, and read the pointer bumps first.** A flat
  "immediates only one side has" list is frame sizes and argument cleanup. Split by
  class (compare, push, store to memory, logic, multiply, `add`/`sub` on a register that is
  not the stack pointer) and the `add reg,K` lines are element sizes: `add edx,0x54` in
  the image against `add edx,0x5c` in ours was a guessed struct whose every used field was
  at the right offset in the first element and whose size was wrong, so a three-level
  search read each later element 8 bytes further on. With the size fixed the function was
  13 bytes from identical and matched. *(1 behaviour fix, 1 match)*
- Audit sweeps that listed only noise on a mature port set, so do not start with them: the
  constants pushed in front of each call per callee and argument position (decoded
  `??_C@` names are not the strings: read the image's bytes; a register push is a
  wildcard), virtual slots called (the slot address cached in a register), writable
  globals referenced, `rep movs`/`stos` counts and `imul` constants. Standing noise in
  every list: `cmp x,N-1; ja` against `cmp x,N; jae`, `(x & M) >> 1` against
  `(x >> 1) & (M >> 1)`, a list at +N entered through its member, `char b[N] = {0}` as a
  byte store plus `memset` of N-1. *(record)*
- `cmp i,N; sbb r,r; and r,i` in front of an indexed read is `a[(unsigned)i < N ? i : 0]`
  with a signed `i`; the signed test gives `xor; cmp; setge; sub 1; and`. `[vc8]`
  *(1 match)*
- **Swap adjacent initialised declarations as a standing sweep.** `T a = x; U b = y;` with
  independent right-hand sides is not order-neutral: the first declared takes the scratch
  register of the value loaded in front of it. One mechanical pass over every port within
  450 bytes (each such pair swapped alone, about 600 one-file compiles) matched one
  function that carried a "register choice" note and was the last step of another; 46
  ports came a few bytes closer. A sweep that writes and deletes its trial copy must not
  share the directory where confirmed edits are staged. `[vc8]` *(2 matches)*
- **Run the audit sweeps over every batch of fresh ports, and open the ones a few bytes away
  first.** A new port 3 bytes from identical differed only in member displacement bytes: the
  derived class's padding member started where the base class ends in the headers, four bytes
  before its comment said, so the constructor stored a window handle one word before the place
  every other member function reads it. Bytes that differ only in displacements are a layout
  error; fixing it is the match. In-place decrement against a new value: `sub r,1; js` is
  `while (--n >= 0)` on the variable itself, `add r,-1` is `m = n - 1`. `[vc8]`
  *(1 behaviour fix, 3 matches)*
- **A fresh port whose only differences are frame offsets has a local buffer of the wrong
  size.** Read the buffers off the image's `lea r,[esp+N]` (less the pushes in flight) and the
  cookie slot; one stack slot used for two purposes is one buffer reused (a file name copied
  back into the pattern buffer), and a frame four bytes larger with two float stores is a
  three-float local of which two are set. A port buffer smaller than the image's is a
  behaviour bug (a trace line formatted into 0x100 bytes where the image has 0x400). Nine
  of a day's new ports matched on buffer sizes alone. `[vc8]` *(9 matches, 1 behaviour fix)*
- `add r,-K` where `x - K` gives `sub r,K`: the value is first held in a named local and
  the subtraction goes to a second local (`int n = f(); int m = n - 1; if (m < s)`;
  `int w = p->ancho; int dx = w - 0x280; x = dx / 2;`). Folding either local away gives
  `sub`. A scan for ports whose image has `add r,-K` and whose build has `sub r,K` lists
  them. `[vc8]` *(2 matches)*
- An argument register exchanged with its neighbour's in one call (`mov eax,[p]; mov
  edx,[this+0x14]` for ours `mov edx,[p]; mov eax,[..]`) is the argument held in a named
  local before the call. A pointer walked from an array and tested through the array
  (`cmp byte [m],0; mov p,m; je`) is `char *p = m; if (m[0]) do { .. } while (*p);`.
  `[vc8]` *(2 matches)*
- **Parameters passed straight through in the wrong registers are pointers in the original.**
  A constructor or function that forwards its `int` parameters to a callee, and loads them
  into each other's registers (or into `ebx`/`ebp` in the other order), matched when the
  parameters that are handles or objects were declared as pointers and cast at the call
  (`void *a, void *b, int c` forwarded as `(int)a, (int)b, c`; a window parameter copied
  into `int h = (int)ventana`). The mangled name changes: keep the old name as an alias
  line at the same address when generated callers declare it. `[vc8]` *(2 matches)*
- Scopes show in destructor order: an object destroyed before the calls that follow it
  sits in its own block; a temporary passed by address (`f(&obj->Get("K"), out)`) has its
  own slot and EH state where a named local in a block reuses one slot. `[vc8]`
  *(2 matches)*
- `mov edi,OFFSET lit; lea esi,[buf]; mov ecx,len+1; xor eax,eax; repe cmpsb` is the
  intrinsic `strcmp(buf, "LIT") == 0` against a literal. A decompiler renders it as
  `memcmp(buf, "LIT", len + 1)` (or `strncmp`), which compiles to the unrolled dword
  loop: rewrite every such compare whose count is the literal's length plus one, and
  count `repe cmps` per function on both sides to find the ones spelled another way.
  `[vc8]` *(3 matches, ten ports hundreds of bytes closer in one sweep)*
- Adjacent small members stored in order while an unrelated constant (`or eax,-1`) or a
  pointer store sits after them instead of hoisted in front: the members are set through
  an inline setter taking the values by value (`color.Poner(0, 0, 0xff)`), not by three
  assignments. `[vc8]` *(3 constructors)*
- A by-value object parameter in front of a scalar one shows at the call as the scalar
  pushed first and the object built in place after it; at entry the object sits below
  the scalar. A port with the two exchanged is self-consistent and still wrong for any
  caller left in the image. `[vc8]`
- Before opening a fresh port, compare the whole pool with the "not matched" markers
  stripped: ports that already compare identical only need their marker removed (14 in
  one round). A self-call that goes through the incremental-link thunk must be followed
  by the comparer like any other call.
- Per derived class, compare the image's vtable slot targets with the functions the
  rebuilt vtable holds (by symbol address). A class whose overrides are declared under
  other, non-virtual names inherits the base's slots in the native vtable: silent wrong
  behaviour (33 dialog classes, three slots each). Also scan the symbol table for one
  name at two addresses: tools that take the first and tools that take the last line
  disagree silently (a base's empty virtual aliased to another class's method).
- `[audit]` A library address under two names hides a wrong call. One CRT entry was listed
  both as `strncpy` and as `memmove`; ports that called `memmove(d, s, n)` compared
  byte-identical (the relocation resolves to the same address) while the native build
  linked libc's `memmove` and copied `n` bytes past the terminator. Read the code at a
  library address once (a copy loop that tests each byte for zero is `strncpy`), keep one
  name per address, and grep the sources for the wrong one.
- `[audit]` Compare floating constants by value and width. A `QWORD` constant whose low
  29 bits are zero is a `float` literal promoted in a double expression (`x < 0.0001f`);
  the port's `0.0001` gives the same code bytes and a different constant, so the function
  shows "0 bytes differ" with a data line, or hides among other differences. A sweep of
  the image's x87 memory operands against the build's `__real@` names found eleven such
  literals in one pass.
- `[vc8]` The order of `if`/`else` arms is the order of the EH state numbers, not of the
  code layout: an arm placed last with the highest states is the last arm of the source
  even when its test comes first in the code.
- `[vc8]` A save call that follows the destructor of the value just stored means the value
  sits in a block of its own (`{ T v(x); b->Set(k, v); } b->Save();`); the same holds for
  a sub-object fetched by value and saved before the outer save.
- `[vc8]` `add eax,-1` on a call's result (against `sub eax,1`): the result is a named
  local and the subtraction is written where it is used once
  (`int n = f(); T t((double)(n - 1));`).
- `[vc8]` Among sibling blocks that each hold one object of the same class, the last block
  never shares the first slot: it gets its own or joins the temporaries of inner scopes.
  A frame larger by exactly one object is not a missing buffer.
- `[audit]` Check library calls by address, on both sides. List every library address
  that carries two names and every C name that the native linker resolves by name
  (`strcpy`, `memmove`, ...), then read the body at the address: one more pair of this
  kind was `_sprintf`/`_strcpy` at one address (`strcpy(d, s)` with the intrinsic off was
  `sprintf(d, s)` in the image). Then, for every functional port, count the calls into the
  library range per target address in the image's function and in the build (the build's
  names mapped through the symbol table) together with the inline idioms (`rep movs`,
  `repe cmps`): a line `strncpy 1/0 ; rep movs 0/1` is a `memcpy(d, s, n)` that is
  `strncpy` in the image (sixteen in one pass), `strncmp 1/0 ; _strnicmp 0/1` a compare
  with the wrong case rule. Names the comparer only "learns" (no symbol line) are never
  checked: list them per address too.
- `[audit]` A port whose image constants are nowhere in the build, not even in its
  helpers, is a rewrite of the idea and not of the code (a polygon distance computed as
  true segment distances where the image has tolerances and a facing test). Extend the
  constant sweep to the callees the image does not call before reading a lead as "the
  constants live in a helper".
- `[vc8]` "0 bytes differ" with a note on `??_R0...` (the RTTI type descriptor reached
  through the constructor's vtable) is a class whose name is not the image's: read the
  name at the address. With a note on a vtable slot it is this file's declaration of the
  class missing an override (`virtual int v02(...)`).
- `[vc8]` Vtable stores first, then pairs of stores high word first, then a pair stored
  low word first with a constant, with the pushes of the body's first call between them:
  the pairs are members with an inline default constructor (`x = y = 0`) and one member
  initialised in the list (`tam(1, 1)`); body assignments let the call's pushes rise
  above the vtable stores.
- `[vc8]` A virtual call through the vtable on a local object (`mov edx,[obj]; mov
  eax,[edx+N]; call eax`) where the port's `obj.Method()` is a direct call: the original
  called through the interface pointer (`IStream *f = &obj; f->Read(p, n);`). A member's
  address pushed from a `lea` with the slot loaded after the pushes is a local for that
  address (`void *p = data; f->Read(p, n);`).
- `[vc8]` A `/GS` cookie in a 4-byte frame of a function that uses no buffer: an
  initialised, unused `char b[16] = "";`. The array is removed, the cookie stays.
- `[vc8]` `mov DWORD PTR [buf],imm32` holding a short literal, then one byte stored over
  it: `char s[4] = "a:\\"; s[0] = 'A';`. A brace list of characters gives byte stores.
- `[vc8]` `x * -c + y` written in one expression becomes `y - x * c` (`fsubr`); with the
  product in a `double` local (`double d = x * -c; y = (float)(d + y);`) the image's
  `fmul [-c]; fadd [y]` comes back.
- `[vc8]` The loop foot `add p,16; jne top` with the terminator test at the head is
  `for (; p; p += 4) { if (p[0] == -1) break; ... }`; a guard `if (!p) return;` and
  `p[0] != -1` as the loop condition put the terminator test at the foot.
- `[audit]` In a fresh port every member offset low (or high) by the same 4 bytes is the
  file's own declaration of the class missing a base word; a build-only function shows it
  in no run. Compare the first mismatching `[reg+N]` with the declaration's offset
  comments before reading anything else.
- `[tool]` Rank variants by an aligned instruction diff (difflib over normalised lines),
  not by differing bytes: one missing `push` at the top makes every variant "204 bytes"
  apart, and a pasted side-by-side shifts after the first missing line.
- `[vc8]` `cmp [this+N],0; je; mov eax,[this+N]` (a member tested in memory and then
  loaded, where a port gives `mov eax,[this+N]; test eax,eax`) has three sources. (1) The
  member is spelled at every use and never copied to a local: `if (img) { r.w = img->w;
  r.h = img->h; dc->Draw(img->id, ...); }`; VC8 gives the uses one temporary loaded at
  the first of them and leaves the test a memory compare, while `T *o = img; if (o)`
  merges. (2) An array slot whose index is read through the member it was just stored in
  (`cur = i; if (tab[cur]) tab[cur]->Show();`), usually because a neighbouring
  `Show(int)` is inlined: call the neighbour instead of repeating its body with the local
  index. (3) In an iterator loop the element is read through the cursor each time
  (`if (*it.p && (*it.p)->r) (*it.p)->r->f(...)`, what `operator*` or a macro leaves), not
  through an element local.
- `[tool]` Count such instruction pairs per function in the image and in the build and
  list the functions where the image has more: one script turns an idiom into a work list.
- `[vc8]` A row of `test r,r; je; mov x,[r+N]; jmp; xor x,x` blocks on one pointer is a
  set of inline accessors returning 0 for a null pointer, used as arguments of one call:
  the blocks come in the right-to-left order of the arguments and, inside each argument,
  left to right. A local for a second object read in both arms is hoisted over the branch
  and costs a callee-saved register: read it from the member in each arm.
- `[vc8]` A zero loaded once in front of a branch and kept on the x87 stack for a later
  store (`fldz; fst [m]`), where the image has `fldz` in the arm and again at the store:
  the arm's zero is a double inside the cast, `(float)(flag ? 0.0 : a * b)`.
- `[vc8]` With `double r = a * k;` used by two calls VC8 hoists an unrelated product over
  the statement that stores `a`; writing `a * k` in both calls (`cos(a * k)`,
  `sin(a * k)`) keeps the statement order and still computes the product once.
- `[vc8]` `sete cl; mov eax,ecx` at the end against `test eax,eax; je one; xor eax,eax`:
  `if (r != 0) return 0;` falling into the function's final `return 1`, not
  `if (r == 0) return 1; return 0;`.
- `[vc8]` A serialiser that writes many values is one text buffer and one named object per
  value: `sprintf(t, "%ld", n); { Block b(t); r.Put("NUM", b); }`. A port's helper
  returning the object from its own buffer has the wrong frame and calls. Read the buffer
  sizes off the frame last; two buffers can hide behind one frame size.
- `[vc8]` Scratch registers for the pushes of consecutive calls rotate `eax, ecx, edx`.
  A value that goes through a named local or an inline function's parameter reuses the
  register of the previous allocation instead of taking the next. When ours is one step
  out of phase from a call on, add (or remove) a local or an inline wrapper for that
  call's argument; an inline `Fmt(t, n)` around `sprintf` shifts the phase the same way.
- `[vc8]` `lea ecx,[obj+N]` in front of a call to a function matched as `__stdcall`: it is
  a member that ignores `this`. Give the callers a member prototype under an alias.
- `[vc8]` `mul; seto cl; neg ecx; or ecx,eax; call operator new[]` then
  `push ctor; push n; push size; push p; call ??_H`: `new T[n]` with a constructor. The
  "init" member the port looped over is the constructor.
- `[vc8]` Locals of a force-inlined helper are packed among themselves, not with the
  caller's statement temporaries: a frame one object larger than ours, with named values
  and small buffers in one slot and temporaries in another, means the readers were
  inline helpers. Plain `inline` helpers holding a destructible local may not be
  expanded; `__forceinline` reproduces it. A helper that stores through a reference
  parameter stores before the local's destructor, one that returns the value after it.
- `[vc8]` Inline `fnstcw/fldcw/fistp qword` next to `_ftol2_sse` calls: that one
  conversion is to `unsigned`.
- `[vc8]` `test eax,eax; je far; lea ecx,[x]; call` (ours: `lea` hoisted over the `je`):
  nest the rest in the `if` instead of returning early; a cold path placed after the
  body is the `else`.
- `[vc8]` `imul r,i,size; add r,[base]` after a strength-reduced loop: the counter is
  declared outside the loop and reused after it.
- `[vc8]` Two callee-saved registers exchanged over a whole function written as a
  `__fastcall f(self, int, ...)` stand-in: make it a real member (a file-local derived
  class and a renamed symbol do).
- `[vc8]` `cmp [this+N],0; je; mov ecx,[this+N]` in front of a virtual call needs a real
  member spelled twice (`if (m) x.Put("K", m->Save());`). Read through an offset template
  or an inline getter the test and the load merge, so a port built on `Field<T>(this, off)`
  cannot show the pair. `if (P *p = g->m) p->Step();` is the merged form.
- `[vc8]` `movsx r,[m+4]; mov cx,[m+2]; mov ax,[m]; ... movsx ecx,cx`: a struct of shorts
  copied through its copy constructor, then its members converted.
- `[vc8]` Listing equal line for line and bytes still different at a `call`: compare the
  call's encoding. `call [iat]` against a direct call was `wsprintfA` for `sprintf`.
- `[vc8]` Two object slots used A,B in one arm and B,A in the other, both pushed by `lea`
  after construction: the first object is a named local in an inner scope, the second a
  named local of the arm. Two inner scopes, temporaries or inline helpers share one slot.
- `[vc8]` `fld m; fstp tmp; fld tmp` with the frame 4 bytes larger: a float returned by
  value from an inline getter into a `const float &` parameter of an inline function.
- `[vc8]` Most arguments of one call loaded before the first push, one in a callee-saved
  register: the call sits in an inline function with by-value parameters.
- `[vc8]` `mov eax,[arg]` first, then `lea ecx,[local]; push ecx; push eax` (ours: the
  `lea` first): the first argument is `this` of an inline member wrapper around the free
  function. A local copy or a static inline wrapper does not give it.
- `[vc8]` `add esi,N` and all later offsets relative to it: a sub-object whose save or
  load is a force-inlined member called on a member. As a second base class the call
  is not inlined and the cast is null-adjusted. A loader that has a callee's load half
  written out while the saver calls it wants that half as a force-inlined member.
- A native regression binary built incrementally from `git archive` trees can keep stale
  objects (every file is stamped with the commit time, which may be older than objects
  built before the commit). Sync by checksum without times, or build clean, before
  claiming two frame logs agree; cross-run the binaries when a count looks off.
- `[vc8]` The scratch-register model (round 106B): a single-use expression temporary
  takes the next of `eax, ecx, edx` round-robin, and the pointer runs on across
  statements, branches and calls (a forced register, `this` in `ecx` or a result in
  `eax`, does not move it). A value that goes through an inline function's parameter or
  return, or is used twice, takes the lowest free register and leaves the pointer alone.
  Write the image's single-use loads as a cycle: where a register repeats or is skipped,
  that statement (or one before it) goes through an inline setter, getter or accessor
  (`o.Pon(v)`, `inline T *Avisos() { return g->avisos; }`, a null-safe getter called
  through a second inline member). One setter alone can move the phase the wrong way:
  fix the cycle from its first break and re-read.
- `[vc8]` An iterator loop whose body has the right loads in the other registers: the
  element is read through `operator*` returning it by value, not `*it.p` at each use.
- `[vc8]` A value kept in a callee-saved register exchanged with a parameter's: the
  expression is written at each use (the compiler's own common subexpression is
  numbered after the parameters), not kept in a local.
- `[vc8]` `[ptr+idx*1+N]` with the pointer as base where ours has the index first: the
  byte offset is a named local (`int o = i * 0x74; p[o + N]`).
- A family written from one macro (message handlers: a handled flag, one `if (!flag &&
  id == X) { flag = 1; ... }` per message, the base called at the end under the flag,
  one result variable) has no early `return`. A port with a `return` inside a handler
  shows `this`, the id, the flag and the result permuted among the callee-saved
  registers from the prologue on, or the base call's block twice. Match one member of
  the family, then list the others by shape (count of `return`, of handlers) and rewrite
  the deviants first. The declaration order of the flag and the result decides which
  zero register is made first.
- A DIFF byte count jumps by a hundred when one instruction changes length (a short
  jump's reach); rank variants by aligned mismatching lines.
- `[vc8]` The result variable behind a "zero kept in a register" (round 106C): `xor edi,edi`
  at the top, `cmp [m],edi` for every zero test, `push edi` for zero arguments, `mov eax,edi`
  at the common exit and `mov eax,1` in a copied epilogue mean one result variable assigned
  on a path and returned once (`int r = 0; switch (...) { case X: ...; r = 1; break; }
  return r;`). The compiler keeps the variable in a callee-saved register and reuses it
  wherever the body needs its initial value; `return 1;` inside and `return 0;` at the end
  fold to immediates. Works with 1 cleared on the failing paths and with a pointer result;
  an `if`/`else if` chain folded where the `switch` did not. Seven ports identical; grep
  the notes of parked functions for "zero" and "keeps". **held** *(match)*
- `[vc8]` A sweep of meaning-preserving source transforms pays when a trial costs a
  quarter of a second: each `if`/`else` turned round, `>= 1` against `> 0`, an early
  return nested, a float constant as a literal, scored by aligned mismatching
  instructions and kept when lower. Over 707 ports: 2 identical at once, 145 closer, and
  most of the hand-finished matches of the round started from its output. Arm order is
  the commonest single difference between a decompiler's port and the original.
- `[vc8]` Two parameters with equal use counts in exchanged callee-saved registers: the
  one loaded in the prologue into the first register is pointer-typed (a handle), the
  other an integer. Check each parameter's register before trusting an all-`int`
  prototype. **held** *(match)*
- `[vc8]` The then arm of `if (a != 0 || b != 0)` in line with the else arm behind it
  (ours: the then arm moved behind the function's end): the condition is an inline
  predicate of the tested object. **held** *(match)*
- `[vc8]` `cmp [p],0` kept on a path where a zero store through the same address is the
  only write before it: store and test do not both go through one local pointer (the
  member is spelled at one of them); with the same local at both the jump is threaded.
  **held** *(match)*
- `[vc8]` An inline function that matches out of line is not yet the source: check it
  where it is inlined. A list walk written `do { n = cur->next; ...; cur = n; ... } while
  (n->next)` under an `if` and one written `while (cur->next) { ... }` gave the same
  out-of-line bytes; inlined, the second copies the forwarded store into a scratch
  register (`mov edx,ecx; cmp [edx],0`) and shifts the scratch rotation behind it, as the
  image has it. **held** *(match)*
- `[vc8]` `sub eax,1; je; sub eax,1; jne` on a state member with the second case's block
  first is a `switch` whose cases are written 2, 1; an `if (s == 1) ... else if (s == 2)`
  compares with immediates. `add r,-1; js` in front of a loop is a second variable for the
  loop (`for (int m = k - 1; m >= 0; m--)`). Destructor calls in front of a function's last
  calls are an inner block around the objects. `neg; sbb; add 1` kept across a temporary's
  destructor is an `int` local holding the comparison. **held** *(match)*
- `[vc8]` Calls checked by address in parked functions (round 106C): `delete[] p` in a
  file that does not declare `operator delete[]` is compiled as a call of `operator
  delete`; the image calls the array operator's own thunk. Count the image's calls per CRT
  address against ours for every parked function before tuning it.
- A variant runner that replaces "from the function's first line to an end marker" must
  not take end-of-file as the marker for a function that is not the last: the functions
  behind it vanish, and vanished functions lower the identical count without a DIFF.
  After every full match compare the count with the base plus the lines you claim.
- `[vc8]` Serialisers, round 107A: a saver whose image has its serialiser's saving half
  written out is defined **in the serialiser's file, behind it**; with a constant mode
  argument VC8 inlines the call by itself and still emits the serialiser. No
  `__forceinline`, no copied body. **held** *(match)*
- `[vc8]` A list walk the port wrote with an index (`for (i = 0; d[i] != -1 && i < n; i++)`)
  where the image has `mov eax,p; sub eax,ini; sar eax,2; cmp eax,[l+0xc]` is an iterator
  object. Search the image for the walk's bytes and list every port that has it: one
  idiom, dozens of functions. The step with a named index (`unsigned i = p - ini; if (i <
  n) return 1;`) and the `&&` form differ by one step of the scratch-register rotation in
  the loop body; an increment in front of the body's destructors is the body's last
  statement. **held** *(match)*
- `[vc8]` A by-value class argument built in place on the stack (`sub esp,N; mov [slot],esp`)
  needs a user-declared copy constructor on the class; declare it under the compiler's
  own `#ifndef` when a second toolchain builds the same source (its ABI passes such a
  class by hidden reference). A temporary destroyed at the function's end that leaves the
  statement temporaries their shared slot is bound to a non-const reference (`T &v = (T
  &)(f())`); a named local and a `const T &` both cost a slot. The second toolchain needs
  a plain local there (the reference dangles). **held** *(frame)*
- A native check that prints frame counts after a failed compile ran the old binary: grep
  the build output for `error:` before reading the counts.
- `[vc8]` A constant in a callee-saved register on our side only (round 107B): `mov
  ebx,2` with `cmp [this+N],ebx; test [flags],bl` where the image compares with the
  immediate (and keeps another constant, or nothing, in the register) means the member
  compared comes through an inline getter (`if (Fase() == 2)`); the getter's constant
  does not join the other uses of the same value. The same getter in a function's
  opening test also moves `this` and the zero register.
- `[vc8]` The scratch-register model is predictive (round 107B): read the image's loads
  as a cycle first, then write the inline accessor, getter or wrapper where the cycle
  breaks (a global's member reached three times: the accessor at all three; a load that
  repeats `eax` behind a three-word copy: an inline getter of the object; `eax` in front
  of a member call where ours has `ecx`: an inline wrapper of the callee).
- `[vc8]` Macro-generated handler families declare their locals at the top: a loop
  counter that reuses the function's zero register is `int i;` at the top and `for (i =
  0; ...)`; two zeroed flags with exchanged stack slots are declared in the other order.
  The handled flag is set on the id alone (`cmp [m],K; mov r,1; jne` is `hecho = 1; if
  (m == K) ...`): a port that tests the member in the same condition changes which
  messages reach the base class.
- `[vc8]` Struct copies by their instructions (round 107B): three dword moves from the
  source are a struct assignment; three `fld/fstp` to the frame and then dword moves are
  a copy-constructed local assigned; `fstp` straight into the outgoing argument area with
  `mov [T],esp` is a by-value argument built by its copy constructor.
- An aligned instruction diff that normalises stack slots hides exchanged frame slots:
  trust the byte count of the compare, not "0 mismatching lines".
- With several agents committing, a private working copy of a file taken before someone
  else's commit reverts that commit when copied back whole: re-copy from HEAD and
  re-apply the edit right before committing.
- `[vc8]` Where the scratch cycle breaks, fix the load before it (round 108B): a copy
  that restarts at `eax`, or an argument in `edx` where the image has `eax` with `ecx`
  busy, is one step ahead because the previous single-use load, often in the handler
  above, is an inline getter in the original. Wrapping the call that shows the
  difference changes nothing.
- `[vc8]` Static guard masks shifted by one bit from some point on, plus a second guard
  word tested with bit 1, mean that one static lives in an inline member (a class's
  "mark changed and tell the parent" helper). A plain inline member with a live static
  stays out of line in VC8; `__forceinline` inlines it with its own guard word.
- `[vc8]` Both arguments of a virtual send loaded before the pushes, with the vtable in
  a callee-saved register that is re-made afterwards (`or ebp,-1`), is an inline sender
  taking them as parameters; the open call also exchanges the function's constant and
  id registers from the prologue on.
- Markers in source comments go stale: check every "not matched" marker against the
  symbol list and a full compare before trusting a candidate list built from them.
- `[vc8]` Sweep one recovered shape over every function whose image bytes show it
  (round 108A): a bytes regex for the shape's signature instructions gave 55 ports of
  the list walk; rewritten with the one iterator object, 13 of the first 20 opened
  were identical, most at the first build. What is left after the rewrite is small
  and local: a variable zeroed behind the iterator's construction, the element read
  again through the cursor, the walk inside an explicit null test.
- `[vc8]` A null test that jumps into the shared epilogue (behind the register pops)
  where the rebuild copies the tail: the test leaves a `do { if (!o) break; ... }
  while (0)` (or a `goto`); `if (o) { ... }` and an early `return r` both copy it.
- `[vc8]` An argument loaded into a callee-saved register in front of a loop and
  copied to a scratch register for an inline `strcmp`: the comparison is an inline
  predicate with by-value parameters; written in the loop the argument is reloaded.
- `[vc8]` `sub esp,16; fstp [esp+12]; mov eax,esp; <three copies>`: a struct by value
  with a copy constructor followed by a float, two arguments, not a four-float
  struct. An argument computed before a sibling argument's call and pushed after
  it, with the object reloaded, means every argument is an inline getter.
- `[vc8]` The "output pointer" in `eax` at every exit of a `ret 8` member is the
  hidden return pointer: the function returns the struct by value. A "register
  choice" note on a tiny copy routine is worth one try at this.
- `[vc8]` Two functions with identical bytes over two types: one `__forceinline`
  function template; a `static` template is called, not inlined.
- `[vc8]` One flag assigned in several arms and tested once: VC8 threads every
  assignment that reaches the test directly and keeps the one followed by another
  condition (`mov edi,1` in a single arm). Write one flag, not nested conditions.
- `[vc8]` VC8 folds a load into a compare only if no other load stands between
  them; `m = n; cmp(*i, m)` keeps two registers, the reverse order folds. Open: the
  image's index-then-count order with both in registers.
- Tooling traps (round 108A): a helper that finds a function's file by its "not
  matched" marker re-copied the pristine file over an edited one once the marker
  was stripped; a compare log cut to its last N lines hid the DIFF line in large
  files and a variant runner reported matches; a build directory reached through
  a symlink was cleaned in another round's tree. Keep a base copy per edited file
  and merge three-way before committing when several agents commit to one tree.
- `[vc8]` A "zero kept in a callee-saved register" note is often not about the
  zero. Count what each side keeps in the callee-saved registers: when ours holds
  one variable more (typically a loop's element local), that variable does not
  exist in the source. Write the element as an inline getter of the container at
  every use (`if (l->At(i)) { ...; l->At(i)->f(); }`); the freed register takes
  the zero and the element is reloaded behind each call, as in the image.
- `[vc8]` A zero register that only feeds the first stores of a stack block and is
  then reused by another variable: the block is initialised in front of the first
  call and VC8 scheduled its stores between that call's pushes. Put the stores
  first, in the image's order, and nest calls whose results are arguments
  (`f(r, s, g(h) != 0, k(h, -20))`: rightmost called first, `neg; sbb; neg` for
  the `!= 0`).
- `[vc8]` `xor ebx,ebx; mov [n],ebx` at the top, `mov eax,ebx` in two early
  epilogues and a load of `n` at the loop's exit, the register reused inside: the
  inner early exit is a `goto` to the single `return n;` under an outer `if`.
  Nested `if`s join the exits (no zero register); `return n;` at each exit folds
  to `xor eax,eax`; a `goto` for both tests joins the two epilogues.
- `[vc8]` An inline sender with by-value parameters (`int Send(int from, int to,
  int a, int b, int c) { return Msg(from, to, a, b, c); }`) loads both id
  arguments in front of the pushes, in argument order, and reads the vtable into
  a callee-saved register; the open virtual call loads the second id between the
  pushes. A tiny forwarding wrapper marked "register choice" is worth this try.
- `[vc8]` A plain `inline` helper that contains a loop is called, not inlined;
  `__forceinline` gives the image's shape. `jns` behind `test r,r` is a mask test
  (`k & 0x80000000`), `(int)k < 0` gives `jge`/`jl`. `and r16,0xfff8` as imm16 is
  an unsigned short destination; a signed one narrows to `and r16,-8` (imm8).
- `[vc8]` `sub [n],1; jne` with the count on the stack and `test eax,eax; jle` in
  front is `for (i = 0; i < n; i++)`; `if (n >= 1) do ... while (--n)` gives
  `cmp eax,1; jl`. A member whose single exit has `xor eax,eax` returns `int 0`:
  a `void` port can never match (rename the symbol, keep the old name as an alias
  for callers that declare it `void`).
- Tooling traps (round 108C): a replace-the-function-at-ADDR helper must anchor on
  the comment at column 0 *below* the class declaration (the declaration carries
  the same address in a trailing comment) and must not assume the function ends
  at the first `}` line (a one-line port has none: the helper swallowed the next
  function). A deleted unmatched port does not lower the identical count, so diff
  every edited file against its base before committing. Drop the base copies
  after each commit or the next three-way merge conflicts with your own commit.
- Uninitialised reads the original has (round 109A): when a matched function reads a
  slot the image never set (an iterator's cursor words on a null-list path), keep the
  VC8 text and give the other compiler a deterministic value through one macro that
  is empty under VC8 (`ITER_NULO(p, ini)` at the head of every iterator constructor:
  `p = 0; ini = 0;` under `__clang__`). One definition in one header; the match count
  must be equal before and after.
- `[vc8]` A frame the size of an iterator that is never stored (`sub esp,0xc`, the
  cursor words only read on the null path) comes from the three-word walk struct with
  its fields set under `if (this)`; two bare pointers set the same way get no slot. A
  by-value struct parameter in the same function takes the slot away again (VC8 gives
  the never-written struct the parameter's own offset): open.
- `[vc8]` Two callee-saved registers exchanged between a parameter and the list in a
  recursive walk: put the element into a local in front of the recursive call
  (`T *c = *it.cur; r = c->F(e, n);`). The sibling with one argument fewer matches
  with the element spelled in the call.
- A port that is a thin member wrapper round a static helper taking the object first
  is usually the member itself: move the body in, read the return type off both
  exits, and keep the old mangled name as an alias line for callers that declared the
  old type.
- `[vc8]` A cursor object whose address goes to a non-inline member (`lea ecx,[it];
  call Borrar`) while its cursor stays in a register over that call and is only
  stored back: the cursor is a separate pointer beside the object (`p++; it.cur =
  p;`); with the member alone VC8 reloads it after the call. An element read twice
  (`cmp [p],-1` in the step, `mov r,[p]` in the body) is an inline getter of the
  object in the body (`*it`): spelled as `*p` the two loads merge and take the
  register the list needs. The list register reloaded at the foot of the loop
  instead of the top is the list read back through the object right after it was
  stored there (`it.lista = m; p = it.lista->datos;`), not a local.
- A workspace object can be older than the source another agent's commit just
  brought in (same-second sync and build): a DIFF on a function you did not touch
  that the main tree does not show is a stale object. Touch the source and rebuild
  before reporting a regression.
- `[vc8]` A search loop whose found block has several exits, with the result's
  zero in `eax` for the loop's compares and a callee-saved register pushed only
  inside the found block: every exit of the block sets the result variable and
  leaves through the one `return r;` (`r = h | flag; break;` at each, or a
  `goto`). A single `return expr;` inside the block folds the zero to immediates;
  the copied tails in the image are VC8's own.
- `[vc8]` Serialisers (one function for save and load, a mode flag and a stream):
  when the image takes `lea r,[this+N]` once and addresses what follows from `r`
  (`lea eax,[r+4]`, `add r,8` on a dead register, a loop pointer with negative
  offsets), each sub-object and array element has its own inline serialise
  member, called by the owner. A port that spells every transfer on
  `this + offset` has the same calls with `this` and the flag in each other's
  registers; nothing local repairs it. Write the members first: several such
  ports matched on the first build.
- `[vc8]` A pointer member transferred and then tested through the same register
  (`lea r,[this+N]; push r; ...; cmp [r],0`) is a block-scoped pointer to the
  member (`{ T **q = &m; xfer(q, 4); if (*q) ... }`). Whether the body then
  spells the member or `*q` reads off the addressing mode of the store behind
  `new`.
- `[vc8]` A local can live in the home slot of a parameter that is dead by then
  (the listing shows the local at a positive offset). A port that reuses the
  parameter by assignment gets an extra register copy; declare a plain local.
- `[vc8]` An inline member that the image calls out of line at one site (a call
  to the COMDAT copy, every other use inlined) cannot be asked for: declare a
  non-inline stand-in and alias it to that copy. Registers behind the call often
  fall into place with it.
- `[vc8]` A serialiser of a neighbouring file written out inside a saver and a
  loader, each with its constant mode: define it in-class in the caller's file
  and let VC8 inline it; do not copy the halves by hand. If saver and loader
  cannot both match from one statement, give the read arm its own locals.
- A DIFF line that says `0 bytes differ` carries a hard note: usually a body
  defined inline in the file (a constructor pushed by address for the vector
  constructor iterator) compared against the thunk the image references.
  Declare it only.
- A variant runner that prints "identical" when no DIFF line comes back also
  prints it for the wrong symbol or a build that did not run. When every variant
  gives the same number, the symbol or the build is wrong, not the idea.
- `[vc8]` Temporaries kept by the pointer their maker returns (`call Token; mov
  edi,eax` several times, the objects built last to first and then used first
  to last) are temporaries bound to the reference parameters of an inline
  helper (`Put(&dst, T(tok(0)), T(tok(1)), T(tok(2)))` with `Put(R *, B &, B &,
  B &)`). Named locals are addressed by `lea` instead. Arguments are evaluated
  right to left, so the destination's address is formed behind the last
  temporary. Scan the image for `call maker; mov callee-saved,eax` to list the
  ports that want the helper.
- `[vc8]` Values read again from two scalar slots behind a member call on the
  struct built from them: the struct must be reloaded after the call, so the
  operands are other objects, the caller's own locals filled by reference by
  the inline that also returns the struct. The order of the reference
  parameters decides the slots; a `const` local changes nothing.
- `[vc8]` A load of `o->p` that stays behind a store to `o->v.y` (ours hoists
  it in front): the store went through a pointer, the vector's own inline
  member (`o->v.Polar(a)` writing `x` and `y`). The folded address is the same
  `[o+N]`, the alias decision was made before folding.
- `[vc8]` `fstp t; fld t; call __CIsqrt; fstp t; fld t; fstp m` is the float
  overload `sqrt(float)` of `<math.h>`; no spelling of `(float)sqrt(double)`
  rounds twice.
- `[vc8]` A callee that takes `this` in a callee-saved register (a
  link-time-optimised static): when the caller already holds `this` there, a
  `__cdecl` stand-in aliased to the callee's address gives the image's call
  (no `mov ecx`, caller pops). Keep the real member call for the other
  compiler behind an `#ifdef`.
- `[vc8]` A user copy constructor added for a by-value argument changes the
  other compiler's calling convention for that type (hidden reference); when
  other files declare the callee with a plain struct, declare the constructor
  for VC8 only.
- A stale `// ADDR ~` marker in another file sends marker-driven tools to the
  wrong source (`lines N 0`, every variant the same number). An edit script
  that cuts a function at the first `// ADDR` may hit a declaration's comment.
- `[vc8]` Plain stores of loaded values come out exchanged two by two (`load
  b; load a; store b; store a`): for a run of `w[i] = argN;` statements the
  emitted order is the source order with each adjacent pair exchanged from the
  first statement on. A port written in the image's store order is off by one
  exchange per pair; apply the exchange to the image's order to get the
  source. A score that normalises offsets cannot see a store order.
- `[vc8]` A message test laid out with both refusals jumping and the body in
  line: a flag set inside the compares and tested once (`int c = 0; if (a !=
  -1) { if (a == k) c = 1; } if (c) {...}`), the shape an inline predicate
  leaves; `a != -1 && a == k` in one condition gives other code. Worth a
  mechanical transform over every `if (A && B)`.
- `[vc8]` `v == 0` still computed (`xor eax,eax; test r,r; sete al`) on a path
  where `v` was just tested: the steps are a row (`if (!v) v = B(); if (!v) v
  = C(); ... if (!v) {ok} else {fail} return v == 0;`) with one `return`. The
  jumps are threaded to the failure arm, the return is duplicated late and
  not folded; a `return` inside the innermost arm of a nest is folded.
- `[vc8]` A known-zero value returned from its register (`mov eax,esi`), not
  as a constant: the result is a variable initialised from the tested value
  and assigned in both arms of the inner test, read once behind the join.
- `[vc8]` A frame of 8 with two word stores, then a dword and a word copied
  out: three shorts are one six-byte record assigned as a struct, not a pair
  plus a separate word.
- Two byte-equal deleting destructors next to each other are the base
  template's and the derived class's with the base inlined; the vtable slot
  that reaches each (through the link thunks) says which symbol goes where. A
  "missing" compiler-generated function may only lack its symbols line.
- An edit helper that ends a function at the first `}` followed by a blank
  line swallows the functions that follow without one; nothing reports
  `different`, the identical count is short. Diff every changed file against
  the head before committing. A commit helper that copies the own file over
  the working file destroys another agent's uncommitted edit there: write the
  blob to the index and three-way merge into the working file.
- `[vc8]` One recipe closes a family of script readers at once: every triple
  `call Token; mov callee-saved,eax` (tokens fetched last to first, read first
  to last) followed by `fstp [t]; mov r,[t]; mov [dst],r` three times is
  `dst = LeerTrio(TEMP(v.Token(0)), TEMP(v.Token(1)), TEMP(v.Token(2)))` with
  an inline that takes three references and returns the vector by its
  three-float constructor; a pair of ints read second-first is the same inline
  with `int y = (int)b.Numero(); int x = (int)a.Numero();`. Six readers of
  700 to 2700 bytes matched on the first or second build. When the temporaries
  die only after several more statements (a `sqrt`, a member call on the pair,
  two stores), all of those statements are inside the inline; if VC8 then
  calls it out of line, `__forceinline` it.
- `[vc8]` An element of a member array spelled `m[i].f` at every use
  (`mov ecx,[this+N]` in front of every store, the index scaled in one
  register) is no element pointer; `T *e = &m[i]` folds the reloads.
- `[vc8]` Three sums `fld [a]; fadd [b]` with the same side first where every
  spelling on scalars loads the other side (or a loop gives a mix): an inline
  `V3 Suma(const V3 &a, const V3 &b) { return V3(a.x + b.x, a.y + b.y, a.z + b.z); }`
  whose result is a named local, read as `w.x`, `w.y`, `w.z` afterwards. Two
  rows of a projection with the first row's middle product first and the
  loaded components kept on the x87 stack for the second row: two `int`
  locals, `int px = (int)(m[1] * w.y + m[0] * w.x + m[2] * w.z); int py = ...;`
  stored into the box afterwards; storing straight into the array elements
  loses the shared loads.
- `[vc8]` The scratch registers restarting at `eax` for a member passed as an
  argument (`mov eax,[f+N]; push k; push eax; lea eax,[v]; push eax`, where
  the rotation would give `edx`) is an inline getter (`f->Volumen()`), also
  where the same member is passed to a second call. A frame 4 bytes smaller
  than ours with a small struct sharing the slot of dead float locals: the
  struct is declared in a block of its own. Three `mov word [t+k],ax` with no
  buffer cookie: a struct of three shorts, not `short t[3]`.
- `[vc8]` A float conversion done inline (`fnstcw ... fistp qword`) in one arm
  with direct stores in both arms: `if (c) m = (unsigned)x; else m = 0;` (the
  `?:` form converts through `__ftol2_sse` and joins). A frame larger than
  ours by a round number with an unused text buffer: the buffer's size (512
  against a measured 452).
- `[vc8]` A constant 1 loaded into a callee-saved register, pushed wherever a
  1 is an argument and copied to `eax` at the success exit, with the failure
  exits `xor eax,eax`: the result is a variable with one `return`. Three
  spellings, each its own code: `int r = 1; if (f1 || f2 || ...) { err = k;
  r = 0; } return r;`; `int r = 1; if (ok) { ... } else r = 0; return r;`;
  `int r; if (bad) { ...; r = 0; } else { r = 1; ...; } return r;` (the 1
  lives over a call and seeds a loop counter). `return 1;` in the arm and
  `return 0;` at the end fold the constant into every use. A result through
  an `else if` chain (`int r = 0; if (on) { if (a) r = -1; else if (b) r = 1;
  else if (c) r = 1; } return r;`) gives one `xor eax,eax` on top where early
  returns give separate exits.
- `[vc8]` `mov r,[p]; add r,[n]` straight from two home slots where ours has
  `lea r,[a+b]`: the parameter itself is advanced (`p = (const char *)p + n;`),
  there is no local pointer. A loop whose guard and bottom test each load a
  member for their own `add` and whose body loads it again is a plain
  `while`, not the guarded `do ... while` the code looks like.
- `[vc8]` `mov eax,[i]; mov edx,[table]; scale eax; add eax,edx` (the table
  in a register in front of the scaling) where ours has `add eax,[table]`,
  the element address formed again in every pass: the table is a vector
  class with an inline `T &operator[](unsigned i) { return data[i]; }`,
  spelled `table[i]` at each test and bound to a reference inside the body.
- `[vc8]` A found exit that tests the pointer again (`cmp eax,edi; je found
  ... found: test eax,eax; jne ok`) while the exhausted exit goes straight to
  the failure return: `while (p && p != x) { p = next(p); i++; } if (!p)
  return -1; return i;`. Spelling the image's jumps as nested tests gives
  other code. A dereferenced argument (`f(*id)`) read into a named local in
  front of the call moves the scratch rotation back one register for what
  follows the call. A pair copied from a pointer into the slots of an array
  used earlier (frame smaller by the pair): the array and its uses are in a
  block of their own and the pair is a struct local behind it.
- `[vc8]` Script readers, more of the token-temporary recipe: tokens of token
  temporaries bind the same way (`m[i] = ReadPair(TK(v.Token(i), 0),
  TK(v.Token(i), 1));`, the outer temporaries built twice); a count test on
  a temporary that shows as `setae [b]; ...destructor...; cmp [b],0` is a
  `bool` local. Read each value block's scope off the EH states: a state that
  falls back right behind the use is a block in braces, one that falls back
  at the function's end is at function scope, and one function mixes both.
  A scan for `call Token; mov callee-saved,eax` must use the incremental-link
  thunk addresses, not the functions'.
- `[vc8]` `xor ecx,ecx; cmp eax,ecx; jle; mov [m],ecx` (the zero loaded in
  front of the compare and reused by the stores) where ours has `test
  eax,eax`: the compared value is a named local (`int n = (int)f(); if (n >
  0) { a = 0; b = 0; }`), not the call in the condition.
- `[vc8]` Three call results stored straight into a struct temporary and then
  copied as three `mov` pairs: an inline that fills its result member by
  member (`V3 r; r.x = f(x); r.y = f(y); r.z = f(z); return r;`, no
  constructor). `return V3(f(x), f(y), f(z))` copies each float once more.
- `[vc8]` A frame larger than ours by one buffer with every instruction
  equal: the buffer is declared at function scope. In the braces of its only
  use it shares the slot of the temporaries of the other scopes.
- `[vc8]` `this` and a counter exchanged between `esi` and `edi`, nothing
  else: try a wrapper's member spelled directly where the port goes through
  its inline getter (`list.p[i]` against `list.Data()[i]`), and the reverse.
  A store through an inline setter of a pointed object (`p->Set(v)`) moves
  the scratch rotation back for what follows, like a getter.
- `[vc8]` `lea r,[o+k]; ...; mov [slot],r; mov [r],4` (the spill in front of
  the first store) where ours stores first: the store is spelled on the
  owner (`o->kind = 4;`) while the sub-object's pointer is a local kept for a
  later call.
- `[vc8]` A port declared `__fastcall (self, int, arg)` for a member whose
  callers load no `edx`: call it as a member through a stand-in struct plus
  an alias symbol line under the original compiler, and keep the fastcall
  call under clang.
- `[vc8]` Open: a field of `p[i]` addressed `[offset+pointer*1+d]` (the
  strength-reduced offset as SIB base) where ours has the pointer as base.
  No spelling of the index expression changes it; a local copy of the pointer
  gives the image's order but keeps the pointer in a register.
- `[vc8]` `[native]` A matched spelling may leave a local unset that the
  original compiler happened to home in a live slot (an inline's output
  local placed in the caller's argument slot, so "unset" reads the
  argument). Under another compiler it is stack residue. After a matching
  rewrite, audit every dropped initialiser and every new output local, and
  give it the value the image's slot holds (the parameter, not always zero)
  under the native compiler's `#ifdef`. A scan of the matching commits for
  removed `= 0`, `= { 0 }`, `= ""` and `memset` lines finds the candidates.
- `[vc8]` A value that lives in one callee-saved register from its `xor`
  at the top, serves as the function's zero, and leaves no constant
  register beside it: declared first with its zero and filled through an
  inline that takes the old value and returns the new one
  (`v = IfPresent(tok, v)`). The reference form (`IfPresent(tok, v)`)
  homes it on the stack; a default-taking form adds a default arm and a
  constant register. Both inlines can stand in one function, line by line.
- `[vc8]` A pair returned by value and read as `mov r,[eax]; mov r2,[eax+4]`
  right behind the call: the result bound to a reference, not a named
  local.
- `[vc8]` Stack slots: temporaries bound to references never share a slot
  with named locals of dead scopes, in braces or inside a forced inline;
  when the image's frame says they do, the earlier sections were inline
  functions (try `__forceinline` members for each section: it can also
  undo an exchange of `this` and a parameter between two registers). A
  forced inline that returns a struct and owns locals with destructors is
  not inlined at all.
- `[vc8]` Matrix runs: the differing-byte count is misleading while the
  frame size is wrong; rank by it, then read the listing of the best few.
