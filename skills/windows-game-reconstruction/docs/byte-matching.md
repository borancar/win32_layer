# Byte-matching ideas, with what survived

Source shapes and methods that brought a reconstruction closer to the bytes the
original compiler emitted, collected from project history. Each entry carries the
compiler it was measured on and whether it lasted.

## Read this first

This header is the part to read every time. Do not read the file whole: find
the section for your symptom below, or grep for the instruction pattern
(`grep -n -i 'setz\|lea eax,\[' byte-matching.md`), and read only the entries
whose tag is your compiler.

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
| a VC8 shape not found above | grep section 14 (raw, unstatused, half the file) |

**Reading an entry.** `[vc6]`, `[vc8]`, `[vc8 rtm]` name the compiler; **held**,
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

Sections 1 to 13 give each idea a status. Section 14 and `vc8-regalloc.md`
(beside this file) are the raw Commandos record, moved from `SKILL.md` in the
order it was found, tagged but without a status; where one of its items has
since been checked, the entry in sections 1 to 13 is the authority.

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
- **Claims that were wrong.** `[vc6]` **reverted.** "A two-byte difference is a
  GOG-patched jump" (the reference already had the patches undone); "declaration
  order decides register order" (section 11); two diagnoses from a second model
  that named the wrong source block. Verify a claim against the bytes before it
  goes into a document.
  `[vc8]` same: "suspect `/GL`", "unwritable", "no candidates left" (below).

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
- **Switch shapes.** `[vc6]` **held.** Separate identical arms give a plain jump
  table; shared labels give a range test or a byte table. `sub/je/dec/je` is
  separate `case X: return 1;` arms. A table slot pointing at the default is an
  explicit `case N: return 0;`. Arm layout follows source order, the table index
  the case value. `mov eax,K; push eax` in every arm is a local set per arm and
  one call after the switch, which VC6 copies into each arm.
  `[vc8]` same for layout and table: `Selector5c01c0::Llamar` (0x5c01c0) is
  written `case 1, 3, 4, 2`, the order of its blocks in the image.
- **A dead `cmp` with no branch after it** is two identical if/else arms the
  compiler merged. `[vc6]` **held.**
- **Small shared tails reached by `goto` get duplicated; do the work in each
  branch and `break`** to get one block (VC6 cross-jumps identical tails).
  `[vc6]` **held.**
- **`xor r,r; test; jle; mov r,X` is `x = 0; if (c) x = X;`**; a ternary gives
  `mov; jmp; xor`. `sub 3; neg; sbb; and K; add B` is the two-constant ternary
  `v == 3 ? B : B + K`. `[vc6]` **held.**
  `[vc8]` same direction: `c ? a : 0` is branchless (`setne; sub; and`), the
  image's `xor; cmp; jne; mov` is `int r = 0; if (c) r = a;` *(record)*.

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
  `[vc6]` **held.** `T *o = obj;` changed base/index order and where a parameter
  is reloaded after an inline `strcpy` (ReceivePlayerMsg 2 -> 0). The reverse
  also holds: renaming the parameter and dropping the copy lost two matches,
  which came back with the copy.
  `[vc8]` same: `int id = *(int *)b;` before the condition closed 0x4721d0
  (2 -> 0), `const unsigned char *q = p;` before the loop 0x570790.
- **Dead argument slots are spill slots.** `[vc6]` **held.** VC6 parks byte or
  address-taken locals in a parameter's home once the parameter lives in a
  register. `mov [esp+N],cl; mov edx,[esp+N]; and edx,0xff` is a `uint8_t`
  local, not a parameter.
  `[vc8]` same *(record)*: a local in a parameter's home is the compiler's
  doing; declare a normal local.
- **Frame size is evidence.** `[vc6]` **held.** A different `sub esp` is a missing
  or extra local. Map every `[esp+N]` to an entry-relative slot (subtract pushes
  and saved registers) and name the image's slots before changing anything.
  LoadMap 502 -> 135 from the frame alone: a buffer's true size, two locals that
  were one, one that was two.
- **Stack arrays.** `[vc6]` **held.** Function-scope arrays never share storage;
  block-scope and inline-local arrays overlap at the lowest free offset. A frame
  too small for one buffer per name means the original reused a buffer
  (CheckMapRules 9 -> 0). Array size moves slot order (HudEdgePaint `text[4]` to
  `text[3]`, 37 -> 2).
  `[vc8]` same *(record)*: one function-scope local reused shares a slot, two
  scoped ones do not; a returned-by-value object per braces reuses slots.
- **Runs of `mov [esp+N], imm`** are a local array initialised from constants;
  declaration order decides the order of the store groups. `[vc6]` **held.**
- **Chained assignment.** `[vc6]` **held.** One load stored to two fields is
  `a = b = src;` (inner first). `a = b = c = 0` stores right to left, so write
  separate statements when the image stores in declaration order.
  `[vc8]` same *(record)*: `a = b = x` against `b = a = x` also decides which
  register keeps the value.
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
- **A float stored and reloaded before a compare.** `[vc8]` **number win.**
  `volatile float` reproduces the `fstp/fld` pair and the frame size
  (`Radio4364e0::Radio` 0x4364e0, 0x4ea6a0, 0x4803b0). A plain `float` local
  gives the round trip in other functions *(record)*, so try that first.

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
- **No unwind frame inside `extern "C"`.** `[vc6]` **held.** VC6 emits no
  `push -1; push handler; fs:[0]` frame in a C-linkage function and treats
  C-linkage callees as non-throwing. A function whose original has a frame must
  have C++ linkage; guard header `extern "C"` blocks so VC6 does not see them.
  `[vc8]` same under `/EHsc` *(record)*: only `extern "C"` and `throw()`
  callees are non-throwing; declare callees and `operator delete` `throw()` to
  drop a frame the image lacks.
- **Callee definition order.** `[vc6]` **held.** VC6 infers "cannot throw" only
  for callees defined earlier in the unit, so keep definitions in original
  address order.
  `[vc8]` stronger *(record)*: a callee defined earlier in the unit is inlined
  or known to keep `ecx`; move it to another file or mark it
  `__declspec(noinline)`.
- **An EH state store around a constructor call is a real `new T(...)`**, not an
  allocation plus placement construction (MpPanelScreen 339 -> 260). Stores
  before the vptr store are member initialisers. `[vc6]` **held.**
- **Write the classes.** `[vc6]` **held.** Constructor/destructor pairs become
  `struct X : Base` with a mem-initialiser base and `~X()`; the compiler then
  writes each vptr store itself, and a destructor's comes first, before the
  unwind state, where no statement can go. A class with no destructor deriving
  through `: Base(n)` needs `typedef Base Base;` to avoid C2437.
  `[vc8]` same *(record)*: real `virtual ~D()` with an inline `~Base() {}`
  gives the scalar deleting destructor; an implicit destructor is a bare `jmp`.
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
  thiscall. `[vc6]` **held.**
  `[vc8]` same, with three more conventions to rule out first: `edx` read
  before written is `__fastcall` (0x5d6200), `eax` is a static function, other
  registers are `/GL` (section 13).
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
  (1,552 -> 1,555). `[vc6]` **held.**
  `[vc8]` same family, by hand: a member or parameter copied to a local, or a
  local dropped for repeated `p->x`, decided many matches (section 4).
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
- `[vc8]` **Solved after being listed unsolved in the raw record (section 14):** the `lea
  eax,[reg+1]` result, `jb` against `jl`, the switch's zero pushed as a
  register (all SP1); the materialised `== 0` (section 3); pointer-difference
  string loops (intrinsics, section 6); the float argument-slot reuse of
  0x58acf0 (plain source once the owner class had its layout: `T *e =
  sonidos[canal]; if (e) return e->VolumenSonido(); return 0.0f;`, identical);
  `eax` and other register arguments (static functions and `/GL`, section 13).

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

## 14. The raw VC8 record (Commandos), no status yet

Moved from `SKILL.md`, in the order found. Nothing here has been checked
against the current tree unless sections 1 to 13 say so; several items repeat
or refine earlier ones. Give an item a status by moving it up, not by editing
it here.

### First classes and the generators

- `[vc8]` **Worked result (Commandos `ClassAUAgacharse`, 10 of 10 functions identical)** and the
  shapes that moved the match: `cl /O2 /Gy /GR /EHsc` with the default `/GS`; `class` not `struct`
  (RTTI name `.?AV` vs `.?AU`); an `int` parameter, not `bool`, when the code loads a dword;
  argument pushes hoisted above an earlier call are normal; a shared `[edx+4]/[edx+8]` tail is an
  inlined helper; a register holding `1` across a static guard is `int r = 1; ... return r;`;
  compare with relocations masked and their targets checked (symbols, string literals, jump-table
  labels, EH funclets, thunks followed through the incremental-link `jmp` table). The comparer is
  `tools/bytematch.py` in the Commandos repo; copy it.
- `[vc8]` **More shapes that moved the match (Commandos Sim*/ClassAccion, 94 identical):**
- `[vc8]` A destructor often lives in its own translation unit; compile it separately or inlining
  changes the caller. A class pair compiled together can inline what the original did not (split
  `ClassAccion` / `ClassAcUsuario`).
- `[vc8]` Implicit vs explicit dtor changes vtable slot 0 and the scalar-deleting stub; keep a
  switch (`SIM_DTORS_INLINE`) while testing.
- `[vc8]` A repeated tail such as `if (guardar) {...}` is an inline helper taking `(int, Class*,
  void*, int)`; write it once and call it.
- `[vc8]` A diff of a few bytes that is a register swap (edi vs esi) is usually a parameter/local
  order, not different logic; permute before rewriting.
- `[vc8]` Struct copies compile differently as ints than as floats (`fld/fstp` vs `mov`); copy with
  the member types the original used.
- `[vc8]` A field read pair load/store ordering (`fstp [this+x]` before a pointer load) is a
  scheduler choice that source reordering did not move; record such functions as known near misses
  (`~name addr` in `symbols.tsv`) and move on.
- `[vc8]` **Do the stateless functions mechanically.** Decode vtable-slot bodies that are fixed by
  their bytes (`ret N`, return constant, field get/set, `lea`, `fld [const]`) and generate the C++
  from a table of byte patterns (Commandos `tools/gentrivial.py`: 491 functions identical on the
  first build). Do this before hand-matching anything bigger.
- `[vc8]` **Template factories are real templates.** `ctorAccionT<T>::Crear` is `return new
  T(a,b,c,d)`; read the size from the `push imm` before `operator new` and the ctor from the `call`,
  then instantiate the template explicitly. 82 of 84 matched on the first build (Commandos
  `tools/genfabricas.py`).
- `[vc8]` **More fixed-shape families (Commandos, +350 identical):** scalar deleting destructors
  (`call dtor; if (f & 1) operator delete(this); return this`) and bare vptr-store constructors can
  be recognised by byte pattern and emitted per function with a placeholder owner; map vtable
  addresses as `extern "C"` arrays in the symbol table. Skip stubborn near misses and keep
  generating.

### From the first mass-matching rounds

- `[vc8]` **Mass byte-matching with generators (2,877 functions).** Lessons: (1) a
  callee reached with `ecx` untouched is a member call on `this`; `edx` used where you'd expect
  `ecx` means `this` is live (write a method, not a free function). (2) A generator must run over
  *unmatched plus already-generated* addresses, else its output file loses earlier matches. (3)
  Group unmatched functions by their instruction text with immediates masked: shapes repeat 5-30
  times (forwarders, accessors, deleting destructors, `Serializar`, constructors) and one generator
  per shape beats hand work. (4) `~name` in the symbol map marks a near miss so the count stays
  honest; try two source forms (`if(a==K)` vs `switch`, temp vs direct) before marking one.
- `[vc8]` Tail jmp after 'mov [esp+4],x' => the wrapper declares the stack arg it forwards (callee
  cleans it).
- `[vc8]` Placeholder class names across many small .cpp files collide (ODR); compile fine, but the
  COMDAT of one is dropped by the linker/bytematch. Check for duplicate class names after each
  batch.
- `[vc8]` Inline strcpy-to-global with 'sub edx,eax' diff indexing: write 'char c; const char*p=s;
  do { c=*p; g[p-s]=c; p++; } while(c);'. Local 'r=0; if(c) r=&a[i]; return r' hoists the xor above
  the cmp.
- `[vc8]` Imports: name an IAT slot as __imp__<Name>@<N> in symbols.tsv; declare
  __declspec(dllimport) __stdcall in src/win32/. Walk the PE import table to map slot address ->
  name (peimports.py prints names only).
- `[vc8]` Branch layout: original 'je Lend ... Lend: xor eax,eax; ret' => 'if (c) { work; return 1;
  } return 0;' (not an early 'if (!c) return 0;').
- `[vc8]` float t = a*a+b*b; return (float)sqrt(t); produces the fstp [esp]/fld [esp] round-trip
  before call __CIsqrt (single-precision rounding); without the local it keeps the value in st0.
- `[vc8]` Null path ending 'xor eax,eax; ret' means the function returns int (return o->M(); return
  0;) even if the result looks unused.
- `[vc8]` Two conditional jumps to one out-of-line block at the end (after the epilogue) = 'if (a &&
  (i = F()) != -1) {...} else G();' (single else), not nested ifs.
- `[vc8 rtm]` **+700 functions after the generators.** (1) Factory
  `Crear`/`CrearEdit` whose epilogue is merged (one `ret`, no `xor eax,eax` on the null path)
  returns **void**; the compiler only materialises 0 when the result is used, so check the return
  type before hunting an expression shape. (2) Scalar deleting destructors: write the real `virtual
  ~D()` with an inline `virtual ~Base() {}` (else a call to it appears) and map `??_G..` plus
  `??_7..`; when the destructor body calls out, declare the callees and `operator delete` `throw()`
  or the compiler keeps a separate destructor with an EH frame instead of inlining it into `??_G`.
  The same `throw()` removes the EH frame from constructors that `new` members (`Nc::Nc` in
  tiny221). (3) Constructors with member arrays call the vector constructor iterator (`push dtor;
  push ctor; push count; push size; lea this+off; call`): declare the element class with ctor/dtor
  and the array as a member; stores before that call are member initialisers, stores after are the
  body. (4) `mov ecx,[global]` before a call means the global is a pointer (`extern T *g; g->M()`),
  not an object. (5) A tail `jmp` with the arguments untouched on the stack needs the callee to take
  exactly the caller's parameters; if the compiler caches them in registers it pushes copies
  instead. (6) (Superseded: the unsolved list and the naked `jmp eax` thunks; see sections 9 and
  12.)
- `[vc8 rtm]` **Tiny291-305.** (1) A plain `ret` in a function that has stack
  arguments means `__cdecl`; `ret N` means `__stdcall`/thiscall. Check before blaming the body. (2)
  A frame with a stack security cookie plus an unexplained 4-byte store (`mov dword [esp],0`) is a
  struct `{int h; char b[N];}` whose header is zeroed, not a plain `char buf[N]` (frame size and
  stack offsets differ by 4). (3) `new T(a)` with an EH frame in the original means the constructor
  is *not* `throw()`; the same call without a frame needs `throw()` on the constructor. (4) A float
  triple written with `fld/fstp [esp+..]` around pushes lists the elements in memory order of the
  pushed pointer: derive index from the pushed address, not from store order. (5) Pointer-difference
  loops (`(p-base)>>2 < n`) get rewritten by VC8 into an index induction variable; the original's
  `mov ebx,esi` shape did not reproduce from any source form tried (about four near misses).

### Array-new with a constant count, and neighbours

- `[vc8]` `new T[11]` with a literal count compiles to a plain `operator new(44)`. The original's
  `mov eax,0xb; mov edx,4; mul edx; seto cl; neg ecx; or ecx,eax; call operator new[]` (unfolded
  overflow check) comes from passing the count through an *inline helper's parameter*: `void
  Alloc(unsigned n){ f14 = new T[n]; ... } ... Alloc(11);`. Also: hoist a compared member into a
  local (`int k = f4;`) when the original loads it before the loop's null test, and keep a result in
  a variable (`int r = 0; ... r = 1; return r;`) when the original returns it from a register. More
  VC8 shapes that matched: a callback called with `ecx` = object plus one stack arg is a pointer to
  member (`typedef void (C::*CB)(int); (a->*cb)(b);`, mangled `P81@AEXH@Z`). A by-value 8-byte
  struct argument (`sub esp,8; mov eax,esp; mov [eax],..`) comes from a temporary with a user copy
  constructor (`f(S(a,b))`). `delete[] p` of a type with a destructor emits the `__vec_dtor` call
  (give the class `~T();` at the destructor's address). A float argument passed with `push ecx; fstp
  [esp]` is just the slot for the float, not a stdcall. A COM-style slot call that passes `this` on
  the stack is a `virtual ... __stdcall` method. Dead locals the original keeps (`mov [esp+..],0`)
  are `volatile`.

### Idioms, in the order found

- `[vc8]` Write `p->x` repeatedly instead of hoisting `o = p`; the compiler then reloads into the
  dead `this` register.
- `[vc8]` A by-value struct argument needs a user copy ctor that copies float-wise; a pair copy via
  an inline-ctor temporary gives loads-then-stores.
- `[vc8]` Operand order `key == arr[i].d` picks the `cmp r,m` encoding.
- `[vc8]` `int r = 0; if (f == r) r = B(); return r;` gives the tail jmp with zero in eax.
- `[vc8]` A ternary store `f = o ? o->g : 1;` gives two return paths.
- `[vc8]` An index loop `for (int j=0;j<8;j++)` gives signed `jl`; a pointer-end loop gives `jb`.
- `[vc8]` A member array of a class with non-inline ctor and dtor makes the compiler emit the
  vector-ctor call.
- `[vc8]` Thiscall returning a 32-byte object by value: `T o = a->G("KEY");`.
- `[vc8]` Config readers (`G` then `M`/`N`/`P`) follow one template and can be generated from
  disassembly (`tools/genprops.py`).
- `[vc8]` When store order differs, hill-climb pair swaps of the store statements until the byte
  difference is 0 (`tools/reorder_hc.py`).
- `[vc8]` Never delete symbol lines by prefix `sed`; filter by exact name (`grep -vF`).
- `[vc8]` **eax-vs-ecx intermediates:** if the original chains pointer loads through `ecx` where we
  use `eax`, an earlier call's return value is probably still live in `eax` (the function returns it
  on a fall-through path). Make the callee int-returning: `int r = a->F(); T *t = ...; if (t) r =
  t->F(); return r;`.
- `[vc8]` **Constants held in a register:** `int r = 1; if (!A() && !B() && !C()) r = 0; return r;`
  makes VC8 hoist the 1 into a callee-saved register (`mov edi,1` up front, `mov eax,edi` on the hit
  path). Preset the result to the common value and overwrite it on the other path. A two-case
  `switch (b) { case X: f(); break; case Y: g(); break; default: r = 1; } return r;` with `int r =
  0;` reproduces `movzx/sub/je/sub/je`.
- `[vc8]` **Serializer idiom (stream `m ? W : R`):** the original picks the stream's read or write
  vtable slot with one merged call (`test m; lea eax,[this+X]; push n; push eax; jz; mov
  eax,[edx+8]; jmp; mov eax,[edx+4]; call eax`). Reproduce it with an inline *member* of the stream
  class, `void RW(int m, void *p, int n) { if (m) W(p, n); else R(p, n); }`; a free helper, a macro
  or an `if/else` at each site does not merge. A `PUSH EBP`/`PUSH EBX` appearing mid-function is a
  shrink-wrapped callee-saved save, not a stack argument. Pointers the original keeps in `ebp` (`lea
  ebp,[this+K]` then `[ebp+4]`, `[ebp+0x48]`) need a local pointer in the source (`int *gp = &g; ...
  gp + 1`). Reloading `this->f1c->f190` after a call means two separate expressions, not a cached
  local. Delete-then-read of a counted array: `RW(&cnt,4); if (!m) delete[] arr;` (not an `if/else`
  around the R/W calls).
- `[vc8]` **Virtual slot by index:** declare N dummy virtuals so the real one lands at slot N. When
  the same slot is called with differing argument counts, declare the zero-argument one as the real
  virtual and call the others through a `__thiscall` function-pointer cast of `(*(void
  ***)this)[slot]`.
- `[vc8]` **Callee is a member of `this` that the caller does not reload `ecx` for:** the argument
  loads go through `edx` (ecx stays live as `this`). If you wrote a free function and see `ecx`
  where the original has `edx`, make the callee a member.
- `[vc8]` **COM-style interface calls** (`push args; push obj; call [vtbl+N]`, callee pops): declare
  dummy `virtual void __stdcall dK();` up to slot N and then `virtual int __stdcall Name(args)`.
- `[vc8]` **`Sub *t = &s;` as a local** before using a sub-object several times makes VC8 hoist `lea
  edi,[this+off]` to the prologue (find-or-add helpers on a list member).
- `[vc8]` **`int c = a && b; if (c) ...; ...; if (c) ...;`** gives the out-of-line `xor reg,reg`
  flag block; `if/else c = 1/0` inline gives a `jmp` instead.
- `[vc8]` **String literals:** a compared function that pushes a string address needs a symbol line
  for the literal (`??_C@_..@text?$AA@  0xADDR`, take the mangled name from the generated `.asm`),
  otherwise the literal looks like a mismatch.
- `[vc8]` **Caller-cleaned thiscall** (`this` in ecx, stack args, `add esp,N` after the call) cannot
  be written: a varargs member passes `this` on the stack and `__thiscall` + `...` is error C2217.
- `[vc8 rtm]` (Reverted: "suspect `/GL`" for scratch-register choices; see section 1.)
- `[vc8 rtm]` The Makefile compiles every `src/*.cpp`, parked files included, so keep them
  compilable; and `grep -v Zzxa` also removes `Zzxa1` lines (use exact symbol names).
- `[vc8]` **Store-then-`sub [mem],reg` per field = inlined struct `operator-=`.** Writing `f3c = x;
  f40 = y; f3c -= f4c; f40 -= f50;` makes VC8 fold into `sub reg,mem` plus one store. The original's
  `mov [m],eax; mov eax,[o]; sub [m],eax` comes from a small struct with `Pt &operator-=(const Pt
  &o){x-=o.x;y-=o.y;return *this;}` applied to member sub-structs.
- `[vc8]` **Small stores into a local struct or parallel arrays: permute.** When only the scheduling
  of 3-4 independent stores differs, try all orderings (24 for four). Cheap and often the match
  (0x64e750, 0x5d67a0, 0x64d180).
- `[vc8]` **Wrong tails from if/else chains: try `switch(msg)`** (0x4b4720, 0x568530).
- `[vc8]` **`cmp; jmp L` where `L` holds the shared conditional jump** = VC8 cross-jumping two
  identical if/else arms. Write the arms out twice (`if (t != X) { if (t != Y) A; else B; } else if
  (F()) A; else B;`); a ternary or a bool flag gives `setne`/`test` (0x4ef9e0).
- `[vc8]` **A register preset to 0 that survives untouched to `ret`** (no `mov eax,..` in the
  epilogue) is the return value: give the function an int result (`int r = 0; ... r = 1; ... return
  r;`) even when the callers ignore it (0x5e1a30, 0x5e1ac0).
- `[vc8]` **`strcpy(dst, "")` compiled as a byte loop from a fixed address** (not `mov byte
  [dst],0`) copies from the compiler's shared empty literal: use `extern char g_ADDR[];` as source.
- `[vc8]` **The loop's pointer anchored on a later field** (`lea eax,[this+0xc]`, fields at
  `[eax-8]`, `[eax-4]`, `[eax]`): take a reference to the element (`T &e = t[i];`) instead of
  indexing each access; a pointer (`p++`) or `t[i].f` anchors elsewhere (0x583d30). An argument
  pointer loaded *before* the loop's zero test means it was copied to a local first (`const char *q
  = p; for (; n; n--, q++)`, 0x570790).
- `[vc8]` **Two-int fields updated together** (`[eax]`, `[eax+4]` with one base per element): an
  inline `ParEntero &operator+=(const ParEntero &)` on a member pair (0x6287f0).
- `[vc8]` **A struct copy as unrolled `mov` pairs instead of `rep movsd`**: give the struct an
  inline memberwise `operator=` (0x587b00).
- `[vc8]` **Zeroing constructors, read the store order as structure:** members that have their own
  zeroing constructors are stored first, in declaration order, each in its own internal order (a
  float triple `z,y,x`; an int pair `b,a`); stores set in the body follow. A triple stored before
  lower offsets is the only member with a constructor; everything else is the body (0x570050,
  0x4dbbd0, 0x612b20). A vptr store in the middle of other stores is the derived constructor's;
  stores before it come from an inlined base constructor (0x5bc9d0).
- `[vc8]` **Hill-climbing stores** (`tools/hcstores.py`): score only the function under test (a
  second differing function in the same file corrupts the score), and never kill the loop while it
  rewrites the symbol table.
- `[vc8]` **Debug-text members (InfoEstado: state-name chain, `sprintf`, window call):** declare the
  locals in the order the image uses the registers; put a `? :` for a name straight in the `sprintf`
  argument list when the image computes the string right at the push (a prior `if` chain loads it
  earlier); load a field into a local first when the image loads it before the call (`int est =
  estado;`); `buf + strlen(buf)` appends (inline strlen loop); a state chain whose middle test is
  `sub eax,K; je` is a `switch`.
- `[vc8]` **A point copy through a temporary** (`fld/fstp` into a stack slot, then integer `mov`s
  into the destination) is `Punto3D t = o->pos; dest = t;` where the temporary's copy constructor
  copies floats and the destination assignment is the plain one.
- `[vc8]` **A flag test of `test ah,0x41` + `lea eax,[reg+1]` + `je`** is `int r = 0; if (a > b) r =
  1;`.
- `[vc8]` **Records of equal size behind different names** (buttons, panel objects, stock tables
  0x94 bytes) are one struct; adjacent embedded copies show up as `this+K`, `this+K+size`.
- `[vc8]` **int copy in one case, `fld/fstp` in another: a `union{int i; float f;}` member**
  (0x459fe0).
- `[vc8]` **Virtual calls are thiscall.** Declare `virtual void S(int,int,int)` without `__stdcall`,
  and pad the vtable with dummy virtuals to reach the slot (0x581e60).
- `[vc8]` **Register assignment follows local declaration order.** Reordering locals steers which of
  esi/edi/ebx a value gets (0x593d50).
- `[vc8]` **More than ~4 stores: hill climb.** `tools/hcstores.py NAME` swaps statements between `//
  BEGIN`/`// END` pairwise and keeps improvements (0x5655d0, 0x59d5b0).
- `[vc8]` **A function that returns "whatever is in eax":** make it return the pointer and reassign
  it through the call result (`r = r->f; if (r) r = P(r); return r;`, 0x456690). A `void` version
  hoists nothing and misses by a few bytes.
- `[vc8]` **Shared return on both paths**: `r = 0; if (g) { r = A(); p = r; if (p) {...} r = p; }
  return r;` steers `mov eax,esi` as the jump target (0x5d6fa0).
- `[vc8 rtm]` (Superseded: this unsolved list; see section 12.)
- `[vc8]` **vptr store first in a constructor**: when the original stores the vptr before the member
  zeroing, model the zeroed members as sub-objects with inline constructors; a flat `f = 0;` body
  lets VC8 sink the vptr store below them (Commandos 0x49e880).
- `[vc8]` **Derived destructor restoring a middle vtable, then calling the base out of line**: the
  middle class has an inline empty virtual destructor; only the root's is out of line
  (ClassWindowsVentana/Dialogo -> ClassWindowsReceptorMensajes -> ClassReceptorMensajes).
- `[vc8]` **`throw()` on a destructor** removes the EH frame when its body calls through a function
  pointer (a C++ function-pointer call may throw; an `extern "C"` call does not).
- `[vc8]` **`mov eax,[a]; or eax,[b]; jne`** is `!(a | b)` on two ints (not `!a && !b`, not
  `__int64`).
- `[vc8]` **A pair of stores kept in source order across branches**: an inline `Reset()` on a
  two-field sub-struct (0x5d5a00); flat stores get reordered.
- `[vc8]` **`lea edi,[eax+1]` after `lstrlenA`**: keep `n = lstrlenA(s)` and write `n + 1` at each
  use.
- `[vc8]` **A parameter's stack slot reused for a local** (`lea ecx,[esp+0xc]` = the argument slot)
  is VC8's own doing once the parameter lives in a register; declare a normal local.
- `[vc8]` **The comparer "learns" unknown relocation targets**, so a wrong import (SendMessageA for
  PostMessageA) can match once and break later; check IAT slots by name.
- `[vc8]` **Owners of non-virtual helpers**: many small members are dead code (no caller, no data
  reference). Their class is the one whose vtable-slot functions sit next to them (one object file
  is one contiguous run under incremental linking). Commandos `tools/owners.py` combines vtable
  membership, callers (through the `jmp` thunk table) and neighbours.
- `[vc8]` **Renaming at scale**: put `// 0xADDR` on each definition and regenerate the symbol map
  from the compiled listing (PROC + first `; Line N`) plus the comparer's `learn` lines (Commandos
  `tools/symsync.py`); never hand-edit mangled names.
- `[vc8]` **Deleting generator stubs can lose a unique match.** A one-line stub in a generated file
  may be the only definition at a *derived* override's address (same body as the base's, a different
  address). Diff the matched address sets before and after (Commandos `tools/distinct.sh -l`, with
  `git stash` for the old state) instead of trusting the total.
- `[vc8]` **Tail-jump wrappers do not show their parameters.** `mov ecx,[ecx+K]; jmp F` forwards the
  caller's stack arguments untouched, so the body says nothing about the parameter list; take it
  from the base class's slot (`ret N`) and forward them (`v->F(a)`).
- `[vc8]` **A `thiscall` free-function lookalike**: a placeholder written as `__stdcall f(a,b,c,d)`
  that calls a member through an untouched `ecx` is a 4-argument member (`ret 0x10`); rewrite it as
  one.
- `[vc8]` **Vector deleting destructors (`??_E`)** are emitted only where the vtable is (a TU with a
  constructor); a TU with an explicit `delete` emits the scalar `??_G`. The same class can have one
  of each at different addresses.
- `[vc8]` **DirectDraw `IDirectDrawSurface` slot 32 is `Unlock`** (`push 0; push surf; call
  [vtbl+0x80]`).
- `[vc8]` **Inherited fields across a real base**: when a member of the base class (by vtable) sets
  a field, move the layout into the base (`class Derived : public Base`) rather than casting; nested
  types then mangle as `Base::Nested`, so regenerate their symbol lines.
- `[vc8]` **Empty base slot, value-returning overrides**: when the base's slot is a bare `ret N` but
  the overrides return a value (an older compiler allowed a missing `return`), declare the slot with
  the return type and give the base an empty body under `#pragma warning(disable: 4716)`; VC8 then
  emits the bare `ret` (Commandos OrganoMotor slot 32). Before reaching for it, check the base
  really is bare: a stub "with no return" may simply have lost its `mov eax,1`.
- `[vc8]` **Never run a line-number-based delete twice without rebuilding**: the second pass reads
  stale listings and removes the wrong definitions (Commandos lost 9 matches once; `tools/defs.py`
  now refuses when a listing is older than its source).
- `[vc8]` **A pointer stored right before a `sprintf("Estado=...")`** that looks like a vtable may
  be a string literal (the class name): read the bytes at the address before modelling it.
- `[vc8 rtm]` **eax/ecx scratch after a conditional call**: when the original reloads a stack
  argument into `eax` (ours `ecx`) after `if (x) F();`, load the first argument into a named local
  before the condition (`int id = *(int *)b; if (t.M(id)) F(); Base(b, c);`). Fixed a long-parked
  Commandos function (0x4721d0).
- `[vc8]` **Temporaries vs scoped named locals**: a function that builds the same temporary object
  twice (`F(T("x")); if (p) p->G(T("x"));`) may have been written with named locals in scopes; the
  image passes `lea eax,[esp+N]` (the local's address) instead of the constructor's returned `this`,
  and the slots come in source order.
- `[vc8]` **Paired stores from two registers** (`xor eax,eax; xor edx,edx; mov [a],eax; mov
  [b],edx`, or `mov eax,K; mov edx,eax` before two stores) are a small struct built by an inline
  constructor and copied: `*(Par *)&f = Par(0x8000, 0x8000);`. The constructor's assignment order
  decides which field gets which register; two pairs filled in opposite orders need two struct
  types.
- `[vc8]` **`movzx reg32, byte [p+2]; and reg32, 0xfc`** reads the low byte of a 16-bit field:
  declare the field `unsigned short` (an `unsigned char` field gives `and dl` + `movzx dx`).
- `[vc8]` **`switch` labels:** `case 3: f(); break; ... case 5: f(); break;` and `case 3: case 5:`
  lay out differently; a range test `a > 0 && a <= 2` becomes one unsigned compare while the image's
  `jle`/`jg` pair is `switch (a) { case 1: case 2: ... }`.
- `[vc8]` **Loop bounds over a global table** compared with `jl` (signed) mean the source cast the
  pointers to int: `(int)p < (int)(tabla + 4)`.
- `[vc8]` **`mov ecx,[eax+K]` straight into a call** (no `mov ecx,eax`): keep the intermediate
  object, not the final pointer, in the local (`PantallaJuego *p = G(); p->visor->F();`).
- `[vc8]` **Learned addresses of tables can be the end**: when the first relocation to a global
  array is its end bound, the comparer learns `base + size`; check learned data symbols.
- `[vc8]` **A build that fails leaves old objects**: count matches only after a clean `make match`.
- `[vc8]` **`mov ecx,N; repe cmpsb`** against a string literal is VC8's inline `strcmp(s, "LIT") ==
  0` (N = strlen + 1); `memcmp` gives a dword loop instead.
- `[vc8]` **Float constants in `.rdata` (`fld [addr]`) are compiler `__real@` literals**: map them
  by value (`__real@44a56000` = 1323.0f) and order the source assignments so each array slot gets
  the constant the image loads first for it.
- `[vc8]` **A base call after an `if`**: when a branch target lands on `mov ecx,esi; call Base`, the
  base call runs on every path; move it out of the `if` (several "almost" matches were this).
- `[vc8 rtm]` **`mov eax,K` followed by the K stored twice or a `jmp` to a shared tail**: VC8
  sometimes duplicates a short tail where the image jumps to one; if case reordering and `goto`
  don't fix it, park.
- `[vc8]` **A loop pointer aimed at a field, reading the struct's other fields at negative offsets
  (`test [esi],1` ... `push [esi-4]`), or two pointers stepping together**: the source indexed the
  array (`for (i = 0; !(a[i].flags & 1); i++) ... a[i].dato`); a `T *p` walk gives a pointer to the
  struct's start instead.
- `[vc8]` **Two constant stores to adjacent fields come out in the reverse of source order** in some
  functions: if the image stores +0xdd8 then +0xdd4, try writing them the other way.
- `[vc8]` **`fadd DWORD [2^32]` after `fild`** is an unsigned-to-float conversion in a `float`
  expression; a `double` expression uses the QWORD constant.
- `[vc8]` **Early `return` inside a nested `if`** (`if (m == X) { if (a != Y) return 1; f = 0;
  return 1; }`) lays out a shared `mov eax,1` the plain `if (a == Y) f = 0; return 1;` does not.
- `[vc8]` **Two signed compares per range (`cmp eax,2; jl; cmp eax,6; jg`)**: VC8 folds `t >= 2 && t
  <= 6` into one unsigned compare (`lea esi,[eax-2]; cmp esi,4; ja`) when `t` is a local; written on
  the member (`tipo >= 2 && tipo <= 6`) it keeps both compares.
- `[vc8]` **A register used as a known value** (`sub edx,1; jne; cmp [eax+0x10],edx`): the source
  compared with the same variable (`int t = o->tipo - 1; if (t == 0 && o->campo10 == t)`), not with
  the literal.
- `[vc8]` **`fld [x]; fld QWORD [K]; fucom st(1)`** (a double constant and an unordered compare) is
  `K != x` with the literal on the left; `x != -1.0` folds to a float constant and `fcom`.
- `[vc8]` **A struct argument copied with plain `mov`s and compared from the stack** is a by-value
  struct parameter (`Poner(int id, float v, Posicion p)`); fld/fstp copies of a by-value argument
  come from a user-written copy constructor.
- `[vc8]` **A local for a pointer chain before a call** (`ClassMano *m = actor->mano; m->Usar(&x)`)
  swaps which of ecx/edx gets the object and the argument.
- `[vc8]` **A duplicated call whose argument is a register known to be 0** (`push eax` after `test
  eax,eax; jne`) where ours pushes `0`: the source had one call after an if/else (`if (c && !r) f |=
  8; else f &= ~8; Acelerar(r);`) and the compiler duplicated the tail late, after constant
  propagation. An early `Acelerar(r); return;` in the branch lets it fold r to 0.
- `[vc8]` **A pair passed by value from two adjacent members** (`mov edx,[ecx+0x20]` reloaded after
  comparing it) is a struct copy through a cast (`f(*(Par *)&a1)`), not `Par v; v.a = a1; ...`.
- `[vc8]` **An "override" with a different parameter type is a new virtual, not an override**: it
  appends slots and shifts every later one, and nothing complains until a virtual call past it lands
  on the wrong offset (`call [eax+0x70]` where the image has `+0x68`). Keep a derived class's
  redeclarations on the interface's exact signature and cast inside.
- `[vc8]` **x87 compares: VC8 loads the left operand first.** `fld [a]; fld [b]; fcompp` comes from
  `a OP b` (and a literal on the left gives a double constant, see above); if ours loads them the
  other way, swap the operands and mirror the operator (`f1 < f3` -> `f3 > f1`).
- `[vc8]` **A value-returning slot whose image leaves eax unset**: keep the declared return type and
  write no `return` (C4716 off); `return 0` adds an `xor eax,eax` the image lacks.
- `[vc8]` **Local struct for two ints that only exist as arguments** (`sub esp,8` reserved, one
  field spilled, the other passed from eax) is `struct { int x, y; } s; s.x = ...; s.y = ...; f(s.x,
  s.y)`.
- `[vc8]` **A returned temporary kept by pointer in a callee-saved register** (`call Valor; mov
  edi,eax; ... mov ecx,edi; call Cuenta`) is the temporary handed by reference to an inline helper
  (`__forceinline int NumeroODefecto(Bloque &v, int d) { return !v.Cuenta() ? d : (int)v.Numero();
  }` then `x = NumeroODefecto(b->Valor("K"), 180)`); a named local gives `lea ecx,[esp+N]` per call,
  and `&b->Valor(...)` destroys it at once.
- `[vc8]` **Member calls that leave ecx alone look like `__stdcall` calls** (`push ...; call X` with
  no `mov ecx`, `ret N`): before parking such a "stdcall" function, try it as a member calling
  members of `this`.
- `[vc8]` **A tail duplicated only in part** (each branch has its own `cmp [x],0; jne; test eax; je;
  store` prefix, then they join): the source called one inline helper from each branch (`if (a)
  return Esperar(c1); return Esperar(c2);`); inlining copies the helper's head into both paths and
  tail merging joins the rest. A plain `int c = a ? c1 : c2; ...` gives one copy.
- `[vc8]` **An address comment far from its PROC's first `; Line`** happens when instructions are
  scheduled before the body's first statement; a sync tool should accept the nearest comment above
  unless another definition starts in between.
- `[vc8]` **`lea ecx,[esp+N]` before a member call on a returned object** (instead of `mov ecx,eax`)
  is a named local, not a temporary: `{ T v = b->Valor("K"); v.CopiarA(p, n); }`, one block per
  value so each destructor runs right away.
- `[vc8]` **Our guessed class name clashing with an RTTI name**: an RTTI name always wins; rename
  the guessed class (and its mangled names in the symbol map) instead of working around it with file
  splits.
- `[vc8]` **An inline function the image has both inlined and out of line** (a header constructor):
  keep it `inline` in the header behind a macro (`#define X_INLINE inline` unless defined) and let
  one source file define the macro empty before the include to emit the one copy.
- `[vc8]` **A template member defined in a header gets inlined into callers the image calls it
  from**: mark the definition `__declspec(noinline)`; its own bytes do not change.
- `[vc8]` **A loop that reloads `*cur` for the call** (`mov eax,[esi]; ... mov ecx,[esi]; call`)
  calls through the iterator (`(*it.cur)->F()`), not through the value it tested.
- `[vc8]` **Register order flips with `a = b = x` vs `b = a = x`**: the assignment chain decides
  which register keeps the value and which takes the copy.
- `[vc8]` **`new T[n]` calls `operator new` unless `operator new[]` is declared.** VC8 only emits a
  call to `??_U` (and `delete[]` to `??_V`) in files that declare them (`<new>`); elsewhere both go
  to `??2`/`??3`. A CRT-looking allocation that builds an EH frame around a base constructor is `new
  char[...]` with `operator new[]` declared (it can throw; `extern "C"` `malloc` cannot under
  `/EHsc`). Declare `operator delete[]` with `throw()` or every destructor that calls it gains an EH
  frame.
- `[vc8]` **A function that leaves an argument in eax on every path** and does its work in other
  registers returns that argument (menu key handlers: `return c;`).
- `[vc8]` **Two byte-identical out-of-line copies of an inline destructor** are the implicit
  destructors of two classes (a subclass with nothing to destroy compiles to its base's body); map
  each name to its own copy.
- `[vc8]` **A base constructor or destructor inlined in one subclass and called in another**: put
  the bodies in the header behind an opt-out macro and define it in the files whose image calls
  them.
- `[vc8]` **`c ? a : 0` compiles branchless** (`setne; sub; and`); the image's `xor eax,eax; cmp;
  jne; mov eax,a` is `int r = 0; if (c) r = a;`.
- `[vc8]` **Repeated range tests on a struct (`x >= r.x && x < r.x + r.w && ...`)** allocate
  registers differently from the same tests through an inline helper (`int Dentro(int x, int y)`):
  when hand-written tests are off by registers, try the helper.
- `[vc8]` **Two locals the image keeps apart** (frame bigger than ours by one slot) can be a
  variable declared at function scope and assigned later, rather than one declared in the inner
  block.
- `[vc8]` **Object files group a class's functions**: when a member compiles inlined where the image
  calls it (or the reverse), check which class's address range it sits in and put it in that file
  (or a header with an opt-out macro).
- `[vc8]` **Sweep the vtables for lone `jmp` slots.** List every unmatched vtable slot whose code is
  a single `jmp` (or `mov ecx,[ecx+N]; jmp`), resolve the target through the thunk table, and write
  each as a one-line override (`return Base::F(args);` or a call to a non-virtual helper). Dozens of
  slots fall out this way; a tail call with no `mov ecx` means the target uses the same `this` (a
  base member, or a sibling helper of the same class).
- `[vc8]` **A type whose destructor maps to another class's destructor is that class.** When a
  by-value return type's destructor resolves to an existing class's, rename it everywhere (and the
  mangled names in the symbol map); nothing else needs to change.
- `[vc8]` **`fnstcw; or 0xc00; fldcw; fistp qword; fldcw` inline** (instead of a call to
  `__ftol2_sse`) is VC8's conversion of a float to `unsigned int`.
- `[vc8]` **A field loaded into a register and tested, then used** (`mov ecx,[x]; test ecx,ecx; je;
  ... call [ecx]`) is a local (`T *v = s->campo; if (v) v->F();`); testing the member directly
  reloads it after the test.
- `[vc8]` **Temporaries in conditions:** a temporary created in `a && f().g()` lives to the end of
  the full expression, so VC8 guards its destructor with a flag (`or ebx,2; test bl,1`). When the
  image destroys it right after use without a flag, the test was its own statement (`bool hay =
  false; if (a) hay = f().g() != 0;`).
- `[vc8]` **A struct copied through a stack temporary as plain dword moves** (`fstp [tmp]; mov
  edx,[tmp]; mov [dst],edx` per field) is `*p = T(a, b)` with a constructor; assigning the fields
  directly stores them with `fstp [dst]`.
- `[vc8]` **All argument temporaries built before any is used** (`Token(1)`, `Token(0)`, then the
  numbers) means they were passed by reference to a `__forceinline` helper; arguments are evaluated
  right to left, the helper's body then runs in its own order.
- `[vc8]` **A guard bit and an EH state that belong to another function** (`test byte [guard],0x10`
  with `mov [esp+N],4` in a function that has one static) mean that function's body was inlined: VC8
  keeps the callee's static, guard bit and state number. Define the callee in the same file; a store
  in the shared tail of a switch that the inlined copy lacks belongs inside one case.
- `[vc8]` **`mov eax,[x]; sub eax,K; jne`** in place of `cmp [x],K; jne` is a `switch` with one
  case.
- `[vc8]` **A loop compare `jl` against the end of a global array** is an `int` index loop that VC8
  strength-reduced; a pointer loop compares with `jb`.
- `[vc8]` **A member reached through `this` the code never reads** (`ret 4`, `ecx` unused) is still
  a member function: thiscall and stdcall compile alike when `this` is dead.
- `[vc8]` **The argument order of a five-argument receiver** (`Mensaje(a, b, id, c, d)`): count
  which `[esp+N]` the id compare reads before assuming the id comes first.
- `[vc8]` **A destructor with an EH frame where the image has none:** the body calls something that
  can throw (a member function, plain `operator delete`). Declare `operator delete(void *) throw()`
  and call an in-file member for the cleanup.
- `[vc8]` **A derived constructor that stores the base's field before its vtable** (`[eax+4]=p;
  [eax]=vt; [eax+8]=k`) runs the base constructor with that argument (`Derivada(P *p) : Base(p) { k
  = ...; }`).
- `[vc8]` **A parameter loaded into a callee-saved register before an unrelated copy loop** and then
  advanced (`y += 35` each line) is a local copy: `int fila = y;` declared before the loop; using
  the parameter itself loads it late.
- `[vc8]` **`shr ecx,16; cmp ecx,K`** where `(x >> 16) == K` gives `and ecx,0xffff0000; cmp
  ecx,K<<16`: the source took the high word through a cast, `(unsigned short)((unsigned)x >> 16) ==
  K` (HIWORD).
- `[vc8]` **Sweep secondary vtables too.** A class with several bases has forwarders (`jmp Base::F`)
  in its second and third vtables that a primary-only sweep misses.
- `[vc8]` **A callback pushed as a plain address** (`push 0x5b5360`, no thunk) is a function without
  external linkage. The byte matcher skips file-static functions, so declare it a static member
  function; the bytes are the same.
- `[vc8 rtm]` **`ecx` used after a call without reloading** means VC8 knew the callee (in the same
  object file) leaves `ecx` alone. Reproducing it needs the callee in the same file; until then,
  park it.
- `[vc8]` **An EH table with more states than the code uses** is a static whose initialiser sits in
  dead code (after a `return`); a dead `static int x = f();` restores the table, not the registers.
- `[vc8]` **A double constant that is a float widened** (`0.001500000013038516` where the source
  obviously meant 0.0015) is a float literal in a double expression: `(double)0.0015f`. The bytes
  match either way; only the constant's address does not.
- `[vc8 rtm]` (Superseded: the switch's zero pushed as a register was the RTM compiler; see section
  1.)
- `[vc8]` **A member that the image's other functions inline** shows up as the same static (same
  guard address and bit) in several functions. Define it once in the class's file and call it from
  the others; VC8 inlines it the same way.
- `[vc8]` **A tiny or scratch file whose every match is also defined elsewhere** can be deleted:
  check by counting distinct matched addresses before and after.
- `[vc8]` **`xor eax,eax` before a search loop that leaves eax alone, and the not-found exit
  returning that eax**: a result variable set inside the loop and left with `break` (`int r = 0; for
  (...) if (hit) { ...; r = 1; break; } return r;`). Returning from inside the loop zeroes eax at
  the exit instead. When the loop also compares a field against that zero (`cmp [x],eax`), write the
  comparison against the variable (`e->hecho != r`).
- `[vc8]` **Sweep the whole image for trivial functions by their bytes, not just vtable slots.** VC8
  (no /OPT:REF here) keeps every out-of-line copy of inline members even when nothing calls it:
  getters (`mov eax,[ecx+N]; ret`), setters, `lea eax,[ecx+N]`, float reads, constant stores, empty
  bodies, `mov ecx,[ecx+N]; jmp T` forwarders, zeroing initialisers. Each shape has one source; a
  generator that walks the function starts (16-aligned after `int3` padding), skips what is matched,
  and emits one definition per address matched ~1000 functions on Commandos in one pass. Name the
  owner from the class code ranges and say it is a guess.
- `[vc8]` **Families of identical constructors** (the editor's per-action dialogs: a base
  constructor call with a per-class table, then three vtable stores) are worth a byte-pattern
  generator too; an implicit destructor is a bare `jmp` to the base's where a user-declared empty
  one resets the vptrs first.
- `[vc8]` **`setae al` (or another `setcc al`) returning a comparison where we get `sbb eax,eax; add
  eax,1`**: the result went through a `bool` local (`bool r = a >= b; return r;`); returning the
  comparison directly takes the sbb form.
- `[vc8]` **Template-instance copies of one member** (ListaObjetos<T>'s constructor, once per T):
  instantiate just that member with a throwaway type, `template ListaObjetos<T1>::
  ListaObjetos(uint32_t, uint8_t, uint32_t);`, which emits that COMDAT alone. If the copies inline a
  base constructor that our build calls instead, force it inline in that file only (`#define
  X_INLINE __forceinline` before the header).
- `[vc8]` **Find byte-identical duplicates of matched functions** (compare with `call`/`jmp` targets
  turned absolute): each needs only a second name; they show which classes share a template or an
  inline body.
- `[vc8]` **A destructor whose body calls something gets an EH frame in our build** (`push -1; push
  __ehhandler...`, the cookie) so that the base's destructor still runs if the call throws. The
  image has none: declare the callee `throw()` (or it is `extern "C"`, as `free`), and the body is
  the bare `push esi; mov esi,ecx; ...; jmp base`.
- `[vc8]` **A subclass destructor that stores its base's vtable, not its own**, is the subclass's
  empty destructor with the base's (empty, inline) one inlined: VC8 drops the dead store of the
  subclass's vptr. Give the base an inline `~Base() {}` and the subclass an out-of-line `~X() {}`.
  Find the owner by the deleting destructor right after it (it inlines the same body), whose address
  is in a vtable slot.
- `[vc8]` **A float wrapper calling into the CRT** (`fld [esp+4]; call X; fstp [esp+4]; fld [esp+4];
  ret`) is `(float)sqrt(x)` with the function made intrinsic (`#pragma intrinsic(sqrt, cos, atan2,
  pow)` calls `_CIsqrt`...); `fstp qword [esp]; call` is a non-intrinsic one (`floor`, `ceil`). Name
  the CRT routine by the string it passes to `_matherr` ("sqrt", "cos"); floor and ceil have none,
  but their x87 fallbacks load a control word with RC=01 (down, floor) or RC=10 (up, ceil).
- `[vc8]` **The byte matcher learns any unknown call target**, so a trial with the wrong CRT name
  still "matches" and records the wrong symbol. Identify a target before letting it be learned.
- `[vc8]` **Let a decompiler write the candidates and the compiler judge them.** For small
  functions, `kuna decompile-all --json --option structsynth off` (raw offset arithmetic, no
  synthesized structs) gives C that VC8 accepts after light rewriting: the convention comes from the
  bytes (ecx read before it is written: a member, Kuna's first parameter is `this`; `ret N`:
  stdcall; else cdecl), Kuna's `(**(void **)(*obj + off))(args)` and its two-step form become
  virtual calls on a stub class with `off/4` dummy slots, `sub_X(...)` calls get the callee's
  convention from the callee's bytes, `dat_X` become externs. Put all candidates in one file, drop
  what the compiler rejects (map error lines back to functions), byte-match, keep only exact
  matches; retry the misses with rewrites (Kuna normalises `x < 5` to `x <= 4`, and VC8 emits `cmp
  5; jae` vs `cmp 4; ja`). On Commandos this matched ~300 of ~1000 candidates (leaf functions ~35%)
  at no manual cost.
- `[vc8]` **Check that a rewritten call reads ecx, not just writes it:** `mov ecx,[ecx+N]` is a read
  of `this`.
- `[vc8]` **A default chosen with `test; mov ecx,K; je; mov ecx,eax`** is a local set to the default
  and overwritten (`const char *q = ""; if (p) q = p;`), not `p ? p : ""` (which branches around two
  loads).
- `[vc8 rtm]` **x87 comparisons follow the source's operand order**: `fld b; fld a; fcomp st(1)`
  with `test ah,5; jp` is `b > a` written that way (`w > limite`); `a < b` gives `fcom` with `test
  ah,0x41`. Likewise `x = x + v` and `x += v` both load `v` first in VC8 (a parked case where the
  image loads `x` first).
- `[vc8]` **Renaming a placeholder class to its real (RTTI) name turns on vtable checks**: the
  matcher then compares the class's vtable slot by slot, so placeholder virtual bodies (`void
  X::v1() {}`) must go.
- `[vc8]` **An inline function's out-of-line copy** (a COMDAT the image kept although every caller
  inlined it) is emitted by calling it from a throwaway helper under `#pragma inline_depth(0)`; the
  helper itself stays unmapped. Cheaper than moving the body out of the header (which would change
  every caller). Do it from another object file when the same file also needs the function inlined:
  once emitted, VC8 stops inlining it there (later calls and template instantiations call the copy).
- `[vc8]` **Near-duplicates**: mask every image address (calls, globals, vtables) and group
  functions by the remaining bytes; an unmatched member of a group with a matched one is usually the
  same source over another type or another object (a template instance, a copy in another class, an
  inline body kept twice).
- `[vc8]` **VC8 /O2 fully unrolls short constant loops** (six iterations of a compare-and-update,
  six 64-bit adds): a straight run of identical blocks in the image is often a `for` loop in the
  source, and writing it out by hand (or with a macro) allocates registers differently. A loop whose
  body calls a function is not unrolled.
- `[vc8]` **A member object whose destructor frees something shows up as an EH state** (state 1 → 0
  around the free) in the owner's destructor; the free belongs in the member's (inline) destructor,
  not in the owner's body. Such a member cannot live in a union, so give the owner the real member
  and rewrite direct field uses as `member.field`.
- `[vc8]` **When an inlined tail keeps a pointer in a callee-saved register** where yours reloads it
  from `this`, the source cached it in a local (`VistaMundo *v = vista; ... v->F(o)`); a value
  computed through a small static inline helper that returns a struct (`entero =
  Redondear(&origen)`) can schedule differently from two separate stores.
- `[vc8]` **Indexing vs reference**: `EntradaTecla &e = tabla[i]; e.tecla` and `tabla[i].tecla[0]`
  compile to different addressing; try both when a table walk differs.
- `[vc8]` **Pairs passed by value**: when a function of `(x, y, w, h)` differs only in register use
  (`lea edx,[ecx+ebp]` against `add ebp,ecx`), try the pairs as structs by value (`Punto p, Tamanyo
  t`): struct members are kept apart from scalars.
- `[vc8]` **A clamp that keeps both values on the x87 stack until the branch** is an inline
  `max`-style helper (`return x < y ? y : x;`) rather than an `if`.
- `[vc8]` **A zero kept in a register across a search loop and returned on failure** is a result
  variable: `int r = 0; for (...) if (hit) { ...; r = 1; break; } return r;` (or `EntradaX *r = 0;
  ... { r = e; break; } return r;`). An early `return 1` / `return e` from inside the loop makes VC8
  materialise the zero only at the exit.
- `[vc8]` **Explicit instantiation (`template class X<T>;`) emits every member, inline ones
  included**: the out-of-line constructor copies a template class leaves in the image come from
  there, not from separate code.
- `[vc8]` **Three members vs an array of three**: a constructor that initialises `T a, b, c;`
  schedules its stores differently from `T x[3];` (the array becomes a loop VC8 unrolls); try both.
- `[vc8]` **A local text buffer without a `/GS` cookie** was not a `char` array: VC8 puts the cookie
  on char buffers (and structs of them); a struct of ints used through a `char *` cast (`struct {
  int t[8]; }`) gets none.
- `[vc8]` **Stores that interleave with other work, or an `xor ecx,ecx` + integer stores placed
  before the first `fst`**: member sub-objects with constructors, run in declaration order (`struct
  ParCero { int a, b; ParCero() { b = 0; a = 0; } }; struct TresCero { float x, y, z; TresCero() { z
  = 0; y = 0; x = 0; } };` as members). A flat list of assignments is scheduled differently. Reverse
  stores (z, y, x) in a constructor are the signature of such a member.
- `[vc8]` **Small functions the accessor generator missed** are often one of: set-and-return-this
  (`mov eax,ecx ... ret N`), a struct returned through a hidden pointer (`mov eax,[esp+4]` before
  any other load; write `T F() { T r; ...; return r; }`), a `movzx` return (declare the return
  `unsigned`, not `unsigned char`), or an `abs`/`cdq` idiom (`#pragma intrinsic(abs)`).
- `[vc8]` **`tools/symsync.py FILE` also defines symbols** from the `// 0xADDR` comments, so a batch
  of small structs needs no hand-written mangled names.
- `[vc8]` **Forwarders.** (1) A forwarder that loads its first stack argument
  into `edx` (not `ecx`) and then calls a thiscall target without touching `ecx` is a **member**
  passing `this` on: declare it as a member and call the target as another member, not a free
  function. (2) `xor eax,eax` before the null test of a chased pointer, then `jmp target; ret` =
  `int r = 0; T *o = ...; if (o) r = o->G(); return r;` (the compiler still tail-jumps). (3) `if (n
  > 0) { n--; return; } Base(x);` with the `jmp` out of line is a plain thiscall member (a
  `__stdcall` member moves `this` to the stack and breaks it). (4) A "scalar deleting" function with
  no vptr store is a hand-written `T *Destruir(unsigned f) { delete[] p; if (f & 1) delete this;
  return this; }`, not a compiler destructor (`gendestructores.py` has the shape). (5) `Base(flag |
  4)` conditional on a field: `Base(campo == 0 ? f | 4 : f)` matches, `if (...) f |= 4;` does not.
  (6) Check float constants in `.rdata` before guessing (0.75f, 255.9f were the "1.0f" I assumed).
- `[vc8]` **More forwarders.** a virtual call that pushes the object itself
  (`push eax; call edx` with `ecx` never set to it) is a `__stdcall` slot (COM style): declare
  `virtual R __stdcall M(args)` and write `obj->M(args)`. `mov ecx,[this+X]; test ecx,ecx` right
  before a call means a local copy (`T *q = p; if (q) q->M();`), not two reads of the field.
  Arguments reordered through `esi` (`mov esi,[esp+8]` after `push esi`) reveal which stack slot is
  really first; count the shift before naming parameters. A memset after the field stores that does
  not return `this` is a plain void member, not a constructor.
- `[vc8]` **X87 compares.** `fldz; fcomp [m]; fnstsw ax; test ah,0x41; jp L; mov
  eax,1; ret; L: xor eax,eax; ret` is `m >= 0.0f` (the operands are loaded in the opposite order: `0
  <= m`), not `== 0`; read the flag logic (PF after `test ah,0x41` is set when C0 and C3 are both
  clear or both set) before choosing the operator. `while (p && *p)` (the null test inside the loop
  condition) produces a loop whose back edge is the flags of `add ecx,4`; `if (p) while (*p)` does
  not.
- `[vc8]` **By-value structs.** a struct passed **by value** that the image
  builds with `sub esp,N; mov eax,esp; mov [eax],ecx; mov [eax+4],edx` (loads from the source first,
  then the stores) needs a **user-declared copy constructor** (`S(const S &o) { a = o.a; b = o.b;
  }`); without it the compiler pushes the fields directly. Put the `// 0xADDR` comment on the
  function that uses it, not above the struct, or symsync names the copy constructor with that
  address.
- `[vc8]` **Destructors and statics.** (1) A hand-written destructor that calls
  out gets an EH frame unless the callees and the global `operator delete` are declared `throw()`
  (`void __cdecl operator delete(void *) throw();` at file scope works). (2) When the image's
  callers reach a function through an incremental-link thunk, map the name to the **thunk** address;
  mapping the body instead breaks every caller's relocation check, so a body only reached through
  its thunk stays unmapped. (3) A function-static object (`static T s;` with a constructor) emits
  the object and its guard word as `?s@?1??F@@...@4...A` and `?$S1@?1??F@@...@4IA`; map both to the
  image's data addresses. (4) Action constructors without an EH frame (base call, vtable, fields)
  match with a stub base `struct Base { Base(args); virtual ~Base(); }`; a float triple zeroed z, y,
  x is a member whose constructor does `z = 0; y = 0; x = 0;`.
- `[vc8]` **Where the vtable store lands.** VC8 schedules plain field stores of
  a constructor body across the vptr store, so the vptr's position is a clue to the source. A field
  stored **before** the vptr comes from a base constructor: if the class has no other base, it is a
  non-virtual base (MSVC still puts the vfptr at offset 0) whose inline constructor sets it. A pair
  zeroed `b, a` right after the vptr is a member with its own constructor; writing the same zeros in
  the body moves the vptr after them.
- `[vc8]` **EH-framed destructors.** members with destructors are what the EH
  states number (the body runs under the highest, each member is destroyed after the state drops);
  declare the member types with their destructors at the right offsets and the frame follows. A
  `delete[]` of an array whose elements have a destructor is the image's `eh vector destructor
  iterator` call (count at `p[-1]`, size and destructor pushed). A freed buffer pushed through `eax`
  is `delete[] (char *)p`; the same call through `ecx` is an `extern "C" free`. A cached object
  pointer reloaded around an import call (`BringWindowToTop(owner->hwnd)` in `eax`) wants a local
  for the owner.
- `[vc8]` **Inline destructors in EH frames.** the unwind funclet of an
  EH-framed destructor calls the out-of-line copy of each inline base destructor, and the image can
  hold several identical copies of one (no COMDAT folding: `~ClassLineaVariable` at 0x5a67f0 and
  0x5a6b10). Give a second copy a distinct class name with the same vtable address so its call is
  inferred, not checked against the other copy. A comparison tool that infers callees from
  relocations must follow incremental-link `jmp` thunks there too.
- `[vc8]` **Small shapes.** (1) A bounds check the image repeats right after a
  guard (`cmp i,n; jae` twice) is an inline accessor `T *Elemento(i) { return i < n ? t[i] : 0; }`
  called twice (`if (l.Elemento(i)) l.Elemento(i)->F();`). (2) Taking the address of a pointer
  parameter (`&s + 1` to reach varargs) makes /GS copy the parameter; declare the function variadic
  (`...`) and the copy goes away. (3) A function that ends `mov eax,1` after side effects returns
  int even when its header said void: fix the declaration and the mangled name. (4) `xor eax,eax`
  before the first guard is the `int r = 0; if (...) { ...; r = 1; } return r;` shape (again).
- `[vc8]` **Same-TU knowledge and copies.** (1) VC8 knows the register use of a
  callee **defined earlier in the same TU**: two member calls in a row then skip the second `mov
  ecx,esi`, and a short callee gets inlined. When the image reloads `ecx`, or calls what you
  inlined, move the callee to another file (or `#pragma auto_inline(off)` around it). (2) A size
  copied as `mov edx,[eax]; mov [ecx+20],edx` right after `cmp [eax],0` (no reuse of the compared
  value) is a struct assignment (`tam = *t; tam2 = *t;`), not member-by-member copies. (3) An
  element read twice (`t[i]->id`, then `t[i]->nombre`) instead of through a local gives the image's
  register order; so does rereading through the cursor (`T **c = it.cur; if (*c) ...`, the image's
  `mov ecx,eax`). (4) Which block comes first after `if (a || b)` follows an inline predicate: `if
  (!t->Nulo())` placed the call block first where the plain condition did not. (5) A loop that ends
  in `jl` over a table is an index loop (`for (int i...) &g[i]`); the pointer loop gives `jb`. (6) A
  `while (Read(...)) { if (!Use()) { r = 0; break; } ... } return r;` keeps `r` in a register where
  an early `return 0` folds it.
- `[vc8 rtm]` (Superseded: the RTM unsolved list; see sections 1 and 12.)
- `[vc8]` **Inlined copies and struct shapes.** (1) When a small function is
  both out of line and inlined into callers, the inlined copies can need a different source shape
  than the standalone one (field order of a temporary box, add operand order): give the callers
  their own inline helper rather than forcing one shape on both. Brute-forcing the 24 orders of four
  field initialisations is cheap and finds the one that fits. (2) A temporary built by `new T(args)`
  with no EH frame in the image means the constructor is `throw()` or trivially inline; a byte-wise
  copy of a struct argument (`mov edx,[eax]; mov [esi],edx` x4) is a member copy-constructed from it
  (`caja(*c)`), not four field assignments. (3) Function-static objects need both the object and its
  `$S` guard mapped; read the guard name from the listing (`?$S1@?1??F...@4IA`). (4) Float literals
  must have the image's exact bits: read the constant and write it with nine significant digits
  (`316.199982f`, not `316.2f`). (5) A conditional forwarder (`if (x) return x->F(a, b, c); return
  0;` as `jmp eax`) needs the callee's slot to return a value: fix the slot's declared return type.
- `[vc8]` **Small closed functions.** (1) A point cleared through a temporary
  (`fldz; fst [esp]; mov eax,[esp]` ... then integer stores) is `T t(0,0,0); p = t;` with the
  **implicit** copy and assignment: the user-written float-wise copy folds the temporary away, the
  default one copies through integer registers. (2) A function's size from a gap list can be short
  when a `je` leaves past its first `ret` (cold tail laid out after it): check the jump targets
  before writing a "small" body. (3) A mislabelled neighbour name (`near2`) is only the nearest
  symbol: read the bytes, never trust it for the class.
- `[vc8]` **Reading the listing before the compiler.** many "register family"
  parks were misreads. (1) Read the exact jump targets: a `je` that lands on `mov eax,1` is a
  *return 1* branch, not the fall-through. (2) Float compares: `fld a; fld b; fcompp; test ah,0x41;
  je` is `b > a`, and `test ah,5; jp` is a strict `<`/`>`; the order the two values are loaded
  follows the operand order (`!(p->tope < limite)` loaded tope first where `limite <= p->tope` did
  not). Strict vs non-strict bounds show up as 0x41 vs 5. (3) `*out = n ? v : 0;` as one ternary
  store gives `xor eax,eax; mov [edx],eax` on one path with a shared return; if/else stores do not.
  (4) `eax` carrying `this` from the first instruction to the end (`mov eax,ecx` ... no `mov
  eax,esi`) means a constructor, even without a vtable store. (5) A `ret N` with no `ecx` use is
  `__stdcall` (declare it so); a hidden extra argument plus `eax` holding it at the end is a struct
  returned by value. (6) For a callee that is only a `jmp` thunk with no symbol yet, declare it
  (free `__stdcall`/`__cdecl` function or member) and add one `symbols.tsv` line for the real
  address; do not add a second name for an address that already has one. (7) A saving helper
  `Hxxx(guardar, obj, f)` is `ES(guardar, f, p, n)` plus `H_4039f9`/`H_40b672` calls on sub-objects:
  they match with the existing macros; look for them by their prologue (`push ebx; mov ebx,[esp+8];
  test ebx,ebx`). (8) `while (x) { if (x==c) return s+1; x = s[1]; s++; } if (*s==c) s++; return s;`
  is the "after the first c, else the end" scan; the end pointer, not null, is returned. (9) A
  function ending `jmp edx` after a virtual slot load is a tail call; the `-1` store before it goes
  through if the statement order is `pos = -1; Soltar();` only when the compiler schedules it so —
  otherwise it is a scheduling park.
- `[vc8 rtm]` **Gap survey of 16-130 byte functions.** (1) `eax=ecx` as the
  first instruction means a constructor even for a table fill (`Bits45a480`: a 256-entry bit-count
  table) and the function-local `static` of that type gives the `test [guard],1 / or / call ctor`
  prologue. (2) A switch whose jump table is **out of case order** (cases 1,2,3,4 call slots
  0,3,1,2) is written with the cases in *code* order (1,3,4,2) and the bodies call the slots the
  table names; read the table bytes, not only the listing. (3) `(float)u` of an unsigned adds the
  float constant 2^32 (`fadd dword`); `(double)u` adds a double (`fadd qword`): a 4-byte constant in
  the image tells you the cast. (4) A loop over an address held as `int` (`for (int p = (int)tab; p
  < (int)(tab+5); p += 4)`) compares **signed** (`jl`), a pointer loop compares unsigned (`jb`); it
  does not help the opposite case (`jb` on an index loop stays `jl`). (5) A local float for a sum or
  root (`float s = x*x+y*y; (float)sqrt(s)`) gives the image's `fstp/fld` spill before the call; the
  whole expression in one line does not. (6) A 4-float point built from a temporary `P(a+b, ...)`
  assigned to a member sub-struct gives the integer copy through `esi`; storing the fields directly
  does not. (7) A tail `jmp` needs the callee to take exactly the caller's stack arguments and the
  caller not to keep them in registers; when the arguments are cached in `edi/ebx` the compiler
  pushes copies and calls instead (parked, `Contar62fcf0`). (8) Factories `new T(a, b, 14, c)` and
  destructors of two members with destructors match first try once the constructor/destructor
  symbols exist; the exception frame is generated.
- `[vc8 rtm]` **Checked-iterator list walkers.** the engine's list class walks
  with `x = *cur; x != -1 && (cur-beg) < count`. (1) Initialise with the chain `it.beg = it.cur =
  datos` when the image keeps the base in a register and with two statements `cur = ...; beg = ...`
  when it spills it. (2) A counter that the image zeroes after the null-test join is declared after
  the iterator setup. (3) Write the key first (`k == it.x->field`) to get `cmp reg,[mem]`. (4) The
  inline list destructor and Vaciar are explicit instantiations of the template (`template
  ListaObjetos<T>::~ListaObjetos();`) plus a symbol line, which is one line each when the element
  type already has its cursor instantiation. (5) Records that own a heap string need a record type
  with an inline `~T(){ operator delete(s); }` so `delete it.x` frees both. (6) A recursive release
  through the incremental-link thunk is a call on a second struct type whose method has its own
  symbol line at the thunk address. (7) When the image shows exactly three stack slots (list,
  cursor, base) and ours four (an element field), no source form found so far fixes it (parked:
  Mandar63d460, EnPunto).
- `[vc8 rtm]` **Small-function sweep, part 2.** (1) "Does any entry pass a
  callback" loops (`Alguna`): write `int r = 0; for (...) if (a[i] && f(dato, a[i], &b[i])) { r = 1;
  break; } return r;`; an early `return 1` inside the loop compiles to a different shape. (2) When
  the listing shows a byte or word load from the middle of a field (`movsx esi, word [ecx+0xa]`),
  the field is fixed-point and the code reads its integer half: read it through a `short *` cast at
  that offset. (3) A struct that is off by one word shows as a different displacement in the first
  mismatching `mov`; compare the machine-code bytes of the object and the image (find the
  instruction bytes in both) rather than the assembly text. (4) VC's inline `strlen` (`lea
  esi,[eax+1]` ... `sub eax,esi`) and byte-copy loops are not reproduced by hand loops or by the
  library calls; park these. (5) A function-local `static` of a struct type with a constructor gives
  the guard-bit prologue and the constructor's store order (a chained `x = y = 0` stores y first);
  its data symbol may be placed by the sync tool on the second word, so fix the symbol line by hand.
  (6) A repeated pointer step of the same size in three registers (index, byte offset, pointer) is
  merged by the compiler into one when the source steps them by the same stride; the image keeps
  three, which no source form produced.
- `[vc8 rtm]` **Script-reader `Leer` functions.** (1) The config readers are
  `ClassMissionScriptBlock v = b->Valor("KEY"); if (v.Cuenta()) field = (int)v.Numero();` (float
  fields store without the cast). Count the frame: if every `Valor` result has its own slot, keep
  the named locals in one scope; if the image reuses two or three slots, put each key in its own
  braces. (2) When the image passes the returned eax straight to a helper, write
  `&(ClassMissionScriptBlock &)b->Valor("KEY")`; when it keeps the returned block, a named local.
  (3) The base-class `Leer` is called first or last exactly as the image does; `Base::Leer(b)` with
  a char-padded base struct is enough. (4) `unsigned n = v.Cuenta(); new T[n]` gives the `mul; seto;
  neg; or` size guard; a signed count does not. (5) Two floats read in one expression,
  `Par(e.Token(0).Numero(), e.Token(1).Numero())`, create the temporaries in the image's order where
  two separate statements do not. (6) A by-value struct with a user copy constructor passed to a
  call: keep the source as two `int`s written in the image's store order (`xy[1] = 0; xy[0] = 0;`)
  and pass `*(Punto *)xy`; a `Punto p(0,0)` local stores them in the other order. (7) Functions
  whose first instructions use esi/edi/ebx before setting them are register-argument (LTCG)
  functions: park them.
- `[vc8]` **Inline fistp.** `fnstcw; or 0xc00; fldcw; fistp qword` in an
  otherwise normal `__ftol2_sse` function is not other compiler flags: it is a float converted to
  **unsigned** (`(unsigned)v.Numero()`, or an `unsigned` field). Also: `neg/sbb/add 1` is `int x =
  !cond` (a `bool` gives `sete`); a local array of shorts adds a /GS pad, a struct of three shorts
  does not; a char buffer's size can be read from the frame (`sub esp`) difference.
- `[vc8 rtm]` **The aligned frame.** a prologue `push ebp; mov ebp,esp; and
  esp,0xfffffff8` in an otherwise ordinary function means a local that needs 8-byte alignment, not a
  different compiler setting. The original source copies a value into a small struct (even a single
  pointer, or two ints) that the compiler aligns to 8: `struct __declspec(align(8)) Saved {
  ItemRender *i; };` filled at the top and read back after a loop reproduces the frame and the
  stores (`mov [esp+0x14],eax`), and the function then matches (0x5690b0 and its fastcall twin
  0x5aa0f0 matched first try). Look for it whenever a parked function starts with that prologue.
- `[vc8]` **Inline string-compare table searches.** `strcmp` as an intrinsic
  compiles to the unrolled byte compare ending `sbb eax,eax; sbb eax,-1`. For a search loop over an
  array of records, write the exit the way the image lays it out: if the image has `return 0` (the
  pops and `xor eax,eax`) *before* the found block, put the found work after the loop (`for (...) {
  if (hit) break; }  if (i < N) { ...found...; return x; }  return 0;`); two identical found-returns
  in the image are one `goto hallado` after the `return 0`. A name checked against `i < cuenta ?
  datos + i * size : 0` (an inline checked accessor) reproduces the image's index guard. When a hit
  jumps over a call (`if (!ok) goto skip`), keep the call after the block instead of duplicating it.
- `[vc8 rtm]` **By-value fetch chains, state-name copies, single-exit results.** (1) A callee that fills a hidden pointer and returns it (`mov eax,esi` after the call) is a method
  **returning a class by value**; chaining `a.F(i).F(0).G(...)` gives the image's temporaries (EH
  states 0,1, destroyed in reverse) and its use of the returned `eax` as the next `this`; the
  class's destructor is a bodiless virtual `~T();` mapped to the real address in the symbol table,
  and an unwind funclet that calls a linker thunk means exactly that (the local's destructor is the
  external function itself). (2) Wrapping a local whose constructor is a call that fills it: a
  struct of `int raw[N]` (a `char` array adds a `/GS` cookie) whose constructor calls the fill
  method. (3) A 25-byte state name copied into a local is the `strcpy` **intrinsic** once per case
  (`#pragma intrinsic(strcpy)`), not `memcpy` of a chosen pointer; declare the name buffer before
  the output buffer. (4) `int r = 0; if (a && b) {...; r = 1;} return r;` (one exit) fixes many
  "zero kept in a register / early returns duplicated" misses; where only some paths call something,
  those return directly and the rest fall through to `r = 1`. (5) A by-value struct of `int w[8]`
  filled by `memmove` (a real call; `memcpy` is inlined) avoids the cookie. (6) A callee defined in
  the same TU is inlined: keep helpers (`Guardar`, `Reiniciar`, `Destruir`) in another file from
  their callers. (7) A plain function that sets its own vtable and ends with the base destructor's
  call (no EH frame) is a member calling `this->Base::~Base()` explicitly, not a real destructor.
  (8) `volatile int x = 0;` reproduces an unused zero stored into a stack slot (an exception-state
  marker in the original). (9) The vector-destructor helper takes element size, then count; a
  deleting destructor that calls it needs the destructor called explicitly and `operator
  delete(this)` (a plain `delete this` expands the destructor twice). (10) A comparison `cmp ebp,1;
  ja` after a `Cuenta()` result needs the count held in an `unsigned`. (11) Class names must be the
  originals (docs/classes.tsv) for any class whose RTTI data is compared. Unsolved: a frame-less
  constructor that builds a base with a destructor (VC8 /EHsc adds the frame; only `extern "C"`
  callees are non-throwing).
- `[vc8]` **The C++ library the game links in.** standard-library functions
  match first try once the class has the right shape, and they are cheap. (1) The RTTI name tells
  `class` from `struct`: `ctype_base` is a struct (`.?AU`), a class gives a different type
  descriptor and the data compare fails with "1 byte differs". (2) An inline base destructor that
  the image does not call needs `throw()` and an in-class body (`virtual ~facet() throw() { }`);
  without `throw()` the derived destructor gets an EH frame, with an out-of-line one it tail-calls
  the base. (3) `basic_string<char>` as a template with `_Myres`, `_Mysize`, `_Bx{_Buf[16],_Ptr}`
  (`_Alval` pad first) plus `template class basic_string<char,...>;` reproduces `_Eos`, `_Grow`,
  `_Tidy`, the `(const char*, n)` constructor (`_Myres = 15; _Eos(0);`) and `assign(const char*,
  n)`; write `_Myptr()` as `16 <= _Myres ? _Ptr : _Buf` (the pointer block comes first). (4) A call
  `call 0x40xxxx` to an incremental-link thunk and a call straight to the body are **different
  addresses for one name**: map the thunk under its own member name (`Asignar40ddc3`) or the
  neighbours that call the body stop matching. (5) A callee taking `const char *const &` that is
  passed the address of the caller's own parameter means the parameter is by value (`bad_alloc(const
  char *m) : exception(m)`). (6) A hidden-return-pointer function that ends `ret 4` is `__stdcall`.
  (7) `ungetc`/`fgetc` take the byte as `unsigned char` (`movzx`). General: windows, DirectDraw and
  surface code (`CrearSuperficie`, `Activada`, `Alternar`) matches when the early-failure block sits
  at the end (`if (r == 0) { ok } else { log; x = 0; } return x;`), a method that the image calls
  out of line is `__declspec(noinline)`, `i++` and `n++` inside the call arguments reproduce
  `imul`/`add` ordering, and a loop that copies a name with two pointers stepping source first is
  written as a `do { ch = *s; *d = ch; s++; d++; } while (ch);` (the `strcpy` intrinsic gives the
  other register layout).
- `[vc8 rtm]` (Reverted by the owner: standard-library instances by scan; see section 1.)
- `[vc8 rtm]` **Small guard-action functions.** (1) `char buf[256] = "NOINFO";`
  is the 7-byte copy from the literal plus a clear of the remaining 249 bytes (a `memset` call):
  write the initialiser, not strcpy. A debug text built as `strcpy(buf, "Estado: "); switch (state)
  { case n: strcat(buf, "NAME\n"); }` with the intrinsics declared (`#pragma intrinsic(strcpy,
  strcat)`) reproduces the unrolled copies. (2) When the image loads a field into a register before
  a float argument and ours loads it after, copy it into an `int` local first (`int d = dato20; ...
  Mensaje(id, 0, a, d)`). (3) Compare with `> 0` instead of `>= 1` when the image keeps no constant
  1 in a register; and `a != 1` then `a - 3` in an image is a `switch` whose cases follow the
  image's block order. (4) A message built from a copy of a record (`Par m = *obj->par; m.a = 0; m.b
  = 0;`) keeps the loads of the original fields in the image: copy, then clear. (5) A sine over a
  period: `sin((double)(t % p) * 6.283185307179586 / p) * amp`, stored in a `float` then sent
  through the first virtual slot of the object. (6) A dead store to an address-taken local array
  (`v[2] = x; ... v[2] = y;`) stays in the image: write all the stores in the image's order. (7)
  Parked classes of misses: a flag store scheduled after the pushes of a call's arguments, `xor
  reg,reg` for a stored zero where ours uses an immediate, the zero return hoisted to the top. (8)
  Several classes share one message pair (interest told once, watch off); once one class's version
  matches, the others are the same text with other offsets: look up the sibling classes by the
  message string's references in the image.
- `[vc8 rtm]` **Register arguments are `__fastcall`.** a function that reads
  `edx` (or `ecx` and `edx`) before writing it and ends with a plain `ret` takes its first two
  integer arguments in `ecx`/`edx` and is `__fastcall`: declare `int __fastcall F(int unusedEcx, int
  id)` and it matches (0x5d6200). A sign test `test edx,edx; jns` is `if (id & 0x80000000)`, not `id
  < 0` (that gives `jge`); keep a masked copy in a separate local (`int v = id; ...`) to get the
  image's `mov eax,edx; lea ecx,[eax-0x3f]` order. Functions that read `eax` or `esi` first are not
  fastcall (LTCG-style custom registers): those stay parked.
- `[vc8]` **Hand written assembly.** the image contains routines that are not
  compiler output: a plain `push ebp; mov ebp,esp` frame (no `and esp`, no security cookie, no EH
  `push -1`), `ret N` after explicit pops, scheduled loops with `test`/`movsw`/`dec ecx; je`. They
  match as inline `__asm`: the compiler's own prologue gives the same `push ebp; mov ebp,esp; push
  <used callee-saved>` and epilogue, so write only the body (arguments as `[ebp+8]` ..., `ret`
  dropped, the `__stdcall`/`__cdecl` chosen by the `ret N`). Rules found: operands of `test r1,r2`
  come out swapped by the assembler (write the image's order reversed); absolute data must be named
  globals (`extern "C" int G7d2b54;`), not `ds:0x...` (that adds a `3e` prefix) nor `[0x...]`
  (operand error); calls go to `extern "C" void F6214c0();`; an extra `push ebp` / `pop ebp` pair
  inside the body (ebp used as a free register) stays and only the final pops of the frame are
  dropped; when the original pushes a register itself that the compiler would also save (a second
  `push ebx` mid-function) use `__declspec(naked)` and write the whole function including `push ebp
  ... pop ebp; ret N`. `tools/asmgen.py ADDR NAME` (and `NAKED=1`) generates it from the image; keep
  only byte-exact results. Do not use this for compiler-generated functions (`and esp,-8`, `push -1`
  EH frames, security cookies): that would copy bytes, not reconstruct source.
- `[vc8]` **Scope.** the C++ standard library and the CRT inside the image are
  **not** transposed (owner decision); the library matches made earlier (iostream/locale/string
  instantiations, `tools/stdscan.py` hits) were removed again and do not count. The entries above
  about instantiating std classes are history, kept only for the lessons about `??_D` vbase
  destructors and probing with real headers if a game ever needs a std function written. Count only
  the game's own code.
- `[vc8]` **Float spills and compares.** when the image computes a float, stores
  it to the stack (`fstp [esp+4]`) and reloads it for the compare (`fld [esp+4]; fld other; fcompp;
  test ah,0x41`), a plain `float` local stays in the x87 register: declare the local `volatile
  float` to get the store/reload pair (it also grows the frame to the image's size). The direction
  of the compare then decides the load order and the jump: write `if (!(dd < other)) return;` when
  the image loads the local first and branches with `jne` after `test ah,0x41`; `dd >= other`,
  `other <= dd` and `!(other < dd)` give the other loads or `jnp`. Brute-force the 3-4 spellings.
  Also: a branch that the image lays out last (the flagged case at the end, the equal case after the
  not-equal one) is written as the `else` of the first test, not an early return. Volatile
  stores/loads also pin the order of neighbouring plain loads (the three-word copy before the first
  float load).
- `[vc8 rtm]` **STANDING RULE (Commandos, owner decision): ignore the C++ standard library and the
  CRT.** Do not port, match or count `std::` functions (string, locale, ios_base/iostream/fstream,
  vector/map/list instances, `char_traits`, `_Copy_s`/`_Move_s`, checked iterators, `bad_alloc`,
  `runtime_error`, facet/ctype) or CRT/runtime code (`sprintf`, `fgetc`, `__ftol2`, `_findfirst`,
  the code above the game's last class around 0x650000). In the gap lists skip them: in Commandos
  the library sits mostly in 0x4b54a0..0x4ba900 (plus iostream pieces near 0x58..-0x5b1) and
  0x650000 up; the generators mislabelled its functions with stand-in names (`NotifyMaster1`, `Vp_`,
  `Vd_`, `CopiarSeguro`, `ios_base_std`) so check the address, not the name. Library entries that
  sneak into `symbols.tsv` or the sources must be deleted, not parked, and never counted. Only the
  game's own code is reconstructed.
- `[vc8 rtm]` **Automatic variant search.** a parked near-miss is often one
  source spelling away, so search for it by machine. `tools/variants.py` (run from reconstruct/ with
  `python3 -I`, otherwise tools/dis.py shadows the stdlib) takes every `// 0xADDR ~` function,
  applies mechanical rewrites (swap two adjacent simple statements, mirror/negate a comparison,
  `volatile` on a float or int local, `int`<->`unsigned`, `x == 0` <-> `!x`), compiles each variant
  with the original compiler in parallel, byte-compares only that function, and runs a 3-round beam
  (`VROUNDS`, `VBEAM`) from the best partial scores. With `--apply` an exact match is written back
  and unparked. On 390 parked Commandos functions it matched 6 in about five minutes (statement
  swaps and `volatile` floats), where hand retries had failed. Run it after every batch of new parks
  and whenever a new trick (a new rewrite) is added to the tool.
- `[vc8]` **Missing return.** when a null-guard path jumps straight to the final
  `ret` with `eax` still holding the tested pointer (no `xor eax,eax`, no store), the original
  simply fell off the end of the function (`if (t) return ...;` and nothing after). VC8 compiles
  that with a warning and leaves `eax` as is; an explicit `return 0` or `return (int)t` changes the
  layout. Also fold the in-range tests into the ternary of the one return (`return f = (a && b) ? x
  : 0;`) to get `xor eax,eax; mov [f],eax` for the else arm.
- `[vc8]` **Constructors.** (1) VC8 devirtualises `slot()` calls on `this`
  inside a constructor; when the image does `mov edx,[esi]; call [edx+0x34]` (a real virtual
  dispatch), write `ClassX *q = this; q->slot();`. (2) A zeroed pair that the image stores after the
  vtable and before the EH state (`mov [esp+0x1c],ebx`) is a member with its own inline constructor
  (`struct P { int x, y; P() { y = 0; x = 0; } }`), not statements in the body. (3) When a vtable's
  data is emitted in the new object, the compare checks every slot: if a slot's name maps to another
  (identical) function's address in symbols.tsv, add a second line for the name at the image's slot
  target. (4) A derived constructor that stores two vtables (base then own) and has the base's field
  inits between is a base class with an inline constructor, not a call.
- `[vc8 rtm]` (Superseded: "plain `ret` with `ecx` is a dead end"; see sections 1 and 13.)
- `[vc8]` **EH-free destructors.** a destructor the image has with no `fs:0`
  frame, that sets a vtable, frees a field and calls a base destructor, must be written with raw
  field stores (`vtabla = 0x...; operator delete(datos); f = 0; Base();`) in a plain struct.
  Modelling the same thing as a member with its own inline virtual destructor (or a base class with
  a non-`throw()` destructor) makes VC8 add an unwind frame, and then it is also not inlined into
  the vector deleting destructor.
- `[vc8 rtm]` **Twin groups and open families.** (1) group every unmatched
  function by its masked bytes (calls and absolute addresses zeroed): a group of two or more is one
  source shape, and one match pays for all (the brightness-from-angle pair, the text-cut-at-`\n`
  pair came this way). (2) Open families, each seen 3+ times, not solved by statement order or
  declaration order: the derived vtable store lands after the first field stores (ours) instead of
  right after the base call (the image); the byte flag/result kept in a different register (`edx` vs
  `eax`); a loop's list/count registers swapped (`edi`/`esi`); the full-buffer branch laid out after
  the epilogue. Note them and park quickly.
- `[vc8]` **Small-method tricks.** (1) a chain of `sub eax,0 / sub eax,2` tests
  in the image is a `switch`; an if/else chain with the same cases compiles differently, so write
  the `switch`. (2) A zero kept in a register through the whole body is reproduced by `int z = 0;`
  that is also the return value (`z = 1;` on the success path, `return z;`), not by a plain `return
  0`. (3) A by-value struct argument whose floats are pushed straight from the source object wants
  the call written `f(*(Struct *)ptr, ...)` with a copy constructor, not a named local. (4) When a
  member has an inline destructor the image's unwind funclet may `jmp` to a thunk; declare the
  destructor out of line and map its mangled name to the thunk address. (5) A member's inline
  constructor that zeroes fields followed by an explicit call in the owner's body (state stored
  before the call) is `Owner(){ sub.Init(); ... }` with `Sub(){zeros}`, not the call inside `Sub()`.
  (6) `memmove` and `strncpy` share the image address 0x651a70 in Commandos; a `memcpy` that will
  not link-match is usually `memmove`. (7) `a > b` and `b < a` can compile to different operand
  orders for float compares; try both before parking.
- `[vc8]` **More small-function tricks.** (1) a function with a plain `ret`
  whose only stack argument is read from `[esp+4]` and that never touches `ecx` is a `__cdecl` free
  function (a factory template `template<class T> void *__cdecl F(int a){ return new T(a); }`
  matched four of them), not a method. (2) Debug-text `InfoEstado` methods have an unused 8-byte gap
  between their two locals in the image: size the first buffer up (`char n[12]`, `char n[0x100]`)
  until the frame size matches. (3) A float conditional the image stores and reloads is `float t =
  cond ? 0.0f : f(); float v = t;`. (4) A `const float` global lets VC8 load the constant once into
  st0 and reuse it; a non-const global is reloaded. (5) For a stack frame that is bigger than the
  visible locals, copy a short string literal through a struct (`struct T25{char c[25];}`) into a
  larger local array.
- `[vc8]` **Allocator aliases.** VC8 `new T[n]` of a plain struct calls
  `operator new[]` (`??_U`), but a binary linked with identical-function folding may call the same
  code under the plain `operator new` address (0x651953 in Commandos) while real counted arrays use
  another (0x65014a). The `symbols.tsv` map is name to one address, so give that struct its own
  class-level `static void *operator new[](unsigned)` and map `??_U<Struct>@@SAPAXI@Z` to the
  image's address; the overflow guard (`xor ecx,ecx; mul; seto cl; neg ecx; or ecx,eax`) then comes
  out right, which a hand written `operator new(-(hi != 0) | lo)` does not (it emits neg/sbb).
- `[vc8]` **Debug-text builders.** the "InfoEstado" family (a text built for the
  debug window, then `Texto(title, name, buf)`) follows one recipe. A literal copied into a local is
  a struct assignment (`struct T9{char c[9];}; *(T9*)buf = *(const T9*)"ESTADO: ";`); a literal
  appended with `strcat` under `#pragma intrinsic(strcat)` compiles to the in-place word copies; a
  state chain is a `switch`, or an `if` chain when the image tests in a non-numeric order; a case
  that prints extra text and then the next case's text is a fall-through with no `break`. Match the
  frame by sizing the buffers (and by a hidden gap between two of them) until `sub esp,N` agrees,
  and add `symbols.tsv` lines for the string literals at their real addresses, or the call sites
  show as differences. `tools/symsync.py` mislearns string addresses when the code still differs;
  add them by hand instead.
- `[vc8 rtm]` **`and esp,-64` frames.** a text builder whose prologue aligns the
  stack to 64 gets that prologue from an otherwise unused `__declspec(align(64))` local; the object
  itself is dead. The prologue then matches, but the slot placement of the other locals can stay a
  few bytes off, so park it.
- `[vc8]` **Float tails.** the decompiler can drop the end of a float function
  (it showed two int casts where the image divides by the cell size). Read the disassembly past the
  decompiled end before writing it. Float locals that the image stores and reloads (`fstp [esp+N];
  fld [esp+N]`) are plain `float` locals in the source. Byte-identical unreferenced functions (a
  second copy of a destructor body) are found by hashing the function bytes and are written as a
  separate non-virtual member.
- `[vc8]` **Hidden gaps.** a function that is only declared `extern` in the
  sources is still unreconstructed, but a gap list built from `symbols.tsv` treats it as done; build
  the list from the matcher's `ok` lines instead. When the definition of a callee is added to the
  same file as its callers, VC8 sees its register use and the callers' code changes (a matched
  caller broke); keep such a definition `__declspec(noinline)` or in a file that does not call it. A
  dead `delete[] p` of a polymorphic element type compiles to `if (count==0) free(&count) else vcall
  slot 0 (3)`; an array of plain-destructor elements to the helper `0x651c04`.
- `[vc8]` **Sweep lessons.** (1) a `mov eax,1` before `add [esi],eax` means the
  function returns 1 and the compiler reuses the register; give it an `int` return. (2) `shr 2; add
  r,r; add r,r` is `x / 4 * 4` on an unsigned. (3) A comparison that loads zero with `fldz` is
  written `f < 0` (int zero); `0.0f > f` gives a double constant. (4) A float compared against a
  global in the image is a `const float` global, not a literal. (5) A by-value float struct is
  copied with `fld`/`fstp` only if it has a user copy constructor. (6) A caller keeps `this` in
  `ecx` across a call only when the callee is defined in the same file. (7) Unsigned count `> 0`
  gives `jbe`; plain truth gives `je`.
- `[vc8 rtm]` **Message senders.** the unmatched functions that call the
  `BuscarMensaje` thunk are a good source of small matchable functions (a function-local `static int
  m = BuscarMensaje("...")`, then a virtual call on an actor or organ). Findings: (1) two zeroed
  reply slots of which one is passed are `int r[2] = {0, 0}` (a pair of separate ints drops the
  unused one); a reply of a flag plus three floats is a struct with the flag first. (2) A member
  tested straight from memory in the condition (`cmp [esi+N],0`) is written with no local copy. (3)
  `int z = 0` kept live with a later `z = 1; return z` holds a zero register across the whole
  function. (4) A thiscall target passed `this` through a thunk can be declared as a method on a
  local struct and given a new mangled name in `symbols.tsv`; a function that returns a value the
  symbol calls `void` needs its own symbol line with the real return type. (5) Fixing a float
  compare: `fucompp` vs `fcomp [mem]` depends on how the operand is declared; not solved by `const`.
- `[vc8]` **Debug text with a hidden gap.** a text builder whose frame is bigger
  than its visible locals has an unused gap between two of them. A bare `char gap[12]` is dropped as
  dead; put the neighbouring buffers and the gap in one struct of used members (`struct { Req e;
  char pos[0x44]; char gap[12]; }`, declared below the large buffer) and the frame and displacements
  line up. Unmatched functions that call the debug-window text thunk are the way to find more of
  these.
- `[vc8]` **The register-argument functions.** a function with an argument in
  `eax` (first line `test eax,eax`, no prologue, stack arguments cleared by the caller) is a
  `static` function: VC8 gives internal-linkage functions a custom register convention (the object
  in `eax`) and still emits a standalone COMDAT copy, which is what the image has. Write it `static
  __declspec(noinline) T F(Obj *o, int x)` and keep it alive with an exported function that calls
  it. The symbol is a static (storage class 3) one, so the matcher must compare those too. This
  turns the "eax argument" family from unsolvable into routine.
- `[vc8 rtm]` **Static functions, details.** (1) which registers take the
  arguments is chosen by the compiler (first argument in `eax`, then `ecx`; a parameter used only
  through its address stays on the stack); swapping the declared order of two pointer parameters
  swapped their registers (`Insertar48a6f0`). (2) A static function may use `esi`/`ebx`/`edi` for
  arguments too (`0x48a710`, `0x526d60`). (3) Two static functions that call each other keep the
  register convention across the call (`ebx` for a table pointer). (4) The matched ones were small
  predicates, ring and table helpers and a text cleaner; those that read a register argument but
  also use `this`-style access to it (`Reenviar5d48e0`) stayed in `ecx` and are parked. (5) A scan
  for "first instruction reads eax" over-reports: ordinary `cdecl` functions with `xor eax,eax`
  first, and mid-function pieces, look the same.
- `[vc8]` **Destructor bodies.** a game with `dtor()` placeholders (a method
  whose body was never written) hides real destructors. Match them from the instructions: `mov
  [esi],vtable` first = a virtual destructor of the class, the `push -1; push handler` frame plus
  `mov [esp+X],state` writes = members with destructors, a final `call base` at state -1 = the
  base's destructor. (1) VC8 treats `operator delete` as not throwing, so a destructor whose only
  calls are `delete` gets *no* EH frame; when the image has one, free through a user-declared `void
  __cdecl Liberar(void *)` bound to the same address. (2) A destructor the image expands inline
  needs the callee class body visible, `__forceinline` on that destructor, and a separate
  translation unit so the standalone copy is still emitted; `delete p;` without `if (p)` keeps the
  null test inside the expansion. (3) `char *q = p; Liberar(q);` changes which register holds the
  pointer, a loop written `do { } while ((int)f < (int)end)` gives `jl` where pointers give `jb`.
  (4) The RTTI hierarchy (`??_R3...` byte +8 = number of bases) must list the real intermediate
  bases, otherwise the destructor shows as different with 0 bytes differing; adjusting thunks
  `??_E...W7AE` need their own symbol line.
- `[vc8]` **Register of the first stack argument.** a member function whose
  first stack argument is read into `edx` instead of `ecx` is passing `this` on (ecx still live):
  look for a call to another member with no `mov ecx,...` before it, and make the callee a member,
  not a free function. And the order of the code blocks of a `switch` in the image is the order of
  the `case`s in the source, not the numeric order: when every block matches but the short-jump
  offsets differ, reorder the cases.
- `[vc8]` **Vtable-store order solved.** a constructor whose image stores floats
  (or other constants) *right after* the vtable store and before the registration call has those
  stores in a **member with an inline constructor** (`struct Cero3 { float x, y, z; Cero3() { z = 0;
  y = 0; x = 0; } }`, the order is the constructor's own), because the vptr is set after the bases
  and before the members' constructors. The same applies to integer runs (`: x64(0)` in the
  initialiser list). Statements written in the body always land after the registration. This turned
  a whole family of "vtable store comes after the first float store" parks into matches
  (`ClassAcDejarRastro`, `Puente12`, `Submarino`, `Lapa`, `ClassAUUsar`): look at the order the
  stores are written in the image and ask which of them precede the registration call.
- `[vc8 rtm]` **Conditional expressions and `add -1`.** two branches that store
  the same value (`if (a < b) x = a; else x = b;` shapes) match only as one conditional expression
  (`x = c ? p : q;`), which shares the store and tail. `add reg,-1` (instead of `sub reg,1`) comes
  out for `switch (x - 1)` and for a counted `if (n > 0) n--` stored back; a plain `x - 1`
  expression, `x + -1`, `i--` and `--i` on a register copy all give `sub reg,1`, so a read-only `add
  eax,-1` (`AnimacionTile::Anterior`) is still unexplained. Merging tiny source files: header-less
  files each declare their own copies of classes, so they cannot be concatenated without a shared
  header.
- `[vc8]` **Unnamed-function pass.** build the work list from addresses that
  appear neither in `symbols.tsv` nor as `0xADDR` in any source comment (the named-but-unwritten
  list misses functions with no symbol at all; a scan of `kuna` function starts found ~1800,
  smallest first). It paid off quickly: 40 more functions in one pass.
- `[vc8]` **Idioms found.** (1) `strcmp(a,b)` of two pointers with `#pragma
  intrinsic(strcmp)` gives the image's expanded compare loop (`sbb eax,eax / sbb eax,-1`); a literal
  operand gives `repe cmpsb`; `memcmp` does neither. (2) `p->T::T();` (explicit constructor call on
  an existing object) calls the constructor without the null test that placement `new` adds. (3) A
  leftover `mov [esp+N],0` before a call is `volatile int t = 0;`. (4) `x = cond ? Call() : 0;` as
  the last store gives `xor eax,eax; mov [this],eax` on the else path. (5) Float compares: the order
  of the two `fld` and the `test ah,N` constant follow the written form (`a < b`, `a > b`, `a <= b`,
  `!(a < b)` all differ); try all four before parking (`Considerar5cf820` needed `b <= m->valor`).
  (6) A key read into a local before each scan loop reproduces the image's load order
  (`Manejador554390`). (7) A loop that tests at the top and branches back at the bottom with `jl` is
  `do { if (e->id == k) {..; break;} e++; i++; } while (e < end);`. (8) `cmp ecx,4; jle` is `int
  len; if (len > 4)`; a callee that pops its own arguments with an `int` result is `__stdcall`. (9)
  A local struct or array needs the right size to place the frame: an unused 60-byte local above a
  52-byte line buffer fixed `Rellenar` (a DIFF of 2 bytes in a `lea [esp+N]` operand is a stack
  offset: run `bytematch.py -v` and `objdump -dr -M intel` on the object). (10) A local one-word
  object carrying the method called on it (`Clave c; c.v = x; y = c.Codigo();`) gives `lea
  ecx,[esp+N]; call`. (11) `static int id = Buscar("NAME");` inside a function gives the EH frame
  and the init guard for free.
- `[vc8]` **Pitfalls.** (1) a function whose mangled name differs from its
  `symbols.tsv` line (a `struct` forward declaration where the image has `class`, `PAU` vs `PAV`) is
  neither reported nor counted by the matcher; look for the function in `bytematch.py -v` output
  before judging it. (2) A merge script that deletes its target on rollback must refuse a target
  name that already exists. (3) The empty string literal is at a fixed address; an argument that is
  `0x6e0280`-like may just be `""`. (4) When a register-argument function (`eax`/`edi` set by the
  caller) looks unwritable, it is a `static __declspec(noinline)` function: `EscribirFase4a8930`
  (path in eax, number in edi) matched that way.
- `[vc8 rtm]` **0/1 argument by branch.** an image that pushes `0` or `1`
  through a `jcc` (`push 0 / jmp / push 1`) where a ternary or `cond ? 1 : 0` gives `setcc`/`sete`
  was written as an `if/else` with **two calls**, one per constant; the compiler merges the calls
  into one push. The branch polarity follows the test written: to get `jge L1; push 0; jmp L2; L1:
  push 1` write `if (h < 0) f(0); else f(1);` (the second arm is the taken one). Three parked
  functions matched this way.
- `[vc8]` **Bit tests and tails.** `test byte [m],0x60; jnz L; xor eax,eax; ret;
  L: mov eax,1; ret` is `return (b & 0x20) || (b & 0x40);` (two single-bit tests on one byte merge
  into one `test` and keep the branch; `(b & 0x60) != 0` and `if (b & 0x60)` give `neg/sbb`). A
  shared `mov eax,1` tail reached by a taken branch plus a second `mov eax,1; ret` for one arm is
  one arm that `return 1;` itself and the other that falls into `r = 1`. A search function whose
  miss and null cases share one `xor eax,eax` is `if (p) { ... if (r == 0) return p; } return 0;`,
  not an early `if (!p) return 0;`. A float compare of a cross product against zero stays in the
  condition (`if (expr > 0.0f) return 0;`): a `float` local for it adds an `fstp/fld` pair. Reading
  a character from a buffer pointer is `unsigned char *c = (*p)++; return *c;` when the image stores
  the incremented pointer before the byte load; a virtual-base call inline in the argument list
  (`f(a, b, ((V *)((char *)this + vt[1]))->g(10))`) matches where a pointer local does not. Copying
  a three-int member to another is a struct assignment (`o->pos = pos;`), which gives the `lea` of
  the source once.
- `[vc8 rtm]` **`fcomp [mem]` after a call.** `call f; fcomp dword ptr [m];
  fnstsw ax; test ah,0x41` (the compare operand in the image's constant pool, often 0.0f at an
  address like 0x6dc058) is `f() > 0.0f` / `f() == 0.0f` with **`f` returning `float`** and a
  *literal*, not a named `extern float`. VC8 emits `fcomp DWORD PTR __real@...` for a float result
  against a float literal (`QWORD` for a double result), and `fld [var]; fcompp` for any global or
  member operand. Before writing `extern float K_xxxxxx`, read the 4 bytes at the address: if it is
  `00 00 00 00` or another plain constant, it is a literal in the source. Fixed two parked
  functions.
- `[vc8 rtm]` (Superseded: `jb` against `jl` was the RTM compiler; `add reg,-1` is still open; see
  sections 1 and 12.)
- `[vc8]` **Zero held in eax across a scan.** when the image starts with `xor
  eax,eax` and compares table slots with `cmp [tab+esi*4],eax`, then returns that same eax at the
  loop's end, declare `unsigned r = 0;` before the loop, assign it only on the found path (`r = h |
  0x80000000; break;`) and `return r;` after the loop. A literal `return 0` and an `r` that is
  assigned on every path both fold the zero to an immediate; the variable has to be live and
  unchanged across the loop. (`Reservar5d6d80`: 87 bytes off -> 66; the image's late `push edi`
  remains.)  Also: a 16-entry count table indexed from the handle less 0x3f is a separate array
  whose base is the handle-indexed base plus 4*0x3f; use the `h - 0x3f` index form.
- `[vc8]` **Check the compiler revision first.** a Rich header lists only
  the major build (`50727`), so the VC8 RTM (14.00.50727.42) and SP1 (14.00.50727.762) compilers
  look identical in the executable, but their code generators differ: with RTM about 650 functions
  stayed a few bytes off in families that no source spelling fixed (`for (unsigned i = 0; i < N;
  i++)` ending `jl` instead of the image's `jb`, `lea reg,[zero+1]` selects, a zero kept in a
  register across a scan, `add reg,-1`, `eax` vs `edx` temporaries); with SP1 forty of them matched
  with their plain source at once, and every RTM match survived. When a family of near-misses
  refuses every spelling and flag, stop tuning and try the other service-pack compiler. Where to get
  it: the Visual Studio 2005 SP1 patch (`VS80sp1-KB926746-X86-<lang>.exe`, ~50 MB, on archive.org)
  is an EXE around an MSP around a cab of **whole PE files** (7z + cabextract; nothing to run):
  `c1.dll c1xx.dll c2.dll cl.exe link.exe`. Swap those five into a copy of the RTM tree and compare
  `make match` on both; keep the build that matches more. Everything written to dodge an RTM quirk
  (a `volatile` local, a swapped operand, an `r = 0` variable returned at the end, a static helper
  that only holds a register) becomes suspect: re-test the plain form against the better compiler
  first. One new SP1 behaviour: after `int a = 0;` a later `int b = 5;` may become `lea b,[a+5]`;
  declare the non-zero constant first (`int b = 5; int a = 0;`) to get `mov b,5`. Also: `link /ltcg`
  runs under the host once OLEAUT32 ordinals 2/6 (`SysAllocString`/`SysFreeString`) are implemented,
  and `/GL` code generation did not change any of those families.
- `[vc8]` **`switch` that matches in bytes but fails on `$LN` labels.** when
  every code byte is identical and the check only reports labels whose address differs, the
  case-to-block mapping is wrong, not the code. Read the jump table words from the image (the dword
  array right after the function, e.g. `25055c00 39055c00 2b055c00 32055c00` = `[0x5c0525, 0x5c0539,
  0x5c052b, 0x5c0532]`), pair each `case k` (index `k - base`) with the block it points to, and
  write the cases in code-address order of those blocks (here `case 1`, `case 3`, `case 4`, `case
  2`). Also: a test of a pointer's field followed by a `switch` on the same field reloads the field
  in the image only when the test goes through the container (`if (list[i]->kind)` then `switch
  (p->kind)`); with one local for both the compiler folds the load.
- `[vc8]` **Register-argument helpers are `/GL` code.** a function in the
  image whose first instruction uses `edi`/`ebx`/`esi` as an argument, with the caller loading them
  just before `call` and stack arguments cleared by the caller, comes from `/GL` + `link /ltcg`
  (whole-program code generation). It cannot be reproduced with plain `/O2` (that only gives the
  `eax` object convention for `static` functions), and a COFF-object matcher cannot see it. Compile
  the file with `/GL` and link with `/ltcg /map`, find each function in the map and compare
  instruction by instruction with the call targets and absolute addresses masked
  (`tools/glmatch.py`). What decides the registers: a **pointer** argument goes in a callee-saved
  register, a scalar (`int`, `short`, `float`, ...) stays on the stack; write the table access as
  `g[i].field` repeated (a local `&g[i]` pointer changes the register roles); a test that the image
  compiles as `mov eax,[arg]; sub eax,1; jnz` is `switch (x) { case 1: ...; break; default: return
  0; }` (`x != 1` gives `cmp [arg],1`, `x - 1` gives `add eax,-1`). First result: 0x5f7520 and its
  two wrappers identical.
- `[vc8]` **Make the `/GL` check as strict as the object check.** masking
  call targets and absolute addresses (`glmatch.py`) lets a wrong global or string pass. Give every
  symbol the source leaves unresolved a stub of its own instead: read the names from the linker's
  `LNK2001` messages (`/force:unresolved`), assemble `PUBLIC name` + 16 bytes per name with the
  original `ml.exe` (it runs under the same host as `cl`/`link`), relink with the stub, and map
  every address in the linked code back to a name through the `/map` file. Then compare per operand:
  a jump inside the function lands on the same instruction offset, a call or data address must be a
  name `symbols.tsv` gives that image address (interior addresses: same name and offset), read-only
  constants and strings compare by content. Statics of a `/GL` link are listed in the map under the
  `CIL library` module, not under the object. Whole-program code generation also **folds a global
  passed as an argument into the callee when every caller passes that same global** (the helper then
  reads it itself and the stack argument vanishes); a stand-in second caller with a different value
  restores the stack argument, which the image shows. Changing a string or a global to a
  neighbouring one must make the check fail: test that once. Commandos: `tools/glcheck.py`,
  `reconstruct/src/gl/gl.tsv`, `make gl`.
