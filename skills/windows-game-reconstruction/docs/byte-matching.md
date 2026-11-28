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
| callee visibility, frame sharing, inline copies, whole-record and branch shapes (VC8) | 13, last subsections |
| owner re-reads, shared exits, duplicated epilogues, store order by declaration, parameter homes, x87 order (VC8) | 13, last subsection |
| an EBP or 8-aligned frame, a cookie, an EH prologue or an EH state width differs; an object the port emulates (VC8) | 13, last subsection, 'Frame kind' and 'Emulated objects' |
| which callee-saved register a value takes; load order, scratch rotation, the hosting of a shared exit (VC8) | `vc8-regalloc.md`, 'The register-role table' and 'Shape rules of the cheap band' |
| a diff figure, a survey or a parked row that may be an artefact | 13, last subsection, 'Method' |
| about to adopt a change that adds EH, an array `new` or constructor/destructor structure | 13, last subsection, 'A build that links is not a build that runs' |
| the second (native) build crashes at start-up after a change that matched and linked | 13, last subsection, 'Diagnose such a crash under a debugger' and 'The native-runtime wrapper trap' |
| a file-local helper duplicated across files; computed locals declared up front where the image computes them just in time (VC8) | 13, last subsection, 'Method' and 'Declaration birth order' |
| driving a headless external agent | 14 |

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
- `[vc8]` **One call behind an `if/else` appears in both arms, each with its own `jmp` to the shared call; the
  copy written in both arms merges with another case instead.** Every case of a dialog `switch` ends in its own
  `SetFocus(GetDlgItem(h, id)); return 1;` (tail-merged at the call: `mov r,[this+hwnd]; push id; push r; jmp`),
  and in the toggle case the call stands once behind the `if/else`: VC8 copies it into the arms itself. Written
  in both arms, the first arm was cross-jumped into another case's member call (`push id; jmp`, an extra
  `mov ecx,esi`). With it: a pointer into `this` that the image takes inside each case (`lea edi,[esi+0x80]`
  four times) is the member spelled at every use, not a local in front of the `switch` (0x4cb750). **held** *(match)*.
- `[vc8]` **Cases in block order where the low cases leave through `sub/je` and the rest through the table**: the
  order is read by hand from both (0x4a81f0: `5, 4, 3, 0x411`, then the table's blocks by address); a scan that
  reads the jump table alone reorders half of it and the count rises. **A tool that reorders case chunks must
  refuse a chunk without `break`/`return`**: `case 0x19: if (!x) break; case 0x18: ...` moved apart falls into
  another case (0x5e9a30, caught by reading the diff; `fallchk.py` lists them). **held** *(match)*.


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
- `[vc8]` **A constructor port is declared, not tuned.** A functional port
  that writes a constructor as stores through `(char *)this` behind a
  base-constructor call misses the image's EH frame (`push -1; push handler`,
  `this` kept in a stack slot, `mov [state],N`). Read the class off the frame:
  the base has a (virtual) destructor (state 0 behind its constructor call);
  each member with a constructor and a destructor adds a state; the unwind
  funclets in front of the handler (`mov ecx,[ebp-N]; add ecx,OFF; jmp ~T`)
  give every such member's offset and destructor. `fldz; fst z; fst y; fstp x`
  triples behind the vtable store are vector members with a zeroing inline
  constructor (declaration order); `fst [tmp]; mov r,[tmp]` triples in the body
  are `v = Vec(0, 0, 0);`; `push ~T; push T::T; push n; push size; call
  eh-vector-constructor-iterator` is an array member; a scalar stored between
  two member constructors is in the initialiser list. Declare the class in the
  port's own file (base stub, members in address order, padding between), reuse
  the names the symbol table already has for the callees, add the vtable's
  `??_7` line. Nine constructors (197 to 789 bytes) matched at the first or
  second build this way. A scan for "image has an EH prolog, ours none" finds
  the candidates. **Held.**
- `[vc8]` **The implicit destructor trap.** A class with a virtual base
  destructor and no declared one makes the compiler emit an inline `??1X` in
  the object; if the symbol table has `??1X` for the real out-of-line
  destructor the whole-tree compare reports a new difference in a function
  nobody touched. Declare `~X();` when the image's is out of line, define it
  inline when the image's deleting destructor inlines it. Check the whole
  object's result, not only the port's line. **Held.**
- `[vc8]` **Where a load sits is where an inline member starts.** A
  `mov r,[this+K]; cmp` that the compiler hoists far up when the statements
  are plain (it carries the flags to the branch) stays in place when the test
  is inside an inline member of the sub-object; the stores in front of it stay
  plain statements (`o.a = x; o.b = 0; o.Activar();`). All four statements in
  the inline put the load in front of the two stores instead. **Held.**
- `[vc8]` **`strcpy` from a constant that is not folded.** A literal source
  becomes immediate stores; the byte loop with `mov eax,OFFSET s` is `strcpy`
  from an `extern const char s[]` the compiler cannot see into. **Held.**
- `[vc8]` **Three shorts and no security cookie.** `short c[3]` brings the
  cookie; a three-short struct is 4-aligned in an 8-byte slot; three stores at
  slot+2 are the tail of a four-short struct whose first is never written, or a
  temporary with an inline constructor (then a later temporary, such as a
  new-expression's pointer, takes the same slot). **Held.**
- `[vc8]` **A lead that leaves three bytes is an empty function.** A sweep
  transform that drops the `return` makes the compiler drop the body; discard
  such results unread. A symbol whose spelling the compiler never produces
  (`PAXPAX` for `PAX0`) is never compared at all.
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
- `[vc8]` **`add eax,-1` and then `xor ecx,ecx; test eax,eax; setle cl; sub ecx,1; and eax,ecx`, where the port
  has `xor ecx,ecx; sub eax,1; test eax,eax`**: the clamp is `max(v - 1, 0)` as the macro expands it, the
  difference written twice (`v = (v - 1) > 0 ? (v - 1) : 0;`). The difference is then a shared subexpression with
  its own `add`; `v = x - 1; v = v > 0 ? v : 0;`, `v--`, `v -= 1`, `+ -1` and an unsigned subtraction all fuse the
  decrement into the test's `sub eax,1` (0x4bbe60, 14 spellings). **held** *(match)*.
- `[vc8]` **An x87 minimum whose two arms both end in the same `fstp [slot]; fld [slot]`** (`fld f; fld g; fcom
  st(1); fnstsw; test ah,0x41; jne L; fstp st(0); jmp M; L: fstp st(1); M: fstp [slot]`): a new variable from the
  conditional (`float m = f < g ? f : g;`), the true arm the fall-through. `if (g <= f) f = g;` stores in one arm
  only; `f = f < g ? f : g` without the new variable too. With it: `flag = (a > K || b > K) ? 1 : 0;` against
  `a <= K && b <= K ? 0 : 1` (the same blocks, `je`/`jnp` for `jp`/`jne`), and `x < -g` (`fchs; fstp t; fld x;
  fld t; fcom st(1); fnstsw; fstp st(1); test ah,0x41; je`) against `!(-g <= x)` (`fcomp; test ah,1; jne`), which
  also differ for a NaN (0x45d1b0, 0x4644c0, 0x463050). **held** *(match)*.


- `[vc8]` **A parameter kept in a register and a local in a slot, where ours spills the parameter into a new
  slot**: the parameter is copied into a local in front of the loop (`unsigned char *d = dst; ... *d++ = v;`)
  (0x569da0). **An element indexed at each use inside a `new` expression's arguments** computes its address in the
  non-null arm (`new T(a[i].x, a[i].y)`); a pointer local is computed in front of `operator new` (0x5213d0).
  **The vtable loaded in front of an `or`/`add` on a field of `this`**: the field is named at each use; through a
  pointer local the store is a fence (0x4234c0). **held** *(match)*.

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

- `[vc8]` **Map every local onto the image's slots before changing code.** Through the aligned instructions
  (same pushes on both sides), each listing name `x$[esp+N]` gives the image's offset below the frame top as
  `N + (image frame - our frame) - image displacement`. Two of our locals on one image slot, or an object on an
  array's slot, name the scope structure: a buffer shared by two uses is one array; an object sharing a later
  block's slot was in a block. Frames hundreds of bytes too large in script-block readers are chained
  temporaries (`b.Token(i).Valor("K").CopiarA(...)`, three slots for all lookups), not named locals; three
  floats side by side are a struct (a by-value argument built from it shows `sub esp,0xc; mov eax,esp; ...;
  mov [esp+N],esp`). **held** *(match)*.
- `[vc8]` **A function-scope object with a destructor stops the earlier blocks' objects from sharing a slot.**
  `{ T a = ...; } { T e = ...; } T v = ...;` gives `a` and `e` two slots; with `v` in a block too they share
  one. Names, declaration order and nesting depth do not matter. A pair of objects that die at the very end
  but share the loop temporaries' slots: a block from their declaration to the end of the function.
  **held** *(match)*.
- `[vc8]` **A down-counter loop (`test n,n; je`, `sub n,1` early in the body, the index in another register) is
  `while (n) { n--; ...(i); i++; }`**; `for (i = 0; n; i++) { n--; ... }` when the index steps after the body's
  destructors. `for (i = 0; i < n; i++)` gives `cmp; jb` whatever the types. **held** *(match)*.
- `[vc8]` **An unreferenced 4-byte slot in the image's frame can be the tail of an 8-byte temporary.** Two
  fields stored through a stack temporary (`mov [t+2],ax; mov [t],0xff; mov ecx,[t]; mov [obj+N],ecx`) and a
  third with an immediate are one 6-byte class temporary with a constructor (`*(Three *)&f = Three(a, b, c)`;
  the third field folds to the constant, the frame keeps 8 bytes). A two-field class gives the same code and a
  frame one slot short, every `esp` displacement off. **held** *(1316 -> 73 bytes on one constructor)*.
- `[vc8]` **A member initialised from a constructor argument in the mem-initializer list** loads the argument
  right after the base constructor and stores it after the vptr stores; assigned in the body, the load comes
  later. **held** *(closer)*.
- `[vc8]` **A cleanup call at the end of a function on a local holder is the holder's destructor** when the
  image's EH states give the holder a state and an unwind entry: declare the destructor at the cleanup's
  address (a second name for it) and, for the native build, define it as the forwarding call. **held** *(closer)*.
- `[vc8]` **`do { x = next(x); if (!x) continue; ... } while (x);`** pushes the loop variable for the first call
  (`xor edi,edi; push edi`), where `for (;;) { x = next(x); if (!x) break; ... }` lets the zero constant be
  pushed; an EH state store between `operator new` and the following call means `new T(args)` with a
  constructor, not an init method. **held** *(closer)*.
- `[vc8]` **Not reproduced: an object sharing a char array's slot in the `/GS` buffer region** (blocks, braces
  per case, arrays per block or at function scope tried); **a register reusing a just-compared constant**
  (`cmp eax,2; jne; ... cmp [ecx],eax` in ours, `cmp [eax],2` in the image); **a strength-reduced index**
  (`pos = (i - k) * size` becomes `add pos,size` in ours where the image recomputes with `imul`). *(open)*.
- `[vc8]` **A `sub eax,K; je` chain on one value is a `switch`, and the case bodies are in source order.** An
  `if/else if` chain on the value gives `cmp; jne` per test; a ternary choosing an argument gives `mov; jne; mov;
  push`, where a `switch` (or `if/else`) with one call per arm gives `je; push [a]; jmp; push [b]; call` (the
  calls merged behind the arms). The static-guard bits and EH states number the cases in source order even when
  the compare chain tests them in another. **held** *(match, eight functions)*.
- `[vc8]` **Per-case temporaries that never share a slot are temporaries, not named locals.** Three float triples
  at three distinct stack slots, each built while the arguments are pushed (`fld; fstp [t]; lea; push; fld;
  ...`), are `Vec(a, b, c)` temporaries passed by address (`f(Vec(p[0], p[1], p[2]).Dir())`, `Dir()` an inline
  `return this`, legal C++ for the native build where `&Vec(...)` is not); named arrays per case share one slot.
  A struct with all fields zeroed in an odd order is a constructor storing in that order. **held** *(match)*.
- `[vc8]` **Members cleared with the call's vtable load after the stores** (`mov [esi+0xc],ebp ... mov
  [esi+0x24],ebp; mov eax,[esi]`): the stores go through a pointer the compiler cannot separate from the vptr
  (`for (int i = 0; i < 7; i++) (&campo0c)[i] = 0;`, or `memset`); named member stores let VC8 hoist the vtable
  load above them. **held** *(match)*.
- `[vc8]` **A loop that re-reads `*p` for each test is an indexed loop.** `cmp byte [eax],0; ... cmp byte
  [eax],0x20` with `eax` incremented is `for (i = 0; s[i] && r == 2; i++) if (s[i] > ' ') r = 1;`; the pointer
  form keeps the byte in `al` across the tests. **held** *(match)*.
- `[vc8]` **Small source shapes that moved whole functions**: a member read into a local before its compare
  (`int m = modo; if (m == 1) return;`: `mov eax,[m]` before the constant load); a field read once for two tests
  (`if (n && plazas) {...} if (n) return 0x11;`); the compare's operand order (`libres > plazas` gives `cmp
  [libres],eax; jbe`); `char texto[0x20] = "";` (first byte stored apart, the rest zeroed) against `memset`; a
  lone `otro[0] = 0;` store; `*(Pto3 *)(b + 8) = p;` (integer moves) against three float stores; a result variable
  declared after the function's statics changes the register order; `new T(...)` with no EH state store has a
  `throw()` constructor; a callee called without `this` is a static member. **held** *(match)*.
- `[native]` **An address comment binds the function names on its line, in order**: an inline constructor added to
  a one-line struct that carries `// 0xTHUNK (0xTARGET)` for a member binds the constructor to that address in the
  native build (`grep dup native/build/report.txt` shows it). Keep added inline members on lines without an
  address comment. **held**.
- `[vc8]` **A port's static helper that the image has inline, with member reloads**: when the image's calls after the first
  re-read a pointer from its member (`mov ecx,[esi+0x54]`) where the helper passes a local, write the calls in place through a
  reference to the member (`T *&m = *(T **)(base + off); m->f(); m->g();`); a store through the pointer between the calls
  (`m->flag = 0`) makes the first call reload it as well. An init call on `operator new` memory where the image has the EH
  state store of a `new T(args)`: declare a constructor at the init method's thunk (address comment and a `~` symbols line).
  **held** *(match, five constructors and slots)*.
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
  registers; that chain is the next entry (a one-word class by value). The inverse case, ours
  `mov ecx,[g]; mov ecx,[ecx+N]` against the image's `mov eax,[g]; ...; mov ecx,[eax+N]`, is an inline getter of
  the global's class (`g->Names()->Index(n)`, 0x4dbb40). **held** *(match)*.
- `[vc8]` **A pointer member loaded in front of the argument to its right** (`mov eax,[this+4]; mov ecx,[a]`
  where ours loads and pushes `a` first): the by-value struct argument is read through an inline getter that
  returns a reference, `Point &Pos() { return obj->pos; }`, `Line(Pos(), a, o->pos, ...)` (0x5b8530). A getter
  of the object pointer, a pointer local and a reference local give other orders. **held** *(match)*.
- `[vc8]` **A member loaded into its base's register, the base read from a global (`mov eax,[g]; mov
  eax,[eax+N]; push eax`, or the same chain in front of a compare), ours `mov ecx,[eax+N]`:** the value is a
  one-word class passed or copied by value. Either the callee's parameter is the class and the caller builds a
  constructor temporary (`struct H { int v; H(int a) { v = a; } }; void Log(const char *, H); Log("X",
  H(g->c4));`) or a POD struct local is copied from the member first (`H a = g->h; kind = a.v != n->res ? 2 :
  0;`). Seventeen measured forms: an `int`, pointer, reference, `bool`/`char`/`short` argument, the struct member
  passed directly and an implicit conversion (built in place through `esp`) all take a fresh register. Same
  stack bytes as an `int` argument, so the callee and its other callers do not change; when five small ports
  with the same "register only" note all call one function, read that function's parameter as a class
  (Commandos 0x5944c0, 0x581d70, 0x5cf530, 0x512640, 0x4e3930, 0x556330). **held** *(match)*.
- `[vc8]` **Two values where the SECOND is computed first and held across the first (`call ftol; mov edi,eax;
  call ftol; mov [m0],eax; mov [m1],edi`; for floats the second goes to a temporary):** a pair filled by an
  inline member *setter*, `p.Set((int)(x * 256.0), (int)(y * 256.0))`. A setter's arguments are evaluated right
  to left, the inline constructor's (above) left to right, so the held value says which. Members stored in front
  of the setter are reloaded from memory behind it. Read both conversions off the x87 stack before trusting the
  port's expressions: here the port had `y*256*x` and `y` where the image has `x*256` and `y*256` (Commandos
  0x47d4a0). **held** *(match)*.
- `[vc8]` **The member of a call's result loaded straight into `ecx` for the next call (`call f; mov
  ecx,[eax+N]`), ours `mov eax,[eax+N] ... mov ecx,eax`:** the call's result is a named local and the member is
  read off it in the call expression (`W *w = g->Window(); w->view->Go(&p, a, b);`). A local for the member, the
  whole chain in one expression, a getter and inline wrappers keep `eax` (0x5e6a50, 0x5f4cc0). **held**
  *(match)*.
- `[vc8]` **A by-value float struct copied with `fld/fstp` and one more float stored through `esp` in front of
  `mov ecx,esp` (`sub esp,0x10; fstp [esp+0xc]; mov ecx,esp`):** two arguments, a struct with a user-defined
  member-wise copy constructor (floats go through the FPU; the implicit copy uses integer registers) and a
  scalar float whose `push ecx` VC8 merged into the struct's `sub esp`. A port that passes one larger struct has
  the same stack bytes and a different store order. A frame larger by two unused floats is a three-float struct
  local of which only one member is not forwarded (0x487ff0). **held** *(match)*.
- `[vc8]` **`mov eax,[esp+4]` in a `ret 4` member that then stores through `eax` and never reloads it:** the
  function returns a struct by value (the hidden pointer stays in `eax`), often an inline `operator-`/`operator+`
  of two vectors; a port with an out-parameter and no return value has the same `ret N` but leaves another
  pointer in `eax` (0x456860). **held** *(match)*.
- `[vc8]` **A named `extern const double` in place of a literal moves the load:** the literal's `__real@`
  constant is scheduled as a constant, the named one as a memory read (one `fld` on the other side of a byte
  store, 0x47d4a0). When a data-naming pass has replaced literals, try the literal first on a port that differs
  by the position of a constant load. **held** *(match)*.
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
- `[vc8]` **`mov [slot],eax; test eax,eax; mov [EH state],n; je` behind `operator new`, where the port has
  `test eax,eax; mov [p],eax; je; mov [T],eax; mov [EH state],n`**: a real new-expression (`new T(args);`) for
  `void *p = operator new(N); if (p) new (p) T(args);` (0x4a81f0). Swept over the 57 ports with the placement form:
  16 closer, and 8 of those only because the stub class is smaller than the allocation (`push 1` for
  `push 0x2fb8` is fewer differing bytes than the placement form). **Compare `sizeof(T)` with N in the listing
  before keeping a new-expression**: a native build of the same source would allocate the stub's size.
  **held** *(match)*, with the trap.
- `[vc8]` **Flag-guarded destructors (`test bl,4; je; and ebx,~4; lea ecx,[tmp]; call dtor`, one bit per
  temporary) around a constructor call behind `operator new`**: a chain of by-value calls is the argument of the
  new-expression (`new T(Dir(a.Obtener("X").ObtenerEn(i).Obtener("Y")), ...)`); the temporaries live in the
  conditionally evaluated part and get a flag each. Named locals inside `if (p)` have plain destructors. The
  constructor's `int` parameter takes the temporary's address through an inline over `const T &` (0x4c4dc0, matched
  from the image in one writing: result variable `r = 1`, `default: r = 0`, cases in block order, the inner
  `switch` with case 7 in front of case 1). **held** *(match)*.
- `[vc8]` **`mov ecx,eax; call member` right behind a call that returns an object by value**: the member is
  called on the temporary (`g.Valor(s + 1).CopiarA(p, n);`); a named local gives `lea ecx,[local]` and two more
  bytes of frame traffic (0x5af720). **A local's destructor in front of a later call on another local**
  (`mov BYTE PTR [state],0; call ~V; lea ecx,[t]; call Guardar`): the local has its own block
  (`{ V v(buf); t.Cambiar("XYZ", v); } t.Guardar();`); the value as a temporary argument is 65 bytes off
  (0x4c8590). **held** *(match)*.


- `[vc8]` **Two EH objects in one stack slot**: each stands in a block of its own (`{ V o = t.Valor("A"); ... }
  { V x = t.Valor("B"); ... }`); objects whose destructors all run at the function's end (states only count up)
  are not in blocks. **The first object in the higher slot, the loop's object below it**: the body stands under
  `if (n) { ... }`; `if (!n) return;` in front exchanges the two slots and nothing else (0x4d8040). **held**
  *(match)*.
- `[vc8]` **A byte-loop `strcpy` from a literal** (`mov al,[edi]; mov [ecx+edi],al; add edi,1; test al,al; jne`,
  where a literal written at the call gives dword moves): the literal is the argument of a callee expanded in
  line; an out-of-line member defined earlier in the same file is expanded too when called with constants
  (`Empezar("CAMUFLARSE", 4, 1, 0);`, 0x4234c0). **held** *(match)*.
- `[vc8]` **Three `fld [m]; fstp [eax+N]` straight into the argument area**: a vector passed through its
  user-written copy constructor (`f(i, o, *(V *)&campo)`); a three-float constructor call builds a stack temporary
  first (0x427bd0). **Two same-sized results, the second copied from `eax`**: `Caja c = Area(); *(Caja *)(t + N)
  = Area();`, a dead named local in front of the assigned temporary; a discarded call shares the slot
  (0x60b5c0). **held** *(match)*.
- `[vc8]` **A COMDAT constructor or destructor our object defines is compared with the bytes at the image's call
  target**: when the image calls an out-of-line one through a thunk, declare both without bodies and bind the two
  names to the thunk addresses in the symbol table (0x56b2d0). **held** *(match)*.

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
- `[vc8]` **A sweep's aligned-lines leads, read as bytes**: of 554 leads the 125 best by remaining score were
  applied one by one and measured in differing bytes (`lb.sh`: keep when lower). Six became identical, each with
  one more hand step that the listing showed at once (a temporary used directly, a new variable for a minimum, the
  `max` macro, the operand order of a compare, a block for a local); none by the lead alone. A lead that leaves
  under about 30 bytes is worth opening; the rest are ports a few statements nearer. Leads of the kinds `noret`,
  `stswap`, `fltdbl` and `inlvar` over a member change what the code does (a value read again after a store, a
  `float` rounding dropped): read the diff before keeping one as a "closer port".


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
- `[vc8]` A destructor declared in a shared header and defined out of line in
  ONE file is inlined into that file's scalar deleting destructor and called
  by every other file. The image shows it as a `??_G` with the body in line
  beside derived destructors that call `??1`; a body in the class would
  inline it everywhere and cost the derived destructors.
- `[vc8]` A record array indexed at every use (`campos[i].id`, reached
  through `this`, the member loaded again per use) keeps the index in memory
  and multiplies it per pass; a cached row pointer or a running byte offset
  strength-reduces and rotates the scratch registers. A local copy of the
  object pointer swaps base and index of the address (`[idx+ptr]` against
  `[ptr+idx]`). Remove the cached row and object pointers first.
- `[vc8]` `mov ax,[esp+N]` for a parameter means the parameter is 16 bits
  wide; an `int` narrowed in the body loads the dword. Callers that pass
  `int`s compile the same, so a stand-in's signature can change.
- `[vc8]` Every frame slot N bytes higher than ours with an equal `sub esp`:
  a buffer N bytes smaller, not a missing local.
- `[vc8]` A bounds-checked getter with the bound compared twice and a register
  copy in front of the element load: the list is a sub-object with inline
  members of its own, and the owner forwards to them.
- `[vc8]` Read the order in which values, temporaries and buffers take their
  frame slots (`lea ecx,[esp+N]` in listing order) and compare it with ours
  before changing anything else. A text buffer declared beside its value
  (`{ Block v = ...; char s[32]; if (v.Count()) ... }`) and one declared
  inside the `if` give different slot sharing for every LATER value and
  temporary of the function; the frame size says which the original had.
- `[vc8]` `movsx eax,cx` ... `shr ecx,16; movsx eax,cx` on one int "parameter"
  followed by a 16-bit one, and locals coloured into the homes of the other
  parameters only: the two are one small record passed by value
  (`struct { short r, g, b; }`). The stack layout is the same, so callers
  through other declarations do not change.
- `[vc8]` A row loop that ends `sub [slot],1; jne` with the per-row skip
  spilled in front of the count was written counting up with the skip
  computed in the body; a hand-written count-down stores the two the other
  way round. Signed masks (`(int)0xffff07ff`) keep `sar` behind an `and`.
- `[vc8]` A scratch-register rotation that is one step off from a field
  read onward: read the field through an inline getter, or do the whole
  replace-a-string sequence in an inline member (`delete p; p = new
  char[strlen(t) + 1]; strcpy(p, t);`).

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
- **Vtables and RTTI are part of the match.** `[vc8]` **held** *(match)*. A class's vtable, complete object
  locator, type descriptor name (`.?AVClass@@`), hierarchy and base class descriptors are compiler output: check
  them like code (`tools/vtcheck.py` here: slots resolved through the symbol table and the image's jmp thunks, then
  the RTTI structures). A class passes when it declares every virtual in the original slot order (destructor where
  the image has it, a placeholder for each slot it does not override), derives from the original bases in the
  original order and access (a private base changes the base descriptor's attributes), uses `class`/`struct` as the
  image does (`.?AV`/`.?AU` in the name string), and is compiled with RTTI on (`/GR`, the default here). `shorter`
  means an incomplete declaration; once a class passes, the native build need not read its vtable or RTTI from the
  exe. clang cannot emit ELF objects for vtables with RTTI on the MS ABI (`largest` COMDAT): go through LLVM IR with
  `comdat any` for a check-only build.
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
  `else f = 0`. `[vc8]` **held** *(match)*. When BOTH arms hold code and the false arm sits after the `ret`, test
  the flag with the else arm first (`f = c; if (!f) A; else B;`: 'Block layout, wrong views and storage shapes',
  the last subsection of this section).
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
- `[vc8]` A byte count measured over *your* object's length makes a port
  that is shorter than the original's function look near. List the ports
  whose original function (to the next symbol, padding stripped) has
  another length, and read the original past your last instruction: an
  arm may be missing (a real bug behind a "register choice" note), a call
  the original repeats per case may be shared in yours, or yours may call
  a helper for a body the original has in line.
- `[vc8]` Audit every port for missing code by comparing call sequences,
  not lengths: take the original's direct call targets by address and its
  indirect calls in order, the same from your compiler listing (the
  assembly listing, not a disassembly of the object: a linear disassembler
  loses its place in COFF objects), align the two with an LCS in which an
  unresolved name of yours matches any direct call, and list the ports
  where the original has calls without a counterpart and yours has none
  extra. Length ratios, branch counts and "targets never called" are
  noise next to it (inlined getters, unnamed neighbours in the extent).
  On 1,383 ports it gave 95 candidates; thirty read, no second missing
  arm, eight matches from the ones that were a loop for what the original
  writes out.
- `[vc8]` Never take a `learn` address from a function that is not
  identical: with another length the relocation sits on other bytes, and
  the value lands in the symbol table as a name inside some other
  function, where it validates nothing and cuts that function's extent.
  Check the table for names at non-instruction addresses.
- `[vc8]` A parameter kept in its callee-saved register and stored from it
  in both arms, where yours stores a constant in the arm that knows the
  value: the store stands once behind the `if`/`else`; the compiler
  duplicates the tail after constant propagation.
- `[vc8]` A `switch` whose cases each hold the same call with a different
  scratch register: every case is written out complete with its own
  `break`, in ascending order; fall-through into a common call shares it.
- `[vc8]` One loop serving several messages with the message tested inside
  (`if (id == A) f(); else if (id == B) g(); else h();`) where the
  original has one loop per message and no test: each message is its own
  `case` with its own loop. A table of constants or of member addresses
  walked by a loop where the original has the calls in a row (six calls
  with six constants, five members ended one by one, eight lookups by
  name) is the same finding: write them out, and spell the object's
  member at every call when the original reloads it.
- `[vc8]` Zero differing bytes with a `label ... image != ...` note and the
  callees of several cases reported exchanged: the case bodies are right
  and in the wrong order. Bodies are laid out in the order the cases were
  written, not in numeric order; reorder the cases to the original's
  block order (here 0x2a, 0x24, 0x26, and a later case in front of an
  earlier one).
- `[vc8]` `push arg; mov ecx,esi; call f` where `f` is known as a free
  `__stdcall` function: `f` is a `thiscall` member that never reads
  `this` (the callee pops the same bytes either way). Declare it as a
  member and call it on `this`; only the caller's code and the callee's
  symbol change. Sweep the original for calls of free-function symbols
  with `mov ecx,reg` right in front.
- `[vc8]` A row of zero stores that uses the function's zero register and
  stays in front of the statement behind it, where yours takes a fresh
  `xor` (a loop) or hoists the next statement's loads over the stores
  (assignments in the body): the stores are an inline member of the
  sub-object, written out one by one.
- `[vc8]` The type of a pushed local takes part in load scheduling: a
  `%s` argument held in a `const char *` local is loaded early, the same
  value in an `unsigned` local late.
- `[vc8]` Constants stored into a box in front of a pending halving, the
  half then subtracted from two values side by side: the pair is shrunk
  in place (`p.x -= t / 2; p.y -= t / 2;`) and then passed to the box's
  constructor; differences as constructor arguments compute the half first.
- `[vc8]` Value loaded before the pointer, pointer loaded again for the
  next store: inline setters of the pointed object. A template over a
  pointer to member (`PonM(o, &T::m, v)` with `o->*m = v`) gives the same
  code without editing the class, so a sweep can try it; a template over
  a reference to the field does not.
- `[vc8]` A value computed twice with every member read again behind the
  first store: an inline member of the *sub-object* called twice. Reads
  through the sub-object's `this` are not shared across a store through
  the owner's; the same expression spelled on the owner is.
- `[vc8]` Copies of one body that differ in a member's offset: a
  `__forceinline` member of a class template over the offset
  (`char pad[D - k]` in front of the member), not a free helper taking
  `(char *, int offset)`: x87 operand order ranks a pointer loaded from a
  member of `this` differently from one loaded through a parameter.
  Differences that stay on the x87 stack across several products are
  `double` locals.
- `[vc8]` **Buffer sizes first.** In a function with several stack buffers,
  read every size off the distances between the buffers' `lea`s (the cookie's
  offset closes the sum) before reading anything else: with four wrong sizes a
  1960-byte function was 1494 bytes off, with them right 150. A size the port
  enlarged can hide an overrun the original has; keep the larger size for the
  native compiler only. **Held** *(match)*.
- `[vc8]` **String intrinsics by signature.** `strlen`: `mov cl,[r]; add r,1;
  test cl,cl; jne; sub r,r2`. `strcpy`: `mov dl,[s]; mov [d+s],dl; add s,1`
  (one increment, the destination as a difference). `strcat`: `add edi,-1;
  mov cl,[edi+1]; add edi,1; test cl,cl; jne` in front of the copy. A byte
  copy with two pointers stepped is a hand loop in the original too. Count the
  signatures per function in the image and in the build instead of hunting
  hand loops in the source. **Held** *(three matches)*.
- `[vc8]` **No `mov ecx,this` between two member calls** is not whole-program
  code generation: the first callee is defined in front of the caller in the
  same file and the compiler saw that it keeps `ecx`. Write the callee's body
  in the caller's file, in front, `__declspec(noinline)`; when it is already
  matched elsewhere under another name, add the second name at the address and
  keep the second body away from the native compiler. **Held** *(match)*.
- `[vc8]` **Try shapes as a product.** For a small function a few lines off,
  build every combination of loop heads (`for`, `while (*s)` with the read
  inside, `do`, the test in the condition or as a `break`), bodies (locals
  declared in either order, an inline, a pair) and tails (the fallback inside
  `if (!e) {...; return v;}` with the found return last, a `switch` with a
  fall-through for a value reloaded at a shared label). Four matches in 20 to
  100 builds each, none reachable by one edit. Score by bytes only while the
  length is unchanged. **Held.**
- `[vc8]` **The cases of a `switch` are written in the order of their blocks,**
  not of the jump table. A wrong order compares as zero differing bytes in the
  function and shows only as two literals at each other's address.
- `[vc8]` **A sweep's alignment score is a lead, not a patch.** Removing a
  local often raises the score while the bytes get worse, and can change
  behaviour (`n--; if (!n)` for `old = n; n--; if (!old)`). Reset the port and
  solve it by hand.
- `[vc8]` **A matched function is a search key.** Class families built from
  one template (particle systems, list classes) are compiled once per class
  and decompiled one by one into ports that differ from each other. When one
  matches, search the image's code for its most specific instruction run
  (bytes without addresses) and list the owning symbols: seven copies of one
  350-byte function matched at the first build each, two more of a paint
  routine by copying an already matched sibling's text. **Held** *(match)*.
- `[vc8]` **Run the sibling search over every unmatched port, not from one
  match.** Normalise every image function (numbers wildcarded, registers
  kept), call an 8-instruction run distinctive when at most 12 functions hold
  it, and for each unmatched port list the MATCHED functions sharing its
  distinctive runs with the share of the port they cover. High cover with a
  high mismatch count is a port written from the decompiler whose twin in
  another class is already matched: copy the twin's text, rename the fields.
  Fourteen matches from the first sixty lines, most at the first build (up to
  1007 bytes); about 950 ports had some matched sibling. Take the matched set
  from a verbose match, not from the symbol table (aliases without a body).
  What the copies still needed, each a general rule: a dereference spelled at
  every use where the twin had a local (`(*m->owner)->x; (*m->owner)->y`
  reloads per switch case; a local lets VC8 keep the function's first load);
  the same call written at the end of both arms of an `if`/`else` (the merged
  tail loads `this` in front of the pushes); a user copy constructor on a
  vector passed by value straight from the object (`fld`/`fstp` into the
  outgoing slot). **Held** *(match)*.
- `[vc8]` **`mov eax,ecx` at the entry and `this` in `eax` to the `ret` is a
  function that returns `this`**, also when every caller ignores the value (a
  constructor called as a plain function by static initialisers). And a run of
  stores from arguments that looks shuffled is usually the source order with
  neighbours exchanged two by two (`b,a`, `d,c`): undo the pairs to read it.
  **Held** *(match)*.
- `[vc8]` **Pointer or integer, member or free function, decide the registers
  of a forwarded argument.** A wrapper `f(h) { char b[256]; g(h, b); return
  !strcmp(b, "{"); }` was 23 bytes off as `__stdcall f(int)`, 4 as a member
  with `int`, identical as a member taking `const char *` and calling
  `const char *g(const char *, char *)`: check whether the callers load `ecx`
  (a member that never reads `this`) and give handles their pointer type.
  Three words stored in order behind a later argument loaded first are one
  struct taken by value (`Set(Vec3 v, int w) { on = 1; wait = w; dest = v; }`).
  **Held** *(match)*.
- `[vc8]` **An inline member called on its own members** leaves a copy of the
  one it overwrites: `p.FromPolar(p.x, base + p.y)` gives `fld [x]; fstp [r]`
  and multiplies the cosine by the member, the sine by the copy. A pair's
  setter `Set(a + u, b + v)` computes the second argument first into a
  temporary and stores the first directly. `int k = (n & 1) ? n : -n;` stores
  once in a dead parameter slot where `k = n; if (!(k & 1)) k = -k;` stores
  twice in its own. An inline that returns `*this` used as the next call's
  argument (`v.Add(a.Scale(f))`) keeps the first product on the x87 stack and
  rounds it through the parameter slot. **Held** *(match)*.
- **Build trap:** a tree extracted with `git archive` carries commit times, so
  a file changed by a newer commit can be older than its object and `make`
  keeps the stale object. Sync by checksum and touch what changed.
- `[vc8]` **An expression spelled at every use is not a local.** `sin(g * k)`
  and `cos(g * k)` with the product written at both calls (VC8 computes it
  once) against `double r = g * k;`: same instructions for the angle, but the
  parameter stays in use up to the second call, so the home slots of dead
  parameters are handed out differently (in the order the parameters die in
  the source), and the operand order of later x87 products changes
  (`fld st(0); fmul [m]` for `fld [m]; fmul st(0),st(1)`). A function with the
  right instructions and float locals in the wrong argument slots wants this
  before any reordering of declarations, which does nothing. **Held**
  *(match)*.
- `[vc8]` **Literals for named constants in x87 code, per use.** A named
  `extern const` float or double is a variable to the compiler: loaded at each
  use (`fld [K]; fcompp` for `fcomp [K]`), not kept on the x87 stack across the
  function, and its load is not moved in front of a store. Where a port is
  off by exactly that, write the literal and say why in a comment. **Held**
  *(match)*.
- `[vc8]` **The equal arm of a two-float test laid behind the body** is an
  inline `bool operator!=` of a pair type in the condition; written out as
  `a.x != b.x || a.y != b.y` the short else arm is moved in front. Behind the
  operator the compiler duplicates the join's first statements into both inner
  arms: write them once. **Held** *(match)*.
- `[vc8]` **Stores through an `int *` local end the reuse of members.** A
  member read again from memory, or a difference computed a second time,
  behind two `add [this+N],reg`: the adds were written through a pointer
  (`int *r = &rect.x; r[1] += dy; r[0] += dx;`). Spelled as members the
  compiler keeps the earlier values in registers. **Held** *(match)*.
- `[vc8]` **x87 compares keep the written operand order** (the left operand
  is loaded first) while products and sums do not. A compare with the right
  relation and the loads exchanged is fixed by exchanging the operands and the
  relation in the source. `if (a < m) a = m;` loads `m` once and stores it from
  the stack; `if (m > a)` reloads. **Held** *(match)*.
- `[vc8]` **`<math.h>` in front changes `fabs` of a float**: the header's
  inline `float fabs(float)` adds a second rounding store behind the port's own
  cast (`fabs; fstp; fld; fstp; fld`). A file that only declares
  `double fabs(double)` gets one. Check the includes before the expression.
  **Held** *(match)*.
- `[vc8]` **A rectangle assigned from a constructor temporary over two pair
  temporaries** converts all four values first (the second pair first, each
  pair left to right) and stores x, y, width, height in order with the
  subtractions between; an inline setter with the same body stores the sizes
  first. **Held** *(match)*.
- `[vc8]` **The cases of a `switch` stand in the source in the order of their
  blocks behind the jump, and this can be scanned.** Group the jump table's
  entries by target and sort by address, in the image and in our listing; a
  port where the two orders differ has most of its body shifted and reads as
  hundreds of differing bytes. Reordering the cases alone matched four
  functions of 260 to 575 bytes; function statics declared in the cases are
  numbered in the same order (guard bits, EH states). Where ours has one block
  for two cases and the image one each, the statements are written out in both
  cases. **Held** *(match)*.
- `[vc8]` **`mov ecx,this` in front of a callee that never reads `ecx`: the
  callee is a thiscall member**, also when it was ported as a `__cdecl`
  function without arguments (`mov ecx,esi; call f` against `call f`). Scan
  for call sites where the image loads `ecx` and ours has none since the last
  call, then check the target for a read of `ecx` before a write. The callee
  stays byte-identical as a member; only its name changes. The opposite sign
  (ours loads `ecx`, the image does not) is a load hoisted in front of a
  branch or a callee defined in front that keeps `ecx`, never a missing
  `this`. **Held** *(match)*.
- `[vc8]` **A function declared `int` whose exits set no `eax`, with exits in
  the middle**: `return;` is an error there (C2561) and falling off the end
  needs `#pragma warning(disable: 4716)`. A macro that is `goto fin` for VC8
  (with `fin:;` as the last statement) and `return 1` for the other compiler
  gives every exit its own epilogue, as `return` does. **Held** *(match)*.
- `[vc8]` **Two float differences stored to adjacent slots and loaded back in
  the order they were stored** (ours loads the second first, 2 bytes): they are
  the arguments of an inline taking two floats (`N2(a->x - b->x, a->y - b->y)`
  with `return x * x + y * y`). Parameter slots are assigned from the last
  argument up and the loads of a commutative sum follow the slots; no
  declaration order, operand order or cast on two locals reproduces it. The
  inline called inside the condition (no local for the result) keeps the
  compared constant on the x87 stack for later uses. **Held** *(match)*.
- `[vc8]` **A symbol table must be scanned for names that are no function
  start.** A `learn` value printed for a port that is not identical is not an
  address; taken over as a symbol it sits inside another function, cuts that
  function's extent in every tool that reads "to the next symbol", and shows
  up in audits as a port ten times longer than its image. Check every code
  symbol against the start list, the extra starts and the thunk table (thunk
  or target), and every name that carries an address (`F<thunk>_<n>`)
  against that address, the thunk and its target. Real starts the list merged
  are 16-aligned behind padding, a `ret` or a jump table. Before removing one,
  check that no source, no hand-kept table and no generated trampoline uses
  it. **Held** *(ten removed, match count unchanged)*.
- `[vc8]` **The written-out kind again, generated**: a port's loop over a
  table of (source slot, destination slot) pairs is in the image one statement
  per row (72 lookups, 1584 bytes). Generate the statements from the port's
  own table. In the same function the image's `mov eax,1` / `xor eax,eax` at
  the exits showed the port's `void` to be an `int`, and a handle stored to a
  global and tested through it (`g = f(); if (g) { int h = g; ...; return 1; }
  return 0;`) gives `test eax,eax; mov [g],eax; je end` with the copy to the
  callee-saved register behind the pushes; a local tested first moves the copy
  in front of the test. **Held** *(match)*.
- `[vc8]` **A `$GSCopy$` slot for a parameter that the image keeps in a
  register is a missing local elsewhere.** The image held a member read once
  in a callee-saved register across calls; with that local (`int est =
  estado;`) the fourth register is in use, the prologue pushes it, and the
  copied parameter goes to a register as well (frame 4 bytes smaller).
  **Held** *(match, 896 bytes)*.
- `[vc8]` **A small member defined in its class and still called**: `/Ob2`
  does not inline a body with an int-to-float conversion and a call;
  `__forceinline` does, where the image has the body in line at every call
  site. **Held** *(match)*.
- `[vc8]` **`mov eax,1; je; cmp; jne; cmp; jne; xor eax,eax; mov [m],eax`** is
  a local set to 1 and cleared under the negated condition, then stored; the
  boolean expression assigned directly materialises with `xor; jmp; mov 1`,
  and an inline helper with early returns with `setne`. Which register holds
  the 1 follows the rest of the function and was not steerable by locals,
  `goto` forms or helpers. **Held** *(structure; registers open)*.
- `[vc8]` **A callee that reads `ecx` under a free-function name is not
  always a caller bug**: check whether the caller is dead code (its thunk has
  no reference), whether the callee only spills `ecx` (`push ecx; mov
  [ebp-4],ecx` of a member with an `__asm` body) and whether the other
  compiler reaches it through a thiscall adapter. Compare `ret N` with the
  mangled argument bytes of every name at the address (struct returns and
  by-value structs counted; a function that ends in a tail jump has no `ret`
  of its own). **Held** *(46 callees, no mismatch)*.
- **Locals can live in a dead parameter's home slot.** A function with no
  `sub esp` that stores bytes into a parameter's slot and returns `mov
  eax,[param]` on paths that never set a result has (a) a result variable
  left unset there and (b) small locals (a 4-byte union, a 3-byte array)
  that VC8 coloured into the slot. A second small local alive at the same
  time gets a frame slot (`push ecx`): put the bytes in one array. Set the
  result for other compilers. **Held** *(one 1028-byte fill, not yet
  identical)*.
- **Three states of what the compiler knows about a small callee.** Only
  declared: calls kill every address-taken local. Defined out of class in
  front: no memory written and the registers it leaves alone are kept
  across the call. Defined inside the class with `__declspec(noinline)`
  (an inline that was not inlined): no memory written, but no register is
  trusted, so a hoisted load goes to a callee-saved register. Try the third
  when a load is hoisted over a call yet sits in `esi`/`edi`. **Held**
  *(measured on one function)*.
- **A callee re-declared in a stand-in struct is an unknown callee.** If the
  image omits `mov ecx,this` behind a call, call the member through the
  class that defines it in front in the same file, not through a local
  look-alike. **Held** *(1 function, the `mov ecx` gone)*.
- **The same value stored twice in both arms**, the image with `xor
  eax,eax` in the else arm: one conditional value (`int d = o ? o->m : 0;`)
  stored twice, not the arms written out. **Held** *(1 match)*.
- **Workspace trap: `rsync -a` keeps file times**, so a header committed by
  someone else with an older time than your objects does not rebuild them
  and a full run shows differences HEAD does not have. Touch the headers
  and rebuild before believing a regression.
- `[vc8]` **Lifting generated placeholders into real classes.** Generated
  stubs (`*(void**)this = vt; dtor(); if (f & 1) delete(this)`) are replaced
  by the class's own destructor declaration, read off the stub: base
  destructor called directly = no destructor (implicit); class vtable stored
  first = `~X() {}`; own destructor called = `virtual ~X();` with its address;
  `delete[] m` before the base call = `~X() { delete[] m; }` with
  `operator delete[]` declared `throw()` (else an out-of-line, EH-framed
  destructor). The compiler then emits `??_GX` wherever the vtable is emitted
  (the constructor's file). `mov eax,ecx; mov [eax],vt; ret` with no caller is
  an inline constructor's out-of-line copy (declare the constructor out of line
  in a small class of its own; the header keeps it inline). Count distinct
  addresses, not names: the stubs were often duplicates. **Held** *(about 460
  placeholder names removed, distinct matches unchanged)*.
- **Native hazard of lifted classes (clang, MS ABI): a class clang sees as
  abstract gets a trap (`ud2`) as its deleting destructor.** Per-file class
  declarations that omit a base's pure slots make it abstract; declare the
  overrides, and keep a stub for real interfaces. Vtables named `??_7X@@6B@`
  are redirected to the image's copy by the loader's name rules; with multiple
  inheritance they are `??_7X@@6B<Base>@@@` and need their own lines.
- `[vc8]` **Before lifting a placeholder deleting destructor, look for the real
  one the objects already emit.** The compiler writes `??_GX` wherever X's
  vtable is; a symbols file generated from RTTI often names only `??_EX` (and
  sometimes at the wrong slot), so the real copy is never compared and the
  stub carries the match. List every object's `??_G` names without a line,
  give each its vtable's destructor slot (thunks followed), keep the ones one
  object matches, then drop the stub as a duplicate. **Held** *(160 names
  this way, no source edit)*. For a native build, removing the duplicate stub
  is safe only if a native object defines the real name (no vtable in the
  file, no `??_G`), no file sees the class abstract (trap), and it calls no
  destructor that has neither source nor address (a declared-only `~X();`):
  a strong stub hides all three until it goes.
- `[vc8]` **A skipped EH state number between two function-scope statics is
  an inlined function with its own static** whose call was folded away (a
  "mark changed" setter called inline with `notify = 0`, the static under
  `if (on && notify)`). Writing the plain store instead renumbers every later
  static one lower. **Held** *(two 708-byte message handlers)*.
- `[vc8]` **The frame size and the cookie offset give a function's array
  sizes.** Two `char[0x400]` that the image lays out as `0x100` and `1000`
  bytes differ in every displacement; read the slots off the image's `lea`s.
  **Held**.
- `[vc8]` **A conditional value pushed after the later arguments is two
  merged calls** (`if (c) f(0.0f); else f(K);`) or a macro that repeats the
  expression (`MAX(e, 0)` inside the argument list): the ternary through a
  local, or a clamped local, is evaluated before the pushes. **Held**.
- `[vc8]` **A nested Win32 call whose own pushes come first is a local**:
  `h = GetDlgItem(...); SendMessageA(h, ...)`; written nested, VC8 pushes the
  outer call's constants before calling the inner one. **Held**.
- `[vc8]` **`strcat`/`strcpy` expansions scan in whichever register holds
  the source**: a source that arrives as a call result (`strcat(A(), B())`)
  is scanned in place; a named local is copied to `ecx` first. **Held**.
- `[vc8]` **Every reload of `global + offset` in `eax` where ours rotates
  `ecx`/`edx`**: the address comes from an inline function returning
  `global + off`. **Held** *(a 559-byte function with eleven calls)*.
- `[vc8]` **Sweep the named float constants back to literals**: a naming
  pass that turned `0.75` into `extern const double K_0_75` makes every load
  a memory read the scheduler cannot move; replacing each function's names by
  the values read from the image is a cheap batch (two matches in 53 ports
  here).
- `[vc8]` **Scan the frame sizes.** List every differing function whose
  first `sub esp,N` differs from the image's. With a cookie, read the arrays'
  sizes off the image's displacements; 4 extra bytes in ours are often a
  local pointer to a sub-object (`T *p = (T *)((char *)this + off)`) that the
  compiler keeps in a slot across calls, where the original recomputes the
  address at each use. **Held** *(two matches from the first look at the
  list)*.
- `[vc8]` **An argument loaded before the callee's vtable is a local**
  (`int j = r->field; r->Virtual(j);`); written inline, the vtable load comes
  first. **Held**.
- `[vc8]` **Group the image's functions by body before matching.** Hash every
  function's instructions with absolute addresses masked: two unmatched ports
  with the same body are one source (a header-inline function emitted in two
  places, or a copied file), and one text matches both. **Held** *(two
  addresses from one 148-byte function)*.
- `[vc8]` **x87 zero tests: the block order and the kept zero.**
  `if (!(x == 0.0f)) { ... } tail` puts the `x == 0` code after the epilogue
  and keeps the first `fldz` for a later compare against memory; `x > 0.0f`
  (constant on the right) gives `fldz; fcomp [x]; test ah,5; jp` and is
  hoisted when both arms of a branch test it. **Held**.
- `[vc8]` **Inline-function arguments are evaluated left to right** into
  DWORD temporaries when the parameters are `float`; a direct call pushes
  right to left. Useful to order two calls; it did not reproduce an image
  that spills a float call result as a DWORD across the next call (VC8
  re-spills as a QWORD). **Open**.
- `[vc8]` **Stub declarations emit vtables too.** A file that declares a
  class only to define its destructor (`class X : Base { virtual ~X(); }`)
  emits `??_7X` with the destructor in slot 0; a vtable check that takes the
  worst object makes that class differ even when the real header is right.
  Declare the real base (with its virtuals in slot order; a `typedef` keeps
  the placeholder name) and the overrides; code bytes do not change, and a
  slot whose function only has a non-virtual name needs a virtual alias in
  the symbol map. **Held** *(15 classes)*.
- `[vc8]` **One slot list per class family, generated from the image.** When
  a base and its subclasses are redeclared in many files (each with its own
  field layout), put the base's virtuals in a macro and each subclass's
  overrides in another (`#define X05_OVERRIDES void v01(int, int); ...`),
  computed by comparing every subclass vtable with the base's in the image
  (thunks followed); each new override name gets an alias line at the image's
  slot target. Missing overrides, a member wrongly attributed to a sibling
  class, and a wrong `??_E` line at another slot's address all show up at
  once. **Held** *(45 classes)*.
- `[vc8]` **Bases listed without `public` are private, and RTTI shows it**:
  base class descriptor attributes 0x4d (77) on the private base, 0x49/0x4b
  below it, against 0x40. `class D : public A, B, C` reproduces the image's
  descriptors; the compiler accepted every existing use. **Held**.
- `[vc8]` **A non-virtual member with a base virtual's name and signature
  silently overrides it** (and a misdeclared base slot hides real overrides):
  check both directions when a slot holds the wrong function; renaming the
  member to its address name (or giving the base slot its true signature)
  fixes the vtable without touching code bytes. **Held**.
- `[vc8]` **A scalar deleting destructor that stores the base vtable inline
  needs `virtual ~X() {}` in that file**; declared out of line it calls the
  destructor instead. Check the subclasses' destructors in the same file:
  if they call it out of line, the header must keep it out of line. **Held**.
- `[vc8]` **Multiple-inheritance vtable names follow the base names**
  (`??_7X@@6BBase@@@`): replacing placeholder bases renames every such
  vtable, so add the lines under the real base names with the same
  addresses; adjustor deleting destructors are `??_GX@@W<disp>AEPAXI@Z`.
  **Held**.
- `[vc8]` **A base whose layout ends early can still carry the real name**:
  when a file lays the subclasses' fields from the end of a shorter base
  (the image's base is 0x78 bytes, the shared header's is 0x80), give that
  file its own declaration of the real class (same name, same slot macro,
  its own fields) instead of a placeholder name; the RTTI and slots then
  match while the field offsets stay. Split the file if it also needs the
  shared header. **Held** *(ClassControl, the windows' constructors)*.
- `[vc8]` **Two identical image bodies of one destructor**: only one of them
  has callers; give the class's name to that one and keep the other matched
  as a plain function (vptr store and tail call), so the subclasses'
  destructors resolve their base call. **Held**.
- `[vc8]` **Audit the native build's own names against the image's slots**:
  a name the native build binds to an image address (only-native symbol
  list) can sit on another class's override; it hides as long as the
  native build uses the image's vtables, and fixing it can unmask base
  classes whose overrides were never declared (their vtables only passed
  through the wrong address). **Held**.
- `[vc8]` **A pure virtual destructor shows as `__purecall` in the slot**:
  `virtual ~I() = 0;` with an `inline I::~I() {}` definition. **Held**.
- `[vc8]` **A placeholder slot 0 whose image function is a scalar deleting
  destructor is the class's virtual destructor**: replace `s00()` by
  `virtual ~X() {}`, and point the `??1` line at the destructor's body, not at
  an incremental-link thunk. **Held**.
- `[vc8]` **A callee name the matcher "learns" from the first object can hide
  a wrong declaration elsewhere**: adding the explicit line can turn another
  object's identical function into a difference. Check every user of the
  name before mapping it. **Held**.
- `[vc8]` **A "learned" callee hides a misassigned deleting destructor**: a
  `??_G` line put on the neighbouring class's body (one byte pattern, a
  different callee) matched only because the callee was unmapped. Order the
  `??_G` bodies by their adjustor thunks and their callees before mapping the
  destructor they call. **Held**.
- `[vc8]` **A derived class may redeclare a base virtual as pure**: the
  image's `__purecall` in a middle class's slot is `virtual void f() = 0;`
  there, even when the root defines it. **Held**.
- `[vc8]` **Inline virtual destructor in a placeholder header, out-of-line
  copy in the image**: the root's `~X() { Free(); }` is inlined into every
  derived destructor, and its own out-of-line copy is emitted by a TU that
  defines it out of line; the derived destructors belong in the TU of their
  class (their `??_G` inlines them only there). **Held**.
- `[vc8]` **`throw()` on a member's or callee's declaration decides the EH
  frame**: an extra `mov BYTE [state], 0` before a member destructor, or a
  whole frame in a copy constructor, disappears when the callee is declared
  `throw()` as in the original. **Held**.
- `[vc8]` **A whole base family can move from placeholder to real name with a
  fieldless stand-in**: one header with the root's slot macro and a
  `class Root { ctor; SLOTS };` stand-in, plus per-class override macros
  generated from the image's vtables (the slot whose target differs from the
  root's), changes no code bytes. Watch members that silently become
  overrides (same name and signature as a now-declared base virtual): they
  change mangling (`QAE` -> `UAE`) and turn the class's own unqualified calls
  into virtual calls. Mark generated alias names `~` when the body at that
  address is known not to match. **Held**.
- `[vc8]` **`struct` vs `class` is part of the RTTI name and the mangling**:
  turning a `struct` into `class` changes `.?AU` to `.?AV` and every mangled
  name that takes or returns the type (`U` -> `V`, `AAU0@` -> `AAV0@`). **Held**.
- `[vc8]` **A scalar deleting destructor that inlines an out-of-line
  destructor comes from the TU that defines the destructor**, and in that TU
  the subclasses' destructors inline it too. When the subclasses' destructors
  call it (matched) but the base's `??_G` inlines it, give the base's
  destructor a file of its own. **Held**.
- `[vc8]` **A vtable checker that reads every object must take the worst
  verdict**: one stand-in copy of a class with fewer slots in some other file
  keeps the class "shorter". Fix every object that emits the vtable. **Held**.
- `[vc8]` **A slot overridden under another name in the matched sources**
  (base `PonerTamanyo(ushort...)`, override matched as `Volcar(int...)`): name
  the slot after the override in the vtable-emitting declarations and give
  the base's body an alias line; no code changes. **Held**.
- `[vc8]` **Both loads of a field pair before both stores** (`mov edx,[p+4c];
  mov ecx,[p+48]; mov [t+20],edx; mov [t+24],ecx`): an aggregate local
  initialised from the fields and copied out (`struct P { int w, h; } v = {
  p->a, p->b }; x = v.w; y = v.h;`). Named `int` locals load first but in the
  other registers. **held** *(match)*.
- `[vc8]` **A derived class's inline default constructor emits its vtable in
  the caller's file**: if the vtable checker then reports a hierarchy with one
  base too many, the header has an invented intermediate base. Derive as the
  RTTI says and redeclare the intermediate's methods on the class with alias
  lines at the same addresses. **held** *(match)*.
- `[vc8]` **`fcom st(1)` keeping both operands and one `fstp [slot]` after the
  two arms**: the ternary's value assigned to a new local (`float m = v < t ? t
  : v;`); assigned back to `v`, the `v = v` arm vanishes. **held**.
- `[vc8]` **One base load and two dword moves per element** is an 8-byte
  struct assignment (`a[k] = *p`); two `int` stores reload the base each time.
  **held**.
- `[vc8]` **A source pointer loaded before a loop's entry test and stepped by
  the element size** is a pointer local stepped in the `for` header (`for (k =
  0; k < n; k++, src++) dst[k].F(src);`). **held**.
- `[vc8]` **`push eax` of a call's hidden return pointer straight into a
  pointer parameter** is the call chained into the argument
  (`x.Leer(&tmp(b.Valor("K")))`); a named block per lookup gives `lea; push`.
  **held** (839 -> 28 bytes).
- `[vc8]` **A constructor inlined at a `new` site that also exists out of
  line**: declare it inline (empty body) in the caller's file; the other file
  keeps the out-of-line definition. **held** *(match)*.
- `[vc8]` **Two by-value structs with equal constant fields** are two locals;
  one local passed twice reuses its registers. The one built from fresh zero
  registers is the one pushed first. **held** *(match)*.
- `[vc8]` **A zeroed `char[3]` stored from the last byte down** is three
  stores written in that order; the `= {0}` initializer stores upward.
  **held** *(match)*.
- `[vc8]` **A jump table whose entry for one value is the shared exit**: an
  explicit `case K: return v;` (an empty `case K: break;` is dropped, and three
  cases become a compare chain). **held** *(match)*.
- `[vc8]` **Open**: a spilled scalar packed into the slot of an 8-byte struct
  whose life starts after the scalar's last use (VC8 does it; the image keeps
  both, declaration order and scope do not move it); two spilled flags whose
  slots stay put whatever their order, names or declarators.
- `[vc8]` **Float-heavy functions: fix the float code first, and read `eax`
  with suspicion.** Every x87 compare takes `eax` (`fnstsw ax; test ah,K`), so
  the integer registers around it follow the compare form; an `eax` sequence
  in the image may be an inline helper the port writes as a call. Order:
  frame slots and float temporaries (named float = `fstp`/`fld`, bare
  expression stays on the stack, float vs double literals, by-value struct
  temporaries), then compare forms (`a < b`, `b > a`, `!(b <= a)`, member
  left), then `math.h` float overloads, then the integer residue. Example:
  `fld [K]; fld d; fcom st(1); fstp st(1); test ah,1; jne` and one `fstp [d]`
  after both arms is `d = d >= K ? d - 360.0 : d;` (`!(d < K)` gives `test
  ah,5; jnp`). **held**.
- `[vc8]` **A down-counter whose registers differ from the image's explicit
  down-count**: the source counted up with a stepped pointer (`for (i = 0; i <
  n; i++, p++)`); VC8 turns that into the down-counter with its own register
  choice. **held** *(match)*.
- `[vc8]` **A member-pointer chain re-read for a second call with the
  argument in `edx` and the outer pointer in `eax`**: the chain goes through an
  inline accessor on the middle object (`a->Figura()->Parte(i)`). **held**
  *(match)*.
- `[vc8]` **Temporaries placed below a named object with a destructor**: the
  body is inside `if (ok) { ... }`, not behind `if (!ok) return;`; the early
  return changes how every later destructible local is packed. **held**
  *(match)*.
- `[vc8]` **x87 statements kept in order with every store, where the port's
  version merges stores and moves intrinsic `cos`/`sin` calls**: the
  intermediate is a by-value struct written to memory (`V t; t.x = ...; t.y =
  ...; d.x = d.x + t.x; ...`), which also gives the image's slots. **held**.
- `[vc8]` **`mov eax,[this+m]; lea ecx,[eax+K]` before a call on a
  sub-object**: the pointed object as a typed pointer local; char arithmetic
  on the member gives `mov ecx; add ecx,K`. **held**.
- `[vc8]` **The load order of a sum of squares** follows a by-value struct
  parameter of an inline (`M2(V2 v) { return v.y * v.y + v.x * v.x; }`); every
  spelling of the plain expression is canonicalised to one order. **held**.
- `[vc8]` **Three `fst` of one `fldz` into a member, last component first,
  right after the vptr stores**: the member has a constructor `x = y = z =
  0.0f` (the chained assignment stores z first). When the member's type is
  shared, a tag constructor (`enum Cero { CERO }; V3(Cero) { x = y = z = 0; }`,
  `meta(CERO)` in the initializer list) keeps every other use unchanged; a
  derived zeroed type with an `operator=` from the base changes the copies
  elsewhere. **held** *(match)*.
- `[vc8]` **A member triple assigned through a stack triple copied with
  integer moves**: a by-value temporary (`(V &)m = V(0, 0, 0)`); a negated
  value used by two temporaries is written `-v` at each use (VC8 CSEs it into
  one rounded slot), not as a named local. **held** *(match)*.
- `[vc8]` **`mov eax,ecx` at the top and `eax` never written again in a
  `void`-looking function**: it returns `this`. **held** *(match)*.
- `[vc8]` **A source pointer chosen between a field and a zero triple built
  only in the null arm**: `const V *p = o ? &o->pos : V(0, 0, 0).Dir();` with
  `Dir()` an inline returning `this` (`&V(...)` is VC8-only). **held**
  *(match)*.
- `[vc8]` **x87 adds with the field loaded first (`fld [esi+x]; fadd
  [esp+d]`) where ours loads the local first**: both operands through pointers
  (`float *pp = &x; float *q = d; pp[0] += q[0];`); any direct spelling keeps
  one element in the other order. **held** *(match)*.
- `[vc8]` **The three integer loads of a triple before its three stores**: the
  copy straight into the destination member as a three-int struct; through a
  local pointer to the destination the copy goes word by word. **held**
  *(match)*.
- `[vc8]` **`fld x; fadd st,st` for a doubling**: `2.0f * x` (also `x * 2`);
  `x + x` loads the value once and keeps it. **held**.
- `[vc8]` **A branch placed after the function's return**: a value set by an
  `if (fail) v = ...; else switch (...)` instead of a `goto` into the shared
  tail; VC8 lays the `else`-less form inline. **held** *(match)*.
- `[vc8]` **`add esi,K` replacing `this` after a call on a sub-object, the
  later fields addressed from it**: the sub-object as a typed pointer local
  and the fields written through it (`T *s = (T *)(this + K); s->f(); ((int
  *)s)[n] = 0;`). **held** *(match)*.
- `[vc8]` **A struct copy whose pointer advance (`add eax,K`) lands after the
  first word in the image and after the second in ours, with identical
  source**: a later `for` loop of clears in the same function changes the
  schedule of the block in front of it; the clears written out as single
  stores give the image's (a sibling that matched had them written out).
  **held** *(match)*.
- `[vc8]` **Integer clears and a by-value temporary's copy interleaved
  differently** (one word of the temporary stored late): the order of the
  source statements decides it, not the members' order; move the clears in
  front of the temporary. **held** *(match)*.
- `[vc8]` **`fld K; fcomp [param]` for a literal compare against a float
  parameter**: `!(r < 2.0f)` or `r >= 2.0f`; `2.0f <= r` loads the parameter
  and compares a double constant, an `extern float` constant gives `fld; fld;
  fcompp`. **held**.
- `[vc8]` **A local's frame slot lower than the image's** where the local is
  reassigned later (`v = k * v`): the new value as a new local (`float n = k *
  v`) puts both in the image's slots. **held** *(frame)*.
- `[vc8]` **A doubling `fld [p+K]; fadd st,st` with the base pointer advanced
  in between (`add edi,K'`)**: one pointer to the record kept for a later use
  (`float *v = (float *)(o + K'); x = v[1] + v[1]; ... f(v)`), the other stores
  after the doubling. **held** *(match)*.
- **No header dependencies in a reconstruction Makefile hide broken users**:
  after retyping a member, delete every object (or all that include the
  header) before trusting the match count; a stale object kept matching while
  its source no longer compiled, and only the native build failed. **held**.
- `[vc8]` **A member constructor's `lea ecx,[this+K]` in front of its argument
  pushes** (ours after them): give the member a small derived type whose inline
  constructor passes the arguments on (`struct R : Base { R() : Base(0, 0.75f)
  {} ~R(); };`, nothing for it in the owner's initializer list). Declare the
  derived destructor and bind its name to the base destructor's address, or VC8
  emits an implicit one out of line and the owner's destructor calls that.
  **held** *(match)*.
- `[vc8]` **A literal compare in the image where ours reuses a register a
  just-tested call result left holding the same constant** (`cmp [eax],2` vs
  `cmp [ecx],eax`): read the pointer through an inline accessor; it loads into
  `eax` and the known value is no longer propagated. **held** *(match)*.
- `[vc8]` **Gotos into shared tails give another block order (and several
  copies of a failure call) than the image**: set a selector variable in every
  arm (`cual = 1/2/0`) and put one `if (cual == 1) {...} else if (cual == 2)
  {...} else fail();` behind it. VC8 threads each assignment to its arm; the
  order of the chain's tests and of the arms' assignments decides which tail
  is fallen into. The same with a result: `if (ok) r = 1; else r = 0;` behind
  arms that set `ok`. **held** *(match)*.
- `[vc8]` **A callee the image inlines once behind two tails, while it lives
  in another file of the reconstruction**: clone it as an inline member of a
  derived overlay and call it once after the chain (the failing arm returns);
  called in both arms it is expanded twice. **held** *(match)*.
- `[vc8]` **Two variables loaded from one dead parameter slot on an edge
  block**: they are uninitialised on that path in the original; leave them
  unset (give the other compiler a deterministic value under an `#ifdef`).
  **held** *(match)*.
- `[vc8]` **Exit blocks (each its own epilogue copy) in each other's place**:
  the blocks follow the source order of the labels, also of labels that only
  forward (`goto fin; found: goto fin; empty: fin: return r;`). **held**
  *(match)*.
- `[vc8]` **A value just copied by a struct assignment reloaded from memory**:
  read it back from the source of the copy, not the destination; the register
  is forwarded. A four-word plain struct copies as three loads, a store, a
  load, three stores. **held** *(match)*.
- `[vc8]` **`mov [m],0; xor eax,eax` at two exits against `xor eax,eax; mov
  [m],eax`**: the early exit was a copy of the function's tail; write it as a
  jump into the one tail and the compiler duplicates the tail after constant
  placement. **held** *(closer)*.
- `[vc8]` **Register roles exchanged through a small loop, every instruction
  otherwise identical**: try the other spellings of a doubling (`x += x`,
  `x <<= 1`, `x *= 2`): all emit `add reg,reg` but number the variables
  differently. `mov reg,1` against `lea reg,[zero+1]`: declare the `1` before
  the `0`. **held** *(match, three functions)*.
- `[vc8]` **A store to a global placed behind the compare that reads memory**:
  write the store in both arms of the test (`if (c <= m[i]) { g = c; ... }
  else g = c;`); the two are merged into the compare's block, behind it.
  **held** *(match)*.
- `[vc8]` **`fstp t; fld t; fstp v` for a float `?:`**: the result
  initialises a `double` local that is then narrowed. **held** *(match)*.
- `[vc8]` **A search loop whose exhausted exit returns directly while the
  found exit tests the index against -1**: start the index at -1 and set it
  where the loop breaks; testing the loop variable after the loop keeps both
  tests. **held** *(match)*.
- `[vc8]` **`fadd DWORD` (2^32 as float) behind `fild` of an unsigned**: the
  unsigned is cast to `float`, not promoted to `double`. Keep the `double`
  form for a compiler that would round. **held** *(match)*.
- `[method]` **A mechanical mutant sweep over the near-misses pays once the
  hand-found idioms dry up**: when one file compiles and compares in well
  under a second, build every single mutant (adjacent statements exchanged,
  commutative and comparison operands exchanged, `if`/`else` arms, signedness
  and float/double of locals, spellings of a doubling, a single-use local
  substituted) and every pair for small functions, in parallel copies of the
  sources. Rank by differing bytes but read each lead against the image: a
  mutant that changes the meaning can score lower, and a pure register
  exchange scores worse than a wrong body. **held** *(six matches in a round
  where forty hand variants per function gave none)*.
- `[method]` **The accessor idiom can be searched mechanically: wrap each value
  read in an inline identity**. Add `template <class T> static inline T
  Same(T x) { return x; }` to the file and build one mutant per read
  (`Same(o->state)`, each prefix of a `->` chain, each plain identifier,
  indexed and dotted places), plus "every read of one expression at once" and
  "all but one". To the optimiser the inlined identity is an inline accessor
  on the object: the value passes through the inline's temporaries and the
  registers are numbered as in the original (`eax` for a member-pointer chain,
  a compare with a literal instead of a register holding it, the object's vptr
  loaded before the arguments). Write each hit as a real accessor afterwards;
  the bytes stay. Cheap (one build per read) and worth running over every
  unmatched function, large ones included. **held** *(seventeen matches in one
  round, 63 to 1666 bytes, where the kinds of the first sweep had run dry)*.
- `[vc8]` **Two reads of one member must both go through the accessor**: either
  alone changes nothing, both exchange two callee-saved registers through the
  whole function. **held** *(match)*.
- `[vc8]` **An assignment whose object pointer is loaded before the value
  (`mov ecx,[this+a]; mov eax,[this+v]; mov [ecx+K],eax`)**: an inline setter
  (`a->SetK(v)`); in the sweep, the identity on both the object and the value.
  **held** *(match)*.
- `[vc8]` **`if (p->m) p->m->Call(args)` with an argument loaded before the
  test**: an inline member of `p` doing the test (`void M(args) { if (m)
  m->Call(args); }`); arguments are evaluated before the inlined body.
  **held** *(match)*.
- `[vc8]` **The vptr loaded and the slot address formed before the argument's
  call**: the object goes through an inline (an accessor, or a member
  returning `this`). **held** *(closer)*.
- `[vc8]` **A jump table with more index bytes than the port's cases, some
  pointing at a second entry that holds the default's address**: those cases
  were written with their own copy of the default's body; the bodies merge,
  the table entry and the range stay. Bare `case` labels in front of
  `default:` are dropped. Dump the image's tables for every switch port.
  **held** *(match, 339 bytes)*.
- `[vc8]` **A temporary's address pushed straight from the call that made it,
  the temporary destroyed before the next statement**: pass it through
  `template <class T> static inline T *AddrOf(const T &x) { return (T *)&x;
  }` (portable; `&f()` is not). A named local dies after the following
  statements. **held** *(match)*.
- `[vc8]` **Two `ftol` results kept in call order and used in store order**:
  the results go into a two-int struct local; as two `int` locals the second
  is evaluated first. **held** *(closer)*.
- `[method]` **What the wider sweep did not find**: loop forms, stores in both
  arms, early return against a wrapped body, chained assignments, float
  against double literals, negated comparisons, a new local for a reassigned
  value gave nothing that held over 950 functions; moving a statement across
  three or four gave many lower byte counts and no match (it mostly moves a
  store across a call, which is wrong). **negative**.
- `[vc8]` **A strength-reduced loop pointer over an array of structs anchored
  on another field than the original's (`lea edi,[this+0x12c]` with the
  later store at `[edi+0x10]`, yours `[edi-16]`), often with one more saved
  register**: the pointer is anchored on the last place the loop body names
  in source order. A store that follows a call is written through a
  reference taken before the call (`unsigned &f = a[i].flags;
  a[i].p->Call(); f |= 0xc;`): named first, stored last. A statement swap
  that gives the right bytes but for the store's position is this lead.
  **held** *(match, 231 bytes)*.
- `[vc8]` **A store scheduled one statement early or late against the
  original**: declare a reference to its place one to three statements up
  (`int &px = a->px; p.y += oy; px = p.x;`). As a sweep mutant: every `place
  op= value;` with a reference of each plain type; a wrong type does not
  compile. **held** *(3 matches)*.
- `[vc8]` **The operand order of a commutative x87 operation (`fld [m]; fmul
  [v]` against `fld [v]; fmul [m]`) follows the number of initialised locals
  declared before the expression, not the way the product is written**:
  dead `int s1 = 0, s2 = 0, ...;` at the top of the body turn it round
  (measured: 5 to 7 match, 1 to 4 and 8 do not; names and types do not
  matter; uninitialised or later declarations do not count). The original
  had that many more variables or inline temporaries; when they cannot be
  recovered, keep the dead locals. As a sweep mutant: 1 to 12 at the top,
  1 to 8 before each top-level statement. Try it on every two-to-five byte
  x87 leftover before anything else. **held** *(3 matches)*.
- `[vc8]` **`lea eax,[this+i*S+base]` and the field at `[eax+k]` in both arms
  of a test, yours folded to `[eax+base+k]` or with the load hoisted out of
  the arms**: the element pointer is a local taken first and the store in
  front of the test goes through a reference or an inline setter.
  **held** *(match)*.
- `[vc8]` **`or ecx,-1` on a search loop's exhausted path, then an unsigned
  compare with the count**: an inline search returning the index or -1.
  **held**.
- `[method]` **Two scans worth running once over every port**: the `cmp` in
  front of each indirect table jump, original against yours (a switch range
  or count that differs: cases the port lacks, loops written out per case),
  and the `lea` displacements (a loop pointer or element address anchored
  elsewhere). **held**.
- `[method]` **Sweep mutants that did not pay**: the identity by `const T &`,
  `*Id(&x)`, the identity around whole arguments and conditions, `x > 0`
  against `x >= 1`: lower byte counts that were wrong when read against the
  original, one match that was the dead-local effect in disguise.
  **negative**.
- `[vc8]` **The original re-tests a condition it has just decided (a loop's
  zero-trip exit jumps to the test behind the loop instead of past it) and
  loads a parameter inside the loop, yours threads the jump and keeps the
  parameter and a zero in callee-saved registers**: the loop's bound and the
  later test name different things that hold the same value (`for (i = 0; i
  < m_count; i++) ...; if (i < n)` with `n = m_count` taken at the top).
  VC8 threads on names, not values. One spelling changed 250 bytes.
  **held** *(match)*.
- `[vc8]` **`mov ecx,edi` at the top of every pass of a loop over records,
  then the store through `ecx`, and the pointer's `lea` behind the zero-trip
  test**: a post-increment walk whose old value is the object of an inline
  member, in a `do` loop inside the counter's own test: `unsigned j = 0; if
  (j < n) { Rec *p = recs; do { (p++)->Mark(v); j++; } while (j < n); }`.
  Without the inline member the copy folds away; a `for` with the pointer in
  front moves the `lea` and exchanges the registers; no indexed form gives
  the copy. **held** *(match, two more loops)*.
- `[vc8]` **`fld [K]; fcomp [x]` where yours loads both and uses `fcompp`,
  or a constant loaded once where yours loads it twice (or the reverse)**:
  check whether `K` lives in read-only data. A float constant that an
  earlier naming pass turned into `extern float g_xxx` is a variable to the
  compiler; the literal folds into the compare and is a different symbol for
  the x87 stack. Scan: every named float/double whose address is in
  `.rdata`, used in an unmatched function, spelled as its literal (all uses,
  and each alone). **held** *(ten leads, 78 -> 17, 143 -> 76)*.
- `[vc8]` **A statement the original runs in front of a block's destructor
  call** belongs inside that block. **held**.
- `[method]` **Typed mutants without `auto`: read the type off a deliberate
  compile error** (`struct T {}; T &r = place;` prints `cannot convert from
  'unsigned int' to 'T &'`). One extra build per place gives references and
  element pointers of struct and pointer type for a mechanical sweep.
  **held**.
- `[vc8]` **Dead initialised locals as a general lever (allowed by the owner
  as a matching tool)**: swept as 1 to 8 at the top, in front of every
  top-level and every nested statement, as `int`, `float`, `double`, `char`,
  pointer, with 0, 1 and -1. Outside x87 operand order the effect is rare:
  in 300 small functions three byte counts moved (frame slot order) and none
  matched; type and initial value never mattered beyond the count; no
  register choice, store schedule or frame size moved in functions opened
  by hand. Worth a sweep, not worth hand time. **mostly negative**.
- `[method]` **A byte count that gets worse can be the right direction**: a
  structural fix shifts everything behind it (218 -> 293 -> 40 -> 0 on one
  function). Rank hand variants by reading the listing, not by the count.
  **held**.
- `[vc8]` **One tail repeated with different scratch registers behind a
  `switch`**: the call is written at the end of every case with its arguments
  spelled out; the compiler merged the copies that came out identical. One
  call behind the switch gives one tail. **held**.
- `[vc8]` **Float and integer stores of one run are scheduled alternately,
  whatever the source order.** Where the original has two float stores in a
  row once, a reference to the float member stored just before, declared in
  front of the run, delays the integer stores by one. For two integer fields
  of a record local, the second through a reference declared first exchanges
  the two stores. Sweep it: split lines of several statements first, then
  try a reference for every store's place. **held**.
- `[vc8]` **`fcomp QWORD PTR [K]` where yours has `fldz; fcom`**: the zero
  (or one) is a named constant used as a variable. Only 0.0 and 1.0 show the
  difference, because every other literal is a memory operand either way.
  The inverse case (a literal where the port has an extern) also exists.
  **held**.
- `[vc8]` **Two pushes in yours, `sub esp,8; mov eax,esp; mov [eax],..; mov
  [eax+4],..` plus an EH state store in the original**: the argument is a
  record by value with an inline constructor and a user copy constructor.
  **held**.
- `[vc8]` **A float result in a dead parameter's home slot, stored and
  reloaded between steps**: a local assigned in statements (`d = sqrt(m); d
  = d - r; if (d < 0) d = 0; return d;`), not one conditional expression.
  A branch that recomputes values the loop already has on the x87 stack has
  its own locals; that also explains a larger frame. **held**.
- `[method]` **A sweep lead is a pointer to a function worth reading, not a
  fix**: three matches of one round came from functions whose mechanical lead
  was noise (a mutant that deleted code, a wildcard symbol), read against the
  listing from the top. **held**.
- `[method]` **Scan the original for a constant's address instead of
  sweeping mutants**: list the unmatched functions whose body holds the
  address of a named constant the source does not name. A mutant sweep with
  a wildcard symbol scores length coincidences (a function whose only
  difference is a shift "improves" under any mutant of the right length).
  **held**.
- `[vc8]` **A first pointer parameter that receives three stores at the top
  and a loads-first 12-byte copy behind each call is a hidden return
  pointer**: the function and its callees return a small class by value.
  Assigning through a cast of a pointer parameter interleaves the copy.
  **held**.
- `[vc8]` **Exit blocks are laid out in label order**: where two early exits
  differ in their pops, a `goto fin` for the first test and a nested `if`
  for the second gives the exit without the pop first; early `return`s and
  nesting give other orders. **held**.
- `[vc8]` **A frame slot nobody touches**: an array or vector local with an
  element that is never written; dead scalar locals do not add it.
  **held**.
- `[vc8]` **Commutative x87 operand order answers to inline getters** on
  the member reads, when exchanging operands, locals and casts do not.
  Bodies shared through a forced-inline helper are invisible to a sweep
  over the ports: open them by hand. **held**.
- `[vc8]` **`fadd QWORD PTR [0.0]` (or `fsub QWORD PTR [5.0]`) in a vector
  sum is a propagated float constant**: an inline `operator+=` or an
  inline mover with float parameters, called with a vector whose z was
  stored as `0.0f` or with literal arguments. The constant comes out as a
  double memory operand, a negative one as `fsub`. A literal `+ 0.0` in
  a statement gives `fldz` forms. A named zero variable is the answer
  only for comparisons. *(two matches)*
- `[vc8]` **A by-value class argument copied float by float with no
  `mov [$T],esp` in a function with an EH frame**: user copy constructor
  AND the call made by an inlined forwarding member; `throw()` does not
  remove the store. *(match)*
- `[vc8]` **Two values derived from one member read, kept in two
  callee-saved registers across a call**: two locals computed from the
  member itself in front of the call; from a local copy the compiler
  keeps the copy and recomputes. **held**.
- `[vc8]` **Two epilogues behind a flag test where yours is
  `neg; sbb; not; and`**: `if (m & 4) r = 0; return r;`, not
  `if (m & 4) return 0; return r;`. **held**.
- `[vc8]` **A byte count that does not move while the source changes**
  means a frame-size or length difference is shifting everything: read
  the differing rows, not the count.
- `[vc8]` **`fild; jge; fadd DWORD PTR [2^32]` is `(float)unsigned`**; the
  double conversion adds a QWORD constant. Decompilers render the three
  instructions as a `long double` temporary plus a hand-written
  `+= 4294967296.0`, and ports keep that: write `(float)u`. Scan the
  image of the unmatched ports for the float constant's address behind
  `d8 05`. *(eight matches from one scan)*
- `[vc8]` **A `char x[2]` member in a local record brings the security
  cookie** (frame four bytes larger, cookie check at every exit): pad
  with separate `unsigned char` members. Stores into a local record come
  out in source order; a chain `a = b = c = 0` stores right to left.
  **held**.
- `[vc8]` **A float result stored twice into one slot** (`fstp [t]; fld
  [t]; fstp [t]; fld [t]` behind a call): an inline helper that assigns
  to its parameter, `v = (float)sqrt(v); return v;`. Returning the cast
  expression gives two slots. *(match)*
- `[vc8]` **One reload of a member pointer behind a test and none between
  two stores through it**: the test reads through a pointer local, the
  stores spell the member. Two pointer locals are merged; the member
  spelled in the test reloads but changes x87 operand order. *(match)*
- `[vc8]` **A body shared by several functions through a `noinline`
  helper in a port** shows as a handful of tiny "differing" functions:
  write the body out in each. **held**.
- `[vc8]` **A register convention noted as unreachable can be a port
  mistake**: a value the port keeps alive across a call (and the image
  does not) takes a callee-saved register and shifts every argument
  register. Re-read what the image stores before blaming link-time
  code generation. A constant kept in a caller-saved register ACROSS a
  call, on the other hand, is link-time code generation and cannot be
  had. *(match / held)*
- **A matcher that reports a difference of zero bytes** is reporting a
  relocation target: a constant with another value (`0.0001` against
  `(double)0.0001f`), or another symbol.
- `[vc8]` **A helper whose arguments arrive in registers no keyword
  gives (a pointer in `ecx` with a plain `ret`, arguments in `esi`,
  `edi`, `ebx`, the caller popping a float) is link-time code
  generation, and it CAN be had**: copy the body as `static
  __declspec(noinline)` into a file of its own, add two callers that
  pass different arguments and are reached through `volatile` function
  pointers from `main`, compile with `/GL`, link with `/ltcg`, compare
  the linked code. Pointers go to registers, floats and by-value records
  stay on the stack. A function "returning into `edi`" is a by-value
  class return whose hidden pointer became a register argument. Keep the
  plain port for the native build; list ports whose notes name a register
  convention and try them all. *(three matches from the list's first
  five)*
- `[vc8]` **A comparer for linked `/GL` code must follow
  incremental-linking thunks**: the original calls `jmp` stubs, the
  symbol table names their targets.
- `[vc8]` **Three floats copied with integer moves between float
  stores** are a record assignment (`*(Pod *)&o->x = *(const Pod *)p;`),
  written in front of the float assignments that surround it in the
  code. *(match)*
- `[vc8]` **`cmp eax,ebx` against a zero register behind a lookup**
  (ours `test eax,eax`): the lookup's result and the object created when
  it is null are one variable. **held**.
- `[vc8]` **Two callee-saved registers exchanged over a whole function
  follow the weight of the uses inside loops**: spell a loop's stores
  through the object (`a->m.v[i]`) instead of a pointer local, or move a
  loop into a helper, and the two swap. Dead locals add no weight.
  **held** *(715 -> 58, open)*.
- `[vc8]` **Frame slots that seem shared by live variables** are one
  vector local reused as scratch (a point's copy, a count as float, a
  sum, all in the slots of a vector that was dead in between); pass it
  by reference to the inline that needs the scratch. Named floats keep a
  later sum's operand order; constructor-argument temporaries are loaded
  first. **held**.
- `[vc8]` **A `{0}`-looking prologue of one dword, one byte and a
  `memset` of n-1** in a function that returns a class by value on two
  paths: the dword is the compiler's "result constructed" flag, the rest
  is `char buf[n] = "";`. *(match)*
- `[vc8]` **A helper with arguments in `esi`/`edi`/`ebx` and a plain `ret`
  is a `static` function of an ordinary `/O2` file, not `/GL`.** VC8
  gives a static whose address is never taken its own convention (the
  listing says `_p$ = edi`). Make the port's helper static with its
  callers in the same file and compare it object by object. `/GL`
  differs: a `double` on the callee's stack realigns every caller
  (`and esp,-8`) and constant arguments are removed. *(match)*
- `[vc8]` **A `/GS` cookie changes a static's register arguments**: no
  cookie in the image means no `char` array in the frame, and the
  register set follows. *(match)*
- `[vc8]` **`sub esp,0xc; mov eax,esp; fld/fstp [eax]` for a point
  argument** is a by-value class with a user copy constructor. *(match)*
- `[vc8]` **`fcom st(1); test ah,0x41; jne; fxch; fstp min; fstp max`**
  is `if (a < b) { min = a; max = b; } else { min = b; max = a; }`. *(match)*
- `[vc8]` **The parameter order of a `static` helper decides which
  register each pointer gets** (and the caller's `mov` order in front of
  the call). Try every order before blaming the body. *(match)*
- `[vc8]` **A local in a static callee can move a register argument**
  (`int k = o->k;` made `o` arrive in `eax` instead of `edi`); the caller
  then matches by itself. *(match)*
- `[vc8]` **Count a custom-convention helper's stack arguments from its
  own `[esp+n]` reads** (one argument read at two push depths looks like
  two). *(match)*
- `[vc8]` **Differences spilled as `QWORD` and hoisted in front of a loop**
  are common subexpressions of a formula written in full at each use, not
  `double` locals. *(match)*
- `[vc8]` **Commutative float sums are ordered by where their leaves were
  defined, not by spelling**: `u.x = e.y; u.y = -e.x;` then scaling both
  gives the y product first; reordering the sum does nothing. *(match)*
- `[vc8]` **Two address-taken locals take their frame slots from the push
  order of the last call that uses them** (swap the helper's parameter
  order), not from declarations. *(match)*
- `[vc8]` **A value loaded before a call and pushed after it, or an inner
  call made before the outer pushes**, is a named local for that argument.
  *(match)*
- `[vc8]` **One `||` condition gives one shared `return 0` tail at the
  end**; separate `if`s place the tail behind the first test. *(match)*
- `[vc8]` **Float differences in two adjacent slots, a scratch slot, then
  the third (`dx, dy, t, dz`)** are the constructor arguments of an inline
  vector `operator-` (`V d = a - b;`), the 2D length through an inline
  taking the vector by reference, and the scratch is the root's. Scalars
  and a hand-filled record both give other frames. *(match)*
- `[vc8]` **`fld x; fld k; fld st(0); fmulp st(2),st; fld e; faddp
  st(2),st` for a scaled-and-offset point** is ONE inline returning a
  temporary, `d = Mix(d, k, e)` with `V(a.x * k + b.x, ...)`; three
  statements or two operators (`d * k + e`) store the products first.
  *(match)*
- `[vc8]` **The same vector with its address taken once** (a member call
  in one branch) shows the constructor slots plus a named copy: write the
  copy (`V dd = a - b; V d(dd);`), compute on `dd`, call on `d`. *(match)*
- `[vc8]` **A jump table with more entries than live cases** has the dead
  cases written out (`case 0: k = 0.0f; break;`). *(match)*
- `[vc8]` **`lea ecx,[local]; call X` where the port zeroes a record by
  hand**: the record has an out-of-line constructor; declare it under that
  class. A record copied with integer moves and one float member written
  after it is `q.p = p; q.p.z = p.z + K;`. *(match)*
- `[vc8]` **Records built by an inline constructor show as stores in
  member-constructor order interleaved with the first statements** (floats
  zeroed last element first, pointers cleared, then the caller's own
  stores): move the zeroing into a constructor of a local subclass.
  *(match)*
- `[vc8]` **`x ? x->m ? x->m : x : x` compiled as `cmp; je L; mov r,[x+m];
  cmp; jne M; L: mov r,x`** is an inline with a local: `{ T *v; if (!b ||
  !(v = b->m)) v = b; return v; }`. *(match)*
- `[vc8]` **A `setcc`/`sbb` value between runs of constant stores in the
  "wrong" register** (the image: `mov eax,1; stores; xor eax,eax; cmp
  [flag],ebx; sete al; ...; mov [m],eax; mov eax,5`; ours computes the test
  in `ecx` in front of the ones and loads the 5 early, for every statement
  order, ternary, `m = flag == 0`, `bool`, helper): the value is a **local
  assigned in both arms of an `if`**: `int k; if (flag) k = 0; else k = 1;
  m = k;` (`k = 100; else k = 0;` for `neg; sbb; and`). The branch-free form
  is made late, after the constants got their registers. One fix matched a
  family of 17 functions. *(match)*
- `[vc8]` **A callee defined in front of its caller in the same file tells
  the compiler what it writes, without `/GL`.** Signs: members of an escaped
  local stay in registers across the call (`mov esi,[d.a]; push esi; call f;
  ... cmp esi,ebx`), a zero stored through the member's register (`xor
  esi,esi; mov [d.a],esi`) with another zero register live. No local copy,
  reference, inline or type reproduces it; moving the callee's body into the
  file, in front of the caller (address order), does, with the plain source
  (`f(d.q, d.a); ... d.a = 0; ... if (d.a)`). Put functions in the file of
  their address neighbours before fighting register differences around
  calls. *(match)*
- `[vc8]` **The callee's register use is known too: `ecx`/`edx` read after
  a direct call means the callee was defined in front of the caller.** VC8
  keeps, per function already compiled in the translation unit, which of
  `eax`/`ecx`/`edx` it writes (and whether it writes memory), and later
  callers use it without `/GL`: `this` is never saved (`call f; mov
  eax,[ecx+0x400]`), or sits in `edx` (`mov edx,ecx` at the top), or a zero
  lives in `edx` across the call. Proof by stub: `__declspec(noinline) int
  C::f(int) { __asm xor eax,eax __asm xor edx,edx }` in front of the caller
  gives the image's code, the same stub with `xor ecx,ecx` the usual
  `mov esi,ecx`. Cure: the callee's real definition in the caller's file, in
  front of it, `noinline` on its declaration when `/O2` would inline it.
  Find every case from the image instead of from diffs: per unmatched
  function, after each direct call, a read of `ecx`/`edx` before any write
  where the callee's own body does not write that register (skip `push ecx`
  in front of `fstp [esp]`, the slot idiom). A file that says "kept apart
  because a visible definition changes the callers' register use" is this
  lever applied backwards for one caller. Four ports matched from eleven
  hits; an over-claiming stub (empty body) changes most callers and proves
  nothing, use the image callee's exact set. *(match)*
- `[vc8]` **The same inline written with `return` in both paths** (`{ T *v;
  if (b == 0 || (v = b->m) == 0) return b; return v; }`) takes its register
  after the other values loaded for the same call; with the result assigned
  to the local (`v = b; return v;`) it takes the first free one. Try both
  when an inlined select is one register off. *(match)*
- `[vc8]` **`mov ecx,[g]; mov ecx,[ecx+off]` in front of the argument
  pushes of a member call** is the object in a local (`M *m = g->mapa;
  r = m->F(a, b);`); in the call expression it loads after the pushes and
  moves the result's register. *(match)*
- `[vc8]` **Sibling functions from one `__forceinline` body with constant
  arguments** (a reach by `const float &`, kinds and sounds as `int`s, a
  differing tail under `if (kind == K)`) keep the bytes of each: diff the
  siblings' image text first to find the constants. *(match)*
- `[vc8]` **Address-taken locals share a dead parameter's stack slot only
  when their lives end at a block.** Sign: the image writes an out-value
  into a parameter's home slot (`lea ecx,[esp+param2]; push ecx`) and a
  second out-value later into the same slot; one variable reused, or two in
  function scope, get slots of their own (`sub esp` 4 or 8 larger). Put
  each in its own `{ }` with the code that reads it. *(match)*
- `[vc8]` **Register-argument wrappers are a work list.** Where a port
  answered "the image passes arguments in registers" with a naked wrapper
  (`push edi; push ecx; call Port; ret`), the original is a `static`
  function and every caller was in its file. List the wrappers, find the
  callers with a rel32 scan of the image, and merge: the callee `static`
  with its real body in front, the callers behind it in address order,
  the wrapper deleted (a static called from `__asm` has its address taken
  and keeps `__cdecl`, so the wrapper itself blocks the match). Callers
  come within a few instructions even while the static is still a port,
  provided its registers come out as the image's; when they do not (the
  convention follows the callee's body, not the parameter order), the
  callers wait. Helpers that only matched under `/GL` matched as plain
  statics once their callers shared the file. *(14 matches in one round)*
- `[vc8]` **A parameter fed with a temporary is a `const T &`.** `call
  Make; mov edi,eax; call F; mov edi,eax; <destructor>; test edi,edi`: the
  temporary dies at the end of the statement that calls `F`, before the
  result is tested (`x = F(o, b->Make()); if (x) ...`). A named local is
  destroyed after the use and its address formed with `lea`. *(match)*
- `[vc8]` **Operand order of a commutative float expression: put the
  operands through an inline.** A plane height written out
  (`y * p->b + x * p->a + p->c`) takes one product first whatever the
  spelling once x and y sit on the x87 stack; an inline member with
  by-value floats (`float Alt(float x, float y) { return b * y + x * a +
  c; }`) gives the other. Likewise a vector zeroed inside an inline
  constructor (`V(float a) { x = y = z = a; }`) against the same chain
  assignment in the function, and a point built by a two-argument inline
  constructor against two member stores: same stores, other product order
  later. Try these before permuting the expression. *(4 matches)*
- `[vc8]` **A comparison loads its plain operand first when that operand
  is written on the left.** `fld p[2]` in front of the computation, then
  `fcomp st(n)` from the computed side, is `p[2] >= (float)(z - K)`; with
  the plain operand on the right the computed value is stored, reloaded
  and the plain operand loaded last. `hb > ha + r1 || hb < ha - r2`
  likewise. *(match)*
- `[vc8]` **`xor eax,eax; push eax` for a constant zero argument** is a
  one-word POD record passed by value; an `int`, a pointer or an inline
  returning 0 give `push 0`. Two `(int)` conversions whose second word is
  stored before its `fild` and sits next to the first are a two-`int`
  record, not two locals (the frame is 4 bytes larger). *(match)*
- `[vc8]` **A file-local copy of a matched static must carry the matched
  body**: the comparison follows the caller's relocation into the callee,
  and a logically equal spelling (`!((f >> 2) & 1)` for `~(f >> 2) & 1`)
  fails the caller with zero differing bytes.
- `[vc8]` **Diff the image against itself before porting a sister.**
  Strip the addresses from two listings and `diff` them: a helper often
  exists two or three times with different conventions (a `static` with
  register arguments, a `__cdecl` copy behind a thunk, a member that
  never reads `this` and ends in `ret N`). One matched source then
  matches the others for a build each; only a constant's type may differ
  (`fcomp DWORD` against `fcomp QWORD` of `(double)0.0001f`). *(3 matches)*
- `[vc8]` **`ret N` with the first argument read from the stack, and
  `mov ecx,reg` at the call sites**: a member function that does not use
  `this`, not `__stdcall`.
- `[vc8]` **A dot product through an inline member of a small record**
  (`V2 n(d.y, -d.x); c = n.Dot(p);`) loads the record's member in front
  of the other operand and keeps the record's unused word in the frame;
  written out, the other operand is loaded first and multiplied from
  memory. *(3 matches)*
- `[vc8]` **A result built member by member** (`r.x = ...; r.y = ...;
  r.z = r.y * b + r.x * a + c; r.x -= ox; return r;`) shares one scratch
  slot between the members and reloads `r.x` for the last statement;
  float locals copied into the members stay on the x87 stack instead. A
  by-value return (hidden pointer, `mov eax,ecx`) and an explicit out
  pointer are different code. *(match)*
- `[vc8]` **Address-taken locals: one variable per block.** When two
  uses of "the same" out-parameter slot are far apart (a loop's
  parameter, later a call's), declare one variable in the block of each
  use; they then share a frame slot. One function-scope variable takes a
  slot of its own whose position no declaration order changes.
  *(match, 708 bytes, the last 32 lines)*
- `[vc8]` **Nested `if`s order the failure tails**: `if (a && b) { if
  (c) { ... } }` lays the inner failure tail first, `if (a && b && c)`
  the outer one. *(match)*
- `[vc8]` **A static called from several files can live in an include
  file**: each includer gets its own static copy (same address in the
  comparison); for a second compiler that needs one external
  definition, guard the body so that one file defines it and the others
  see a declaration. *(match, three callers)*
- **Do not delete a register-convention wrapper before the wrapped body
  is close.** The wrapper gives the callers the original's registers; a
  far body as a real static gives its own, and near-matching callers
  fall back by hundreds of lines. Body first, wrapper last.
- `[vc8]` **Rank every unmatched port against the MATCHED functions, not
  only against exact twins.** Mask every number in both instruction
  streams, prefilter by length and mnemonic histogram, take the `difflib`
  ratio; at 0.9 and above the matched neighbour's source text with the
  constants changed matches at the first build (a decompiler-shaped port
  differs in everything the neighbour's author already solved: member
  records with constructors, static arrays of constructor temporaries,
  typed scratch records). Read the neighbour's source before the image.
  *(5 matches in one round)*
- `[vc8]` **Fields of `this` reached through a cast pointer change a
  virtual call**: with a by-value record argument copied between the
  vtable load and the call, `char *t = (char *)this` (or a shadow struct
  pointer) gives `add ebx,4` ... `mov edx,[ebx]`; real members of the
  class give `mov edx,[ebx+4]`. Declare a local class with the members
  (only the mangled name matters). *(match)*
- `[vc8]` **A two-word record with a copy constructor passed by value
  from a named local** (`{ T p(a, b); f(p); }`): both words loaded, `sub
  esp,8; mov ecx,esp`, both stored. The constructor temporary as the
  argument loads the second word first. *(match)*
- `[vc8]` **Inline accessors called through a cast in each statement are
  not merged**: `y = ((A *)p)->Y(); x = ((A *)p)->X();` reloads the
  member both accessors test; one typed local for both merges the loads.
  *(match)*
- **Tool trap**: `objdump -d -r` on a COFF object garbles functions with
  static locals (`.byte` runs), and a diff count built on it is wrong in
  both directions. Take the function's bytes from its section, fill the
  relocated fields, and disassemble both sides as raw binary.
- `[vc8]` **A port that builds its object through a stand-in is the
  real class's constructor.** `new (this) Base(...)` inside a member of
  a placeholder struct, raw vtable stores, a `Construir` member: write
  `X::X(...) : Base(...)` on the class the RTTI names, with the
  virtuals its vtable overrides declared and the members that have
  constructors or destructors as real members. Several such ports
  matched at the first build. Give the deleting-destructor adjustor
  thunks and any helper class's vtable their symbol lines, or the
  comparison walks into them. *(match)*
- `[vc8]` **The image's unwind map is the list of sub-objects with
  destructors.** Dump it per function (state, parent state, funclet:
  `this + offset`, destructor) and compare the state count with yours
  across all open ports: each difference is a member, base or local
  whose destructor the port lacks or has in excess. One state missing
  in a constructor was a member with an empty out-of-line destructor;
  declaring it renumbered every later state store. *(match)*
- `[vc8]` **Clears between a member's constructor call and the EH state
  store belong to that member's inline constructor.** The state is
  stored when the member is complete: with the clears in the outer
  constructor's body the store comes in front of them. Write `struct M
  { M() : sub(1) { a = 0; ... } }` and leave the outer body with what
  follows the state store. *(match)*
- `[vc8]` **A member with a compiler-generated destructor is destroyed
  in line on the normal path and out of line in the funclet.** The
  funclet names the member's own offset, the normal path the offset of
  the sub-member inside it. A port with the sub-member directly in the
  class has identical code and a different unwind table. *(match)*
- **Check a stack offset against the pending pushes before reading it
  as the EH state slot.** A `mov byte [esp+N],0` in front of a call in
  a loop was `buf[0] = 0` on a local buffer, eight bytes of pushes
  away; the port lacked the statement.
- `[vc8]` **A load that reuses its base's register (`mov ecx,[g]; mov
  ecx,[ecx+4]; push ecx`) is the field read as a one-word record by
  value.** Every pointer spelling (member, local, pointer or reference
  accessor, cast, wrapper, `/O1`..`/Ox`) gives `mov ecx,[g]; mov
  edx,[ecx+4]`. Declare `struct Ref { T *p; }` and read the field as a
  struct copy: a local `Ref t = g->field;` or an accessor `Ref F() {
  return *(Ref *)&field; }`, with `.p` at the use; an accessor that
  fills the record member by member does not do it. The same cures a
  member reloaded through `this` into a fresh register (`mov
  [ecx+8],edx; mov ecx,[ebp+8]; mov [ecx+0xc],eax`). The original's
  members were small handle classes. Reproduce such a puzzle in a
  ten-line scratch file first: forty variants cost one minute there.
  *(match, 6 functions)*
- `[vc8]` **An inline setter taking that record by value sinks the
  store behind the pushes of the next call** and keeps the base's
  register (`mov ecx,[g]; mov ecx,[ecx+N]; push ..; lea edx,[buf];
  push ..; push edx; mov [eax+4],ecx; call`); the plain member store is
  emitted at once and frees `eax`. *(match)*
- `[vc8]` **Small records built inside a constructor call are
  temporaries, not named locals**: `T obj(&Box(0, 0, 0, 0), &Colour(0,
  0xff, 0), 3, 0, 2);`. Signs: their slots overlap other scoped locals,
  their stores come behind the pushes (last argument's first), the
  frame is smaller than with locals. A `short c[3]` or `char n[32]`
  local brings a `/GS` cookie the image lacks: use records and `int
  w[8]`. *(match)*
- `[vc8]` **A pair set from another object has three spellings with
  three register footprints**: a pair returned by value (`P p =
  f->Tam();` filled second member first), a pair constructed in place
  (`*(P *)&dst = P(a, b)`, first member loaded first), and a two-`int`
  inline setter, which emits the first one's instructions but leaves
  the object's register taken, so later loads and a four-word copy
  (`ecx, edx, ecx, edx` against `edx, ecx, edx, eax`) shift. Read the
  shift as "a register is still held", not as a type difference.
  *(match)*
- **A constructor argument stored right behind the vtable store and in
  front of a later member's constructor is a member initialiser**; in
  the body it sinks behind all member constructors. *(match)*
- **When a shared block (a macro, a header) matches in one function,
  apply it to its siblings before reading them**: one function that had
  resisted several rounds matched at the first build. *(match)*
- **Turn a proven idiom into a diff scan.** Once a spelling is known
  (here: the one-word record read, `mov r,[r+N]` in the image where
  ours has `mov r2,[r+N]`), grep every unmatched port's diff for its
  signature and open the hits smallest first: two ports matched at the
  first build. Filter by hand: a vptr read reuses its register in any
  virtual call, and a list walk `p = p->next` is itself. *(match, 2
  functions)*
- `[vc8]` **A pointer chain read twice is spelled per site**: `mov
  eax,g; mov eax,[eax+A]; mov eax,[eax+B]` (every link a record by
  value) at one use and `mov edx,[g]; mov eax,[edx+A]; mov eax,[eax+B]`
  (a plain first link, the last a record) at the next. Using the
  all-record accessor at both sites exchanged two callee-saved
  registers across the whole function; the mixed pair matched.
  *(match)*
- `[vc8]` **A named record plus a temporary in one constructor call
  stores the named one first** (`Box box; ...; T obj(&box, &Colour(255,
  0, 0), ...)`), two temporaries store the last argument's first. A
  named record built by a constructor that takes a pair *by value*
  (`BoxP c(x, y, *(Pair *)&m->w)`) loads both words in front of the
  stores. *(match, 3 functions)*
- `[vc8]` **`mov [esp+N],esp` in front of a call with a record built in
  the argument area** is a by-value parameter whose class declares a
  copy constructor. Declare it for the original compiler only: a
  non-trivial copy changes how other ABIs pass the argument. *(match)*
- **A port's helper with no address of its own, in line in the image,
  may need `__forceinline`**: in a function with an EH frame a plain
  in-class definition stayed a call. *(closer: 62 lines to 12)*
- **Check the virtual slot number of every stand-in class against the
  image when the diff is down to one line**: a `[eax]` against
  `[eax+4]` was a port bug (one virtual too few), not a codegen
  difference. *(match, behaviour fix)*
- **Variant scripts must rebuild after restoring the source**, or the
  next diff reads the last variant's object. *(tool trap)*
- `[vc8]` **A zero register that is also the return value is a result
  variable**: `T *res = 0; if (...) { ...; res = p; } return res;`
  gives `xor ebx,ebx` at the top with every zero of the function read
  from it and `mov eax,ebx` at the failing exit; `return 0` on those
  paths gives literal zeros and `xor eax,eax`. The cousin: a single
  final `return 0;` behind nested `if`s shares one `xor eax,eax`
  epilogue between all failing paths. *(match, 3 functions)*
- `[vc8]` **A plain inline setter flips the first free register**: `p->m
  = v;` took `edx` for `p`, `p->Set(v)` (`void Set(T *s) { m = s; }`)
  took `ecx`, and the rest of the function's registers followed (57
  diff lines to 9). Try the setter when a store's base register is the
  only difference. *(match)*
- `[vc8]` **Floats moved with integer `mov`s are a record copy; its
  spelling places the loads.** Through an inline setter taking a
  reference, with the source spelled as the *member* (`o->SetPos(*(P
  *)m)`): destination base first, offset folded. Same source through a
  pointer local: source word first. Written at the use (`*(P *)&o->x =
  *(P *)r`): an unfolded `add eax,8`. All three loads in front of the
  stores: the record *by value* (an inline `Set(P v) { *this = v; }`,
  or a by-value record parameter where the port had three floats).
  *(match, 4 functions)*
- `[vc8]` **A member tested through an inline accessor puts the compare
  in front of the previous statement's store**: `cmp [this+N],0; mov
  [this+M],eax; jne` is `m = v; if (!Get()) ...` with `T *Get() {
  return p; }`; the plain member test stores first. *(match)*
- **A port's cached pointers and hand-written `p ? p->f : 0` locals are
  often the class's inline accessors called at each use** (`int H() {
  return r ? r->h : 0; }`, `void Pos(int x, int y) { if (r) { r->x = x;
  r->y = y; } }`): repeated tests, a reload for the second store and a
  late callee-saved push in one arm are what the inlines give. Two
  ports matched at the first build. *(match)*
- `[vc8]` **A value read into a local in front of a test** (`int c = m;
  if (p->q) p->q->F(..., c);`) loads it between the test and the jump
  and frees the registers of the calls behind it; as an argument it
  loads behind the jump. *(match)*
- **Leaving a loop with `break` against `return flag;` inside it, an
  index against a walking pointer**: the first merges the exit (a
  zero set in front of the call and returned through a register), the
  second exchanged two callee-saved registers. *(match)*
- **A variant script that `exec`s another variant file must not share
  its option variable**: the inner assignment wins and every variant
  builds the same source. *(tool trap)*
- `[vc8]` **A constructor whose first stores sit in front of the body's
  has members with inline constructors.** VC8 stores the vtable
  pointer, runs the member constructors, then the body; fields
  cleared in the body (or in an initialiser list of plain fields, or
  through an inline helper) are scheduled freely, so the port shows
  a body store in front of the vtable store, or the zeros behind the
  pushes of the base call. `struct Par { int a, b; Par() { b = 0;
  a = 0; } } par;` as a member pins them. Try the layouts on a mock
  class of the same layout in the port's own file before touching
  the header. *(match)*
- `[vc8]` **A pair assigned from a temporary takes one register per
  word** (`xor edx,edx ... mov [a],ecx; mov [b],edx`, `mov
  ecx,K; mov edx,ecx`): `*(Par *)p = Par(K, K);`; the order of the
  constructor's two stores decides which word gets the register
  that is already live. *(match)*
- `[vc8]` **The arm's first instruction in front of the conditional
  jump (`test eax,eax; mov ecx,esi; jne`) is the else call written
  twice**: `if (n) { if (!f()) T(); else N(); } else N();`. The `&&`
  form, early returns, a flag, goto and the ternary keep the jump
  first. *(match)*
- `[vc8]` **A run of equal stores with a load or constant in the
  middle is a counted loop** the compiler unrolled (`for (i = 0; i <
  32; i++) g[1 + i] = 0;`); the same stores written by hand hoist
  the load and keep one zero register across earlier calls.
  *(match)*
- `[vc8]` **An inline helper is a boundary for hoisted argument
  loads**: blocks of stores done through a small inline keep a
  later stack-argument load behind the first block. *(match)*
- `[vc8]` **A second constant register loaded next to the first
  (`xor eax,eax; or ecx,-1`) with its stores behind the first's**:
  the second constant's statements are written BETWEEN the first's
  (the first is still live; the scheduler sinks the stores). Written
  after them the register is reused. *(match)*
- **Compare positionally when a function repeats a block** (two
  inlined `memcmp`s): a text diff misaligns and reports the whole
  function. *(tool trap)*
- `[vc8]` **A statement behind an `if`/`else` that the port pulls
  into both arms in front of their stores stays last when the
  `if`/`else` is an inline function** (a list append as an inline
  helper, then `count++`). *(match)*
- `[vc8]` **Word copies whose loads run in address order with the
  stores interleaved are one record copy among plain copies**
  (`*(Tres *)&a = *(Tres *)&pa;`); plain assignments load them in
  another order whatever the statement order. *(match)*
- `[method]` **Classify every unmatched port by its FIRST differing
  instruction pair** (image shape against ours, registers, memory and
  constants normalised) and rank the pair types; cache all diffs once
  so further scans cost a second. Cross the list with the notes: the
  ports mentioned at most twice are the unopened ones. *(scan)*
- `[vc8]` **A call that lacks `mov ecx,this` in ours is a callee that
  is a member**: a member that never uses `this` matches as a free
  function, and only its callers show it. *(match)*
- `[method]` **Arguments that "stay on the stack as the rest of the
  format" are a getter without arguments**: `sprintf(buf, fmt, Dir(),
  a, b)` pushes `b`, `a`, calls `Dir`, pushes its result; do not port
  it as `Dir(a, b)` with a variadic trick. Read the callee: `mov
  eax,K; ret`. *(match)*
- `[vc8]` **`add eax,N; lea edx,[eax+M]` is the parameter advanced in
  place** (`e += 6; f(e, &e[4]);`); `&e[6]`, `&e[10]` both compute
  from the old value and a new local exchanges the registers.
  *(match)*
- `[vc8]` **`xor r,r; cmp; jae; load` is `p = 0; if (c) p = x;`**; the
  ternary puts the zero into the else arm behind a jump. *(match)*
- `[vc8]` **A pair built from two reads where the SECOND is read into
  a temporary first** is an inline constructor `P(float a, float b)`
  (arguments right to left); member stores read in statement order.
  *(match)*
- `[method]` **A diff tool that reads as many image bytes as OUR
  function has hides the ports that call what the image inlines**:
  compare each port's real length in the image (distance to the next
  start, padding stripped) with ours and open the ones the image has
  longer; write the port's helper in line (or as a forced inline) and
  diff against the full image listing. *(scan)*
- `[vc8]` **The first argument read in front of a record's stores is a
  record TEMPORARY in the argument list**: `f(t->frame, &Box(x, y,
  t->size), ...)` with a constructor; a named local stores everything
  first. *(match)*
- `[vc8]` **`mov esi,eax` behind a call returning an object and `mov
  ecx,esi` for the next call are chained calls on temporaries in the
  argument list** (`F((int)b.Token(0).Num(), (int)b.Token(1).Num())`):
  both temporaries are built first; named locals take `lea ecx,[slot]`
  and exchange the slots. *(match)*
- `[vc8]` **`push eax; lea eax,[m]; push eax` (the address through the
  register just pushed, every later scratch register one off) is a call
  through an inline wrapper** (`inline int Peek(MSG *m, HWND h) { return
  PeekMessageA(m, h, 0, 0, 1); }`). *(match)*
- `[vc8]` **x87 comparisons: `st(0)` holds the RIGHT operand of the
  source** (`d >= q.len` loads `d` first; `q.len <= d` gives the swapped
  test), and **a float literal compares as `fld DWORD K; fcom x` only
  when written on the right** (`p >= 180.0f`; `180.0f <= p` gives `fcom
  QWORD K`); an `extern float` global loads both operands. A transform
  that flips every literal-left comparison brought 19 files closer; the
  regex must not take a literal behind an arithmetic operator (`a + 1.0
  < t`) for the left operand. *(match, tool trap)*
- `[vc8]` **A wrapped value stored on BOTH paths (`jne` over the `fsub`,
  one `fstp` at the join) is a ternary assigned** (`p = p >= 180.0f ? p
  - 360.0 : p;`); `if (c) p -= 360.0f;` stores inside the arm only.
  *(match)*
- `[vc8]` **`push 2; jmp; push 3` in front of ONE call is the whole call
  written in both arms** (`if (c) A()->P(2); else A()->P(3);`); the
  ternary argument gives `mov eax,K; push eax`. *(match)*
- `[vc8]` **Word-by-word copies into stack temporaries followed by
  dword + word stores are small records passed BY VALUE (a member-wise
  copy constructor) to an inline setter that assigns them.** *(match)*
- `[vc8]` **An inline helper whose image reads its arguments AFTER its
  own first statements has reference parameters** (`void Push(const
  float &nx, const float &ny)`). *(match)*
- `[vc8]` **`fld [p+8]; add eax,8; fld [eax+4]` is a pointer to the
  member** (`float *o = &a->x; *o, o[1]`). *(match)*
- `[vc8]` **A port belongs in its callee's FILE when the image inlines
  the callee.** Without `/GL`, VC8 only inlines what is defined earlier
  in the same translation unit, so a port in a file of its own can only
  CALL a matched callee the image has in line. Scan: for every unmatched
  port, the game callees OUR object calls (relocations) that the image
  function never calls (`E8` targets, thunks followed), with the file
  that defines each. Moving the port behind the callee took three ports
  from 60-130 diff lines to 0-30 at once, and regrouping one library
  (a socket layer split over three files) matched eight. The reverse
  holds too: a helper the image CALLS must not be visible, or it gets
  inlined and a matched caller drops. *(match, scan)*
- `[vc8]` **A temporary with a destructor that takes NO unwind state
  (the image never bumps the state byte around it) sits across a callee
  declared `throw()`**: `a.Assign(&Queue(-1))` with `Queue
  &Assign(const Queue *) throw();`. The specification does not change
  the mangled name. *(match)*
- `[vc8]` **Two constructors can be told apart by the store order
  alone** (the member default-constructed with `sock = -1` first, the
  temporaries with the zeros first and the socket last); and **zero
  stores in FRONT of a call whose result lands in the same object are a
  constructor that makes the call in its body**, not a constructor
  argument (argument: call first) and not an assignment behind a default
  constructor (keeps the dead store). *(match)*
- `[vc8]` **A scratch register one off for the rest of a function, or a
  pointer kept in `eax` while its target goes to `ecx`, is an inline
  accessor on the object** (`(*obj)->Radius()`, `q.Socket()` both as an
  argument and in the `== -1` test). Return `const float &` from a float
  accessor: a `float` return stores and reloads. *(match)*
- `[vc8]` **A rotated `while (c) { }` has an UNPADDED loop top; `if (c)
  do { } while (c);` gets the alignment padding** (`lea esp,[esp+0]`).
  A diff that is one padding instruction is the loop's spelling.
  *(match)*
- `[vc8]` **`add ecx,eax` right behind `mov eax,1` is `r = 1; n += r;`**
  (`n++; r = 1;` keeps the count in `eax`). *(closer)*
- `[vc8]` **An inline `operator=` taking the temporary by reference uses
  the constructor's return value** (`mov edi,eax; push edi`): `r =
  Pair(0);`. *(match)*
- **Build trap:** when every object depends on every header, `make
  match` after a header edit rebuilds the tree serially; run `make -jN`
  first. *(tool trap)*
- `[vc8]` **Stores that sit between the derived vptr store and the first
  member-constructor call are member initialisers: those fields belong
  to the DERIVED class.** Body statements can never come in front of a
  member constructor. When a port's shared header puts the fields in
  the base and the base constructor out of line, give the constructor a
  file of its own with stand-in classes of the same names: a small base
  with its constructor inside the class (the image has it in line),
  the fields as members with default constructors and initialisers.
  *(3 matches)*
- **Stand-in classes for a constructor are safe with single inheritance
  only.** A non-MSVC native build takes the vptr's address point from
  the class as declared; with several bases the secondary address
  points depend on the number of virtuals in front, so a shortened
  stand-in stores wrong vptrs. Use the real header there. *(hazard)*
- `[vc8]` **A float "global" in `.rdata` is a literal.** Read its value
  from the image before declaring it `extern`: the same `fld` is
  scheduled differently for a variable (two bytes in one constructor).
  *(match)*
- `[vc8]` **A small record at an odd slot (`[esp+0x1a]` for six bytes)
  is a temporary in the argument list**; named locals sit at 4-byte
  boundaries, and local arrays take a security cookie. Give the record
  a constructor and a conversion operator for the pointer parameter.
  *(2 matches)*
- `[vc8]` **A virtual called on a local of known type is inlined**: the
  image has the callee's test and call in place of the call. *(closer)*
- `[vc8]` **`(float)(c - K) / 80.0` repeated ends in `fidivr`**; `(c -
  K) / 80.0` gives `fild; fdivrp` for the last one. *(match)*
- **Variant-tester trap:** a tester that restores the source with its
  old timestamp must delete the object before BOTH builds, or a variant
  written in the same second as the last build shows the previous
  result; and filter compiler output on `error`, not ` error `. *(tool
  trap)*
- **The reverse inline scan:** list callees the image CALLS that ours
  does not, defined in our same object: the helper is visible where it
  must not be. *(scan)*
- `[vc8]` **A tested member is spelled out at every use when the image
  copies the tested register per case** (`mov ecx,esi`, `mov edx,esi;
  pop esi`) and reloads it in front of stores: `if (!s->vector)
  return;` then `s->vector->x`; a function-level cached pointer keeps
  one register everywhere. *(match, three functions)*
- `[vc8]` **A float local lives on the x87 stack**: `fld K; fcom
  st(1); fnstsw; fstp st(1)` then `fcomp [m]` against the same register
  is `float eps = K;` used twice; two literals load twice. *(match)*
- `[vc8]` **The operand order of a product follows the numbering of
  its leaves, and named temporaries in front shift it**: when `fld
  st(i); fmul [m]` against `fld [m]; fmul st(i)` survives every
  spelling of the product, fold the named float temporaries of the
  statement in front into its expression (or split them out). *(match)*
- `[vc8]` **A member defined behind its caller in the same file is
  inlined too**: what the image calls must live in another file; what
  it inlines must live in this one. *(closer)*
- `[vc8]` **Scratch registers one off through a whole function** can
  be one named local for the last word of a copied record: copy the
  record in one assignment. *(match)*
- `[vc8]` **An array of records indexed per use** (`T[i].a`, `T[i].b`)
  gives the unbiased base `this + i * size` in one register with full
  member offsets; a named element pointer gives the biased one.
  *(closer)*
- `[vc8]` **`fild [k]; mov [k],eax` is one int reused** (`k = i % n;
  ... k = i / n + 1;`). *(match)*
- `[vc8]` **Two conditional temporaries with destructors (a flag
  register with bits 1 and 2) are one `&&` expression**; `return ok !=
  0` on a `bool` gives `xor eax,eax; cmp [ok],al; setne al`; an inline
  accessor on a returned temporary gives `mov eax,[eax+off]; cmp
  eax,-1`. *(match)*
- `[vc8]` **A performance counter read as one `long long`** has no
  `xor; or` pairs; `(float)(unsigned)x` adds a float 2^32, `(double)`
  a double one. *(closer)*
- **The callee-set scan:** per unmatched function, the image's call
  targets (through thunks) against the port's call relocations, both
  ways. It finds forwarders the port skipped (the port called the
  leaf), callees the port has in line from any file, and wrong
  callees; a tail call (`jmp`) is a call. *(scan, three matches)*
- `[vc8]` **A helper called WITHOUT the jump-thunk table is a static
  of the caller's original file** (incremental-link images: every
  external call goes through a thunk). Scan every unmatched function
  for direct call targets; put the helper and ALL its callers in one
  file, the helper `static`. Two `/GL`-parked helpers matched this way
  once their callers shared the file, and five callers with them; the
  naked register wrappers disappear. With only some callers in the file
  the convention differs (a parameter in `ebx` instead of `esi`) and
  the callers get worse: all or nothing. *(match, seven functions)*
- `[vc8]` **The register parameter of a static is found by permuting
  its parameter list against one caller**: six orders of three
  parameters, one loop of the variant tester; here the LAST pointer
  took the register. *(match)*
- `[vc8]` **`sub esp,N; mov ecx,esp; push src; call copy-ctor` in
  front of a call is a class passed BY VALUE**; with the result's
  address in `esi` the callee is a static. Changing `const T &` to
  `T` matched the helper and three callers. *(match)*
- `[vc8]` **A functional cast around a call that returns a class
  builds the temporary and copies it** (`return T(p->Get(i));` gives
  `call Get; push eax; call copy-ctor`): a named local passes `lea`,
  a reference or a cast to `const &` is elided. The copy constructor
  must be DECLARED (an implicit one is copied in line). *(match, two)*
- `[vc8]` **A destructor's body port is the real destructor**: unwind
  states N..1 at the top are the members' and the base's destructors;
  write `T::~T()` with only the statements. An inline member destructor
  whose stand-in class name differs from the RTTI name stores the vptr
  explicitly, or the object carries a differing vtable. *(match)*
- `[vc8]` **A fall-through case laid out BEHIND its target (`cmp; je
  exit; jmp body`) is the body written twice** and tail-merged; the
  fall-through spelling puts the test in front, and a `goto` into the
  other case folds back to that. *(match)*
- `[vc8]` **`xor r,r` at the top, `mov eax,r` at one epilogue and
  `mov eax,1` at another is one `return result;` behind a label**
  with the other value assigned on its path; two `return` statements
  fold the constant. *(match)*
- `[vc8]` **A callee whose result the image reads through `eax` after
  a tail call inside it returns that value**: a `void` port that
  repeats the callee's body hides it; the callee-set scan finds it.
  *(match)*
- `[vc8]` **`/GS` sorts `char` arrays above other locals**: a find
  buffer below the path arrays is a struct, not `char[0x140]`; and a
  frame 0xb0 short of ours is a path array of 0x50, not 0x100. *(match)*
- `[vc8]` **A scalar's dead slot is reused, a record's is not**: a
  frame four bytes larger with the scratch in its own slot is a record
  for the three differences. *(match)*
- `[vc8]` **`js`/`jns` after `test r,r` is `h & 0x80000000`**, `h <
  0` gives `jl`/`jge`. *(match, two)*
- `[tool]` **`__forceinline` on an out-of-class definition leaves no
  symbol under clang**, and a native build that resolves missing names
  to the original code hides it: count those fallbacks before and
  after. Use a `static __forceinline` helper that the real function
  forwards to; that also keeps a caller that CALLS it matched.
- `[tool]` **A disassembly diff that masks addresses does not compare
  constants** (`0.1` against the double of `0.1f` showed no
  difference): confirm with the byte comparison. *(near miss)*
- `[tool]` **List the direct referrers of an address** (`E8`/`E9`
  rel32 hits plus absolute dwords, with the containing function): a
  function with direct callers only is a static, and its callers name
  the functions of the original file. Rebuild that file whole, in
  address order; several ports match by the move alone. *(match, four)*
- `[vc8]` **`mov eax,f; and al,2; neg al; sbb eax,eax; neg eax; add
  eax,1` is `(f & 2) ? 2 : 1`**; `((f & 2) != 0) + 1` gives `shr; and;
  add`. `sbb; and eax,K; add eax,L` is the same `?:` with two
  constants. *(match, two)*
- `[vc8]` **A dead `jmp top` plus padding right behind an early `ret`
  is `do { r = f(); if (r == LOST) g(); } while (r == LOST);`**: the
  recovery inside the body, the test written twice. `for (;;)` with a
  `break` and `while ((r = f()) == LOST) g();` both duplicate the
  call. *(match)*
- `[vc8]` **Locals set to the same constant are permuted as a set**:
  the first declared gets the stack slot, the last the register the
  early compare borrows; declare them behind the early return. Six
  orders, one loop. *(match)*
- `[vc8]` **Zero stores in a scrambled order (0, 3, 1, 2, tail) are an
  inline `memset` of a record**, followed by member stores in the
  record's own order; the `/GS` cookie of such a function may come
  from a `char[8]` inside the record (a GUID). *(match)*
- `[vc8]` **A running value kept in a stack slot across calls is a
  small class with inline members** (a checksum: constructor, `Add`,
  `Value`); a table word named in a temporary inside the member gives
  `mov; shr; xor reg,reg`, the single expression folds the table into
  the `xor`. *(match)*
- `[vc8]` **The register parameter of a static need not be the first
  in the list**: when the stack parameter is loaded in front of the
  register copy, move the register parameter behind it. *(match)*
- `[vc8]` **`test al,al; je F; mov al,1; jmp end` at several exits
  with one shared `F: xor al,al` is `if (f()) return true;` in every
  arm of an `if`/`else if` chain with ONE `return false;` behind the
  chain**; `test al,al; setne al` is `if (f()) return true; return
  false;` together. `return f() != 0;` on a `char` callee gives `neg
  al; sbb; neg`, on a `bool` callee nothing. *(match)*
- `[vc8]` **`mov r,[z+tab]; shl i,5; add i,r` is the record array
  indexed at every use** (`z->tab[i].p[0] + z->tab[i].q[0]`); a named
  element pointer folds the table load into the `add`. *(match)*
- `[vc8]` **Original translation units as address intervals**: with
  incremental linking every cross-file call goes through the jump
  table, so a direct `call` joins two functions of ONE original file.
  Union the direct calls, turn the components into address intervals
  and list, per interval, which of your files hold its functions. Read
  each split before merging: copies of a small static that the callers
  CALL are tidiness only; a merge moves a diff where the image inlines
  the neighbour or the neighbour has a register convention. *(scan)*
- `[vc8]` **Unreferenced out-of-line helpers right in front of a big
  function are the inlines it was written with**: rewrite the port
  with them as `static inline` helpers instead of hand-expanded
  statements (a best-first search went from 270 differing lines to
  46). Whether the caller reads a value itself or takes the helper's
  result decides the registers: try both. *(closer)*
- `[vc8]` **`mov eax,1; mov edx,size; mul edx; seto cl; neg ecx; or
  ecx,eax` in front of `new[]` with a literal count** is `new T[k]`
  inside an inline whose `k` is a parameter; the literal at the use
  folds to `push size`. *(closer)*
- `[vc8]` **The custom convention of a static follows its body and its
  callee, not the parameter order, once the body differs from the
  image**: a parameter the image passes through untouched sits in the
  register the callee takes it in. Settle such a family bottom-up; a
  caller cannot match before its callee. Retire the naked wrappers
  first (statics called direct) so the diffs show only what is left.
  *(closer)*
- `[vc8]` **Instruction distance as a scan.** Disassemble every port and
  its image function at the image's full length and class them: first
  differing instruction late (60% or more), differences only in the tail
  or the head, same mnemonics with other numbers, same mnemonics with
  other registers, only jump targets, the SAME instructions in another
  order. Each class has its own lever (the last one: scheduling, see the
  sub-object inline below); the classes are short lists, read them all.
- `[vc8]` **Count the `fstp [t]; fld [t]` round trips per function, image
  against yours, and read what stands in front of each.** Behind a
  `call` returning float: the call sits in an INLINE ACCESSOR (`static
  inline float Ang(B *b) { return b->orient.Angulo(); }`; written in the
  argument list it also gives the image's evaluation order). Behind an
  `fadd`/`fild` in front of a float store: the expression is CAST
  (`p[2] = (float)(x + q[2]);`), and the pointers are read again for the
  next statement. Behind a call returning `double`, with the
  destination's base loaded AFTER the `fld`: a `double` local stored
  through a cast (`double f = v.Numero(); a[i].vel = (float)f;`); a
  float local loads the base between `fstp` and `fld`.
- `[vc8]` **Stores kept in source order in front of a later load of the
  same object (yours hoists the load above them): the load is an inline
  member of a SUB-OBJECT.** `((Ref *)(p + 0x128))->Listo();` with `void
  Listo() { if (h) h->listo = 1; }` keeps the three stores in front of
  `mov eax,[esi+0x128]`; a local, a typed view of `this` and the test
  written out all hoist it. For reads: a pointer local to the sub-object
  with inline getters (`Tam *t = (Tam *)(p + 0xdc); t->Ancho() / 2`)
  keeps each read behind the store in front of it.
- `[vc8]` **`xor reg,reg` in front of the first call, `mov eax,reg` at
  the early exits, `lea r,[reg+1]` for a 1**: a result variable zeroed at
  the top (`int r = 0; ... if (a && b) r = 1; return r;`). `return 0;` at
  each exit gives `xor eax,eax` there.
- `[vc8]` **A float stored with `fstp [esp+0xc]` in front of `mov
  eax,esp` and three float copies** is a by-value position followed by a
  separate float argument, not a four-float record (whose constructor
  stores through `eax`). Declare the callee under a second name with the
  split signature and give the symbol table both names.
- `[vc8]` **Range tests keep the tested value first** (`a > m - 5.0 && a
  < m + 5.0`: the value is loaded right behind its round trip and stays
  on the x87 stack); the bound written first loads it last. The operand
  order of every x87 compare is the image's `fld` order.
- `[vc8]` **One float variable stepped in place** (one slot for the value
  read, stepped and compared at the end): `if (obj < v) { if (v > 1) v -=
  a; else v -= b; if (v < obj) v = obj; } else {...}`; a second variable
  or the step chosen by `?:` first changes the whole shape.
- `[vc8]` **Two arms with the same tail that the image does not merge:**
  one arm ends in its own `return f();`, the other in `break`.
- `[vc8]` **One result variable, every arm ending in `break`** (state-step
  functions: a `switch` on the phase whose arms return 0 or the base
  class's step). Signs: a callee-saved register zeroed right behind the
  load of the switch value, `mov eax,reg` at the exits, and the base's
  step followed by its OWN copy of the epilogue in each arm that ends in
  it. Source: `int r = 0; switch (s) { case A: ...; r = Base::Step();
  break; ... default: r = Base::Step(); break; } return r;`. `return
  Base::Step();` in each arm merges the tails behind a `jmp`; `return 0;`
  gives `xor eax,eax` at each exit. Scan for it: more `ret`s or more `mov
  eax,<callee-saved>` in the image than in ours.
- `[vc8]` **An arm that ends in the default's code is `goto label;` into
  `default:`** (`je <default code>` from the middle of an arm, or from an
  inner `switch`). Writing the call again puts a second copy in line.
- `[vc8]` **The jump table's size names the source's case labels.** An
  entry that points at the return is `case N: break;`; an entry that
  points at the default's code is `case N: goto label;` (`case N:
  default:` together is dropped and the table turns into a `sub`/`je`
  chain). For a `sub`/`je` chain the source order of the cases does not
  change the bytes.
- `[vc8]` **`fld d; fcomp [k]; test ah,0x41; jp far`** is the positive
  test with the near body inside (`if (f(x) <= k) { near } far`), the
  value not named. `if (!(d <= k)) goto` and `if (d > k)` both load the
  constant first.
- `[vc8]` **A record argument built straight into the argument area**
  (`sub esp,N; mov eax,esp; fstp [eax]; ...`) is a temporary of a record
  with a DECLARED copy constructor passed by value (`f(Pt(x, y, z), ...)`);
  without the declaration the temporary is built in the frame and copied
  with integer moves. Declare it for the original compiler only; it is
  never called and the mangled name is the same.
- `[vc8]` **Both pointers loaded in front of the pushes of a member call**
  (`mov eax,[a]; mov ecx,[b]; add eax,8; push eax; ...; add ecx,8`) is an
  inline taking both as parameters; `this` chains and first arguments read
  in front of float pushes are inline getters in the argument list.
- `[vc8]` **/GS copies every pointer parameter into the frame**, so a
  parameter copied in the prologue is a pointer whatever the port's type
  says; the dead copies' slots are reused by float temporaries.
- `[vc8]` **Conversions written inside a loop are hoisted, but in
  statement order:** `fild; fstp t; fld t` for the second table entry
  BEHIND the first element's store means `(float)X[k] + d[2 * j]` was
  written in the loop body, not two float locals in front of it.
- `[vc8]` **A second absolute value with one round trip less** than the
  first is written inside the comparison (`fabs(y1 - y2) + ax < k`) while
  the first is a named float local.
- **Tool trap: a diff that masks jump targets hides a `case` wired to the
  wrong block.** After a control-flow rewrite that does not reach a full
  match, compare the raw bytes outside relocations or read the targets.
- `[vc8]` **The result variable is everywhere, not only in state steps:**
  window-message handlers (`int r = 0; switch (m) { ...; goto resto; ...
  default: resto: r = Base::Msg(...); break; } return r;`), searches
  (`T *r = 0; ... if (ok) r = a; return r;` gives the zeroed callee-saved
  register, the null test against it and `mov eax,reg`), nested `if`s
  with the base call in the last `else`. A `1` parked in a callee-saved
  register in front of a call is `r = 1;` written before the call.
- `[vc8]` **A float root compared value-first against a DWORD constant,
  with a round trip on each side of the call,** is the root written
  inside the condition through an inline (`if (Raiz(dx * dx + dy * dy) >
  17.0f && ...)`); a named float local loads the constant first, and
  `k < d` compares against a QWORD constant.
- `[vc8]` **`shr eax,16; cmp eax,5` behind a 16-bit compare of the low
  word** needs the high word cast (`(unsigned short)(c >> 16) == 5`);
  without it the shift folds into `and eax,0xffff0000; cmp eax,0x50000`.
- **Read a field's width from the compares before tuning anything
  else:** one key typed `unsigned short` where the image compares a
  dword kept three functions of a file apart; retyping it matched one at
  once.
- `[vc8]` **A nullable sub-object read twice:** pointer local + ternary
  for the first and the owner's inline getter for the second gives
  `mov eax,ecx; test eax,eax` (a copy); two getters re-read the member;
  two ternaries on the local merge into one test.
- `[vc8]` **A member tested in memory and loaded again for the call**
  (`cmp [b+off],0; je; mov ecx,[b+off]`) is the member written twice
  through a typed local of the owner; with a call among the arguments
  the receiver is cached behind the test, which is an inline getter
  behind the member test (`if (b->m) b->M()->f(..., g())`).
- `[vc8]` **A local with a destructor that only the unwind funclet
  calls:** the body's inlined destructor folded because the local's
  pointer field is known null; keep the destructor, the EH frame needs
  it.
- `[vc8]` **A counted loop your build unrolls (four copies of the body)
  and the original does not:** VC8 unrolls `for (i = 0; i < n; i++)` when
  the bound is a local or a parameter. Written bound-first
  (`for (i = 0; n > i; i++)`) or with the bound read from the member in
  the condition (`i < t->n`, no local copy in front) it stays one body
  with the guard (`test n,n; jbe`). Scan for ports whose instruction
  count is a quarter above the original's: they are these loops.
- `[vc8]` **A missing EH state restore (`mov [state],-1`) behind a
  function static's initialiser means a callee is too visible:** the
  compiler drops the store when nothing after it can throw, and it knows
  that of a callee defined in the same file. The original calling it
  through the incremental-link thunk says it lived in another file: move
  the callee out.
- `[vc8]` **`jmp; xor eax,eax` between a builder's call and the store of
  the pointer** is `p ? p->Init(args) : 0` with the builder declared as
  returning the object (a constructor in disguise); `mul; seto; neg; or`
  in front of `operator new[]` is `new T[n]` of a record type,
  `new char[n * K]` has no overflow check.
- `[vc8]` **Declare the result variable in front of a function static**
  (`int r = 0; static int id = Find(...);`): the zero register is set
  ahead of the guard test and serves the state store.
- `[vc8]` **Read the frame size before the body:** a buffer the original
  neither clears nor sizes shows as a larger `sub esp` and missing
  stores; keep the zero fill under the portable compiler only.
- `[vc8]` **A clamp inside the cast** (`(uint8_t)(v > 0 ? v >> 2 : 0)`)
  gives `xor reg,reg` / `sar` and one byte store; the cast inside the
  ternary gives a `movzx`.
- `[vc8]` **A callee that ignores `this` is still called with `ecx`
  loaded:** declare it as a member under a second name for the same
  address rather than as a free function.
- `[vc8]` **A commutative operand order that no spelling gives follows
  the occurrences of the operands in the function, so move it from
  outside the expression:** name one operand in a local in front
  (`int w = d->w; int n = d->h * w;` loads `h` first where
  `d->h * d->w` loads `w` first in either written order), name the
  operands of neighbouring sums pairwise (`float y4 = p[4]; float x3 =
  p[3];` used in the first sums only), or drop one occurrence by using
  the copy the original keeps in a slot at ONE other use. It works for
  integer `lea [a+b]`, `imul` and x87 sums and products alike. Search
  per operand (named or not) and per use (the name or the expression);
  a background grid of a few hundred builds is cheap.
- `[vc8]` **Scan for ports whose difference is only an order:** same
  instruction count and the same multiset of tokens as the original.
  Operand-order rows go to the lever above; an integer move scheduled
  between `fld` and `fstp`, or a store placed behind a call's pushes,
  did not yield to any source form.
- `[vc8]` **Two vector additions where one exchanges the middle
  component's operands and the next does not** are both the loop
  `operator+=` (`for (i < 3) a[i] += b[i]`); `fadd QWORD [0.0]` on the
  third component is the addend's constant zero folded, not a global.
- `[vc8]` **A re-test of an inlined callee's result that should have
  folded is the callee's found flag** (`if (hit) { r = e; ok = 1; } if
  (ok) break;`): the callee alone compiles the same with `break`, only
  its caller shows which form the source had. Vary the callee.
- `[tool]` **A diff that masks jump targets can show zero lines for a
  function that still differs:** a loop guard jumping to the other
  `return` is one byte. Read the guard's target first when a function
  has two returns (here the original answered 0 for an empty count).
- `[vc8]` **One bound compared from memory, the other loaded:** the
  loaded one is a `const float` object, the other a literal.
- `[vc8]` **An integer `add reg,[m]` scheduled behind the SECOND
  float-to-int conversion is a two-int record filled first and adjusted
  afterwards** (`P r; r.x = (int)a; r.y = (int)b; r.x += o[1]; r.y +=
  o[2];`). Scalars add in front of the second conversion in any order.
- `[method]` **Where the original is far longer than the port, port what
  was left out even when it is unreachable** (a flag test that can never
  hold): the dead branch's locals fix the frame, and the rest can then
  be read off the frame offsets: member default constructors at the top
  (store order names each), a temporary built by a constructor and
  assigned with integer moves, an inline accessor returning a record by
  value (`return p ? *p : V(0, 0, 0);` with a declared copy constructor),
  an iterator whose constructor leaves fields unset for a null list
  (slots read that nobody wrote), a member re-read behind a call that
  got the record's address (the source compared the member, not a
  local).
- `[vc8]` **A record's size decides which shared slot zone it takes:**
  with every instruction right and one local in the wrong shared slot,
  try the record one field longer (a three-float point with two fields
  set went to the zone the two-float one did not). Locals that share
  slots with earlier statements' temporaries are declared in the inner
  block.
- `[vc8]` **A constant-initialised local is declared where it gives the
  registers**: `int r = 1;` moved behind an inlined member call swapped
  the two callee-saved registers back, though the store is scheduled
  earlier.
- `[vc8]` **A member reloaded for the return right behind stores that
  made it known is the function's common `return m;` copied into the
  arm**; a `return m;` written in the arm folds to the constant.
- `[vc8]` **A port that calls its own `static` helper two or three times
  where the original is several times longer has the helper's body in
  line: write it as the matched sisters' macro.** The instruction-count
  ratio is the tell (111 against 25). Per block, grid the few spellings
  the sisters show: the plain member store against the inline setter
  that takes a one-word record by value; the global read in front of
  the member's re-read (`{ T *t = g->tabla; m->F(t->Datos(n)); }`)
  against the chained call; a parameter read in front of the `new`
  (`const char *nm = nombre;` first). Two blocks of one function may
  use different spellings. *(4 matches)*
- `[vc8]` **A constructor whose zero sits in a callee-saved register
  through the body, with the EH state store scheduled late, has real
  members with default constructors**, not stores through
  `(char *)this + off`. Zeros then ones stored in the order y, x, w, h
  is a box of two base records (`Pos() { y = 0; x = 0; }`, `Tam() { w =
  1; h = 1; }`); one four-store constructor schedules the ones first.
  A record returned by value and stored field by field through `eax`
  is the assignment straight from the call, not a named local.
- `[vc8]` **A switch head laid out twice with shared case bodies is an
  inline written with every case complete (`case 0: if (!c()) act();
  break; case 1: act(); break;`) and inlined twice, the caller's zero
  case last** (`if (v != 0) { ...; step(); return; } act();`). A
  fall-through switch merges the two heads in every caller form.
- `[vc8]` **A `/GS` cookie ours has and the original lacks**: a record
  holding `char c[32]` (use `int w[8]` with a `char *c()` accessor) or
  a `char[4]` initialiser list (assign a `char[3]` element by element).
- `[vc8]` **A deleting destructor that inlines the destructor WITH an
  EH frame while the standalone destructor has none** was reproduced
  only one way at a time (`__forceinline` without `throw()` plus a
  `throw()` `operator delete` for the first, `throw()` for the second):
  check both symbols before applying either. **open**
- `[vc8]` **Scan for the `/GS` cookie in one build and not the other**
  (references to `___security_cookie`): it names the type of ONE local.
  Ours has it, the original not: a `short[3]`/`unsigned short[3]` colour
  is a three-short record with a constructor (often a temporary in the
  call), `char pad[N]` fillers of an API descriptor are `unsigned`
  words (check the real size against `sub esp`), `unsigned char
  desc[0x6c]` is a record of words. The original has it over an EMPTY
  frame (`push ecx`, only the cookie stored): a dead array with its
  address in a dead pointer (`char dead[16]; char *q = dead;`); the
  array alone or a store into it does not give it. A `LARGE_INTEGER`
  union gives the cookie, a two-field struct does not.
- `[vc8]` **A local kept in a PARAMETER's slot** (`lea reg,[esp+N]`
  above the return address, no frame slot): written through the
  parameter's address (`int *dc = (int *)&surface;`, or a placement
  `new (&p) Color(...)` over a by-value record once its fields are
  read). Read the fields into locals FIRST, construct second, fill the
  other record third: that order schedules the stores behind the
  pushes as the original has them.
- `[vc8]` **`cmp eax,<zero register>` after each of several HRESULT
  calls is one result variable tested before each call**: `r = a();
  if (r >= 0) r = b(); if (r >= 0) r = c(); if (r >= 0) return;`. The
  `&&` chain and nested `if`s give `test eax,eax` and other registers.
- `[vc8]` **Zero differing bytes but the matcher reports a difference:
  a relocation names another function.** List the relocations against
  the original's targets; here the port called the wrong sibling (a
  full shutdown where the original only releases two objects).
- `[vc8]` **A search with unsigned jumps, the value compared first and
  no `i == n` re-test** is an inline: `unsigned n = s->n; for (unsigned
  i = 0; i < n; i++) if (a == t[i].key) return (int)i; return -1;`. A
  dead `lea eax,[element]` at the end is the `this` of the element's
  inline setter; `t[j].flags |= 3` leaves a dead `lea` of the field.
- `[vc8]` **A virtual call with no `mov ecx,this` right behind a call
  of a small leaf** is whole-program (`/GL`) knowledge that the leaf
  preserves `ecx`. Only a `/GL` unit holding both reproduces it (take
  the leaf's address so it keeps its convention); no pointer cast in a
  plain build does. Check what else `/GL` moves before relying on it.
- `[vc8]` **The xored cookie pushed in the prologue with NO check call
  at the end is an EH frame, not a buffer** (every EH prologue of a
  `/GS` build has `mov eax,cookie; xor eax,esp; push eax`). A port
  without it wrote a `new T(args)` as allocation plus an init call, a
  constructor as a member function, or destroys a local by hand. Scan
  for it: the original starts with `6a ff 68`, the port does not.
- `[vc8]` **A local object with its destructor**: declare the real
  class (the RTTI name, so the vtable symbol is the original's) with
  its constructor, and either an INLINE destructor whose body is what
  the function's tail does (the vtable store and the last EH state are
  its) or none at all (the normal path then calls the base's
  destructor directly while the unwind funclet uses the class's own
  implicit `??1`, a real one-jump function: give it its address, or the
  matcher reports zero differing bytes).
- `[vc8]` **Everything a constructor stores before its first
  out-of-line member-constructor call is the initialiser list**; the
  rest is the body. A matched sister class's source text gives the
  member records (default constructors for the zero and one stores).
- `[vc8]` **A static helper with a buffer, called from every arm**, is
  in line in the original and the buffer (and its cookie) is the
  caller's; with it goes one result variable and a single return.
- `[vc8]` **`and esp,-8` with a frame pointer in the port only**: a
  `char[8]` local (the compiler aligns it). The original's 8 bytes are
  a smaller array rounded up (`char[5]`).
- `[vc8]` **A zero the original does not thread past the test behind
  an inlined call (`jmp; xor eax,eax; L: cmp eax,r`)** is a result
  variable INSIDE the inline (`int r; if (c) { ...; r = f(); } else r
  = 0; return r;`); `return` per arm, a ternary or a nested inline
  thread it.
- `[vc8]` **An inline that sets a member and the caller's local
  through a reference parameter** keeps the value in one callee-saved
  register on every path (the fail path's `xor r,r; mov [member],r`),
  and later uses need no reload.
- `[vc8]` **A record kept in memory in ascending order where the port
  scalarises it backwards** is default-constructed and filled field by
  field, not built by its constructor.
- `[vc8]` **A table of words stored under a guard bit** is a function
  static array of records with constructors, not a const table copied.
- **Compiler-conditional vtable stores are usually redundant**: when
  the native build maps a class's vtable symbol to the original's data,
  the other compiler's own constructor already stores that address.
  Check by counting the stores of the constant in the native
  disassembly before and after removing the guard.
- `[vc8]` **Call arguments loaded before the first push, or member
  loads sitting in front of an inlined callee's `test; je`**, are NAMED
  LOCALS at the call site, declared in the original's load order (`int
  b = g_b; int a = g_a; int v = vista; obj.f(&z, v, table, a, b);`).
  Expressions written in the argument list, or a wrapper inline, sink
  the loads behind the branch. Naming one call's arguments can also
  fix the instruction order of a later, unrelated call.
- `[vc8]` **A 64-bit difference stored through `eax`/`ecx`** is named
  before the store (`__int64 d = b - a; *p = d;`); stored straight
  from the expression the pair is `edx`/`eax`.
- `[vc8]` **`mov ecx,parent; <loads [ecx+field]>; add ecx,OFF; call`**
  is an inline member of the SUB-OBJECT at OFF whose fields are the
  sub-object's own; an inline of the parent, or locals, take another
  register for the parent and `lea ecx`.
- `[vc8]` **By-value record arguments built in the argument area**
  (`sub esp,N; mov eax,esp; mov [slot],esp`) have declared copy
  constructors; a record result of a member call lands there directly.
  Without them the compiler makes frame temporaries, which also moves
  the frame's slot groups.
- `[vc8]` **Read the frame layout from the compiler's listing** (the
  `name = offset ; size` lines in front of `PROC`: named locals, `$T`
  temporaries, `tv` spills) before touching a float-heavy function:
  locals with disjoint lifetimes share a group sized by its largest
  member; two 4-byte values adjacent in ONE group are an 8-byte record
  (a pair copied whole, a float pair); a named address-taken record
  has a slot of its own, a temporary joins a scalar group.
- `[vc8]` **Virtual calls on a local object** are devirtualised; the
  original's `mov edx,[obj]; call [edx+N]` goes through a pointer to
  the local. `shr r,4; not r; test rl,1` is `~(f >> 4) & 1`.
- `[vc8]` **A local built into the register that held a parameter**
  (`lea edi,[local]` where `edi` was the argument) is the parameter
  reassigned (`if (!c) dc = &a;`), not a second pointer variable.
- **A vtable check regresses when a stand-in class emits the real
  vtable**: a stand-in with its own vtable line at the real address
  compares as different (slots and type name). Use the real class name
  with its overrides declared, and give a new class's deleting
  destructor slot its own address line. The redundancy of
  compiler-conditional vtable stores holds for multiple inheritance
  too, once every `??_7X@@6B<base>@@@` symbol has its address; a class
  named differently from its vtable's class keeps the guard.
- **A marker line whose name no object defines is a port nobody
  compares.** A stand-in struct that derives from the real base and
  declares a member with a virtual slot's name and signature makes that
  member virtual: the object says `@@UAE...`, the hand-written symbol
  line `@@QAE...`. Every scan that walks object symbols skips it, the
  byte comparison never sees it, and a native build that binds by name
  leaves the port unbound (the slot keeps calling the original code
  while the port counts as "has native code"). Scan the symbol table
  for marker names absent from every object after each round; fix the
  name, but treat the fix as a behaviour change (a port that never ran
  goes live).
- `[vc8]` **`mov r1,[p+OFF]; add p,OFF; mov r2,[p+4]`** (first word
  through the owner, pointer adjusted, second word through it) is a
  two-word record passed BY VALUE to an inline constructor
  (`Caja(int a, int b, Tam o) { x = a; y = b; t = o; }` with
  `*(Tam *)&tile->ancho`); a const reference reads both through the
  owner.
- `[vc8]` **A member loaded into a register for a plain zero test,
  with `xor eax,eax` at the exit**, is `int v = m; if (!v) return v;`.
  **A masked bit kept and tested twice** (`and ecx,8; je A; cmp [m],0;
  jne S; A: test ecx,ecx; jne out`) is `((e & 8) && m) || (!(e & 8) &&
  (e & 1))` with the mask written twice; a named mask threads the jump.
  **A counter that is only counted** (`sub r,1; jne`) is `for (i = 0;
  i < n; i++)`, not `for (; n > 0; n--)`. `mov eax,A; jne; mov eax,B`
  is `x = A; if (!m) x = B;`, the ternary gives `neg; sbb; and; add`.
- `[vc8]` **Open: a local kept in memory while a callee-saved register
  stays unused** (a result variable at the frame's lowest slot, or a
  pointer stored to a home slot at each assignment and reloaded later).
  `volatile`, one-field records, arrays of one, unions, references and
  inlines taking `T &` all enregister it; a record whose address
  escapes gives the frame but reloads behind calls. The tell is a frame
  4 bytes larger than ours with one `push` fewer.
- `[vc8]` **Pick the work list by how seldom a function was opened.**
  Join the closer scan (diff lines per unmatched port) with the count
  of mentions of each address in the round records; ports with zero to
  two mentions and 15 to 45 diff lines match within a dozen forms far
  more often than the 2-line closers with ten mentions each.
- `[vc8]` **A box built from two pair temporaries.** Stores in the
  order y, h, x, w, with the first-computed coordinate kept unfinished
  in a register until its store, are `Box b(Pair(x, y), Pair(w, h))`
  with the constructor taking both pairs BY VALUE. The four-int
  constructor stores the first coordinate at once in any store order.
  A record temporary inside a call sits at the END of its padded slot
  (a 6-byte colour at +2 of 8), a named local at the start.
- `[vc8]` **Filled, then adjusted in place.** `mov r2,r1; add r2,3`
  and `sub r1,3` where `x + 3` / `x - 3` would give `lea` / `add -3`
  are two records filled from the argument (`a.x = p[0]; b.x = p[0];`)
  and then changed (`b.x += 3; a.x -= 3;`).
- `[vc8]` **Re-reads name the source.** A global re-read behind a call
  where the port used the local it stored there is a local read back
  from the global at that point. A member table re-read behind every
  call and indexed with `imul` per iteration is the member written at
  every use (`for (i = 0; m[i].kind; i++)`), not a cached pointer. A
  pointer copied (`mov eax,edx`) at the top of each `case` is the owner
  chain written in the switch and in each case (`c->shape->v[0]`).
- `[vc8]` **Inline getters and sub-object inlines.** `movzx eax,WORD
  [m]; cmp ax,K` is a getter returning `unsigned short`; a pointer
  argument loaded in front of a pushed scalar is a getter too; `lea
  r,[this+OFF]` kept over three uses is an inline member of the
  sub-object. A getter may be right for one twin and wrong for the
  other: try twins apart. Float `fsub [p+8]; add p,8; fsub [p+4]` with
  one difference stored and reloaded is a distance inline taking both
  points by const reference with the root inside.
- `[vc8]` **`test eax,eax; setne bl` across a destructor call** is a
  `bool` local holding `f() != 0`. **A 2-D table** indexed `t[row][i]`
  at every use gives `lea r,[row*64+base]` for the walks and `shl; add;
  mov [base+r*4]` for the store; a named row index does not.
- `[vc8]` **A local declared in front of an early return** puts its
  store between that test's `cmp` and jump, and can keep a ternary's
  constant arm from folding into the test.
- `[any]` **Ours longer with whole calls among the added lines is a
  port defect.** Read the image before trying forms: the port may make
  calls the original never made (copied from a sister function).
- `[any]` **`new T(args)` pushing more than the port's `sizeof(T)` means
  the class is short**, not that the original called a raw `operator
  new`. A port that writes `m = operator new(N); m ? new (m) T(..) : 0`
  to get the size has an extra EH state. Find the missing member (the
  constructors write it through a cast), add it to the class, and remove
  the fillers derived classes carried at that offset; rebuild everything
  and confirm no match is lost. The native build then stops allocating
  too little for derived classes without members.
- `[vc8]` **Under /GS a pointer parameter is copied to a frame slot in
  the prologue** (`mov eax,[param]; mov [esp+L],eax`, frame 4 bytes
  larger) and read back from the copy; an `int` parameter is not. A port
  that takes a pointer as `int` lacks the copy: retype the parameter
  (rename the symbol, update every declaration and caller), or, where
  the signature is shared widely, keep a typed local copy at the top,
  which takes the same slot. Scan unmatched ports for the copy in the
  image's prologue.
- `[vc8]` **A difference stored first-first and loaded last-first in a
  12-byte slot** is a vector record local filled field by field inside
  an inline taking both points by const reference (`V d; d.x = a.x -
  b.x; d.y = a.y - b.y; atan2(d.y, d.x)`); two float locals compute the
  second difference first.
- `[vc8]` **A handle kept in memory and re-read at the loop top** is a
  member of one local record together with the buffer whose address is
  passed out (`struct { int h; Find d; } b;`): the record escapes whole.
  A loop entered by a jump into its test is `do { if (..) break; ..; }
  while (..)`.
- `[vc8]` **A pointer loaded behind the loop's entry test** needs the
  loop spelled `if (n <= 0) return; p = table; do { .. } while (..)`; a
  `for` hoists the load above the test. **`mov ecx,eax` on a returned
  temporary** is a chained call, a named temporary gives `lea`.
- `[any]` **A value that stays in `eax` from entry to `ret` while all
  scratch values use other registers is the function's return value.**
  A `void` port keeps it elsewhere and differs by a register rotation
  throughout. Make the function return it (rename the symbol, update
  every declaration); callers that ignore the result are unchanged.
- `[vc8]` **A table scan with three induction variables (index, record
  pointer, byte offset compared `jb`) is an UNSIGNED index with the
  record pointer taken in the body**: `unsigned i; for (i = 0; i < N;
  i++) { E *e = &table[i]; .. goto found; } .. found: use(&table[i]);`.
  The compiler turns the unsigned test into the scaled offset and makes
  the pointer itself (`xor i; mov p,table; xor off`). Three explicit
  variables give the same code with the offset's `xor` in front of the
  table load, whatever their order. One idiom matched five functions.
- `[vc8]` **Two loads then two stores of adjacent ints** is a two-int
  record assigned from a constructor temporary (`*(Pair *)&r->w =
  Pair(m->w, m->h);`); named ints and setters load the lower address
  first. The same record copied twice from a local loads x in front of
  the y stores.
- `[vc8]` **A float member stored and reloaded from the member itself**
  (`fstp [m]; fld [m]`) is written inside an inline operator of a vector
  record (`pos += vel`, `vel *= k`); plain member statements round-trip
  through a frame slot. A value assigned in both arms of an `if` merges
  into one `fstp`; a float local for it brings the round trip back.
- `[any]` **`mov r,1` between a compare and its jump** means the flag is
  set in front of that test (`if (a) { done = 1; if (b) {..} }`), not
  inside it: a behaviour difference in the port. Read where the jump
  goes before trying forms. An extra call whose result is dropped
  (`fstp st(0)`) is a statement the port lost.
- `[vc8]` `sub eax,0; je; sub eax,1; je` is a `switch`; a chain of `cmp
  eax,N; jne` in an odd order is an if / else-if chain. `movzx r,ax` is
  `(unsigned short)w`, `and r,0xffff` is `w & 0xffff`.
- `[any]` **A diff tool that masks jump targets reports 0 lines for two
  functions that differ in which block a jump reaches.** List the raw
  byte differences outside relocations before promoting a match; two
  textually equal exits (`Close(); return 0;` twice) are not one label.
- `[any]` **A stand-in with another calling convention for a member
  whose `this` is still in `ecx`** (a `static __stdcall` under the
  original compiler, the member under the native one) is not needed:
  the plain member calls its sister without a `mov ecx` and matches.
- `[vc8]` **An inline function that the compiler saw but did not inline
  (a COMDAT that is called) hands its callers its memory effects and the
  fact that it cannot re-enter them, but not its clobber set.** Sign: a
  value such as the caller's static-guard word stays in a callee-saved
  register across the small callee and is loaded again behind every
  unknown call, while `this` is still loaded into `ecx` from a
  callee-saved register in front of it. Write the callee's body in the
  class view with `__declspec(noinline)`; an ordinary definition in the
  file also hands over the registers, a bare declaration nothing. It is
  per caller: give that caller its own view of the callee, and list the
  view's name at the callee's real address (not its thunk). A sweep with
  stub bodies only proves something when the port becomes identical;
  "closer" is noise.
- `[vc8]` **The receiver held in `eax` across the argument pushes
  (`mov ecx,eax` last)** is a named local, and the object one step in
  front of it may be one too; the second local can move the
  callee-saved registers of the whole function. A constant hoisted to
  the entry where the image rematerialises it (`or r,-1` in both arms of
  the first guard) is register pressure from such a local further down.
- `[vc8]` **Every early exit ends in `mov eax,REG` and none in
  `or eax,-1`:** the function has one `return result;` behind an
  if/else nest; early `return -1;` statements free the register.
- `[vc8]` **An 8-byte record built at the address a 4-byte local had a
  few lines earlier, in a frame 4 bytes smaller than two scoped
  variables give:** one local used twice (`float d[2]`: `d[0]` first, the
  pair later). The compiler packs a block local with whatever neighbour
  is free (a call's returned temporary), and no placement of braces
  moves it; read a slot two "locals" share as one local before trying
  scopes.
- `[vc8]` **The receiver in `eax`, another member in `ecx`, then
  `mov ecx,eax` in front of the virtual call:** the receiver is not a
  named local; the member is tested, and written out again at the call
  and in the other arm. A local takes `ecx` at once.
- `[vc8]` **A window procedure whose default call sits behind every
  case, with one exit loading a frame slot nothing wrote:** `int r; int
  handled = 1; switch (m) { case ...: r = ...; break; default: handled =
  0; } if (!handled) r = DefWindowProc(...); return r;` with one case
  leaving `r` unset. Returns inside the cases or a `default:` that calls
  put the default body in the middle whatever the case order.
- `[vc8]` **Write a stubborn port from the image's frame, slot by
  slot.** List every store, load and `lea` of each frame offset
  (normalised by the pushes in front) and name one variable per slot;
  a slot with two roles, a slot in the argument area and a slot inside
  another object's bytes each name an idiom below. Rank the unmatched
  ports by `sub esp,N` differing from the image's: that list is where
  these apply.
- `[vc8]` **Address-taken scalars are laid out by reference count,
  highest first, ties by first use;** declaration order does nothing.
  A local that lies lower in the image than its count allows is two
  variables.
- `[vc8]` **A plain local kept in memory shares the slot of an
  address-taken local of an earlier block:** `{ int a = 0; i = f(&a);
  ... } int v = m;` puts `v` in `a`'s slot (stored once, reloaded at
  each use). Only real braces do it; one variable for both roles gives
  the frame size and the wrong order. The same for a `float` behind a
  by-value temporary of the enclosing block.
- `[vc8]` **A local in a parameter's home slot** (zeroed there, its
  address passed out, the parameter itself in a register): the source
  reuses the parameter (`T *el = e; *(int *)&e = 0; g(el, (int *)&e)`).
- `[vc8]` **A jump landing between `mov ecx,this` and the pushes of a
  call:** the call is written out in both arms and merged; an early
  `return` form stores on both paths and moves the exit to the end.
- `[vc8]` **An aligned frame (`and esp,-8`) whose `ebp` is pushed again
  and used as a variable:** the aligned local is aligned by a member
  (`__int64` in a union), not by `__declspec(align(8))`, which keeps
  `ebp` out of the allocator. A stale "not matched" marker is found by
  comparing with every marker dropped.
- `[vc8]` **A port that differs only in where stores to a member are
  scheduled** (the image sinks them behind the next call's first push):
  name the member directly; a reference local (`T *&a = member`) pins
  each store where it is written. Small arrays each handed to one call
  go in a block with that call (the frame is 4 bytes smaller).
- `[method]` **Rank the unmatched ports by how seldom the notes mention
  them, integer-only first.** Ports written "from the control flow
  only" that nobody reopened match once the body follows the image's
  block order: the arm that lies first is the `if`'s first arm, the
  early-exit case comes first with a `return`, a pointer loaded behind
  the guard's jump is initialised inside the `if`.
- `[vc8]` **A branch into the middle of another arm is `||`:**
  `jne B; ...; cmp m,0; je skip; B:` is `if (j != i || m) {B}`; two
  arms with the same body compile to two copies. `jbe` in front of a
  loop is a `for` with an unsigned bound, `je` is `if (n) do ... while`.
- `[vc8]` **A sum stored once and used twice is a variable** (`int tam
  = n + m;`): the frame then has the image's slot count.
- `[vc8]` **No EH state restore in front of the last destructor call:**
  the destructor is `throw()` there (the function sits in the class's
  own file). Declare it so for that one file through a macro; other
  files' matched functions need the plain declaration. A potentially
  throwing call behind the destructor (an `operator delete` without
  `throw()`) keeps the restore.
- `[vc8]` **A vector difference stored, copied, and its slots reused by
  later floats:** the difference is the temporary of an inline
  `operator-`, copied by a user copy constructor (`Vec v(a - b);`).
  Two limit tests with the value kept on the x87 stack are `if (l < 0
  && g > 0) ... else if (l > 0 && g < 0) ...`, not nested `if`s.
- `[vc8]` **Stores in front of a constructor's `mov BYTE [state],N`
  are member initialisers** (scalars `m(arg)` and members with inline
  default constructors); body stores come behind it.
- `[vc8]` **Four distances per port, not one** (raw, registers masked,
  `esp` offsets masked, both): a port that is equal once `esp` offsets
  are masked has a wrong buffer size or a buffer too many. Read each
  array's extent from the `lea`s and the cookie (minus the pushes in
  front). A second small buffer the image lacks is the first one used
  again; a spare `char t[8]` shows as `and esp,-8` in ours.
- `[vc8]` **Every scratch register turned by one from some call on,
  nothing else different:** name the value read at the first turned
  instruction as a block local (`{ int l = m_lista; Send(hwnd, l, ...);
  }`, `T *g = guardian; if (g->tipo == 2) ...`). The rotation runs to
  the end of the function, so fix the first turned load; a local added
  upstream turns everything downstream the other way.
- `[vc8]` **`mov ecx,eax` from a by-value call into the next member
  call:** the value is used as a temporary (`v.Valor("X").CopiarA(t,
  n);`), not through a named local (`lea ecx,[local]`). Two such calls
  kept in callee-saved registers and used in reverse order are two
  temporaries in one argument list. A flag zeroed at the top and tested
  behind a `new` is a temporary inside the new-expression's arguments.
- `[vc8]` **An argument computed before the `push` of a constant that
  precedes it** is a call through an inline whose parameters were
  evaluated first; a virtual called by address is a qualified call
  (`Clase::v13();`).
- `[vc8]` **A matched function can still count as different through the
  names it references** (a constructor of a stand-in class names the
  stand-in's vtable and RTTI record): run the comparison after dropping
  the port marker and read the count before committing.
- `[vc8]` **Remove the porter's cached pointers first.** A port that
  starts with `T *d = (T *)member;`, `Row *r = &table[i];` or `G *g =
  global;` and uses it throughout differs from an image that re-reads
  the member, re-indexes the table or re-reads the global at each use
  (`mov ecx,[esi+0x5c]` in front of every call). Write the expression at
  each use (a macro in front of the function) and let the compiler do
  its own CSE: it gives the image's registers and load points.
- `[vc8]` **Same-size locals of disjoint scopes share a slot, buffers
  and objects alike.** A frame smaller than the sum of the port's
  buffers means the buffers are block locals of sibling blocks (one
  `char t[0x20]` per arm or per loop body), each sized by its own use;
  a `lea` at a second offset for the last calls is a second, smaller
  buffer the port merged into the first.
- `[vc8]` **A tester that masks relocation targets cannot see a wrong
  string literal.** Run the full comparison (with symbol resolution)
  before dropping a marker: a default key `""` compared equal to the
  image's `"TIP"` in the masked listing.
- `[vc8]` **Static helpers with register arguments** (a buffer in
  `ebx`, a path in `eax`: whole-program optimisation of file-local
  functions) put their callers out of reach as well: check a port's
  callees for such a convention before spending forms on it.
- `[vc8]` **A port whose object code is a handful of lines while the
  image has hundreds is a forwarder** to a free function that holds the
  body (`void C::F() { F_impl((char *)this); }`). Make the body the
  member itself; such rows show up in a length scan (ours against the
  image) and often match at once after the move.
- `[vc8]` **A table and a loop in the port, N copies in the image, each
  with its own EH state number**: the original wrote the fields out.
  Write them out too (a local macro per field keeps it readable).
- `[vc8]` **A block built from a literal whose address is formed again
  with `lea` behind its constructor, in a slot of its own** is the
  temporary of an implicit conversion (`f(0, "POLY")` over a `const T
  &` parameter). The functional cast `T("POLY")` pushes the
  constructor's `eax`; a named block shares the other named blocks'
  slot.
- `[vc8]` **A `switch` lays the `case` bodies out behind the default
  body** even when they are written first (their EH state numbers stay
  in source order), **and the locals of its arms do not share stack
  slots**; sibling blocks of an `if`/`else` do. A frame that holds every
  arm's objects side by side, with the lower-state arm at the end of the
  function, is a `switch`.
- `[vc8]` **SDK macros expand in place**: `ListView_SetItemText` is a
  block `{ LVITEM it; it.iSubItem = ..; it.pszText = ..; SendMessage(..,
  &it); }` whose stores are scheduled between the pushes; an inline
  function with parameters loads the handle first and gives another
  order.
- `[vc8]` **A call that bypasses the incremental-link thunk table is a
  call to a `static` of the same original file.** A port whose image
  calls such a target can only match in the file that holds the static
  (and all its other callers): the static gets a register convention of
  its own. Sweep the unmatched ports for direct call targets that live
  in another object of yours and move the static next to its callers
  instead of matching it alone behind a keeper function.
- `[vc8]` **Which register a static's argument arrives in follows the
  argument's type**: a window or tree-item handle declared `int` came in
  another register (or was loaded later in the caller) than the same
  argument declared as a pointer. Handles are pointer-typed.
- `[vc8]` **A record local sits below the `char` arrays in the frame**:
  an API's data block written as `char buf[N]` lands among the buffers,
  as a struct of its fields below them (`WIN32_FIND_DATA`).
- `[vc8]` **`test eax,eax; setne BYTE [flag]; ~T; cmp BYTE [flag],0`**
  is a temporary's result compared explicitly (`if (f().Count() != 0)`);
  the bare `if (f().Count())` keeps the value in a register across the
  destructor.
- `[vc8]` **Every converted literal in a slot of its own inside a
  `switch` arm, its address formed again with `lea`** says the callee's
  parameter is a `const &` and the call site relies on the implicit
  conversion; fix the declaration (the mangled name changes) instead of
  writing named blocks.
- `[tools]` **Compare the EH tables too.** A function with the image's
  code bytes can still differ in its unwind table (one state more or
  less): the comparison has to follow `__ehfuncinfo`/`__unwindtable`,
  and a tool that derives function ends from a list of starts gives
  absurd distances where the list misses a start.
- `[vc8]` **An original two to five times larger than the port is a
  helper it has in line.** VC8 never inlines the port's `static`
  helper; list the ports whose object calls a same-object function that
  has no address in the image, force those helpers (`__forceinline`, or
  the body as a macro where the two expansions differ in operand order)
  and then rewrite the caller from the listing: the helper alone is
  seldom enough.
- `[vc8]` **Four copies of a loop body behind `cmp n,4; jl`, the bound
  as `add r,-3`,** is VC8 unrolling a plain counted loop (`for (i = 0;
  i < n; i++)` over an inline getter, the count in a local). A port
  with the loop in a helper, a comma expression in the condition or an
  index computed by hand does not unroll.
- `[vc8]` **A member stored and loaded again in the next instruction**
  is an inline member function working on `this` (`p.Negate()`), and
  `mov r,[a]; add [b],r; mov r2,[a+4]; add [b+4],r2` is an inline
  `operator+=(const T &)`; the same statements written at the call site
  forward the stored values.
- `[vc8]` **`call new; xor r,r; cmp eax,r; je; ...stores...; mov r,eax`**
  is `new T(args)` with an inline constructor (its body is the store
  order), `test p,p; je; mov [p],vtable; ...; call delete` is `delete p`
  with an inline destructor. A hand-written `operator new(n)` plus
  field stores keeps one register and never matches.
- `[vc8]` **`jmp callee` at the end of a cdecl function that has stack
  arguments:** the callee takes none. VC8 does not turn a call that
  pushes into a jump.
- `[vc8]` **`cmp x,1; je; cmp x,3; jbe; cmp x,5; ja`** is a `switch`
  with cases 1, 4, 5; the jump table's targets in address order are the
  source order of the `case` labels.
- `[vc8]` **A 16-bit parameter handed on to a 16-bit parameter is pushed
  as the whole dword of its slot** (`mov edx,[esp+8]; push edx`), so a
  virtual handler whose image loads words (`mov ax,[esp+N]`, `mov
  di,[esp+N]; movzx eax,di`) has 16-bit parameters in the WHOLE
  hierarchy (`OnCommand(WORD code, WORD id, ...)`), and so have the
  helpers it forwards them to. Change every declaration and symbol at
  once; the matched overrides keep matching.
- `[vc8]` **A dialog procedure**: `xor eax,eax` in front of the first
  range test is `int r = 0;`, nested `switch`es, `r = 1; break;` for arms
  that share the `mov eax,1` exit and `return 1;` for arms with their
  own. `LOWORD`/`HIWORD` are `(unsigned short)w` and `(unsigned
  short)((unsigned)w >> 16)`: the plain shift compares against the
  masked dword.
- `[vc8]` **A returned constant formed as `lea eax,[r+K]` behind a
  compare of `r`, every arm with its own epilogue:** the function has
  one exit (`r = 1;` in each arm, `return r;` at the bottom). VC8 copies
  the exit into the arms and reuses the known register; `return 1;` per
  arm gives `mov eax,1` and one shared tail.
- `[vc8]` **A flag word tested bit by bit around destructor calls behind
  a constructor call** (`mov ebx,7; test bl,4; je; and ebx,-5`): the
  temporaries are written inside the `new T(...)` expression (chained
  getters returning by value, a `?:` between two such calls). A pointer
  argument computed into its own register in front of the others is a
  base-class conversion with its null test (`this ? (char *)this + N :
  0`).
- `[vc8]` **`xor eax,eax; cmp [m],eax; setne al; test eax,eax` in one
  arm while the other arms call the tester:** the tester is in line at
  that call; no expression at the call site gives the `setne`. A frame
  4 bytes larger than the image's with every slot equal can be the
  temporary of an intrinsic `strlen`.
- `[vc8]` **`xor eax,eax` in front of the first test and refusing exits
  that load nothing** is `int r = 0;` with the work inside the
  `if`/`switch` arm and one `return r`; a search loop is `for (...) { if
  (hit) { work; r = 1; break; } } return r;`. `sub eax,K; je; sub
  eax,K2; jne` is a `switch`, not two comparisons.
- `[vc8]` **A null-tested send whose last argument is loaded in front of
  the jump** (`cmp [obj+N],0; mov edx,[this+M]; je`) is an inline member
  of the object (`void Motor(a, b, c, d) { if (motor) motor->Mensaje(a,
  b, c, d); }`); the test written at the call site loads the arguments
  behind it. Scan the ports for `if (X) X->M(` written by hand.
- `[vc8]` **`add r,8` behind the first `fsub [r+8]`, the next fields at
  `[r+4]`:** the vector is reached through a pointer local (`float *a =
  &actor->x; d[0] - a[0]`). Three differences stored and then copied are
  an inline `operator-` returning by value.
- `[vc8]` **A float local with a slot of its own while its sibling takes
  a dead parameter's slot:** both were filled through an inline with
  out-pointers. A copy of one of them (`float qy = py;`) used in the
  loop exchanges the order of two products without moving the hoisted
  loads.
- `[vc8]` **`fabs` between two store/load pairs** is an inline `float
  Abs(float v) { return (float)fabs(v); }` applied to the expression.
- `[tool]` A variant runner that replaces a declaration line must keep
  the trailing address comment on that declaration when the generator
  reads addresses from comments. Functions whose image reads an
  uninitialised slot stay unmatched when the port must zero it: note
  them once and stop.
- `[vc8]` **A constructor port whose image has more EH states than the
  port is a real constructor in every piece.** `mov [state],0` behind
  the first constructor call is a base (or first member) with a
  destructor; a state byte around each `operator new` + constructor is
  `new T(args)` with a declared constructor. Read the unwind funclets in
  front of the handler address for which destructor each state runs.
  No vtable store behind that first call means the "base" is the first
  member. Rank the remaining constructor ports and rewrite them this
  way: most match at the first or second build.
- `[vc8]` **Int stores that lag behind the float stores that follow
  them in the source are stores to pointer fields** (`void *a; a = 0;`
  against `int a`). The type of a zeroed field moves its store.
- `[vc8]` **A field reloaded and tested right after it was stored**
  (where the port folds the load): a store in between goes through a
  pointer local (`int *q = &m.c10; q[0] = 0; q[1] = d;`), which VC8
  treats as a possible alias. An inline member or a reference does not.
- `[vc8]` **A constant loaded into a register early while its stores
  come last:** the assignments stand early in the source; VC8 sinks the
  stores and leaves the load at the statement's place.
- `[vc8]` **A zero in a register of its own used for one field twice**
  is a named default-constructed local assigned twice, not two
  temporaries. A compare on a field in memory followed by a second load
  for the copy means the copy is a record assignment.
- `[vc8]` **A virtual call on a member object** goes through a pointer
  local (`T *t = &m; t->F()`); on the member itself VC8 calls directly.
- `[vc8]` **A struct assignment from a float temporary is hoisted above
  the int stores written in front of it.** The image shows the
  `fst [tmp]; mov r,[tmp]` pairs and most member stores right behind
  the previous call, a constant loaded between them, and the LAST dword
  store behind a run of unrelated int stores. The source order is the
  int stores' order, with the assignment standing where that last store
  is. Scan the split point (how many int statements stand in front)
  instead of permuting statements.
- `[vc8]` **A pointer member loaded early and loaded again behind a
  call** is the member read at each use, not a pointer local; the local
  also swaps registers at a later virtual call.
- `[vc8]` **`cmp [m+N],0; je; mov r,[m+N]` where the port loads once
  and compares the register:** the test is an inline `bool` getter
  (`bool Hay() { return p != 0; }`) and the use a pointer getter. Two
  spellings of the field, a local, nested `if`s or an int-typed test
  all load once.
- `[vc8]` **Same-valued constant stores grouped where the image keeps
  them interleaved:** VC8 pulls the stores of the SECOND constant it
  meets to the front; write the table in its natural (row by row)
  order, not in the image's store order.
- `[vc8]` **An inline library constructor whose stores come in another
  order at one site** may be a different class with its own constructor
  (a template instance); changing the shared header loses the matched
  out-of-line copies. Keep the variant local to the file.
- `[vc8]` **A temporary two bytes larger than the member it is copied
  to** (frame 4 bytes bigger, a dword reload at an odd offset): the
  temporary is a derived or larger record sliced on assignment.
- `[vc8]` **A store through a pointer is a fence.** When ours hoists
  something over a run of member stores that the image keeps behind
  them (the EH state store, the next call's `mov ecx,..`, an argument
  push, a constant kept in a register), one of the stores in the run
  goes through a pointer: a pointer local (`int *p = &b; *p = 0;`) or,
  more often, the induction pointer of a small loop the compiler
  unrolls (`int c[4]; for (i = 0; i < 4; i++) c[i] = 0;`, descending
  for stores in descending order). Four scalar fields and four
  `c[i] = 0` statements are not the same thing as the loop. Look for
  runs of equal stores to adjacent offsets: they are arrays.
- `[vc8]` **Two ints converted (`fild`) from the slots of a later
  temporary instead of dead parameter slots:** they were taken as a
  pair by value from an inline getter (`Tam t = dev->Tam();`), not
  read into two int locals.
- `[vc8]` **Int stores interleaved with the float stores of the member
  constructed in front of them** (instead of sunk behind): the
  initialised fields are pointers, the zero one included.
- `[vc8]` **A field read back from memory right behind its own store**
  (`mov [esi+0x2c],eax; mov [esi+0x30],ecx; mov ecx,[esi+0x2c]; push
  ecx`): the two fields were assigned as one aggregate (`*(Tam *)&ancho
  = Tam(a, b);`). The values of an aggregate copy are not forwarded to a
  later field read; two scalar stores are.
- `[vc8]` **A member store in front of a pointer load and its test,
  where yours sinks behind the test** (`mov [esi+0x80],edi; mov
  eax,[esi+0x74]; cmp eax,edi`): the test and what follows are an inline
  member of the sub-object (`void Marcar() { if (paso) paso->listo = 1;
  activo = 1; }`), the store is a plain statement in front of the call.
  One member holding all three statements is not the same.
- Keep the diff of every unmatched function in one directory and search
  it for an idiom's signature (a reload behind a store, a pure reorder
  of stores) before opening functions one by one.
- `[vc8]` **Vector copies by `fld/fst`, vector assignments by three
  `mov`:** the values are objects of a vector class with a float copy
  constructor and the default assignment, not float locals. Then: three
  `fstp` into a temporary and three `mov` back into the vector the
  difference was taken from = the result is assigned back (`q = q -
  pos;`), no second variable; `lea eax,[tmp]; mov ecx,[eax]; mov
  edx,[eax+4]; mov eax,[eax+8]` in two arms = a ternary of two
  temporaries written at the assignment (`v = m ? a - m->off : V(0, 0,
  0);`), not an inline returning it (one more float copy) and not
  `if/else`; `cdq; xor eax,edx; sub eax,edx` = `abs()`; a zero stored
  into a pointer parameter's home while the parameter lives in a
  register = an int local, not a write to the parameter.
- `[vc8]` **`(cur - base) >> 2` recomputed and compared with the count
  in every pass** (yours keeps an index): the walk is an iterator record
  with an inline test (`x = *cur; return x != (T *)-1 && (unsigned)(cur
  - beg) < n;`), not a hand-written loop.
- A function that shrinks when its callee's arity is fixed can become
  inlinable into a caller in the same file and cost that caller's
  match: move it to the file of its address neighbours.
- A diff tool that masks jump targets and constants shows nothing for a
  function that differs only there: keep a raw byte comparison at hand,
  and build the diff directory from the objects' symbols (a marker line
  whose first name no object defines hides the function from a
  name-driven scan).
- `[vc8]` **Read the direction of every float compare off the load
  order and the mask before anything else.** `a > b` is `fld a; fld b;
  fcompp; test ah,0x5; jp`, `a < b` is `fld a; fld b; fcompp; test
  ah,0x41; jne`; `x > 0.0f` is `fldz; fcomp st(1); test ah,0x5`, while
  an int zero (`0 < x`) or `0.0f < x` compares with a double constant
  in memory. A port taken from a decompiler has the operands swapped:
  same behaviour, other bytes. `fabs` in the original against a
  compare-and-`fchs` in yours is the intrinsic `fabs()`, not a ternary.
- `[vc8]` **A store that the original has in both arms of an `if/else`,
  behind each arm's constant load, where yours stores once in front:**
  the statement is written behind the `if/else`; arms that end in
  separate returns each get a copy. In front, or in both arms, it is
  hoisted.
- `[vc8]` **The EH state stored in front of a function-local static's
  initialiser counts the inline expansions in front of it:** every
  expansion of an inline that owns a static takes a state, even when
  the branch holding that static folds away. A higher state than yours
  says how many times the inline was expanded (two separate `if` arms
  each calling it, not one `||`).
- **A bare `cmp; jmp` into a shared `je` (two tests ending in one
  conditional jump) is a `switch` with repeated arms**, not a `?:`:
  `switch (k) { case A: if (f()) X else Y; break; case B: Y; break;
  default: X; }`. The compiler merges the equal bodies; every `?:`
  spelling materialises a flag (`setne`, `neg; sbb; neg`).
- **Two copies of one body, one entered behind `fstp st(0)` and one by
  falling through behind `fcomp`, are ONE `if (a || b)`** whose
  operands leave different x87 stack depths: the body is copied per
  stack state. Two `else if` arms with the same body stand in source
  order instead.
- **With a memory operand the zero is loaded and the memory compared**
  (`x < 0.0f` = `fldz; fcom [x]; test ah,0x41; jne`), and the zero stays
  for the next compare; with the zero written on the left the operand
  is loaded first. Read the load order before trusting a decompiler's
  operand order.
- **A constant not folded into a scaled index (`add eax,2; lea
  edx,[eax+eax*2]`) says the index came out of an inline helper** taking
  it by value (`inline int Sel(int i) { return (unsigned)i < 5 ? i : 0;
  }`, `rows[Sel(i) + 2]`).
- **Ports nobody opened beat ports everybody tuned:** rank the remaining
  ports by size against how often the notes mention them; the ones
  written straight from the decompiler (no locals, `if` chains, compare
  directions reversed) fall in a few edits.
- **x87 intermediates spilled through one slot are casts, not locals:**
  `fstp [t]; fld [t]` several times inside one expression means the
  source had `(float)(a - b)` operands (or float parameters of an
  inline) and no named float locals; a `fstp QWORD` hoisted behind a
  loop guard is an invariant subexpression written inside the loop
  body, not a `double` local declared in front of it.
- **A `<<` pack that VC8 turns into `lea; xor; and; xor`** (a bit-field
  merge) is avoided by writing the pack with multiplications
  (`(r >> 3) * 64 | ...`) or by shifting in separate statements; the
  plain `sar; and; shl; or` sequence in the image says one of the two.
- **`fcomp DWORD [const]` against a constant the port loads first:** the
  constant is a literal in the source (`q < 1.0f`), not an `extern
  const float` standing for its address; externs are loaded like
  variables and hoisted.
- **Two parameters in each other's callee-saved register plus a scratch
  rotation one step off:** a record built at the call site was built
  by an inline constructor taking those parameters.
- **A frame four bytes too small** with otherwise equal code: a
  two-float local is a three-float record whose last field is never
  written; a later eight-byte value shares its twelve bytes.
- **A member re-read in front of every statement** (`mov eax,[this+8]`
  ten times) is the member spelled at each use; a decompiler-written
  port caches it in a local and loses every reload.
- **A masked diff of zero lines that still does not match:** compare the
  LENGTHS. A conditional jump with a 32-bit displacement to the last
  copy of a duplicated tail (ours: 8-bit to the nearest copy) is a
  `goto` to the function's single `return`; every later byte is equal
  and only the displacements over the gap differ.
- **`mov ecx,esp; fstp [ecx]; fstp [ecx+4]; ...` for float arguments** is
  a record with a copy constructor passed by value; plain float
  arguments are stored `fstp [esp+N]` after one `sub esp`. Both can
  stand in one call.
- **The caller reloads `this` from a slot behind a call although the
  callee is in the same file:** the callee clobbers `ecx` (often a
  result variable, `xor ecx,ecx` at its top and `mov eax,ecx` at the
  failing exits). A port of the callee that leaves `ecx` alone makes
  the caller keep using it; match the callee first.
- **One callee-saved register exchanged between a handle and a sum:**
  the handle variable was reused for an earlier value
  (`h = Create(); Close(h); h = Open();`); the longer life wins the
  first register and moves its `push` in front of the first call.
- **A scratch rotation one step off behind a call:** try an inline
  getter on one of the call's ARGUMENTS, not only on the receiver.
- **Hand-written calls of the `eh vector` constructor/destructor
  iterators, or a placement `new (operator new(N)) T(...)`, in a port:**
  write `delete[] p; p = new T[n];` and a plain `new T(...)` on a class
  with a declared destructor. The placement form hides a class view
  that is shorter than the size the image pushes; fix the view.
- **A receiver loaded BEHIND the call that makes its argument**
  (`call Token; mov ecx,[this+N]; mov edx,[ecx]; push eax; call [edx+M]`):
  the argument is a temporary in the call (`obj->Read(&v.Token(1))`),
  not a named local in a block.
- **One destructor call in the port, two in the image at `[x]` and
  `[x+N]`:** the record's destructor is inline (a base and a member
  with out-of-line destructors, `__forceinline ~T() {}`).
- **A compare against a memory operand with every later scratch
  register one step on:** read the operand through an inline getter,
  after the other operand has been put in a local
  (`unsigned d = cur - beg; if (d < Count(l)) ...`): the getter's
  temporary takes a rotation step and is then folded into the `cmp`.
- **Disjoint-scope buffers that do not share a slot, frame larger by a
  buffer:** declare the big buffers at function level. Compare the
  whole `lea reg,[esp+N]` sequence of image and port before anything
  else in a function with many EH objects; the order of same-sized
  slots is not the declaration order and moves with any edit.
- **A null test without the zero register the function uses later
  (`add eax,[tbl]; je`, the `xor` only behind the next call):** the
  lookup is written out in the function with a pointer that starts
  null (`T *b = 0; for (...) if (...) { b = &t[i]; break; } if (b)`);
  an inline helper returning the pointer gives the early zero.
- **A vector loaded by `fld/fstp` into a slot and stored by `mov`,
  with an int store among the `mov`s:** a class with a float copy
  constructor and default assignment, the copy a named local and the
  int statement between copy and assignment (`V t(src); flag = 1;
  dst = t;`).
- **An operand order of a sum of products that no spelling moves:**
  put the whole test in an inline `bool` helper taking the floats by
  value; a helper returning the float does not do it.
- **`p[n - 1]` read as `[p+n*8-8]` then `lea [p+n*8]; [-4]`:** the
  index expression is written at each access (reference parameters of
  an inline), not passed as an index.
- **Two calls with side effects in one argument list:** the compiler
  under test and the native compiler may evaluate them in opposite
  orders; write them as locals in the image's order (same bytes).
- **A constant kept in a callee-saved register for the whole function
  where the image writes immediates (and a parameter spilled to its
  home slot):** not a spelling of the constant. Read the whole function
  against the decompiler and give every block the image's shape (locals
  declared in the branch that uses them, a default plus an `if` for a
  selection, the tail's flag tests as the image has them); the
  allocation follows once the structure is right, and single fixes can
  move the score the wrong way on the way there.
- **Skipped EH state numbers (unwind-table entries with a null
  action):** an `inline` member with function statics was expanded with
  a constant argument and its static initialisers died; they keep their
  state numbers. Decode the table (the handler's `mov eax,FuncInfo`,
  then `maxState` pairs of toState/action), find the class's setter
  with that many statics, declare it `inline` and write the expansion
  as a call. Two initialisers testing the same guard bit in one
  function are the same inline's static expanded twice.
- **A message handler's tail:** `mov reg,1; call Base; mov eax,reg` is
  a result variable set in front of the base call; `mov eax,[flag];
  test eax,eax; je end` with no `xor eax,eax` is the base's answer
  assigned to the flag and returned; `sete al; setne cl; and eax,ecx`
  is a bitwise `&` of two tests.
- **One integer store scheduled ahead of the first float load of an
  expression:** the expression is an inline helper taking pointers.
- **A member vector copied by integer moves through a `lea` that is
  also the next call's receiver:** a pointer local to the member.
- **Scratch registers rotated by one from a statement to the end of
  the function (`eax, ecx, edx` against `eax, eax, ecx`):** the load
  at that statement is a named local or an inline getter with another
  statement between load and store (`int v = estado; pasar = 1;
  anterior = v;`). Fix that one statement before trying anything
  else; every scratch choice behind it follows.
- **Repeated written-out state blocks in a handler** are one setter
  member expanded with constants. If `inline` leaves `push k; call`,
  the image has no out-of-line copy: force it in line. A setter that
  is expanded at one site and called at another is plain `inline`.
- **Two EH states without code between two expansions of an inline
  with statics:** the source calls it twice in a row with the same
  constant; the second expansion folds away and leaves its statics'
  states.
- **A receiver computed into `eax` in front of the argument pushes
  and moved to `ecx` behind them** is a chain of inline getters, not
  a pointer local.
- **"0 bytes differ" but the string literals are reported different:**
  the `switch` arms are in another source order; the jump table hides
  it, the relocations do not. Arms are laid out in source order.
- **A `void` handler whose image ends `mov eax,[flag]; test eax,eax`**
  returns `int`: check the vtable slot's other overrides.
- **A flag and a parameter compete for one callee-saved register and
  yours has the wrong winner:** declare the flag `bool` for the
  experiments. It cannot be registered, the parameter takes the
  register, and the diff shows what else is wrong in every block. Fix
  those, then go back to `int`: the allocation usually stays.
- **A named local the image does not have costs a scratch rotation
  like a missing one.** When a rotation starts at one statement try
  removing a local as well as adding one.
- **All arguments of a call in registers in front of the first push**
  with a callee-saved constant clobbered and reloaded behind the
  call: named locals for exactly the values loaded first, in the
  image's order; or the call goes through an inline wrapper that
  takes the arguments.
- **An argument loaded in front of a null test of the receiver's
  member** (`cmp [o+N],0; mov edx,[this+M]; je`): the call is an
  inline member that tests and forwards; its arguments are evaluated
  in front of the test.
- **A callee-saved register skipped** (the image pushes `ebx, ebp,
  esi`, yours `ebx, ebp, edi`): a variable the optimizer removes
  still takes its turn in the allocation, e.g. a result variable
  (`int res = 0; ... res = 1; ... return res;`).
- **A file-local `static inline` left as a call with a register
  argument** has a private convention; if the image has it in line,
  force it (`__forceinline`).
- **`fld [m]; fstp [t]; fld [x]; fsub [t]`:** the operand comes
  through an inline function returning `float` (the returned value
  is rounded through a slot).
- **Only the order of the register reloads behind a block differs:**
  the block's test is nested `if`s, not one `&&` chain (or the other
  way round).

### Statics with internal linkage: where the guard test and a flag word's load may go

- **A function static's guard test in front of stores that precede it
  in the source (`test [guard],m; mov [a],r; mov [b],r; jne`, yours
  the stores first):** the static's function has internal linkage or
  is a plain (not `inline`) function, so nothing else can address the
  guard word and the compiler moves its load over the stores. An
  `inline` member's statics are COMDAT data: the load stays behind
  the stores. Two spellings: a plain member defined in the caller's
  file that `/O2` (`/Ob2`) expands on its own (check that both the
  expansion and the out-of-line copy match), or a `static
  __forceinline` free function when a plain one stays a call.
- **A global tested in front of stores through pointer parameters
  (`test [g],4; ...; mov [edx],eax; ...; je`):** the global is a
  `static` of the original's file. Declare it `static` for the
  matching compiler only (a macro that is `extern` for the native
  build keeps the one object).
- **The guard bits of a function's statics are assigned in source
  order:** in a `switch` they give the order the `case`s were written
  in (the jump table is by value either way); the EH state numbers
  say the same.
- **Dead EH states between a function's own statics:** count them in
  units of the setter's statics: each unit is one expansion of the
  setter with a constant whose `switch` arm is empty. Blocks a port
  wrote out by hand (`clear(); state = k;`) are those calls.
- **`push k; jmp` to one shared `mov ecx,esi; call setter`:** the
  setter is called at each place (`Set(9); break;`), not through a
  `next` variable with one call behind the `switch`.
- **A handler's epilogue behind the info path with the base call's
  block after it:** the tail is two flag tests (`if (!done && id ==
  idInfo) { done = 1; Info(); } if (!done) { res = 1; Base(); }`),
  not a nested `if / else`: same instructions, another block order.
- **A base-class slot that takes a `float` where the class declares
  the virtual with `int`:** declare a second, non-virtual member with
  the float signature bound to the same address and pass `*(float
  *)&arg`; no override's name changes.
- **A constant in a callee-saved register from the top of a function
  and an immediate at the exit:** a result variable set at the top
  (`int r = 1; if (bad) r = 0; else {...} return r;`). The reverse
  (yours registers a constant, the image uses immediates): look for
  early returns that are really one exit (`if / else if` with one
  statement behind it).
- **Float constants loaded from `.rdata` in front of integer stores**
  are literals; an `extern float` is reloaded and cannot move.

### A guard word shared between functions names an inline; the scratch rotation is global

- **A function static whose guard word is not the function's own
  (one static tests `[A]`, the others `[B]`, and its EH state still
  sits in the function's numbering):** it belongs to an inline member
  expanded there (COMDAT data: one guard word for the out-of-line
  copy and every expansion). Search the image for every instruction
  that sets that word (`or [G],bit`): the users are the inline's body
  and all its expansions, and the body shows what a port wrote out by
  hand. Scan all ports for guard words with more than one user.
- **The inline must be one definition:** move the caller into the
  file that defines it, or repeat the definition token for token.
  With it in scope the compiler chooses the image's mix of calls and
  expansions by itself.
- **Scratch registers (`eax/ecx/edx`) permuted all the way down a
  long function:** the allocation rotates through the function, so
  one extra or missing temporary near the top shifts everything
  behind it. Work on the first differing line only (a named copy of
  a member, a flag stored on both paths of a test) and rescore; the
  difference count of a rotated function says nothing.
- **Code equal, the function's unwind table one state longer:** a
  second expansion of a setter whose static is dead (a plain store in
  your source was `Set(1, 0)`).
- **The same tail twice with two frame slots for a by-value result:**
  one forced inline called in both branches, not shared code behind
  the `if`.
- **A store hoisted in front of a branch:** the same store written in
  both arms, not once in front.
- **Count the unwind states before adding expansions:** read
  `maxState` from the function's FuncInfo (the handler stub's
  `mov eax, offset`). An inline with N statics takes N states per
  expansion, reached or not, in source order; the count tells how
  many expansions the original has and rules out a dead one that
  merely happens to fix the registers.
- **An inline member with statics that several files expand belongs
  in the class header, defined once:** two token-identical copies
  over two class views merge silently and are an ODR trap. Try the
  header `inline` and compare the identical set; ports in other
  files then turn out to hold its expansions written by hand.
- **A small record read by value and a message block of the same
  shape in one frame slot:** the block is the same local, written
  again and passed by address, not a second array.
- **In-memory `or`/`and` on an array element, or a repeated test the
  original drops inside an expansion:** index the element at each
  use; a reference to the element makes VC8 load, combine and store
  once and keeps the test.
- **A loop's induction base at a member that is not the first one
  read:** a pointer local to the neighbouring sub-object
  (`P *p = &a[i].pos;`), the other members by index.
- **Inline members copied into several files are found from the
  native objects, not from the sources:** every weak function
  symbol defined by two or more objects whose class has more than
  one source definition is a candidate (the ODR check only shows
  the copies whose bodies differ). Token-identical copies go into
  one header with `#pragma once`, included at the old place;
  copies over different class views need the union view first
  (same offsets, every file's member names, casts at the uses
  whose pointer types differed). Compare the identical set after
  each header; leave a copy where two files need different
  spellings of a shared base class or record to match.

### The unwind state count is a scan: what the front end saw and the code no longer shows

- **Compare the image's `maxState` with your object's for every
  port** (the FuncInfo behind the handler stub; in the object the
  `__ehfuncinfo$NAME` symbol, dword +4), and print the image's
  unwind map with each funclet's target: it names every object
  the original source had, by address and destructor. States are
  assigned from the source before inlining and dead-code removal,
  so they survive where the instructions do not.
- **One state more, whose funclet destroys the base part of the
  result or of `this` (`mov ecx,[ebp+N]; jmp ~Base`), with no
  store of that state anywhere in the code:** an out-of-line
  constructor whose body can throw is defined in the class header
  and expanded at a call with a constant argument that folds the
  body away (`T(owner, 0)` expanded, `T(owner, &x)` a call of the
  same constructor). A hand-written one-argument constructor with
  the same stores gives equal code bytes and no state: functions
  whose code is byte-equal and whose `__ehfuncinfo` differs are
  this. An inline setter, a member with a constructor or an
  intermediate base do NOT make the state; only a call the front
  end cannot see through does.
- **A state with a null action** is a static local of an inline
  expansion whose initialisation is dead for the constant
  arguments; count them to get the number of expansions. Not the
  cause: `try`, empty inline destructors, placement `new`.
- **A funclet on a parameter slot (`lea ecx,[ebp+4]`)** is a
  local object packed into the dead parameter's home, not a
  by-value parameter: write a local class with an inline
  destructor and let the compiler place it.
- **A named local with a destructor at the end of a function
  takes one slot too many** when a temporary of the same type
  precedes it at function scope: put the tail in a block
  (`{ T v = f(); ... }`) and the two share the image's slots.
- **Classify every remaining port by its first differing hunk**
  before choosing work (prologue/frame, EH, register exchange,
  order, constants): the class sizes show where one idea closes
  many, and which classes are the residue of every earlier round.
- **A deleting destructor with an EH frame over a destructor
  without one**: the destructor's body calls a helper declared
  `throw()` (`~T() { Unload(); }`). The destructor needs no
  frame; the compiler-generated `??_G` expands both and keeps
  the state that destroys the base. Delete any hand-written
  "destroy and free" port and let the compiler emit `??_G`.
- **Declare `operator delete` / `operator delete[]` as
  `throw()`** in a file where the image has no state store
  (`mov [state],-1`) in front of an inline destructor's delete.
  A diff that is only state stores is a wrong `throw()` on a
  callee or on delete.
- **An explicit "destroy" call at a function's tail is a
  missing unwind state**: make it the class's destructor (an
  inline one in a header when the image expands it; its
  out-of-line copy is what the funclets call). A record with no
  destructor of its own but members that have one gets a state
  per member while the implicit destructor runs.
- **/GS shadow-copies pointer parameters, not integers**: in a
  function with a cookie, a frame one slot short plus a missing
  `mov [esp+N],reg` at the top is a parameter typed `int` that
  is a pointer. A `char` array inside a class view makes the
  cookie itself: pad views with `int`.
- **Name the value that survives a call**: a check word or a
  receiver pointer in a named local takes a callee-saved
  register; as an expression it takes a scratch register and
  rotates every scratch choice after it.
- **A static function's register convention follows the order
  of its local declarations**; try reordering them before
  anything else when only the incoming register differs.

### Hand-built pieces that were language constructs: static arrays, by-value records, calls kept out of line

- **A flag test, a row of placement constructions and an `atexit`
  call** are a function-static array with brace initialisers:
  write it as one (`static T a[N] = { T(...), ... };`) and the
  guard word, one unwind state per element and the exit destructor
  come from the compiler. The element view needs its real
  destructor.
- **A box (or any record) built from two smaller records passed BY
  VALUE** evaluates the second record's operands first, then the
  first's; one constructor with four scalars, field assignments in
  any order and locals for the operands all load the first field
  first. When four stores match and only the order of their loads
  does not, try `Box(Pos(x, y), Size(w, h))` with by-value
  parameters.
- **A record pushed by value whose trailing words come from a fresh
  zero register** while the leading ones use the function's zero
  register has a member with its own constructor behind plain
  words.
- **The image calls a member your build expands**: the frame is
  larger by the expansion's temporaries and hundreds of lines
  differ. Mark the member `noinline` before reading anything else.
- **A member loaded behind the construction of a temporary
  argument** (constant, `lea tmp`, `push`, stores, then the member
  load) is read through an inline getter; the plain member
  expression is loaded first and every scratch register after it
  rotates.
- **Sibling blocks share stack slots, expansions of one inline do
  too**: six objects in two branches at three slot sets are three
  named locals per branch, not an inline builder called six times.
- **A virtual call on `this` in a constructor is devirtualised**;
  a `call [vtable+n]` there went through a pointer local.

### Ports that never had the image's structure: rank by the mnemonic sequence, then rewrite from the listing

After many rounds of register work the remaining ports fall into
two populations that a line diff cannot tell apart (a frame of
another size moves every `[esp+N]`). Diff the MNEMONIC sequences
alone (first token of each instruction, registers, offsets and
constants dropped) and divide by the image's length:

- **a ratio near or above 1, no x87, few mentions in the notes** is
  a port written from a prose description: caches the image does not
  have, early returns, hand-expanded helpers. Nobody tried register
  forms on it because no form was close. Read the image's listing
  and write the function again in its block order; these match in a
  handful of compiles (five of five tried in one round).
- **a ratio below 0.3** is register residue; leave it to the idiom
  lists.

What the rewrites needed, in the order they paid:

- **Read members through the object at every use** where the image
  reloads them (`panel->menus[i]->x` five times, `buffer[i]` in every
  arm). A cached local is the description-port's habit.
- **One `return` and the parameter in memory**: arms that end
  `mov eax,[param]; pop; ret` are `break`s in front of one
  `return c;`; an early `return c;` in a first `if` block puts the
  parameter in a callee-saved register for the whole function. Write
  `if (...) {...} else switch (c) {...} return c;`.
- **A load placed in FRONT of the previous statement's store** (the
  image `mov ecx,[this+0x28]; mov [this+0x30],1`, yours the store
  first): the stored field is reached through a cast or a reference
  to a cast (`int &cursor = *(int *)(pad + 4)`), which the compiler
  must assume aliases the load. Declare the fields as real members.
- **A switch laid out `cmp 2; jle B; cmp 4; jne out; A; B`** is
  `switch { case 4: A; break; case 1: case 2: B; break; }`: the
  blocks are in source order, the tests are not.
- **Dead zero stores of a member's default constructor that survive
  in front of the real values** (`mov [eax+4],0; mov [eax],0; mov
  [eax],esi`): the values arrive by an aggregate assignment from a
  NAMED record (`caja = k;`), which the dead-store pass does not see
  through; member-wise assignments are cleaned up.
- **By-value records pushed with `mov [esp+N],esp` in front** have a
  constructor and a copy constructor; without the copy constructor
  the temporaries are built in the frame.
- **The box from a position and a size**: by value, by const
  reference, or by value with a named copy of the position give
  three different load orders. Try all three before anything else.
- **`push ebp; mov ebp,esp` in an /O2 function with `dec ecx` loops**
  is an inline `__asm` block of the original inside a C function;
  it is not a C loop to be matched.
- **Switch arms that jump into one shared call tail** (`mov ecx,ebx;
  jmp tail` in some arms, a second entry that loads other arguments
  and falls into the same pushes): the arms only assign variables and
  the call is written once behind the switch (`default: goto fin;`).
  A call per arm with a `return` never merges that way.
- **Nested inline members and the width of their parameters**: a
  16-bit value widened once (`movzx edi,cx`) and reused by two inlined
  bit operations means the inner inlines take `unsigned`, the outer
  one the 16-bit type. The same width at both levels redoes the
  widening and changes which value is spilled.
- **An element's field reached as `lea r,[i*8]` plus a later `add
  base,r`** is an inline MEMBER of the element (`tabla[i].Tiene(v)`),
  not a field access spelled out at the call site.
- **VC8 does not unswitch loops**: an invariant flag tested once in
  front of two loop copies is two loops in the source. `do { ...; n--;
  ... } while (n)` behind `if (n)` keeps the count's register; `while
  (n--)` moves it.
- **A search loop with `xor eax,eax; push eax` as the 'previous'
  argument and `mov eax,ebp` at the bottom** has two variables (the
  previous one and the found one), not one reused.
- **A dead `lea eax,[...]` behind each `add`/`and` on a member** is an
  inline getter that returns a reference; `x = x + 1` through it keeps
  the immediate where `x++` hoists the constant 1 into a register.
- **A frame 4 bytes larger than the image's with every instruction
  equal** can be a `new` temporary that shares an address-taken
  local's slot in the image: record it and move on if blocks and
  inlines do not give it.
- **The same short store at the bottom of every `switch` arm, the
  default jumping into the last arm's copy** is ONE statement behind
  the switch (`switch (m) { case ...: p = &a; break; ... } *p = 0;`):
  VC8 copies a short tail into each arm. A port that guards it (`if
  (p)`) differs from the image, which writes through whatever the
  default left in the pointer: check the callers' values and record it.
- **One slot for every point and box of a painter** is one
  function-level record set again for each call, not a temporary per
  call (those get a slot per type); a position that is loaded whole and
  then adjusted in registers is a by-value copy with `+=`/`-=`, not a
  constructor from two sums (that evaluates the second sum first).
- **A post-increment inside an index** (`p[y++ * w + x]`) leaves a
  register copy in front of the multiply; the increment as its own
  statement does not.
- **A variants harness pays off quickly**: a script that takes named
  `(old, new)` text replacements, builds and diffs each and restores
  the source lets one try six spellings per compile round.
- **Time-box each port** (six compiles or variant batches; stop when
  the diff has not halved after three): a round that opens twelve ports
  this way matches more than one that spends itself on a single
  register question. Save the try, restore the source, move on.
- **Compare frame sizes across all unmatched ports**: a frame larger
  than the image's by a multiple of a class's size means named locals
  where the image has temporaries (`a.Get("K").CopyTo(buf)`, an
  address-of-temporary argument); a smaller one means a block-scoped
  local that the image keeps at function level.
- **The same null-checked load twice in a row for one object** is an
  inline getter of a typed object called twice through `this->member`,
  not a helper taking a cached pointer.
- **A member reloaded between a store and the next load of the same
  element** means no element pointer local: index through the base at
  every use (`base[r].v[i].x`). The opposite, a pointer held in a
  register over several calls where yours reloads the member, is an
  inline helper taking that pointer as a parameter.
- **A pair returned by value and copied through the returned pointer**
  is an assignment to the member from the call (inline `operator=`),
  not a named local copied field by field.
- **Check for patched bytes** (`nop`s between two `cmp`s, a `jmp` where
  a conditional jump belongs) before working on a port: a patched
  image function cannot be matched by any source.
- `[vc8]` **Explicit returns are shared, jumps to one exit are copied.**
  The same `{ log(this, "..."); return 0; }` written at many places
  comes out as `push str; jmp TAIL` everywhere but one; the copy that
  stays is the function's last return block in source order (the first
  case's in a `switch` whose every case ends in `x = 1; return;`), so
  the END of the function must be the image's (a nested success path
  with the failure returns behind it). A `break` to one statement
  behind a `switch`, a nested `if/else` pyramid without `return`, and a
  `goto error` are all copied into every arm (the base call and the
  epilogue once per arm). Scan: count `ret` instructions per function,
  image against yours; more in the image = the source jumped to one
  exit, fewer = it wrote separate returns. **held** *(match, six
  functions)*.
- `[vc8]` **A result variable in a stack slot, reloaded at every exit**
  is `int r = 1;` with nested ifs and one `return r;` in a function
  whose registers are all taken; early returns fold it into constants.
  The same code behind two arms with each arm keeping another half in
  line is that code written in both arms with opposite `if` polarity.
- **A masked diff hides jump distances.** An empty or table-only diff
  with different lengths is a short jump against a near one (another
  epilogue chosen for an early `return`): read the raw byte compare. A
  `case A: f(); case B: return 0;` fall-through makes B jump into A's
  tail.
- **An `ebp` frame in a function of a frame-pointer-free build** means
  an inline assembly block of the original inside a C function (a loop
  with `dec ecx` where the compiler writes `sub ecx,1`): it cannot be
  matched from C alone.
- `[vc8]` **An unrotated search with the success exit before the exhaustion
  exit** is `int i = 0; int r = 0; while (!r) { if (i >= n) break; if (hit(i))
  return 1; else i++; } return 0;`: `while (!r)` keeps the head test and the
  backward jump, `break` and the explicit `return 1` put the two constant exits
  in the image's order. Assigning the predicate to `r` and returning it folds
  the exits into the wrong shape. **held** *(match, two functions)*.
- `[vc8]` **A count guard that is one `test; jbe` with no second loop-entry
  guard** is `unsigned i = 0; if (i < cuenta) { do { ...; i++; } while (i <
  cuenta); }` with the count member re-read at every use (`if (cuenta)` gives
  `je`, and a `for` inside it adds a second guard). The signed twin, `int i = 0;
  if (i < n) { do { ... } while (i < n); }`, keeps one signed `jle` entry guard
  (84 -> 4 lines on an unmatched port). **held** *(match)*.
- `[vc8]` **In an unrolled search the frame slot can be a FOUND FLAG, not the
  counter**: `int hallado = 0; for (unsigned n = 0; n < 16 && !hallado; n++) {
  if (occupied) hallado = 1; else { c = c + 1; c &= 0xf; } }`, with the type and
  the cell address re-formed at every use (a cached base merges the increment and
  the mask, the image keeps them as separate memory operations). **held**
  *(match)*.
- `[vc8]` **`if (a && x + 1 < v) x++; else if (c) y++;` swaps the roles of the
  constant 1 and the count; the goto form with the incremented value in a local
  gives the image**: `if (a) { unsigned n = x + 1; if (n < v) { x = n; goto fin;
  } } if (c) y++; fin:`. **held** *(match)*.
- `[vc8]` **A two-state `if`/`else if` whose image tests the LATER case first is
  a `switch` with its cases in the image's block order** (case 3 before case 2):
  look at the image's block order, not the source's natural one, for every
  `if (fase == K) ... else if (fase == L)` chain. Pair it with an explicit local
  for a member that is loaded and tested (`int ocupado = c->ocupado; if
  (ocupado)` gives `mov; test`, where `if (c->ocupado)` gives `cmp [m],0`).
  **held** *(match)*.
- `[vc8]` **A two-sided test whose image keeps a redundant SECOND mode test**
  (`test mode; ...; test mode`): read both members directly at every use, no
  locals: `if ((!carp && !(a & 0x10)) || (carp && (a & 0x10)))`. A local copy of
  the mode lets the compiler delete the second test. **held** *(match)*.
- `[vc8]` **Write the member store from the source read, not through a named
  temporary**: `y = s->f4; x = b; z = 0; w = 0;` puts the pointer load before
  the by-value argument load and the `xor` between the two stores, where `int t
  = s->f4; x = b; y = t;` loads the argument first. **held** *(match, two
  names)*.
- `[vc8]` **A pointer local to an array defeats strength reduction**: `for (i =
  0; i < l->cuenta; i++) if (l->v[i] == x)` walks a pointer (`add edx,8; cmp
  [edx],edi`), the image indexes (`cmp [edx+ecx*4+8],edi`). `int *v = l->v;`
  before the loop and `v[i]` in it gives the indexed form. **held** *(match)*.
- `[vc8]` **`volatile` on the re-read use means "load again here"**, and is a
  legitimate LAST step to a match when it reads the same member as before:
  (a) the second read of an array element, `Notar(((volatile int *)r[1])[i])`
  (the image reads the element twice; volatile on the first read or on the
  pointer is worse); (b) an inline getter through `T * volatile` plus a local
  receiver gives the null test, the member read again and the receiver kept in
  `eax` behind the pushes; (c) `Cambiar(*(T * volatile *)&cuerpo)` loads the last
  pushed pointer fresh in `eax` where the plain read picks `edx`; (d) an
  argument's second use `indice = *(volatile int *)&nombre;` selects the
  argument's stack home instead of a retained register; (e) a member read into a
  local, `int e = *(volatile int *)&espera;`, selects the other scratch register
  without adding a load. Rules that kept it honest: keep the volatile only in a
  function that ends up MATCHED (it never helped a port that stayed unmatched, in
  many tries), with a one-line comment, and never where it changes what
  is read. **held** *(match, six functions)*.
- `[vc8]` **A result variable that lives in memory with a zero register held
  through the whole function**: `volatile int ok = 1;` gives the `xor ebp,ebp` at
  entry that a plain `int ok` folds away (`ok = 0` once before the cleanup arm,
  `return 0` inside the `if (e)` cleanup, `return ok` after it). Zeroing
  constructors on a local derived struct (`struct N : T { N() { a = 0; b = 0; }
  }` and `new N`) give the `test eax; je; mov [eax+4],0; mov [eax],0` shape; a
  success test written `int fin; if (e && (fin = ok))` is what puts `this` in
  `ebx`; the called member must be `__declspec(noinline)`. **partial** *(79 ->
  16 lines)*.
- `[vc8]` **Three floats copied as THREE INTEGER words**: where the image copies
  a point through `mov`, not `fld/fstp`, cast both sides to a three-int record
  (`*(V3i *)v = *(V3i *)p`, `struct V3i { int x, y, z; }`). A local float record
  filled in x/y/z order and then copied to a float global through the int record
  gives the local `fld/fstp` and the global's integer loads and stores; a zeroed
  local copied the same way gives the image's stack temporary plus three int
  stores. A memberwise-copied second record restores a copy the plain assignment
  drops. **held** *(match, two functions; 57 -> 8, 79 -> 27 on others)*.
- `[vc8]` **A named pointer to an embedded record can be REQUIRED, and it is a
  raw-offset cast**: `S *s = (S *)((char *)actor + 0x194); s->animando = 1;` gives
  `mov eax,[this+actor]; add eax,0x194`, where the typed member
  `&actor->suscrito` folds into a direct `+0x1a0` store and the schedule of the
  whole function moves; the same cast inside an inline accessor of a derived view
  (`struct A : Base { float Angulo() { return ((O *)((char *)this + 0x44))->
  Angulo(); } };`) gives the repeated `mov eax,[actor]; lea ecx,[eax+0x44]`. When
  a typing pass would remove the cast, keep it in functions that match.
  Conversely, `*(int *)((char *)actor + 0x194)` and `+ 0x19c` can hide ONE
  embedded object: `struct L { N *primero; int f4, f8; }; L *l = (L *)((char
  *)actor + 0x194);` gave `x = l->primero; if (x) x->f8 = 1; l->f8 = 1;` and the
  whole function's register roles (21 -> 0). **held** *(match, three functions)*.
- `[vc8]` **An owner accessor plus a WHOLE by-value addition inline** gives the
  repeated `lea` receiver and the float rounding store after a call: unique inline
  methods on a derived actor view for the read and the write, and `static
  __forceinline float Sumar(float a, float b) { return a + b; }` between them:
  `mov eax,[actor]; lea ecx,[eax+0x44]; call; fstp [t]; fld [t]; mov eax,[actor];
  fadd; ... lea ecx,[eax+0x44]`. Direct raw casts give `add ecx,0x44` and no
  spill; splitting the assignment from the addition is optimised away. It is NOT a
  generic x87 lever: it did nothing for three other float ports. In the same
  function test and re-read a member directly (`if (*(T **)(...)) (*(T **)(...))
  ->Orden(...)`, no local) to get the image's virtual-call sequence. **held**
  *(match)*.
- `[vc8]` **A by-value result used through a reference uses the hidden result
  pointer** (`Par &q = REF_TEMPORAL(Par, camara->Proyectar(p));` gives `add
  reg,[eax]` / `add reg,[eax+4]` instead of reloading both words from the named
  result's slot); direct receivers (`conexion.colas->Cerrar()` twice) can be
  closer than a retained pair pointer. **partial** *(192 -> 87, 55 -> 21)*.
- `[vc8]` **A float intermediate used only to CHOOSE AN ARM is a `double`**:
  `double medio = h * 0.5 + base;` keeps the midpoint on the x87 stack as the
  image does; the `float` cast adds an `fstp/fld` pair and displaces the next
  locals. In the dot product that follows, naming the FIRST product as a `double`
  (`double dx = (x - p0x) * tx; float s = (float)(dx + (y - p0y) * ty);`)
  restores the image's x-before-y evaluation and stack depth. Either alone is not
  a match. Look for it where the first differing hunk is an `fstp dword; fld
  dword` pair the image does not have. **held** *(match)*.
- `[vc8]` **Float details that moved x87 code**: a `float` FIRST factor gives
  `fild; fimul; fild; fidiv` where explicit `double` casts on every factor give
  `fmulp/fdivp`; `(float)nivel`, not `(double)nivel`, gives the dword 2^32
  correction of an unsigned-to-float conversion and `(unsigned)((double)x * f)`
  removes the conversion-helper call; `abs(int)` needs `<stdlib.h>` and gives `cdq;
  xor; sub`; plain float arithmetic with a float local for a squared length and
  in-place divides beat `long double` casts (34 -> 11); the values of a
  frame-less function live in the DEAD HOMES of pointer arguments, so write
  `*(float *)&pa = ...` on every path including the null defaults, and compare
  through those addresses (`pa > pb`, not `pb < pa`, gives `fcompp; test ah,5; jp`).
  **held** *(match, two functions; partial on the rest)*.
- `[vc8]` **A constant compared in an equivalent spelling can change the
  register allocation of the whole function**: `if (nivel <= 2) return;` against
  `if (nivel < 3) return` changes `cmp eax,3; jl` into `cmp eax,2; jle` and fixes
  the saved registers of the prologue (33 -> 13 lines on an unmatched port); try
  both spellings of every compare against a small constant. **partial**.
- `[vc8]` **A box built into a local needs a reference to one member**: `Caja c;
  int &cy = c.y; c.x = x; cy = y; c.ancho = ancho; c.alto = alto; return
  c.Dentro(px, py);` constrains which operand VC8 picks as the sum's destination
  (`add y,alto`, not `add alto,y`). **Two truncations whose first result must stay
  in a register** while the second runs are written through a two-int array
  first (`int p[2]; p[0] = (int)..; p[1] = (int)..; int x = p[0], y = p[1];`), the
  rectangle filled afterwards. **held** *(match, two functions)*.
- `[vc8]` **A three-short ARRAY adds a /GS cookie a three-field short RECORD
  does not** (`struct { short a, b, c; }` with a zeroing constructor restores the
  image's frame); a `char` array in a class view is a /GS buffer and creates a
  cookie too (pad views with `int`). **held**.
- `[vc8]` **Frame slots are not steerable by scopes or declaration splitting**:
  two ports kept their exact instruction stream with reversed slots under scoped,
  plain-record, default- and direct-constructor, address-taken, split-declaration
  and array forms. Stop at the second such form.
- `[vc8]` **Two EH states with no code between them** are the same inline
  setter called twice; **an unwind state count that differs** is a scan of its
  own (`ehscan`: compare the image's `maxState` with the compiler's).
- **Tooling trap: a signature-changing variant prints a false zero** when the old
  mangled symbol disappears, because the differ finds nothing to compare:
  rebuild and compare under the NEW mangled name before trusting it.
- **Compile-identical, do not retry**: `(float)(float)sin`, an implicit
  double-to-float, inline float wrappers for sin/cos, `float` against `(double)`
  arithmetic in a rotation, reordering `n &= ~2; hechos = 0; n |= 1`, `f - i`
  against `f - (float)i`, the `e |= f()` forms (goto, switch, inverted if), a
  named point temporary against the plain assignment, `bool` for a flag that is a
  stack slot, a cached `n = count` before a loop whose image re-reads it.
- **What the loop learned about yield** (rounds 225-241: eleven Kimi and six Sonnet
  rounds, about 330 ports opened, 21 matched addresses): a round matches 0-4
  ports; breadth alone did not pay (a round with 38 big ports matched none, the
  next, sorted ascending by image size, matched 3); the productive rounds opened
  SMALL ports (image up to ~150 instructions) whose first differing hunk was one
  of the idioms above and spent up to 12 compiles on a port already within 15
  lines; grep this file for the instruction pattern of the first hunk before
  rewriting; ports whose image reads an UNSET slot cannot match under a
  deterministic-zero rule, and ports with patched bytes or an inline assembly
  block cannot match from C at all.
- `[vc8]` **Name an indexed embedded record only for the loads that need it**:
  `E *e = (E *)((char *)this + 0xd38) + i; casa = e->casa; ficha = e->ficha;` for
  the initial loads, with the later `ENT[i].x` accesses left direct, gives the
  image's register allocation; using the pointer everywhere adds register pressure
  and removes `ebx`. **held** *(match; the cast is again the matching form)*.
- `[vc8]` **Different `switch` arms can need different copy forms for the same
  struct**: in a colour setter the red arm needs a by-value inline helper that
  copies the first two shorts as one record and stores the third separately
  (`*(Par *)d = *(Par *)&t; d->b = t.b;`), while the blue arm must keep the plain
  full assignment to get `mov ax,0x64` and the right store schedule. Do not unify
  the arms. **held** *(match)*.
- `[vc8]` **By-value inline parameters are not a general way to get dead
  argument-home temporaries** (tried on two float ports: identical code to the
  plain form); the dead-home layout comes from writing the values through
  `*(float *)&arg` (see above). **failed**.
- `[vc8]` **Write an x87 bound comparison with the value on the LEFT when the
  image loads it first**: `if (x < xmin) xmin = x; if (x > xmax) xmax = x;` gives
  `fld [x]; fld [xmin]; fcompp` with the image's `test ah,0x41` / `test ah,5`;
  `if (xmin > x)` reverses the loads. Direct FLOAT member operands in a
  projection, `(xmin - f48) * f50 + f4c`, give `fld [bound]; fld st0; fsub [f48];
  fmul [f50]; fadd [f4c]`, where cached locals or explicit `double` factors give
  stack arithmetic. **partial** *(169 -> 74 lines on a port with a different
  frame)*.
- `[vc8]` **A two-int record keeps dimensions that scalar locals rematerialise**:
  fill `d.y`, load the screen, then fill `d.x` through an explicit `int *t =
  &obj->ancho` (`add eax,0x20`, early height load, retained width); owner
  accessors and the record took 60 -> 12 lines. **partial**.
- `[vc8]` **Repeated zero stores select a retained zero register**: two guarded
  assignments that repeat an inline constructor's `canal = 0; pausado = 0` make VC8
  keep zero in `ebp` for all tests and stores (19 -> 6 lines); one repeat is not
  enough, and a constructor-local zero or explicit null tests fold back. A
  `volatile` SECOND load of a list slot (`T * volatile c = *p`) restored a whole
  loop allocation (103 -> 60) on a port that stayed unmatched, so it was not kept.
  **partial**.
- `[vc8]` **Ports to exclude at once** (uninitialised reads, found three times in
  one round): a table slot read before any write, a loop over uninitialised stack
  slots, and extrema read on the zero-count path. A deterministic-zero rule cannot
  match them; do not spend compiles.
- `[vc8]` **An owner local before `atan2` plus an inline owner orientation
  accessor** restores the retained-owner `edi` and the `lea` addressing of a
  vehicle-heading routine (45 -> 23 lines); a discarded float comparison is kept
  only when its flag takes part in an UNROTATED loop condition, and walking both
  pointers (not an index) took one loop 79 -> 32. **partial**.
- `[vc8]` **`Vista v = {0};` on a 72-byte local still emits a `memset` call**; the
  image's unrolled float/word stores come from an inline constructor with explicit
  member assignments (174 -> 139 on a port with a different frame). **partial**.
- `[vc8]` **A signed pointer-parameter inline changes the frame by one slot** (0x74
  -> 0x68) but the unsigned spellings that restore the right `fistp` restore the
  larger frame too: the frame and the conversion are coupled, look for a third form
  instead of choosing between them. An unwind table entry that points at a member
  destructor (`+0x588` list, data at `+0x58c`) names the member's real type even
  when the wrapper forms do not reproduce the third state. **partial**.
- `[vc8]` **An explicit inline copy constructor on an embedded base-like record
  moves the zero initialisation of the next embedded member into the image's
  schedule** (`struct R { int w, h; R() { w = 1; h = 1; } R(int a, int b) {...}
  R(const R &o) { w = o.w; h = o.h; } };`, 0x6327f0 `Rect::Rect`): the implicit
  copy lets the next member's zeroing float away from the copy. **held** *(match)*.
- `[vc8]` **A FOLDED pointer tautology inside a real condition keeps an address
  computation ahead of the branch without emitting a pointer test**:
  `if (bpp == 8 && (c || !c))` holds the palette address computed before the
  `bpp` test where `if (bpp == 8)` computes it inside; `&& c` emits an unwanted
  `test`. Try it where the image takes an address (a `lea`) before a mode test and
  ours takes it inside. **held** *(match)*.
- `[vc8]` **A three-float local with an UNUSED third member reproduces a frame
  word the image reserves** (a 12-byte slot whose third float is never read): a
  frame size short by 4 is a missing member, not a missing local. Pure float matrix
  expressions (no `double` intermediates) keep the shared x87 vector across two
  coordinate conversions. **partial** *(19 lines, 81 on a projection stream)*.
- `[vc8]` **Pass the image's own `extern float` defaults BY POINTER through a
  double conditional** (`static inline double F(Block &v, const float *d) { float r
  = (float)(!v.Cuenta() ? (double)*d : (double)(float)v.Numero()); return r; }`
  called with `&K_6ebd90`): the default arm keeps `fld DWORD`, and the join keeps
  its store/reload; a literal default becomes `fld QWORD`, a by-value global is
  loaded too early, a plain float ternary loses the join store. A zero default is
  its own helper with `0.0`. Declare the constants `extern const float` (the values
  come from the image, not the source). **held** *(match)*.
- `[vc8]` **When an unrelated change flips an x87 addition, a free per-axis inline
  add with a pointer destination restores the first component's order**
  (`p->x = p->x + o.x; p->y = o.y + p->y; p->z = o.z + p->z;`): write each
  component's operand order as the image loads it, one helper for all three, and
  the other components do not move. **held** *(match)*.
- `[vc8]` **One variable for the zero failure result AND the best element**
  (`mejor` in a nearest-segment search that returns null on failure): using one
  variable for both corrected the count/best register allocation, and combining the
  null and count guards then fixed the failure-exit order. **held** *(match)*.
- `[vc8]` **An outer guard plus ONE shared exit label retains a result variable and
  removes duplicated epilogues**: `if (n < 10) { ...; goto fin; } fin: return ok;`
  with every arm of the `switch` jumping to `fin` (a `goto fin` in each failure
  arm), where early `return ok` statements copy the epilogue. **held** *(match)*.
- `[vc8]` **A dead-`this` member call explains a redundant `mov ecx,esi`**: a helper
  that never reads `this` but is called with the receiver in `ecx` is a MEMBER of
  the class (declare it in the class, and give the helper's address a same-address
  member alias in the symbol table); a free function leaves the `mov` out.
  **held** *(match)*.
- `[vc8]` **A NAMED volatile reload on a member's second use can repair a whole
  arm's register allocation, not just force a load**: `int c = *(volatile int
  *)&g_recursos->c8;` (a resource member read a second time after a call) and
  `T *id = *(T * volatile *)r;` (the second use of a record's id) each closed a
  function; plain locals and inline getters did not. Same rules as before: keep it
  only in the function that ends up matched, one-line comment, same member read.
  **held** *(match, two functions)*.
- `[vc8]` **Frame-slot reuse for a dead argument**: access the argument through a
  reference (`float &tx = *(float *)&a;`) while the other two coordinates live in
  a two-float record, and name the first product of the squared length `double`
  (`double dx2 = (double)t.z * t.z; float cuadrado = (float)(dx2 + (double)tx *
  tx);`): the dead home takes the third coordinate and the comparison keeps its
  shape. **held** *(match)*.
- `[vc8]` **An iterator difference computed only on the non-sentinel path with a
  comma assignment** keeps the branch order and fixes the scratch rotation of a
  list walk (58 -> 22 lines on a port that stayed unmatched). **partial**.
- `[vc8]` **A named REFERENCE to a pointer member schedules its receiver load
  inside a by-value float copy**: `BuscadorPuntoAPunto *&b = buscador; return
  b->BuscarDesde(destino, modo, 0);` where a plain pointer local, an accessor or a
  helper did not reproduce the image's load position. **held** *(match)*.
- `[vc8]` **A named reference to an OUTPUT POINTER argument keeps the stores in
  source order**: `int *&p = o; p[1] = y; p[0] = x; p[2] = w; p[3] = h;` where
  direct stores, a whole-record copy, and even a plain pointer local are reordered
  by the scheduler. Try it where the image stores a record's words in a non-memory
  order. **held** *(match)*.
- `[vc8]` **Notes from ten restored near misses**: a width local must be
  address-taken so `fild` reads its stack home, and a colour sum needs its operands
  commuted (0x557430, 8 lines); an iterator whose 12-byte home the image reserves
  but never uses cannot be reproduced by pointer or reference access (16 lines);
  epilogue gotos and argument references were inert on branch-byte-only residues.
- `[vc8]` **Per-arm local colour records copied WHOLE reproduce the image's stack
  temporaries and its dword-plus-word copies** (a marker loop that builds a colour
  in each `switch` arm); address-taken scalar float/int temporaries kept both
  conversions and restored a function's frame and arithmetic, where equivalent
  two-field records overshot the frame; the image rounds both source floats before
  the conversion and uses float products. **partial** *(173 -> 114 lines)*.
- `[vc8]` **Named references were inert** across the global, handle, list, count,
  text, engine and track schedules tried in one round (18 ports, no match): the
  reference-to-pointer idiom above is not a general lever. Before spending compiles,
  scan the PARKED ports after every typing or header change (`all-markers`): none
  became identical on its own.
- `[vc8]` **A row-copy loop with a contiguous and a strided arm** (a blit of an
  image into a surface): write the NON-contiguous arm first so the contiguous arm
  lands last; compute the derived stride (`bytes = paso * 2`) only inside the arm
  that uses it; decrement the row count BEFORE the copy call when the image does
  (`do { filas--; Copiar(d, ancho); d += bytes; } while (filas);`, not `while
  (--filas)`); and declaration order (destination, width, row count) fixed the whole
  `ebx`/`ebp`/`edi` allocation. **held** *(match)*.
- `[vc8]` **An explicit memberwise float copy constructor on a by-value point**
  turns VC8's integer-word copy into the image's x87 copy (124 -> 36 lines on a
  port that stayed unmatched); the retained local that stole the `this` register
  was the next obstacle, and an explicit actor local reached 32. The opposite of the
  three-integer-word idiom above: read the image to see which one it does.
  **partial**.
- `[vc8]` **A round of 19 mid-sized ports (50-600 diff lines) matched nothing and
  found no new idiom**: yield comes from ports already within ~15 lines, not from
  breadth (see the yield note above). Useful negatives: a three-float record
  restores a frame and a value-left comparison fixes an x87 mask, but borrowing the
  dead argument home for a float disturbs the surrounding allocation; moving or
  assigning a flag inside a guard loses its required stack home and disrupts the
  EH state allocation; shrinking text buffers improves the score but is a behaviour
  change (reject it); a named table pointer is necessary for an image's early table
  load and vptr preservation (a direct receiver is worse, 32 -> 42).
- `[vc8]` **A pointer member compared IN MEMORY and reloaded for EACH store**
  (`cmp [this+rect],0; je; mov eax,[this+rect]; mov [eax+..],..; mov eax,[this+rect];
  mov [eax+..],..`): keep the plain guard and read through `T * volatile` at each
  store: `if (rect) { (*(Rect * volatile *)&rect)->ancho = ancho; (*(Rect *
  volatile *)&rect)->alto = alto; }`. This preserves the memory compare and both
  reloads; each access reads the same member. The same volatile-on-the-re-read rule
  as above (matched function only, one-line comment). **held** *(match)*.
- `[vc8]` **Yield note**: the round that told the matcher to open ONLY ports within
  ~15 diff lines (and 12 compiles each) matched one of 15 after the breadth round
  matched none of 19; the 2-line leads (a `push` placement, a constant's placement,
  a null test before a virtual `delete`) are the residues that no idiom here has
  moved yet.
- `[vc8]` **Compile-identical or inert, do not retry** (round of 19 near-miss
  ports, no match): inline OR helpers fold a known-zero `or edi,eax` into `mov`;
  unsigned-byte locals and inline restore helpers do not force a `movzx`;
  `volatile` null-path and member accesses reproduce single instructions but disturb
  the surrounding layout; a constructor body that stores `c4` before `c0` brings a
  copy constructor to 2 lines but no temporary or volatile anchor puts an integer
  load inside an x87 `fld`/`fstp` pair.
- **Tooling trap, again: a signature-changing variant (`QAEI@Z` -> `QAEXI@Z`)
  printed zero twice** because the differ could not find the old mangled name; two
  "wins" were false zeros. Always rebuild and diff under the NEW mangled name from the
  object (`nm`) before believing a zero.
- `[vc8]` **Three iterator slots of a list walk** are the standard
  `RecorridoLista<T> { x, cur, beg }` record with a SEPARATE list local (a
  16-byte formatting buffer beside it); an inline prefix that returns its result
  can keep the allocation while moving a constant between branches; named zero
  temporaries and typed destination references each change zero-store scheduling
  and shrink-wrapped saves; interleaved member copies can coalesce source and
  destination frame slots and remove intended integer-word copies; a volatile
  argument read preserves a comparison but may give a register compare where the
  image has a memory compare. **partial** *(no match in the round; the 2-line
  residues, a `push` placement, a constant's placement and a reset/save order,
  survived every form)*.
- `[vc8]` **`const_cast<int &>(ok)` on a cleanup read gives the image's `mov
  eax,ebp`** (the zero register returned as the result) but leaves the scratch
  rotation of the rest of the function; an early declaration plus a typed
  zero-destination reference brought one port to a single missing global reload (the
  second load is common-subexpression-eliminated, and volatile, alias, inline-getter
  and reassignment forms restore the load but delay a `push`); a local second operand
  plus a forced-inline second `memcmp` improves a comparison but not its inlined
  byte-load order. **partial**.
- **Where the small-port method stands** (rounds 254-256, 43 near-miss ports, no
  match): the surviving residues are one-line differences (a SIB encoding order, a
  `movzx` against `mov al`, a CSE'd second global load, a `push` placement). They are
  the class no source form in this file has moved; further rounds on the same ports
  are repeats. Stop and switch the effort (a different compiler flag set, an
  unmatched-port typing pass, the structural ports) when three consecutive rounds
  find nothing.
- `[vc8]` **BIGGER PORTS PAY WHEN THE BUDGET MATCHES THEIR SIZE** (round 257: four
  ports of 20-200 diff lines, up to 40 compile batches each; two matched, 98 -> 0 and
  124 -> 0, after three barren rounds of 12-compile near-miss attempts): fix the FIRST
  differing hunk at each step, the prologue/frame and register-role hunks first (a
  `push` or frame mismatch decides every later register), keep the new best source in a
  file and continue from it. The small-port loop stops finding things; the large-port
  loop finds idioms that the small residues never expose.
- `[vc8]` **Direct reads of a GLOBAL pointer give different register roles than a named
  local**: `g_p->x` at each use (not `T *p = g_p;`) fixed a display/background-width
  allocation, and combined with the right independent-statement order repaired the
  frame, prologue and query scheduling of a 90-instruction function. **held** *(match,
  two functions)*.
- `[vc8]` **An inline accessor for an EMBEDDED receiver keeps a late global load and
  changes the literal-zero argument from `push ecx` to `push 0`** (the zero no longer
  reuses `ecx`): `inline List *Rayos() { return &rayos; }`. **held** *(match)*.
- `[vc8]` **Naming the first float addend `double` reverses an x87 operand order
  without adding a store** (`b.z` loaded before the sum), the same shape as the
  midpoint idiom above. In place normalisation of a turn value and reuse of a dead
  argument home repaired every later frame slot of a heading routine (23 -> 5 lines)
  but not the initial squared-length x87 order. **held** *(match)* / **partial**.
- `[vc8]` **Two changes that only work TOGETHER** (a heading routine, 5 lines -> 0):
  reversed named `double` operands give the image's load-both-then-square x87
  sequence, and reusing the DEAD argument home for the next temporary
  (`*(volatile float *)&atras = (float)(giro + 180.0);`, a volatile STORE into the
  dead home) repairs every shifted frame slot. Either one alone leaves the diff where it
  was; when a hunk resists, try the pair of a value-order change and a slot change.
  **held** *(match)*.
- `[vc8]` **A six-float local record declared in IMAGE OFFSET ORDER restores a frame**
  (a bounding-box routine: 74 -> 6 lines), with four-int conversion and destination
  records restoring `ebp`, the conversion order and the rectangle stores; in a
  two-pointer comparison loop, the matrix/view declaration order and `v++; q++; i++;`
  select the matrix-pointer walk and the pointer-difference addressing (32 -> 6).
  **partial**.
- `[vc8]` **A port whose image reads an uninitialised iterator cursor** (the null path
  skips the cursor/start initialisation of a three-slot iterator) is excluded by the
  zero-initialisation rule; check it before spending the 40 compiles.
- `[vc8]` **Walk a TYPED fixed-size element directly as `e`, not through a second
  byte-pointer cursor** (a bounding-box loop over 0x58-byte elements: `struct E {
  char c00[4]; float x, y; char c0c[0x4c]; }; E *e = ...; do { ...; e++; } while
  (n)`): the typed `e` restores the early element-base load and the loop addressing;
  a typed `e` PLUS a separate `q` cursor was not enough. Combined with the six-float
  local record in offset order and the four-int conversion/destination records, this
  closed a 74-line port in three compile batches after the two prior rounds had
  left it at 6. **held** *(match; the layout of the element is declared, so check its
  size against the image's stride)*.
- `[vc8]` **Partial findings from a 111 -> 66 line port**: copying `p` before reusing
  its argument home for `d2` repairs a float home; integer-word position copies, an
  actor-position pointer and a guarded pointer scan (`if (i < n)`, the unsigned
  zero-index guard, not `if (n)`) improved the same port; an inline iterator `Count()`
  fixed a loop-foot reload order (20 -> 15). **partial**.
- `[vc8]` **A receiver formed DIRECTLY in each `switch` case, not retained across the
  switch**: in cases that all call a method on the same computed receiver (a burst
  object), write the receiver expression in each case instead of one local before the
  switch; and copy a vector through a NAMED pointer to a member sub-record
  (`T *a = actor + 2;`-style), which restores the load/test schedule of an aim copy.
  **held** *(match)*.
- `[vc8]` **Float trig overloads (`cos(float)`, `sin(float)`) restore the missing
  call-result stores** of a dot/cross-product routine but add a frame word of their own;
  pair them with removing the padding word and restoring the copy to the record base
  (a 42-line port). Naming the first product of each dot/cross expression `double` is
  the documented lever for the remaining x87 stack order. **partial**.
- `[vc8]` **A frame that is 32 bytes larger than the image's means temporary
  script-block (or other object) slots**, not register noise: the image shares ONE
  temporary between the first read and a loop's value read where ours allocates a
  third; explicit blocks, direct owner reads and a shared inline read helper did not
  remove the extra slot (an editor placement routine). **partial**.
- `[vc8]` **Negatives from a bigger-port round that matched nothing** (12 ports, closest
  4 bytes and 6 lines): reusing a slot with a `double` old value fixes ratio homes but
  disturbs the dimensions and a maximum join; nested capacity guards fold identically to
  the conjunction; loop `goto`/`for`/`while` variants do not remove an extra join; an
  arm-local shared constant (`int uno = 1`) is folded away instead of producing the
  image's retained register; volatile probes broke an already matched neighbour (re-run
  `make match` after every volatile try). A sweep of all parked markers after the round
  found none already identical. Bigger-port rounds so far (257-261): 5 matches in 5
  rounds, so the barren round is part of the rate, not a signal to stop.
- `[vc8]` **A `switch` whose source order follows STATIC GUARD BITS, not the emitted block
  order** (a handler with three cases, `0x53c350`): write the cases in the order 2, 0, 1
  (the order the image's dispatch tests the bits), with six function-scope position
  records, the `strcmp` intrinsic, and an integer-word tail vector copy; together they
  repaired the frame and the global register allocation. Check the image's compare chain
  first: its test order, not the block order, is the source order. **held** *(match)*. For a
  JUMP-TABLE switch the block order IS the source order, and the EH state numbers of the
  static guards give it directly (the last subsection of this section).
- `[vc8]` **A duplicated 16-bit colour can need TWO STORES**: `dst = Pack(c) << 16; dst
  += Pack(c);` preserves the recomputation and the retained high word; `dst = Pack(c) *
  0x10001` lets VC8 CSE the expansion into one `imul`. Matched a ten-argument colour
  setter in one compile. **held** *(match)*.
- `[vc8]` **Notes on a 706 -> 144 line wave-front port (`0x489430`, unmatched)**: direct
  member table access, incrementing the wave at the loop top, and separate current/new
  front counts were all necessary together for its closest form; plain `inline` stays out
  of line and an `__forceinline` expansion over-unrolls (`0x610e40`); the intrinsic
  `fabs` took one 361-line port to 346. **partial**.
- **A matched source can still be wrong in the NATIVE build: never copy adjacent SEPARATE
  locals through the address of the first** (`float x, y, z; ...; *(V3i *)dst = *(V3i *)&x;`).
  VC8 laid the three floats out contiguously and in order, so the bytes matched; clang
  orders and spaces the locals differently, so the native build copied garbage (soldiers
  at y = -1, 3,582 sightings against 296). Declare ONE record local (`struct { float x, y, z;
  } p;`) and cast that. The class members of a declared layout are safe; locals are not.
  The matcher cannot see it (the VC8 bytes are identical): after a match that rewrites a
  copy as integer words, rerun a behaviour log of the function's callers in both builds
  (call log of a hot callee, names and state, hybrid against native).
- **A key built from a name must truncate where the image truncates**: a NUL stored into a
  stack buffer right before the call (`mov byte ptr [esp+..],0` after a `sprintf`) is a
  `buf[4] = 0;` and it decides the result of the callee (a four-character tag): a
  byte-matched callee returned 0 for the six-character name, and every creature of a mission
  got key 0. Read the instructions AROUND a call to a matched function, not only the callee.
- **After changing a static callee, recheck the PARKED SIBLINGS that call it**: a port that
  was one hunk off because its callee's convention or body differed can become identical
  with no edit of its own (`Hoja5ab7e0` after the callees of `CrearObjeto5ac1a0` were
  rewritten: the marker was all that was left). Run the all-markers comparison (every
  `~` name compared as if matched) after each match that touches shared helpers.
- `[vc8]` **A SIB operand order (`[ebx+eax+4]` against `[eax+ebx+4]`) follows which element
  is a TYPED pointer**: form ONLY the element used for the first access as a typed pointer
  (`LineaMuerte *e = (LineaMuerte *)lineas + k;`) and keep the scaled byte-offset
  expression (`(char *)lineas + off`, `off` built by three doublings of `k + k * 4`) for the
  later access; typing both gives the wrong register order and an array index gives
  another. A loop whose index must stay in a register is `int i = 0; if (i < n) { do { ... i++; }
  while (i < n); }`. `k` set in the loop and read only after `if (!hallada) return;` is not an
  uninitialised read. **held** *(match, after 21 batches on six 1-2 line residues)*.
- `[vc8]` **Decouple a `memcmp` call's operands from its guard's**: with both operands read
  from members, `if (o->tabla2 && tabla2) r = memcmp(o->tabla2, tabla2, 0x200)` moves a
  load ahead of the first null jump or reverses the byte-load order of the inlined
  comparison; a LOCAL for the second operand only (`unsigned char *a2 = tabla2; if (o->tabla2 &&
  a2) r = memcmp(o->tabla2, a2, 0x200);`, the guard and the first operand read directly
  from the member) keeps both the branch load schedule and the intrinsic's byte-load
  order (a round 234 note recorded each half separately). **held** *(match)*.
- **A masked instruction diff can be a FALSE ZERO**: `0x536f50` reached instruction-for-
  instruction equality with one branch-target displacement still different (call targets
  and displacements are masked); `make match` / the strict comparison is authoritative.
  Name a double for one product of an x87 dot product (`double t = x; t *= factor;`) and the
  second product is fixed (6 -> 4 lines); compound and inline dot-product forms did not fix
  the first product's operand order. **partial**.
- `[vc8]` **An outer-loop termination flag may have to appear in the loop CONDITION**:
  `for (; !hallado && i < cuenta; i++)` where a flag set inside and tested after the loop
  otherwise lets VC8 delete the flag and its frame slot. Empty and successful early returns
  become one shared return through an outer input guard and a nested failure arm. A by-value
  point passed as `*(Punto3DCopia *)p` (a record copy of an existing record) can remove a
  named argument temporary and its frame space; two wall expressions written in the image's
  order (`t` before `a3`) improved an x87 layout. Inert: unlink helpers with reference or
  predicate results, reset-and-difference helpers, comma-result cell stores. **partial**
  *(174 -> 44 and 133 -> 46 lines on two ports)*.
- `[vc8]` **Idioms that took a wall-sweep port from 46 to 9 lines**: a queue of a fixed
  number of entries (`0x3ff`) plus a CACHED count reproduces the frame and offsets; recompute
  the second arm's `d*d` where the first arm's is a named value (it avoids an unwanted x87
  CSE); mirror the first squared-distance comparison to restore the product order; write
  a range test as `!(d >= r)` for the image's ordered-compare mask (`jnp`); an inline helper
  taking `this` and reading `actor` internally reproduces both by-value vector arguments.
  The last residues were the order of the initialisation stores, which of the outer and the
  inner loops is the ALIGNED block (a `do` loop with a single count test equalised the
  instruction count but moved the constant reloads), and one `jnp`/`jne`. **partial**.
- `[vc8]` **The DECLARATION ORDER of independent locals decides the initialisation-store
  order AND which loop is the aligned block**: four independent locals (`int i = 0; int
  cuenta; Zona *lista[0]; int hallado = 0;`) declared in the image's order closed the last two
  residues of a 9-line port (4 batches). For a residue that is a store order or a block
  alignment, PERMUTE the declaration order of the independent locals SYSTEMATICALLY (all
  orders of up to five; one batch each) before trying any other form. For an image `test
  ah,0x41; jne` the direct ordered `sum < r*r` gives `jne`, `!(sum >= r*r)` gives `jnp`
  (and they differ for NaN: the compiled image's behaviour is the reference). **held**
  *(match)*.
- **Declaration-order permutations are INERT on most small residues** (round 275: all
  orders of the independent locals of twelve 1-4 line ports, 102 batches, no match): the
  lever worked on a port whose residue was the initialisation-store order and a loop's
  alignment (`0x439510`); a residue that is a branch target, a `movzx`, a returned-box
  store schedule or a block layout does not move with it. Named key and line locals made
  a two-branch-byte port much worse; an early state initialisation worsened another.
  A residue unmoved after the permutations, the typed-element, volatile-per-use and
  named-temporary forms, and several rounds is a candidate to leave. **failed**.
- `[vc8]` **A volatile read of the image's own float constant keeps it on the x87 stack across
  two expressions**: `double c = *(volatile double *)&g_6dcdb8;` (the 0.5 constant at its
  address, declared `extern double`) shared by both formation calculations, instead of the
  literal `0.5` in each (the literal is folded into two separate loads); `c += f64;` for
  the second use. **held** *(match; the volatile is in the matched function only)*.
- `[vc8]` **An inline null-tested store on an EMBEDDED owner field preserves a store order**:
  `((AvisoCampo *)((char *)this + 0xb8))->Hecho();` with `void Hecho() { if (aviso) aviso->hecho
  = 1; }` where the plain `if (aviso) aviso->hecho = 1;` on the member reorders the store
  against its neighbours. The same family as the named pointer to an embedded record.
  **held** *(match)*. Strict comparison note: a function with a jump table can report
  `d3` lines that are table DATA, not code; `make match` is authoritative.
- `[vc8]` **A VOLATILE read of an ARGUMENT into one local shared by every call keeps it live
  from function entry** (a window procedure whose image loads `l` into `ecx` at the entry
  and reuses it for `Crear`, `Comando` and `DefWindowProcA`): `int lv = *(volatile int *)&l;`
  and `lv` at every use, in place of `l` and of a per-arm `VistaVentana *v = this` local. The
  value is the same; the volatile is in the matched function only, with one comment.
  Plain copies of the argument fold away; a volatile copy of a LOCAL (`b`) on a port whose
  residue was a register-role swap (`b=ebx, ok=ebp`) made it worse. **held** *(match, one
  batch)*.
- **The shared volatile argument did NOT generalise** (round 278, five residues whose image
  keeps an argument in a register from entry: 2 -> 32, 4 -> 18, 2 -> 99 lines, two inert): it
  matched a window procedure whose argument reaches three calls; check the image prologue for
  the argument loaded once at the top and reused across SEVERAL calls before trying it.
  `bool ok` on a register-role swap recovered one role but brought byte storage, `setne` and
  another swap (59 -> 38 lines, not kept). Constructor initialisers, switch-base subtraction,
  one-word records and by-value helpers were inert on the others. **failed**.
- **A SURVEY of every parked port by real diff lines (round 279, 627 bodies)** shows where the
  work is: after the 1-60 line ports were either matched or tried exhaustively, NONE untried
  was under 60 lines; the remaining pool is 300+ line functions (best fresh results in 30
  batches: 364 -> 120 and 392 -> 242). Build the survey once (compare every `~` name as if
  matched, count REAL differing lines excluding jump-table data and masked noise, sort) and
  re-run it after a round that changes shared helpers; it replaces the hand-kept lists of
  near misses. Useful on the big ones: pair-wise countdown zero loops, a float projection as
  a parameter, a direct existing by-value point record, direct owner reloads, a guarded `do`
  loop to stop VC8 peeling a second copy of a `for (;;)`. **partial**.
- `[vc8]` **A 364-line port reached 14 lines in 90 batches over three rounds** (a floor arc,
  `0x63c580`): fan-point RECORDS (one record per fan, not loose floats), a two-int projection
  result copied with an intrinsic `memcpy`, an inline centre-copy helper, and the order of the
  first fan's angle and pointer updates; owner receivers owned by inline NOTIFICATION
  helpers improved another (242 -> 208); restricting owner reloads to the exact instructions
  the image shows (41 lines); an UNNAMED script-block temporary plus a tile pointer or
  reference live across the iterations (43 -> 38). Big ports keep paying with budgets of
  30-60 batches per round, three rounds in a row, each starting from the saved best.
  **partial**.
- **A port that stopped moving**: `0x63c580` stayed at 14 lines through 60 further batches
  (pointer/store schedules and two exchanged stack homes) and `0x48daa0` at 208 through 30 (all
  variants byte-inert: result slot reused by a float temporary, early `ok`, scoped `a`):
  stop after about 30 batches without a changed line count and leave the saved best for a
  later idea. Assigning an index BEFORE the `-1` test fixed one schedule (38 -> 36).
  **partial**.
- `[vc8]` **Closest point on a segment (a 66-line port, 21 batches)**: load the segment's `p1` into
  a local BEFORE `p0`, compute `ex` before `ey`, keep ONE shared result variable (`int resultado
  = 0; if (l2 > 0.0f) { resultado = 1; ... } return resultado;`, not two returns), write the
  clamp's upper test as `t > 1.0f` (not `1.0f < t`), and name the y numerator `double ny`
  before adding the x term: the dot-product order and the success-result schedule follow.
  Also on three other mid-size ports: a name routine called with `ecx = this` in the image
  is a MEMBER (the free `__stdcall` declaration gave the wrong frame: 93 -> 75 -> 32 -> 19
  lines), and a direct `tabla[ruta].puntos` access cut a route search by 40 lines.
  **held** *(match)* / **partial**.
- `[vc8]` **A script reader (104 lines, 31 batches): when the instruction STREAM is already the
  image's and the diff is script-TEMPORARY SLOT assignments, change the SCOPES**: scoping the
  position block (`vPos`) through the final visibility decision (an extra `{ }`) repaired
  nearly every shared slot (104 -> 2); the last two lines were a `mov ecx, esi` before the
  wrong store: raw-address ONLY the embedded `visible` store
  (`*(int *)((char *)this + 0x144) = 1;`) and write the next store through a REFERENCE to the
  embedded object (`ClassOrientador &po = orientador; po.activo = 1;`). Also: evaluating
  `s07()` before the point stores took one port 63 -> 32; ordered near/far clamps, an
  intrinsic origin `memcpy` and one shared trigonometric temporary took another 99 -> 69.
  **held** *(match)* / **partial**.
- **PARALLEL AGENTS (round of 2026-11-23, three Opus agents + Kimi, four matches in about two
  hours)**: give each agent its OWN git worktree (a sibling directory: the VC8 toolchain is
  found relatively), its OWN single function from a survey, and no right to commit or to edit
  the shared docs; verify each result in an integration worktree (apply the agents' patches
  there, rebuild any object copied from a tree that had work in progress, `make match`
  must show baseline + the matches and the same name set) before the main tree. Pitfalls met:
  two agents given the same function (the lists overlapped: keep ONE source of truth for the
  assignments), objects copied from a tree with an experiment in progress (one stale object
  looked like a regression), a symbols.tsv line that changes with a signature (the old name
  looks lost, the new one gained).
- `[vc8]` **A callback that the image calls with its point as ONE class by value** (user copy
  constructor: 0x6125a0): declare the callback type and the callback with the same by-value
  class so no function-pointer cast is needed; `float, float, float` parameters change the
  frame and a plain record makes VC8 use a temporary. Under the MSVC ABI of the native build
  the three dwords sit on the stack after the other arguments, the caller pops, and the callee
  reads them in place (check the asm). Several float products stay exact when two terms of ONE
  addition are swapped. **held** *(match)*.
- `[vc8]` **A colour-mix loop (0x575ae0)**: reuse ONE pixel variable (`int p = *a; ...; p = *b;`)
  or VC8 folds the first pixel's single-use products into the final expression; the statement
  order picks the registers (1 of 720 orders); pointer increments in the `for` header give the
  shared `mov esi,4`. A polyline (0x5eeba0): copy order sets operand order across the whole
  function (x,y,z 33 lines, y,x,z 14, an inline stores helper 8). A ternary stored to a float
  gives one `fstp`; `unsigned rad` stops a spill; `a = 1 - a;` gives `neg; add 1`.
  **held** *(three matches)*.
- **Ports whose image holds inline `__asm` (an ebp frame, instructions VC8 does not
  generate from C) cannot be matched under the asm rule; check the image prologue and the
  source for `__asm` BEFORE spending a batch** (six particle painters of 0x4869f0..0x4878b0 were
  in the survey's eligible pool). **failed**.
- **Round of 2026-11-23, continued (24 more matches by Opus agents and Kimi; the gates)**: (1) A BYTE-IDENTICAL function can
  still break the native run: round 287's `Construir` matched with `memcpy(&ox, origen, 0xc)` (a 12-byte copy through the
  address of ONE float member: UB) plus float math overloads and passed `make match`, but 11 of 35 lockstep scenarios diverged;
  reverting ANY ONE of its five changes passed (codegen-dependent). `make match` is necessary, the native smoke is the gate:
  run it on every integrated batch, bisect by file then by hunk (a work tree + `NOBUILD=1` reruns take ~2 minutes), fix
  forward. (2) `#include <math.h>` makes the float overloads visible to every function in the file: declare ONE local
  `inline float sqrt(float)` instead. (3) Parallel agents: keep ONE assignment list (two agents matched the same functions);
  worktrees must be reset to master after each integration; patches from different worktrees that touch the same
  file merge with `git apply` (different functions), and symbols.tsv hunks are applied BY CONTENT (the index mismatch refuses
  the second patch). (4) Idioms: `d++; p++;` at the loop body's end with only `i++` in the header; two identical defaults are
  merged where the LATER one stands (`goto` a label at the end); `__restrict` on a record-member source copied into a local
  array lets the loads run ahead; `int x;` with no initialiser when it is only read after the assignment that guards it; a
  non-rotated read loop `do { r = f(); if (!r) break; ... } while (r);`; the masked instruction diff HIDES jump-table targets
  (confirm with `make match`/`firstdiff.py`); a switch lowered to a compare tree ignores the case-label order. **held**
  *(six matches)*.

### The compiler's own bookkeeping decides the residues: the node counter, the double rounding, same-file callees `[vc8]`

Rounds B28-F3 (about 90 more matches, 12507 -> 12570 identical) ended the era of blind variants: the cheap rules were exhausted (six agents in a row found 0-1 matches) and what paid next was **micro-experiments on the compiler** plus a few structural classes. **held** *(Fable rounds F1-F3: 7 matches and 6 proven rules; Opus rounds B28-B53: ~55 matches)*.

- **The x87 operand order of a commutative `reg op member` is set by a counter, not by the spelling.** The front end numbers every DISTINCT expression node as it reads the function (the same counter that names the `tvN` temporaries in the `/Fa` listing; identical expressions share a number). A memory operand that already occurred earlier has key `N & 3` (N = the counter at its first occurrence); a register, parameter or plain local has key 0; the larger key is loaded first; ties between memory operands go to the higher displacement; the spelling order never matters. Costs: store through a pointer 2, plain copy from `P->f` 3, direct global/parameter/local 0, each int operator or compare 1, float sum stored to a float 2, unary or cast 1, `if (x) return;` 1, named or dead locals 0. **Zero-code units to pay with**: the outermost `(float)(a+b)` on an expression stored to a float anyway (+1; a cast on an INNER product adds a rounding pair), a pointer alias for one access (+1). Method: put fillers of cost 1, 2, 3 before a site's first occurrence, read off the residue that gives the image's order, solve the small mod-4 system across sites. The law predicted three experiments before they ran; a site whose operand is first seen at the site must be probed, not assumed.
- **The double float rounding (`fstp [s]; fld [s]` twice through one slot) is a product by the DOUBLE constant 1.0**: `r = (float)(r * 1.0)` (also `/ 1.0`, `1.0 * r`): VC8 folds the product away but keeps its rounding store. `* 1.0f`, a static const, casts through a named double, `+ 0.0` do not produce it. **A plain float MEMBER load rounded through the temporary (`fld [m]; fstp [T]; fld [T]`) is a by-value float parameter of an inline function that its body uses 2-3 times, called with a member argument** (one use folds away).
- **Frame slot order and reload order are two rules**: the reload order at a loop exit is the reverse of the variables' first-reference order; the slot order of spilled counters follows the number of MEMORY references to the slot (more references, lower address; a tie to the later-referenced variable); register uses and `if` nesting do not count.
- **Reuse one variable for the second angle** (it stays in the home the image uses; a fresh local takes the other dead parameter's home); **read the just-stored members** in the next product (VC8 forwards the stored value and takes the member as the memory operand); **parameter-home reuse follows which parameter's last use comes first**: a home the image never reuses means the original pinned that parameter, no inline reference form reproduces it.
- **A callee defined earlier in the same file is visible to the original's compiler**: signs in the image: a value kept in a register across a call that received an object's address, address-taken locals sharing slots by liveness, a global loaded once across several calls: put a scratch copy of the callee's body above the caller (4 matches of 20 ports no ledger had mentioned: 60, 93, 160 and 169 lines). The opposite also exists: a body in the same file gets inlined or known to leave ecx alone (`Zzme::Zzme` had to move to its own file).
- **Wrong shared declarations are worth more than one port**: a by-value 12-byte record with a user copy constructor passed by value (2 pushes of 3 words), a stdcall-looking free function the image calls with ecx set (a thiscall member even if its body ignores `this`), `mov eax,ecx` at entry with eax untouched until `ret` (returns `this`), a countdown `jns` zero loop then `return this` (a CONSTRUCTOR parked under a wrong name), a missing symbol boundary in symbols.tsv (an unlisted function inside another's diff), a missing alias for an inline constructor reached through a thunk.
- **Loops**: a `for` with a `break` unrolls only when the break block is empty (`if (!(r = cond)) break;`); the bound must be a LOCAL copy (a member of an address-taken local is never unrolled); a counting loop with named operands; the rotation `do { if (!n--) break; ...; } while (cond);` for a top-tested count; a zeroing constructor gives the countdown zero loop.
- **Small rules that held**: `__restrict` on a block-local receiver whose call has arguments with calls or loads (also on non-virtual calls); a named int local inside the arm that converts it for `fild` (the store lands after the branch); `hecho = 1;` in both arms; a post-increment in the condition; a reused dead accumulator; two `return` statements place the shared exit; an inline helper holding the call and the whole tail expanded in both arms merges them; a global used by one file only becomes a file-static (the compiler then knows a pointer store cannot alias it: also remove its row from the data-names table and regenerate the native declarations).
- **Negative results worth keeping**: one internal form for a whole 12-byte record copy (every spelling gives the same bytes; only member-wise copies change it); an equality test propagates the constant to the compared variable and to the variable it was copied FROM, not to a variable copied from it; VC8 deletes every dead zero; the `delete` expression instead of `operator delete` did nothing on five ports; brute-force substitution sweeps over 26 small ports found nothing; when removing a statement fixes a register rotation, the cause is a hidden register count INSIDE that statement, not an earlier arm.
- **Process**: the diff tool cut OUR listing at its last `ret` (blocks placed after the return counted as image-only lines): ports looked 5-30x bigger than they were; the productive class was ports no earlier ledger mentioned; a matched-but-moved function needs the file move kept (git add of a staged deletion fails the whole commit: check the index); one Fable agent doing micro-experiments found more rules in three rounds than the six variant-trying Opus agents before it.

### The frame cell allocator, variable identity and the remaining x87 rules (rounds F4-F10, K1-K2) `[vc8]`

Ten Fable rounds and two Kimi rounds after the node-counter section above (12,570 -> 12,584 identical in this stretch: F4 3, F5 7, F6 1, F7 2, F10 1 matches, plus Kimi's volatile removals); the rules came from micro-experiments, each proven on a tiny function compiled with the harness flags before it was applied. **held** *(14 matches; rules 7-20)*.

- **A product `elem * k` on a LOCAL array or record is decided by how the element was FIRST occurred**, not by the node counter: first occurrence direct (`v[1] = 0`, `v.y = e`) makes it the memory operand (`fld st0; fmul [e]`); first occurrence through a pointer (`q[1] = 0` with `q = v`, an inline setter/constructor/`operator[]`) loads it first. The node-counter law governs seen memory operands of globals and `this`/pointer members, not stack operands. The doubled rounding after a root is a DOUBLE LOCAL holding the float-rounded result cast at the use (`double s = (float)sqrt((double)k); ... (float)s`), a second route to the `* 1.0` form. **Unused x/y parameter homes mean ONE by-value two-float record parameter** (an old idiom, re-read against stuck families it closed three more functions).
- **A pool constant declared as a non-const extern is not a literal to the optimizer**: a call or a store through a pointer kills it and VC8 recomputes the product; the literal written at both uses pins the angle and saves the product; an extern float load may alias neighbouring int stores and is scheduled after them while a literal's `fld` is hoisted above (when the only diff is where a `fld ds:K` sits among stores, read K's bytes: a round decimal means a literal). **A float the image keeps in a dead parameter home is not a variable**: write the expression at each use. An int by-value parameter of an inline used more than once keeps its `$T` (the `fild` temporary is stored inside the branch); `fld [spill]; fxch; fstp [T]; fld [T]; fcompp` means the left operand is written in the compare and the right is an inline's double result cast there; an 8-byte frame with only `[esp+4]` referenced is a two-float local of which one element is written; passing the INTEGER and writing `(float)n` at each inline-operator call gives the image's fresh rounding copies where a float parameter forwards the value.
- **Variable identity: VC8 treats a re-assigned existing variable and a fresh single-definition local as different objects.** Update both variables IN PLACE (`f -= (float)i; i %= 360;`: a new single-definition local is forwarded); one result variable joined by `goto fin` (an early exit then jumps into the shared tail with the value on the x87 stack); a pointer variable that changes role in place (`p = ((T *)p)->pos;` instead of `int *pos = s->pos;`); a loop value assigned to a DEAD parameter instead of a new local. When a port differs from the image only in a register role with identical instructions, assign the value to a variable that is dead there. Measure with branch targets kept: a masked diff can show a false zero.
- **THE FRAME CELL ALLOCATOR (measured model)**: objects are every stack symbol of the final code (named locals, `$T`, `tvN` behave alike). Sort by (size ascending, reference count descending, last reference ascending): the count is the number of final-code instructions naming the symbol, loops add no weight, a tie goes to the earlier last reference. Place each object in the most recently created cell whose occupants are all dead while it is live, else create a new cell above; a cell grows to its largest occupant. Lifetimes: scalars by true liveness with holes; records written member by member are one hull; address-taken at function scope live the whole function and never share; address-taken in a block live from first reference to the block end; 'address-taken' is a front-end property and survives inlining. The frame is the cells in creation order, lowest first, `__$EHRec$` on top. Counts and lifetimes are in IL order, before the scheduler. Parameter homes (a dead parameter's home takes 4-byte objects in some functions and never in others) follow rules P1-P3 of the last subsection. Block-scoped address-taken records share their slot unless the function's EH frame holds only static-initialisation guards (then the record stays on top); an out-of-line destructor state turns sharing on and moves the record to the bottom. A frame-size difference in a large port is therefore a question about which objects the original had: two roles in ONE object (`B = g; p->Avanzar(B, e)`), a record where we have scalars, a temporary we fold.
- **Method lessons**: rewriting a large port from the image's listing driven by the frame slot map beats greedy inlining (0x61bd30 247 -> 6 lines against 115); a source transformer found nothing under 135 lines (the near ports already had the levers); a reaching-definitions scan of the image finds ports whose original read an uninitialised local (a rounding temporary's slot read at a join), but a hit is not enough: run the diagnostic compile that leaves the else path unwritten; the volatile-of-the-same-type debts can be replaced by legal forms once the underlying rule is known (the `sqrtf` header with an outer parenthesis pair: `return ((float)sqrt((double)x));` keeps a separate return temporary where the unparenthesised form coalesces it; plain locals after the double-rounding rules); a whole double-valued conditional cast `(int)(t > 100.0 ? 100.0 : t)` converts on both arms where `? 100 : (int)t` folds the constant arm; repeating the destination access `(*out)->F()` reloads the receiver where a named local keeps it in a register.

### Visibility, frame sharing and source shapes (rounds K3-K22, B54-B109, G1-G3, D2-D4: about 53 matches, 12,590 -> 12,655 identical) `[vc8]`

The rules below came from micro-experiments compiled with the harness flags (`/O2 /Gy /GR /EHsc`) and were then applied to full ports. **held** *(matches by address as named)* unless marked **partial** (improved a port, did not close it) or **no effect**.

**Callee visibility (what the original's compiler could see)**

- **Intra-TU interprocedural Mod/Ref: a callee defined ABOVE the caller in the same translation unit is analysed even when it is not inlined (`__declspec(noinline)` too).** Symptoms: an out-parameter local that the callee only writes has no store before the call (`ta = 0.0f` absent); two address-taken locals share one stack cell (frame 0x38 where ours is 0x3c); registers the callee does not touch stay live across the call; a value kept in a register across a call that received an object's address. Source: put the callee's real definition above the caller in the same file (`micro/vis_modref.cpp`, `uninit_share.cpp`: 0x43a460). **held.** Extends the 'callee defined earlier' entry of the F-rounds section: the effect reaches the frame allocator and EH metadata, not only registers.
- **A visible out-of-line OUTPUT WRITER lets a `new`-expression cleanup temporary share the cell of the address-taken scalar it writes** (frame 4 bytes smaller). Symptom: our frame is exactly 4 bytes larger, the body has an address-taken output scalar passed to a callee plus a `new`. Source: define the writer (`__declspec(noinline)` is enough) above the caller (`micro/new_visible.cpp`: 0x59f9d0 `Juego`, 0x49d2b0). **held**; the sign predicted 1 of 4 candidates. **Boundary: for an address-taken RECORD the sharing is opposite** (an external reader declaration shares the 12-byte cell, a visible no-inline reader does not: `micro/visible_record.cpp`); do not apply the rule blindly.
- **A visible `noinline` constructor changes the caller's value webs without being inlined** (a recomputed `lea` becomes the image's `mov eax,ecx`; 0x61e820, needs the DEFINITION, a declaration is inert); visible versions of other callees there were inert, so it is not a general rule. **partial.**
- **Deliver the caller, not the callee, when the callee inlines something from its own file** (a callee's own match fixes which file is its TU: 0x49bb70). **Only THUNKED (incremental-link) callees in the same TU are a signal**; a direct `call` with no thunk does not mean 'same TU' where whole families are called directly. Tried and **no effect** on ten ports (the callee stub above the caller left the diff unchanged; in one it worsened, so the original did NOT see it).
- **`__declspec(noalias)` on PURE address-taking helpers** (they write only their own out data: a projector, a polar-to-cartesian) lets VC8 keep values in registers across the call (`p.y += dy - h` a register add, not a memory RMW) and pack disjoint-branch locals into one slot (frame 0x24 -> 0x1c, 157 -> 35 lines). Mark only helpers verified pure. **held** *(0x50cb00 and three more; later 0x5107f0 32 -> 4 by `noalias` on an output-writing `Medir`, which also restored the QWORD-spill sharing, and 0x49bd10 9 -> 0)*. The annotation works on a file-local MEMBER declaration (a view of the callee in the port's own file) as well as in a header: without it `&h` escapes to the first call and a value computed from `h` goes through `eax` before reaching its callee-saved register, and no variable-identity form repairs that (0x49bd10). It also lets VC8 reuse constructor temporaries: reads of `c.x`/`c.y` after `Angulo(&c)` compile as reads of the temporaries that built `c` only when the callee is `noalias` (0x4dcdc0: 497 -> 95 inside a chain). A blanket sweep (`noalias` tried on every callee that takes `&local`, and file-static on every extern scalar used by one file) found nothing beyond these: the lever is spent once the verified-pure helpers are marked.
- **`noalias` composes across ALL the pure callees that see a local's address, and its parameter must be a pointer.** Marking one of two callees that take `&v` does nothing (24 lines before, 24 after); marking BOTH proves `&v` never escapes and PackStack slot swaps in unrelated branches snap (0x4ed460: `Largo` reads x,y, `Polar` writes x,y). The same holds for consecutive calls that keep a value in a register: a value survives two calls only when both are `noalias` (0x61e820: each alone 22 lines, both 4). A reduced micro did not reproduce the pair, so treat it as a full-port composition fact. **The noalias parameter must be a POINTER type**: passing `(int)&local` to an int-typed noalias callee DELETES the stores to `local`; type it `const void *` and give the call direct arguments (0x480cc0: frame 0x74 -> 0x68). The annotation only affects the VC8 build (a native clang build ignores it). **held** *(0x4ed460, 0x61e820, 0x480cc0)*.
- **A named block-scoped local needs `noalias` to share a slot; a sequential ANONYMOUS temporary does not.** `item.Valor("A").CopiarA(..)` twice shares one cell with no annotation; `{ Bloque v = ...; v.CopiarA(..); }` needs `noalias` on the callees, and enabling it for one function broke another in the SAME translation unit (-32 bytes): prefer the anonymous temporary. **held** *(0x5acc30)*.
- **File-local `static` globals whose address is never taken do not alias `p->field`; an `extern` global may.** The extern one is a read/write scheduling barrier; with `static int g_a, g_b;` VC8 hoists `cmp [esi+0x14],ebp` above the two global zero stores (0x645020). `__restrict` and caching `l->cuenta` in a local were worse. **held.**
- **A file-local `static` function whose address is not taken gets a custom calling convention chosen by PARAMETER ORDER**: up to three parameters travel in registers in declaration order, so put the stack parameter (`const char *nombre`) FIRST and the register parameters after it (`item, padre, juego` take esi/ebx/edi without saving ebx); with the stack parameter last it is passed in `ecx` and another goes on the stack (0x5aca80; also the buffer size `0x80` against `0x78` decided `sub esp,0xe8`). **held.**
- **Intra-TU Mod/Ref on BOTH read-only callees folds a temporary and removes its destructor.** With `Texto()` AND `Copiar()` defined above the caller, a freshly built temporary's fields (`o.cuenta > 1`, `o.simple`) constant-fold across the calls and its destructor is eliminated (138 instructions -> the image's 113); moving only one callee above the caller did nothing. **held** *(0x5afcb0)*.
- **Equality knowledge is not always used by the image.** The image re-tests a value it knows from an earlier `==` branch, or keeps `or edi,eax` after `if (!e)`, where VC8 SP1, RTM and every form (switch, const-ref, volatile, `/O1 /Os /Ox`) fold: **no effect**, put such ports on the unreachable list. The same family seen from the other side ('fold against home-slot reload'): VC8 propagates a constant through a guard where the image re-reads the parameter's home slot (0x5ffca0, 0x536f50, 0x5d9370); it would take a 'this call may clobber the caller's locals' annotation, the opposite of `noalias`, and MSVC has none short of `volatile`. Duplicating the shared `return r` to get the image's edge split folds to `setcc` (VC8 value-propagates `r == 1`).

**Frame cells and the stack packer**

- **`PackStack` orders locals high to low by size class (12, 8, 4 bytes) and, inside a class, by loop-weighted reference count.** Symptom: two interfering floats or ints in the adjacent halves of an 8-byte slot (`b@-28, c@-24`), or our frame one cell larger with identical instructions. Source: replace the two scalars by an 8-byte POD (`struct Par2D { float x, y; }`) used in the second loop / by the by-value inline; the slots snap to the image (`micro/pack_62df90_2.cpp`, 0x62df90; `pack_55f580.cpp`: `sub esp,0x24 -> 0x20`, 0x55f580). A by-value `Par(nm.x, a).Largo()` loads `y` first where an inline `sqrt(x*x + a*a)` canonicalises the other way. **held** *(two matches)*.
- **The parameter-home rule.** VC8 gives a parameter's home slot to later locals only if the parameter is NOT first used inside a conditional region (`if (d) {...}`, an early `if (!d) return`). Copy the parameters into locals BEFORE the guard (`Par pb(b); Par pa(a); float ang = w;` generate no code). Symptom: identical instructions, our frame larger, the image's floats or 8-byte records at parameter offsets. **held** *(0x43c110, micro m1-m4)*. **The parameter-USE identity (mixed parameter path):** copy into a local before the guard but spell ONE use through the ORIGINAL parameter (the image reads the home slot there and a copy elsewhere); each use's identity is part of the match, so sweep the uses one at a time (0x61e820: 62 -> 44 -> 26 lines). **partial.** It does not apply when the guard is an equality test (0x536f50).
- **A value the image keeps in a dead parameter's home needs no pun**: a local whose address is taken lands there.
- **Read the cells in the listing.** `/Fa` lists every frame cell (`tvNN`, `_x$N`); an unused 4-byte cell is the `strlen` intrinsic's temporary and is shared by arrays declared in a nested block after the `if (strlen(t))` test (0x4ca9c0); two locals at one offset mean sibling blocks (0x64ce30); blocks destroyed after the last statement are function-scope locals with their own cells. **held.**
- **Inline block-local records and the following `new` temporaries share one cell only when each record is built by an INLINE HELPER THAT RETURNS THE DESTINATION BY VALUE, in a function whose EH map has an out-of-line destructor state** (`micro/temp_color.cpp` F0-F3, D3-D5: function-scope records, reused records and direct sibling blocks do not give the layout). **partial** *(0x603da0 94 -> 36 lines)*. A real block-local out-of-line destructor enables sharing but adds an EH entry the image may lack (`new_slot.cpp`).
- **Frame-cell order is steerable by variable identity**: `p.x = f[3] + o.x` as an in-place update changes the reference counts and swaps the cells, as the allocator model predicts (`record_cells.cpp`). **held** *(as a micro result)*.
- **A UNION of an int pair and the record that views it** (`union { struct { int ancho, alto; }; Tam tam; };`) replaces an address-cast pun and gives the same slot (0x61bd30). Anonymous unions in shared headers can create false layout conflicts between includers: check the layout tool.

- **Separate an input scalar from an address-taken out-parameter**: `int x = r->x;` and `int cx; F(..., &cx, ...);` keep `&cx` in the dead parameter home while `x`'s live range stops outranking `this` for `edi`; coupling them (`int cx = r->x; int x = cx;`) swaps this/x and the operand order. Safe only when the callee writes `*px0` unconditionally. **held** *(0x501000 family)*.
- **`~T() throw()` plus `operator delete[](void *) throw()` removes dead `mov [EHRec],-1`**: when nothing after the second call can throw, VC8 drops the state store and the `or esi,-1` setup, and two EH-unwound objects share one slot (frame 0x20 not 0x40, EH table byte-identical; 0x5afe60). **The EH state COUNT is part of the match**: a function whose 808 code bytes were identical still differed in `__ehfuncinfo` by one byte (`maxState` 7 against 30); compare `maxState` and the unwind table, not only the code. **held.**
- **A two-float RECORD assignment and `char name[N] = ""`** (K20): assigning a derived two-float record spills both floats and copies the words with integer `mov`s (the image's temporary and exact frame; scalar member stores emit two `fstp`); `char name[0x20] = ""` is a byte, seven unaligned dword stores, a word and a byte, NOT `memset` (eight aligned dword stores). An unread buffer is optimised away: give it a use in the micro. **held** *(0x5ab9e0, improved)*.
- **An early subobject pointer prevents shrink-wrapping and a member-pointer store controls store placement** (K20): `Bloque *pb = &b;` before a dispatch saves `edi` on EVERY exit, direct access saves it only in the arm that needs it; `int *w = &w3c; *w = 0;` places the store after the float-record copies where `w3c = 0` places it before. **held** *(0x48cf10)*.
- **Real `new[]`, `memset` and `delete[]` instead of hand-written construction**: with only SCALAR `operator new`/`delete` declared and a nontrivial element constructor/destructor, a real array expression emits the scalar allocator, the overflow/cookie handling and the `vector constructor/destructor iterator` helper calls (0x6143c0 644 -> 445). A hand-written loop is the wrong shape. **held** *(improved)*.

**Scratch-register phase, scheduling and block shape**

- **A post-switch deferred-action flag** (`int fin = 0; ... case 4: fin = 1; ... } if (fin) Terminar();`) controls BOTH the tail-jump target AND the `eax/ecx/edx` rotation phase: VC8 emits the cases after `case 4` before advancing the rotation for case 4's virtual `Terminar()`, tail-duplicates it into `case 4` and merges the upper cases' failure jumps into that copy; it also shrink-wraps the `push edi` past the bound-check `ja` (`micro/switch_fin.cpp`: 0x430830, 0x430e00). **held.** Case + default places the default first; a post-switch `if/else` places the case body first (`switch_default.cpp`). The register-rotation clauses are in `vc8-regalloc.md`.
- **A constructor taking `const Par &` bound directly to an sret-returned 8-byte struct reads through `[eax]`**; `Par p = f()` reads the caller's `[esp+N]` (`sret_eax.cpp`). **held** *(0x50b740 partial)*.
- **Whole-record assignment for integer `mov` pairs.** A float field copied with integer `mov`s (or words copied with `mov` where ours goes through x87) is a whole two- or three-float RECORD assignment (`bicho->v08 = r;` with `V3 r; r.x = ...`); written field by field it gives `fld/fstp` and can move the x87 order of the products ahead of it. A record initialiser `{ clave, 0 }` materialises a zero register where scalar zero stores fold to immediates (`index_zero.cpp`). **held** *(0x5e0570, 0x62d960, 0x549f40)*. A whole-record store through a plain struct instead of an inline `operator=` moves the last store against a following call's pushes.
- **An explicit `if (n > 0)` before `for (int i = 0; i < n; i++)` flips the callee-saved tie (`ebx` <-> `ebp`) between a persistent zero and the loop bound with ZERO instructions** (the rotation already emits `cmp n,0; jle`; the explicit compare changes the priority tie; 0x501000: only ebx/ebp swapped throughout, 0 lines after). Try it when a masked-register diff shows exactly an ebx/ebp swap. **held.**
- **Direct array indexing against a loop-body pointer alias shifts the scratch cursor across the following merge**: `nodos[i].peso` inside an `if` where the loop also has `T *p = &nodos[i]` emits identical loop instructions but changes `ecx,edx` to `edx,eax` after the next call (0x5865f0). **An indirect pointer (`T **pde`) passed into a `__forceinline` helper (`T *de = *pde;`) preserves `pde` in `ecx` and gives a CSE-friendly load**; dereferencing `*pde` at the call site made VC8 CSE it into `edi` so early that the object died and the rotation shifted one step through four hunks (0x451800). **held.**
- **A ternary pair puts the null block out of line**: `o ? o[0] : 0.0f` for each component leaves the `else` block after the function body where an `if/else` keeps it inline (0x4381b0). **A per-branch `return r` keeps the cross-jumped tail in the first branch; assigning in each branch and returning once keeps it in the second.** **held.**
- **Plain branch FIRST**: when the image places blocks A and B after block C, write the plain (negated-compound) branch of the if-chain first: `if (id != a && id != b) {...} else if (id == a) {...} else {...}` (0x5e0570). **held** *(partial: 58 -> 35)*. The same condition written with a different tree shape (`if (!(h && s != 0) && p == 0)` for `if (h) { if (s == 0 && p == 0) .. } else if (p == 0)`) has equal control flow but swaps `eax` and `ecx` (0x49a5a0, 0x49a6a0: 30 -> 0): write the tree as the sibling block below it is written.
- **Nested guards with the failure assignment written in every arm**: `&&` chains and two-level forms use immediate zeros, a THREE-level nest can materialise a persistent zero register (`nested_zero.cpp`); a `&&` guard whose failure arm sets the result may need nested ifs with `r = 0` written twice (0x57d8c0, 0x563f80). **Positive nesting** of the body under the non-rejection condition moves a cold `return 0` to the image's cold block (0x61e820). **The success return as the loop's exit test** (`for (i = 1; ; i++) { if (i > n) return 1; ... }`) puts the epilogue after the shared return-0 block (0x49d2b0, 47 -> 8). **A pointer local with a fallback** (`t = tablas[j]; if (!t) { if (j != 16) continue; t = base; } t[i] = v;`) can be an `if/else` with the store repeated: when the residue is register roles, the shared-store local is what the image lacks (0x626690). **held.**
- **Null-test facts are tied to the VARIABLE**: VC8 drops the null test before `delete n` because `n` was dereferenced; a SECOND variable read from the same memory keeps it (0x49e9b0). A test read through the member instead of a local copy stops VC8 skipping the repeated `o && o->x` (0x5e0570); an address-taken result local (`C *c; C **pc = &c; *pc = f(i); if (*pc && ...)`) preserves the common null test (0x623960). **held.**
- **A pointer read through its OWNER at each use, not through a local**, flips the x87 product load order (0x642300); and after the first occurrence each later expression can need its own access path (pointer for the first, direct for the second: 0x51f8c0). If the image re-reads a member before every call of a repeated sequence, the member was never held in a local. **held.**
- **A spilled local loaded early and stored late** means its assignment comes later in the source (the load is hoisted, the spill store stays at the statement); `Nodo *c = primero` after the unlink statement (0x4da190). **Register-held constants of a record copy**: sweep the constructor's field-assignment order (4! = 24) first; the order is class-specific (0x6339b0 order 4; 0x55c370 and 0x603da0 none). **held.**
- **Repeat a shared sum directly in a clamp conditional** when the image keeps its addend in a separate scratch register (`w = a + b >= 100 ? 100 : a + b;`: the repeated sum lets VC8 CSE it and the addend gets `edx` instead of `ecx`; 0x525100, the last two bytes). **held.**
- **`0.0f == x` (constant on the left) is `fld [x]; fucomp st(1)` against a kept zero; `x == 0.0f` gives `fcom [x]`.** A named `const double k` for a constant used twice flips a later x87 product into the image's order (counter shift). A byte-indexed jump table is a SPARSE `switch`, not a range test (0x602320). **held.**
- **A restricted receiver hoists loads**: a READ-ONLY `__restrict` local (`ClassLista *__restrict l = g;`) moves record-source loads above local copy stores (0x61bd30: 6 lines -> 0); a `__restrict` receiver also moves an allocation's `push` and EH-state store above member stores (`scoped_receiver.cpp`). **Close the restrict scope after the last read** (a nested block ending before any write through another path) so the rule stays legal (`scoped_read.cpp`); it is NOT a write-receiver lever (a whole teleport block restricted: 78 -> 662 lines) and a parameter restrict is not interchangeable with a legal local interval. **held** *(a scoped sweep improved 13 ports; none closed alone)*.

**Method: finding inlined copies and invented helpers**

- **Inline-of-matched: a parked function often contains an inline copy of a function that already matches.** Scans that found them: `inl_gap` (the helper's size in the image minus its size in our object: the ranking that worked; the plain 4-gram vote was mostly noise), `callgap` (calls our object makes that the image lacks), `staticscan` (parked functions whose object calls a function defined in the same object with no symbol-table row: a port-invented helper cannot exist in the image, so the image has it inline; check first that the callee is not an out-of-line image function that merely lacks a row), the REVERSE scan (a parked function as the inlined helper of a matched one), the SIMILARITY scan (a parked image equal to a matched function's image: port it in that function's exact shape: 0x43c110 from 0x44caa0, 0x5d0840 from 0x5305f0; the scan also pays at partial similarity, see 'the matched partner's shape' in the last subsection), plus `regscan`, `pushscan`, `argscan` (wrong parameter counts), `iterscan` and `ecxscan` (ecx loaded before a call more or fewer times than ours: a THISCALL member that never reads `this`, 0x515dd0). **held** *(8 matches: 0x60b290, 0x589680, 0x4684d0, 0x4c8760, 0x514320, ...)*. Source: copy the matched body into the parked file as a file-local `inline`/`__forceinline` (an inline constructor `inline T::T()` with a plain `new`, not a placement new with a null test), reading each site separately (the image may inline an `Anyadir` at three sites and call it at the first). The CONVERSE family (the port inlined a callee the image calls out of line) has its own scan, `callscan`, in the last subsection.
- **A MACRO must reuse the caller's real locals; a file-local `__forceinline` helper DUPLICATES the caller's scratch objects** (`scratch_macro.cpp`: `sub esp,532` against `268`, exactly one 0x100 text buffer and one 8-byte point): where the image has ONE box and ONE point reused across a whole function, use a macro over function-scope scratch. A helper with its own function statics (message ids, guard bits) at each site is a macro too (separate guards per site). **held** *(0x514320, Red screens)*; a `__forceinline` helper or a block-scoped macro local gave two cells.
- **`__forceinline` does NOT remove a returned-record call**: a caller of a `__forceinline` helper returning a record still CALLS it, while the returned-record expression written DIRECTLY through an inline member removes the call (`inline_return.cpp`, 0x5ab9e0 484 -> 472). **Port-invented returned-record wrappers distort the frame and the EH map**: replace them by direct values (472 -> 431). **held** *(improved, not closed)*; not a licence to unbrace every record.
- **The original may FALL OFF a value-returning function** instead of returning 1: `return Valor() + 1` is `call; add eax,1; ret`, falling off is `jmp Valor`; an image that lacks the port's final `mov eax,1` falls off (needs `#pragma warning(disable: 4716)` under VC8; a port for another compiler must not rely on `-fstrict-return`). **held** *(three ports, instruction counts exact)*.
- **A sudden `eax` in the image usually marks an inlined small function** (`mov eax,[esi+0x1c]; mov eax,[eax+0xe4]` is an inline getter taking the object: 0x55f580). If a port calls a helper the image lacks, the image re-reads the helper's arguments from memory: `const int &` parameters of a `__forceinline` helper reproduce that. **held.**
- **A wrong view is not scheduling**: check the call's argument count and receiver against the callee's real signature, taking a matched function with the same byte pattern as the reference (0x492c00 slot-17 call with two arguments was a one-argument accessor: 12 -> 0 lines each; 0x5ee200 a hook with three arguments, not four: 172 -> 0). Also check thiscall versus stdcall (`mov ecx,this` before a call) and the return type: a hidden return pointer for a by-value record changes the decorated name (0x4381b0).
- **`__movsd` + `__movsb` produce the image's variable-length block copy** (`shr ecx,2; rep movsd; and ecx,3; rep movsb`); `memcpy` even through `__forceinline` emits `call _memcpy`, `strcpy` a byte loop (`copy_string.cpp`). **A factory must receive the RAW text and scan it after the allocation** to reproduce the image's order. **An inline call site can need a different store order from the standalone helper** (`n->lista = this` before `n->siguiente = c`). **held** *(improved)*.
- **The wrong-shape fix is the productive lever; allocator spelling sweeps are not** (D4). The persistent-constant, `this`-register and aligned-frame rules (vc8-regalloc.md) describe the IMAGE's codegen but did not transfer to the port as a source lever on three ports: dead locals, named copies, re-reads, reorders and splitting a value never reproduced the allocation. What moved a port was a corrected view: the target velocity spelled `(*ruta)[3] - y` versus the image's inline `Velocidad` shape (`float *p = ruta + 2; Dif d; d.x = p[0] - x; d.y = p[1] - y; sqrt(Cuad(d))`, order `v.y*v.y + v.x*v.x`), the radian conversion split from the float round-trip, a re-read after a call (297 -> 225, then 207). Look for the wrong shape first, then Gemini-style source rules (noalias, guards, static globals). A `symbols.tsv` with two conflicting rows at one address (matched and parked signatures) is a discrepancy to resolve, not to move silently. **held.**
- **Compose saved models**: a saved structural best of one round plus a later independent rule (store fence, variable identity, restricted receiver, literal) closed ports that neither closed alone (0x5d5aa0: a guarded `do-while` plus a member-pointer store fence `int *u = &t->m[i].uso; *u = 1;`, which keeps the store before another). Keep every best (`best_ADDR_N.cpp`), and apply a stored best when it beats the tree.

**EH states and class layout seen from the compiler**

- **EH-only operations preserve member unwind states**: a dead call to a function declared WITHOUT `throw()` in a containing destructor (`if (0) PuedeFallar();`) keeps the front end's complete member-unwind map without emitting an instruction; with a non-throwing `operator delete` VC8 then emits the next state transition early (0x549d20). It is specific to containing destructors and member-unwind maps: dead calls in constructor bodies or elsewhere are **no effect** (`ctor_deadcall.cpp`). **held.**
- **`throw()` placement**: a `throw()` on a constructor removes the cleanup of a `new` of it (required where the image's `new` has no EH cleanup); derived constructors must NOT get a blanket `throw()` (their per-site states are in the image); a `throw()` destructor avoids an extra state store; the implicit global `operator delete` is nothrow, so a base's unwind state vanishes unless the memory is freed through a function declared without `throw()`. Unwind-funclet suffixes (`$1`) are NOT state indices: follow the relocation at `__unwindtable$X + 8*state + 4`. **held.**
- **Real class structure shows in the RTTI hierarchy and vtable slots**, not in a zero function diff: validate nested RTTI and vtables when reconstructing a class, and give a function that becomes a virtual override its symbol row or it drops out of the compare.
- **VC8's output depends on how many declarations and files a translation unit has**: an empty extra `#include` in a header changed one instruction (`lea eax,[ebx+3]` became `mov eax,2`); deleting dead `typedef char layout_...` lines or a 12-member class that the file never uses restored a load; line numbers and comments did not matter. Do not move a class out of a widely included header into a new one (select the part with a macro), keep size-assert typedefs out of widely included headers, define a shared inline constructor `inline` only in the one file that needs it, and a ctor-less base with a zeroing default constructor zeroes unrelated locals. `__declspec(align(8))` on a record changed a matched frame. **held** *(0x51de00, 0x48f250, 0x50e3d0)*.
- **VC8 does not inline an ICF-folded destructor into the scalar deleting destructor.**

**Method: reading diffs and what not to trust**

- **A diff tool that cuts our listing at the last `ret` counts a cold block placed after it as a difference**: measure with the cut off before ranking candidates (0x49d2b0 is 8 lines, not 66); a masked diff hides jump-table targets; stale objects in the build directory can supply false identical names (13 did): rebuild before comparing sets, and delete the objects of removed sources first (section 14: they inflated a status count by 20).
- **If a micro-test with the port's own spelling already matches the image, the cause is how the operands are REACHED** (a named pointer to the embedded list before the loop gave the image's SIB order), not the expression. **A micro success may not survive the full allocator context** (seven ports); say so before applying it. Write predictions BEFORE compiling a micro-test: four of four were right for the rotation clauses.
- **Residue classes the later rounds left at 2-35 lines**: scratch-register ties, a compare-tree layout tie, a reload order after an unrolled loop, one extra frame cell, a constant-allocation race (the image keeps the constant 5 in `ebx` where ours keeps 0: one use more or fewer; a real extra call flips it but costs instructions), x87 term order invariant under counter shifts 0-3, an uninitialised stack read the image keeps (the loop result read from a dead parameter home), custom-convention or `/GL` callees, assembly originals. Treat these as reachable only through a new compiler model, not another spelling sweep.

### Block layout, wrong views and storage shapes (rounds K23-K28, B110-B121, Q1-Q2, D5: 14 matches, 12,655 -> 12,669 identical) `[vc8]`

By these rounds every cheap parked port was a known residue (register roles, frame cells, x87 scheduling), and five agents on three model families agreed that spelling sweeps were spent. Nearly every match came from a STRUCTURAL correction read out of the image: a wrong callee view, a case order, a switch hidden behind a compact port, a member constructor, a storage class. The scans that found them line the port's calls and blocks up against the image's. **held** *(matches by address as named)* unless marked **partial** (improved a port, did not close it) or **no effect**; `micro/NAME.cpp` is a micro-experiment compiled with the harness flags, with old objects and listings deleted first.

**Block layout: flags, switches and exits**

- **A comparison whose false arm the image lays out AFTER a `ret` is materialised into a FLAG and tested with the ELSE ARM FIRST.** Symptom: inside a case or branch the else arm (`fstp st(n); xor edi,edi; fld K; jmp back`) sits out of line behind the block's `ret` and jumps back, where ours has it inline. Source: `listo = z >= m->pie.z; if (!listo) v = 4.0f; else v = (float)(m->pie.z - z);`. Every other spelling keeps the false arm INLINE: `if (c) { listo = 1; ... } else { listo = 0; ... }`, `listo = c; if (listo) A; else B;`, a ternary, a preset flag, the negated compare `!(z < p)`, a `goto`. **held** *(0x443400, 41 -> 0, after four rounds had given it up as 'every form canonicalises')*. **The same for a select**: a ternary on a flag VARIABLE (`int valor = !sin || cont; T t(valor ? "1" : "0");`) keeps the image's `mov eax,"1"; jmp; mov eax,"0"`; a ternary on the condition itself is if-converted (the value preloaded: `mov eax,"0"; je`) (0x4a2c90, **partial**). Not universal: one test block stayed inline under ten forms including this one (0x4d2980). A scan for blocks the image lays out after a `ret` that ours has inline found only big ports. The one-armed case (a flag that is zero on the false path, 'Jump-table blocks in address order') is earlier in this section.
- **VC8 lays the case bodies of a jump-table `switch` out in SOURCE order: write the cases in the image's block order, with one result variable and one return.** Symptom: a message handler hundreds of lines off whose cases each look right; the port makes the SAME calls as the image in a different order (`orderscan`: a high common share of calls, a low in-order share). Source: decode the jump table to name each block's case value and write the cases in address order; cases the port merged with an inner test (`d == -0x24 ? Bajar() : Subir()`) are separate cases; `int r = 0; ... r = 1; break; ... return r;` with `break` everywhere, the last case falling through into `default: Base::Mensaje(..); r = 1;`. Only ONE `return` keeps the result in its frame slot (a `volatile` result pushes the locals out of the dead parameter homes; an address-taken result is still constant-folded); null tests stay a literal `return 0`; locals declared per case land in the dead parameter homes. **held** *(0x4dcdc0: the first step, 879 -> 810, of the chain that matched it)*; **partial** *(0x64d930 906 -> 174, 0x4985a0 301 -> 147, 0x4dea90 695 -> 247)*: what stays is register roles (which of result and node takes `edi`/`ebp`), and one sibling got worse in image order (0x493320). **Static guards give the original case order directly**: where each case holds a function-static with a dynamic initialiser (`static int x = BuscarMensaje(..);`), the EH state number stored before each initialiser call rises in SOURCE order, so the states number the cases as the author wrote them. This is the jump-table rule; a switch lowered to a compare tree ignores the label order (the 0x53c350 entry above reads the order from the test chain instead).
- **Explicit cold-exit labels move the ownership of a cold cleanup block, and the x87 pops move with it.** Symptom: a handful of strict bytes: branch displacements that swap WHICH of two failure paths owns the cold cleanup, plus `fstp st(0)` against `fstp st(1)`. Source: one label per failure (`goto falloT;`, `goto falloD;`) instead of after-return labels or a shared one; the ownership and both pop opcodes are fixed TOGETHER, never one at a time (`micro/cold_constants.cpp` shows the layout effect). The CFG change then flipped three arithmetic groups, restored by a named `const double uno = 1.0` at ONE expression (a node-counter shift): that part is port-specific. **held** *(0x61e820, 359 -> 0 over ten rounds; its sibling 0x6206b0 365 -> 61 with ONE shared cold restore)*.
- **Loop exits**: `for (...) if (match) { rest; return; }` (the whole body inside the found branch) gives the image's loop exit straight to the epilogue, where `break` plus `if (i == 3) return;` leaves a `cmp`; a loop count copied into a local (`unsigned cnt = t->cuenta; ... if (j >= cnt) e = 0;`) lets VC8 thread the not-found exit; `i++; test r; je top` is `for (...; !r && i < n; i++)`. **partial** *(0x46cd00, 0x48e1b0)*.
- **An object the image destroys before a later statement of a loop body sits in its own block** ending before that statement: this fixes the call order (masked diff 0) but permutes the frame cells. **partial** *(0x4a2c90)*.
- **Two consecutive tests of the same flag are JUMP-THREADED.** Symptom: an INTERLEAVED layout: two two-armed chains whose first items are merged into the header blocks, so the save path and the load path each run straight through with no second test. Source: `if (g) A1; else B1; if (g) A2; else B2;` (a header `if (guardar) { mask; Escribir(..); } else Leer(..);` followed by a second `if (guardar) { saves } else { loads }`), not one merged `if/else` and not two unrelated conditions. **partial** *(0x55da20, part of 498 -> 28)*.

**Loops: rotation, unrolling and the switch behind a memcpy**

- **An UN-ROTATED loop head (a label at the head, reached by `goto`) stops VC8 peeling the head.** Symptom in the image: all callee-saved pushes BEFORE an aligned loop head (`mov edi,edi` padding), and the head's calls appearing only ONCE, where ours has the head's calls twice (a peeled first pass). Source: `for (;;) { otra: r = Leer(..); if (vacia) goto otra; ... }`: a label at the head of the body with `goto otra` from the arm that restarts. `continue`, `while (1)`, a pure goto loop and close-and-return inside the loop all let VC8 peel. **partial** *(0x619b20, 134 -> 39 by this alone)*. Earlier entries on guarded `do` loops against peeling describe the same pass.
- **A GUARDED `do/while` suppresses the 4x unrolling of a forced-inline loop.** Symptom: the image has ONE copy of a loop body where ours has the four-way unrolled path. Source: `if (n > 1) do { ... } while (c < n);` in the `__forceinline` helper instead of `for` (`micro/unroll_do.cpp`: 260 instructions with the `for`, 97 with the guarded form; on 0x5467c0 three helper expansions went from 1103 to 564 instructions against the image's 570). **held** *(micro; then 0x5467c0, 684 -> 0, once the helpers themselves were right: see 'Forced-inline helpers, site by site' below)*. A first try on that port was not retained because frame and register differences scored worse overall: the rule was right and the helper bodies were wrong.
- **N explicit calls against a loop swap the frame cells.** Manual tail unrolling changes the reference counts the cell allocator sorts by: `micro/unroll_pack.cpp` gives `_p$ = -404, _h$ = -272` for the loop and `_h$ = -404, _p$ = -132` for FOUR EXPLICIT CALLS; declaring `h` earlier with the loop changes nothing. Symptom: N identical call sequences in the image where the port has a loop (or the converse), with two large locals in swapped frame regions. **partial** *(0x6206b0: the four recursive child calls unrolled, 365 -> 61)*.
- **`while (p && *p)` against `if (p) while (*p)`: the null test rotated to the bottom.** Symptom: a string-copy loop whose latch re-tests the POINTER (`add eax,1; jne`) instead of only the character. Source: `const char *p = Buscar(..); char *d = nombre; while (p && *p) ...` (the source pointer declared before the destination); the guarded form tests the pointer once outside the loop. **partial** *(0x55da20)*; the same idiom as the `add ecx,4` entry in section 3.
- **An INDEXED SIBLING LOOP controls the register and constant allocation of a DIFFERENT loop in the same function.** Symptom: unrolling a table loop into the image's explicit blocks gives the right instruction shapes with every persistent constant in the wrong register, and the diff gets WORSE. Source: look at the other loops of the function; the image's second loop was a running-pointer down-counter (`T *mg = (T *)(t + 0x7c); int mk = 8; do { new (mg) T; mg++; mk--; } while (mk >= 0);`) where the port had the indexed form (`for (k = 0; k < 9; k++) new (t + 0x7c + k*0xdc) T;`). With the indexed sibling VC8 allocated the whole constructor differently; the down-counter aligned the held constants (0, 1, -1, -2 in `esi`, `ebx`, `eax`, `edx`) so the unrolled blocks matched (the constants written as named `const int`). **partial** *(0x603da0: 946 -> 1329 by unrolling alone, 220 with the sibling rewritten too)*: never judge an unrolling by its own score while a neighbouring loop still has the wrong shape. Left there: a frame-size and second-`/GS`-cookie difference, the placement-new null test.
- **A port that compressed hand-written per-type loops into `memcpy` or a row table hides a whole `switch` of hand-unrolled cases.** Symptom: the image is MUCH longer than ours, and the call scan shows an accessor called once in every case that the port never calls. The original is a `switch` on the pixel size with one body per case, each row being: single units until the destination is 4-aligned, then whole 32-bit words (`((q3<<8|q2)<<8|q1)<<8|q0`, `q1<<16|q0`), then the tail; `int r = 1; ... default: r = 0; ... return r;`. VC8 unrolls simple counted loops 4x (an n/4 counter in a frame cell) and does not unswitch, so a `switch` INSIDE a loop in a port may be a `switch` OUTSIDE five loops in the original. A faithful over-read (a dword read of the last 3-byte table entry) is part of the match: keep it and mark it. **held** *(0x47b110 453 -> 0, 0x47a420 479 -> 0; 0x479ce0 544 -> 23)*. In such a function per-case loop variables are separate objects with low counts each, while ONE function-scope variable gets the summed count and a lower frame cell (0x479ce0: 124 -> 72 -> 23, as the cell allocator model predicts).

**Wrong views: the calls the image makes and the records it passes**

- **The converse of inline-of-matched: the port INLINED a callee that the original called OUT OF LINE; restore the call, found by address.** The scan (`callscan`) lines up each parked function's calls against the image's by callee address with thunks followed, and per call compares the pushes, `sub esp`, the `ecx` loads, the hidden `lea`/`push` return pointers and a chained `mov ecx,eax`. Signals: an image-only call (a small method, an out-of-line constructor, a projection the port wrote inline), `sub esp,N` against none (a by-value record argument), a swapped call order (separate branches, or the case-order rule above), a different callee address for the same job (plain `delete` where the port wrote `delete[]`; a real `new T(..)` of a constructor the port reached through manual allocation plus a `Crear` view). Rows that differ only by name or thunk are noise. **held** *(the two matches above came from its list)*; **partial** *(0x4d4590 405 -> 235, 0x601e50 515 -> 270, 0x611b80 320 -> 108, 0x5129d0 700 -> 587, three more)*.
- **A by-value return chained into a member call is `A(n).B(arg)`, not an out-parameter.** Symptom: `push arg; ...; lea eax,[tmp]; push eax; call A; mov ecx,eax; call B`: B's argument is pushed BEFORE A's hidden return pointer, and B runs on the returned temporary. A port with a pointer-out view (`Leer(&p, n); p.Igual(&pos)`) has the pushes in the other order and a named cell. Source: a file-local view whose method returns the record by value, `Dar(tramo.cuenta).Igual(&posicion)`; check the return type in the callee's matched decorated name. The same wrong view on a subtraction helper (`Resta(out, p)` returning `out`, `Largo` called on the returned pointer; a by-value `Resta` call where the port subtracted inline) was worth 587 -> 385. **held** *(0x48e940, 278 -> 0, with the next entry)*.
- **A NAMED plain record resists copy-propagation; a constructed one does not.** `P3 p(a, b, c)` with an inline constructor is propagated away and its three arguments are spilled as separate 4-byte function-scope temporaries (a frame 12 bytes larger); a plain `{x, y, z}` record filled field by field and passed as `*(P3 *)&p` keeps ONE cell, which can share with block temporaries. **held** *(0x48e940)*.
- **A by-value record argument built IN PLACE in the argument area needs a user copy constructor.** Symptom: the image writes the record straight into the outgoing arguments (`fld/fstp` into the reserved area; for an int record `sub esp,8; mov eax,esp; mov [eax+4],..; mov [eax],..`), where ours builds a temporary and copies it as integer words, with a larger frame. Source: give the record type a user copy constructor (micro-tested). **Native danger on a SHARED type**: the copy constructor makes the type non-trivially-copyable, which changes how another ABI passes it (GCC passes it by hidden reference; clang in the MS ABI still passes it on the stack), so a call that reaches a differently-typed native function through an alias breaks silently. Before adding a copy constructor to ANY by-value type, list every function that takes that type by value (the alias lines of the native build report); if the type is shared, use a DEDICATED derived type (`struct Destino3 : Vector3 { Destino3(const Destino3 &); ... }`) taken only by the callees concerned, and rename their symbol rows (the decorated name changes with the parameter type). A file-local type, or a callee whose real type already has the constructor, is safe. **partial** *(0x448d40, 0x452c70, 0x48e1b0 444 -> 193, 0x601e50)*; open where the callee is reached through several trivially-copyable views. Superseded as the preferred form by the dedicated derived type with only a CONVERTING constructor (last subsection, 'By-value records'): it builds in place with no copy constructor and needs no compiler-specific guard.
- **A member reached through an inline returning a REFERENCE at each use, not through a pointer local.** `fld [eax+8]; add eax,8; fld [eax+4]` (the address advanced between the loads) is an inline function returning a reference; a local pointer is folded into the addressing, and a pointer local declared at the top is hoisted into a register for the whole function where the image recomputes it per use. For three-float differences an inline `operator-` returning the constructor expression `V(x - o.x, ...)` puts the temporaries in the dead parameter homes. **held** *(0x48e940, 0x4dcdc0 787 -> 583 -> 497)*.
- **Match the image's reload pattern in both directions.** A global's field the image reloads at each use is read at each use (a macro, not a cached local: 0x48d1c0, 312 -> 218 by this alone); a pointer the image holds in one register across several stores is the NAMED local, not a re-read of the owning member (0x4d4590, 0x5107f0 4 -> 0). An x87 subexpression the image keeps on the stack without a store is written TWICE in the source (`(x1 - x0)` twice, not a float local). Two int parameters converted once and kept in their OWN home slots are the float parameters of a `__forceinline` function called as `Rayo((float)a3, (float)a2, ..)`. **held** / **partial** *(as named)*.
- **A member receiver before an argument call**: `{ T *p = this->m; p->F(0, G()); }` loads the receiver BEFORE `G()` runs; `m->F(0, G())` loads it after. **partial** *(0x64d930, 277 -> 189)*.

**Forced-inline helpers, site by site**

- **Check EACH inline expansion against the image separately: N similar sites may be different helpers.** Symptom: a port with one `__forceinline` helper expanded at three sites has the right instruction count and hundreds of diff lines; site by site, one expansion in the image skips a step the others perform (the own-posts site skips the distance measure for a soldier on guard; the other two always measure and use the guard only in one rule). Source: two helpers (or one with a flag parameter), each written from its own site; an earlier round had assumed three identical expansions and the port even behaved differently from the image at that site. **held** *(0x5467c0, 684 -> 0)*.
- **A forced-inline helper's RETURN VALUE stays a separate register after the call sites join.** Symptom: an extra `mov eax,ecx` in the image after several expansions merge, where a helper writing its result through an out-parameter or a shared local has none. Source: the helper RETURNS the value (`return mejor;`) and the caller assigns it. **held** *(0x5467c0)*.
- **A loop counter that sits in ONE image cell across several inline expansions, while their other locals spread over separate cells, is ONE caller variable passed BY REFERENCE** (`unsigned &c`), not a local of the helper. This was a frame-cells-only last step: the instructions were already identical. **held** *(0x5467c0)*. The general by-reference signs are in section 6.

**Callee visibility, continued** (extends the first group of the previous subsection)

- **The effect reaches the x87 stack, and there it needs the REAL definition.** Symptom: the image does not reload floats after a call where ours reloads four or five, and the image's locals sit in parameter home slots. Source: the callee's real definition EARLIER IN THE SAME FILE as the caller; a `noinline` view holding a COPY of the body, which is enough for the register and frame effects, does not work here. Moving a function drags its dependants: a function that inlines the moved callee loses its match when they are separated and must move with it. **partial** *(0x448d40 353 -> 270 and 0x452c70 388 -> 269 by the move alone, 485 -> 38 and 414 -> 118 with the rest)*. A scan for this sign over the remaining ports found only false positives: the CRT intrinsics (`__CIsqrt`, `__CIatan2`, `__CIcos`) take their argument on the x87 stack.
- **A callee that receives the ADDRESS OF A LOCAL changes the whole caller once its real definition is in front of it**, even though the image calls it out of line: VC8 then knows what it reads and writes through that address. Scan: ports whose image passes a frame address to a nearby callee that our tree defines in another file. **partial** *(0x46cd00 362 -> 266 by the move alone; 0x64caf0 15 -> 13)*. **Small functions parked in catch-all files that sit INSIDE a class's image address range belong to that class's translation unit**: the address order is the evidence for the move. The converse is a diagnostic: a callee made visible that gets INLINED where the image calls it was not in front of its caller in the original. The hit rate is low: about thirty further pairs, including every hand merge of two files' view structs to make a callee visible, were inert or worse (14 -> 110 in one).

**Constructors, statics and storage**

- **Zero stores INTERLEAVED between the mem-initialiser stores of a constructor are member sub-object constructors with zeroing bodies, running in declaration order.** Explicit stores in the constructor body can never land there (they follow every initialiser). Source: make the zeroed fields members of small record types with zeroing constructors (the zeroing order inside each, `z, y, x`, is the member constructor's own); a vector followed by a float that the image resets together is ONE four-float record. A scan of constructor member-store order finds candidates. **held** *(0x436130, 68 -> 4 -> 0)*.
- **A function-local `static` array shows as accesses at ABSOLUTE image addresses.** Symptom: a function indexes several small arrays at fixed data addresses (`[eax+0x780980]`) where the port has local arrays (and with them a `/GS` cookie the image lacks) or file globals. Source: `static unsigned char otro[25];` inside the function; each gets a row under its decorated static-data name (`?otro@?BI@??F@Clase@@QAEXXZ@4PAEA`), which maps it to the image address. **held** *(0x5467c0, 785 -> 684 by this, later matched)*. **The scope id in that name (`?BI@` against `?BH@`) depends on the function's inline-helper count**: after a rewrite that adds or removes a helper the decorated names change (three rows went from `?BI@` to `?BH@`), and the strict comparison checks the data relocations against these rows: re-derive the names from the new object after ANY rewrite of such a function. The brace-initialised kind (a guard, placement constructions and `atexit`) is in 'Hand-built pieces' above.
- **`/GS` places char arrays together at the top of the frame**: when the image has a buffer BELOW a known char buffer, the buffer that sits with the char one is a char array too (`char palabra[0x100]` read with `memcpy(&n, palabra, 4)`, not an int array). **partial** *(0x619b20, 39 -> 31)*. The cookie itself names a type (the three-short array entry above; `micro/color_gs.cpp`: an address-taken record with `unsigned short c[3]` gets `__$ArrayPad$`, three scalar fields do not).
- **VC8 deletes a store of a field's own just-read value; the image keeps it when the value arrives as an inline member's PARAMETER.** Symptom: the image re-stores a field (+0x114) that a direct assignment in the port never writes. Source: an inline setter on a small view struct cast onto the address (`((Par110 *)((char *)this + 0x110))->Poner(a, b)`), so the stored value is a parameter and not the recognisable read. **partial** *(0x55da20)*.
- **Address-taken locals the image loads into callee-saved registers BEFORE `operator new` are copied into locals before the new-expression**: `int x = a, y = b, w = c, z = d; new Area(x, y, w, z, n);` where `a..d` were written through their addresses (`sscanf`); `new Area(a, b, c, d, n)` reads them only after the allocator call. **partial** *(0x619b20)*.
- **A local pointer to an UNUSED parameter keeps the argument at the call sites of a file-local `static` function.** VC8 drops an unused parameter of a custom-convention function from every call (`sub esp,8` for two floats where the image passes three with `sub esp,12`); `float *pz = &z;` in the callee keeps it without changing the callee's body (`micro/static_arg_home.cpp`). Dead reads, `(void)&z`, `noinline` and moving the definition below the caller do not. **held** *(micro; the callee 0x570db0 stays byte-identical)*.
- **Scope an optional record through its LAST use** and it shares the cell of a later record (0x6206b0: the exact 0x44-byte frame gap closed). A plain three-float record passed BY VALUE to an inline can change the temporary cells and the x87 row order, but only in some callers (`micro/projection_vector.cpp`): **partial**.

**Spellings that held, and negative results**

- **The matched partner's shape.** A parked function similar to a matched one (similarity 0.80 is enough; same-file partners first) is rewritten in the partner's exact shape: one `||` reject chain with `return 0`, then the member expressions written out at each use, NO named locals (named copies of the fields were the stack-cell residue). **held** *(0x479320 from 0x4793f0, 28 -> 0)*.
- **Conditional out-parameter stores as ternaries.** In x87 code `if (s) *s = d < k ? 0.0f : p / d; if (t) *t = d < k ? 0.0f : q / d;` gives the image's shared `fldz` and out-of-line null-pointer exits; the `if/else` form with an early `if (!t) return r;` puts those exits inline. **held** *(0x437540, 68 -> 0)*. The byte-faithful form can differ in behaviour from the old port on a NaN operand: the image is the reference.
- **Float compare spelling flips the test; equivalent condition SHAPES do not.** `P > R` against `R < P` and `S >= Q` against `!(S < Q)` flip the polarity (`q >= 0` is `test ah,1; jne`, `!(q < 0)` is `test ah,5; jnp`; `q <= 0` loads `fldz` mirrored; `p > -alcance` loads `p` first); the ORDER of independent terms in an `&&` or `||` chain decides where the `fxch` lands and which block owns the `fstp st(n)` cleanup (`micro/x87_nesting.cpp`: 82 against 80 instructions). A flag variable, `!(a || b || c) || ...` and nested guards all compile to the same layout. A rounded `fabs` argument (`fstp t; fld t; fabs`) is a float PARAMETER of an inline. **held** *(0x479320, 0x437540; the rest micro)*.
- **A `volatile` read of a value copied out of an address-escaped array.** Symptom: `int msg = ev[1];` with `ev` passed to a call compiles as `mov ecx,[m]; ... mov ebx,ecx` where the image loads straight into the final callee-saved register. Diagnostic: writing `ev[1] + 1` gives the image's register at once. Source: `int msg = *(volatile unsigned *)&ev[1];`, one per value copied out (both were needed). Use it only where it makes the function byte-identical, with a one-line comment; never as an improvement left in a parked port. **held** *(0x62bba0, 129 -> 0)*; the same family as the volatile argument read above, and as narrow.
- **A file-local inline accessor returning an array element's ADDRESS, called at each use, gives two DISTINCT reads where a variable forwards one.** Symptom: the image reads the same element twice (once for a test, once for the use) and ours reads it once; a scoped second variable or a named page pointer forwards the first read through the test. Source: `static __forceinline T **Pagina(C *o, int i) { return &o->pantallas[i]; }`, then `if (!*Pagina(this, actual)) return; T *p = *Pagina(this, actual);`: the accessor call repeated, not its result named. **held** *(0x584a90, the last hunk of a 354-instruction function)*.
- **The order of an OR chain of ternaries is decided by VC8 from the terms' EXPRESSION IDENTITY, not their source order.** Every source permutation, a `|=` chain and a `+` form were inert or worse; changing how ONE term is spelled (an alias `org[i]` against the member `organos[i]`) changes the emitted order. So a wrong term order points at a term reached through a different expression, not at the order written; mixing spellings only to steer the order is not evidence and was not kept (the natural resolution, a term that is really a distinct member, is in the last subsection). **partial** *(0x55da20: the 28 lines left)*.
- **A squared sum on a temporary built by an in-place constructor loads x first**: `t.y*t.y + t.x*t.x` on the constructed temporary gives the image's load order; on a plain local the operand order is inert. **held** *(0x5467c0)*.
- **Negative results.** Respelling a read does not defeat `__restrict` forwarding (inline reference accessors, address-taken scalars and reference locals all forward the same value: `micro/accessor_reload.cpp`). The increment spelling of a returned-record store is canonicalised (`*r++ = R()` against explicit increments: `micro/returned_increment.cpp`). Reusing one scalar for two halves is not a general spill rule (identical code in `micro/reused_halves.cpp`; one port gained from it through its own value webs). A dead zero store in a constructor is deleted in every member, base and pointer form. An explicit five-way expansion of a five-iteration loop gave the image's instruction count and a worse layout. Loop-top padding differences are not a lever of their own: they vanish once the frame offsets match (disp8 against disp32 changes the code size before the loop). **Running pointers in the image are not evidence of running pointers in the source**: VC8 strength-reduces `dst[i*3+k]`, `src[i*3+k]` and `*(unsigned *)(src + i*4)` into them, and a write-only pointer rewrite scored worse (243 -> 262); the same port turned out to read values the image never initialises (carried over when a format code falls outside the handled cases), so under a deterministic-value policy for uninitialised reads it is not deliverable at all: check what the image initialises before rewriting a loop (0x478480). **no effect.**

**Method**

- **The scans of these rounds**: `callscan` (calls by address, above), `orderscan` (same calls, different order: case-order ports), a similarity ranking of each parked port against the matched functions, a constructor member-store scan, the frame-address-to-nearby-callee scan, an instruction-multiset distance against diff lines (a small multiset distance with many diff lines is an order- or layout-only residue), a cold-block scan (blocks after a `ret` in the image, inline in ours). Each found one or two ports and was then empty: run a scan once over all parked ports and work its whole list.
- **Three mechanical scans over all parked ports, each tagging a ranked worklist** (a fresh diff of all 358 parked rows, 328 of them real ports): an ABSOLUTE-DATA scan (the image touches absolute data addresses: function statics or globals the port may hold as locals; 163 ports), an ALIGNED-LOOP-HEAD scan (padding before a loop head: the un-rotated and guarded loop shapes; 77 ports) and a COLD-BLOCK-AFTER-`ret` scan (the flag rule above; 42 ports). The hit lists are wide, not precise: about 18 ports opened from them gave one large improvement and otherwise the known residue classes, so use the tags to pick which lever to try first on a port, not as a list of matches. Caveats: a diff figure can be an unnamed-neighbour artefact (a '199' whose real diff was about 4), and a call comparison BY NAME reports false image-only calls for view methods that alias one address: compare calls by address.
- **A masked diff of zero is not a match** (jump targets are masked too: last subsection): the strict comparison also checks that each call relocation resolves to the intended callee (two constructor views needed alias rows before 0x584a90 counted). A view that has no row of its own is reported, not matched.
- **Some size deficits and parked rows are artefacts**: a function whose extent runs into an unnamed neighbour (no symbol, not in the function-start list) looks far shorter in the port than it is; a parked row at an address that is another, matched function is an inline COMDAT with no address of its own; a 5-byte incremental-link `jmp` is a thunk, not a port. Check the extent before working a 'size deficit'.
- **What is left**: by every agent's estimate four to eight source-reachable ports, through static storage, wrong views, translation-unit and EH reconstruction, object lifetimes and loop shapes; the rest are the residue classes listed at the end of the previous subsection plus two more: a zero store that gets its own `xor reg,reg` (nine spellings, no effect) and commutation ties (`add reg,reg`, a loop-top `cmp`).

### Re-reads, shared exits, birth order, frame kinds, by-value records, class views and what the diff cannot see (rounds K29-K40, Q3-Q42: four matches) `[vc8]`

Sixteen rounds over a parked set in which every cheap port was already a known residue: two matches and about fifteen large improvements. What paid was, again, structure read from the image (who reloads what, where the exits join, which objects are real constructors) and two walls turned into tables by micro-tests; what did not pay is listed under Method, mostly as corrections to the measuring tools. Fourteen further rounds (through K33 and Q30) added no match and about twenty-five improvements: the frame-kind tables, the emulated-object lesson, the audit of saved models, three more tables in `vc8-regalloc.md`, and one adopted change that linked, matched and crashed at start-up. Seven more (K34, Q31-Q36) added no match and eight improvements: the crash diagnosed (a runtime wrapper of the second build, not the change) and the change re-adopted, one wrong view with a precision change applied across a family of files, declaration interleaving, and three negative surveys (register roles, x87 storage class, emulation sites) that each say which residue class a lever cannot reach. Five more (K35-K37, Q37, Q38) added no match and twelve improvements, all in one family: by-value records, as arguments built in place and as returns consumed in place, with the forms that stay safe in a second build, and two negative sweeps (declaration interleaving, the consumption of the returned pointer) that ended in two named blockers, both overturned one round later. Four more (K38, K39, Q39, Q40) added no match and fourteen improvements: a micro-matrix of about 600 functions that dissolved both blockers (a temporary does not renumber the unwind states; one inline call with several reference arguments holds the returned pointers in registers), the unwind-state numbering rules N1-N7, and an automated subset search over independent source toggles, which showed that single-lever verdicts go out of date. Three more (K40, Q41, Q42) added TWO matches, both on ports that several earlier rounds had recorded as unreachable and both by the same move: the port emulated classes that the original really constructs, and rebuilding the real class view (base, members, constructors, every `new`) closed diffs of 360 and 400 lines; with them a static custom convention, the nested-block lifetime lever, a re-check that found twenty 'excluded' ports already matched, and a second crash that no gate saw. **held** *(as micro-tests under the harness flags, old objects deleted first)* for the tables; the levers are **partial** (each improved a port and closed none alone) unless marked.

**Reloads and owner pointers**

- **A CACHED OWNER-POINTER LOCAL is a whole-function allocation lever.** Symptom: the image re-reads the owner member (`mov reg,[this+off]`) before EVERY use, where the port holds `Derived *a = (Derived *)owner;` in a register. Source: a re-read at each use (`#define A ((Derived *)owner)`). On one port this alone cut the diff by two thirds (587 -> 185 lines) and aligned the allocation of the WHOLE function (the register of `this`, the register of the held zero) before any other fix. It generalises the re-read rule of the previous subsection ('Match the image's reload pattern in both directions') from globals to the owner pointer. Limits, each measured: (1) it applies only to MEMBER reads (`this->x`, `owner->x`) that the image reloads across calls; a macro re-read of a PARAMETER CAST is inert (VC8 forwards it); (2) a macro NAMED LIKE A MEMBER breaks every `v->X` (the member token expands): spell the cast at each use instead; (3) ports already within a few lines of the image have IDENTICAL load profiles in image and ours: the lever does not exist there; (4) it is rare: a sweep of all parked ports improved two more, since most ports that cache an owner cache it where the image caches it too; (5) it can lose (one port 397 -> 411): compare the load counts per member first. **partial** *(three ports)*.
- **A bound re-read through its GLOBAL breaks the CSE of the bound**, but VC8 then merges a signed and an unsigned compare of it into one `cmp` and two branches; an image with two separate compares stayed out of reach (about 22 forms; a CSE across the branch would need `volatile`). **no effect** as a closer.
- **An OR-chain term that reads a DISTINCT MEMBER takes a different place in the expression-identity order than an alias index.** Completes 'The order of an OR chain of ternaries' above: a chain of five presence bits whose fifth term the port read as one-past-the-end of an alias array (`p[4]`) was really a separate member that follows the array; written as that member (`(extra ? 0x10 : 0)`) VC8 placed its term first, as the image does (28 -> 12 lines). This is the natural form of the 'mixed spellings' that the earlier entry rejected: a wrong term order means a term is reached through the wrong OBJECT. The order of the remaining four alias terms had no lever (all 23 permutations identical, member spellings worse).
- **A null-tested member that the image tests IN MEMORY, reloads for the receiver, and whose sibling field it loads BEFORE the null branch is an INLINE HELPER's argument**, not a `__restrict` cached local: the helper's argument is evaluated ahead of the `if` in its body. A FORWARD `static inline` declaration lets VC8 inline a helper DEFINED LATER in the file. A named pointer to a record before the copy (`T *d = o->m; T v = *d;`) frees one scratch-rotation step.
- **A member receiver re-read at each use, the case bodies in image order, and DUPLICATED tail calls that VC8 cross-jumps** matched a message handler: the recipe of the previous subsection's block-layout group applies unchanged to the next handler. **held** *(match)*.

**Exits, epilogues and block placement**

- **Six levers that took one port from 549 to 166 lines and another from 119 to 111**, in the order applied: **(L1) the SHARED-FIN GOTO**: when the image's early exits jump to one common tail that writes the final record, every exit is `goto fin;` to ONE label, not a tail per branch (549 -> 332); **(L2) the NAMED-PRESET TERNARY LOCAL**: `f(c ? A : B)` written `int v = B; if (c) v = A; f(v);` gives the image's push per arm AND flipped the callee-saved roles of `this` and the result for the whole function (332 -> 232); **(L3) the FUNCTION-SCOPE LOAD**: a loop bound declared and loaded at function scope closes a 4-byte frame gap (the early LOAD moves the allocator; a declaration there with the assignment later is worse) (232 -> 182); **(L4) the DOWN-COUNTER LOOP**: `for (i = n; i > 0; i--, b++)` gives the image's `sub reg,1` counter (182 -> 166) and is WORSE on a port whose image counts up: read the latch before choosing; **(L5) INVERTED NULL-TEST BLOCK PLACEMENT**: `if (!d) { A } else { B }` puts B in the fall-through position; **(L6) MEMBER LOAD ORDER**: the members the image loads last are read last in the source (the shift fields last). Negatives: an expression repeated at each use inside a `||` chain is worse where the image enregisters named locals (the opposite of the clip-test case); the flag form of the block-layout group is inert on short-circuit `&&`/`||` chains.
- **Function-scope POD arrays WITH initialisers give ONE shared EH epilogue; block-scope arrays crossed by an exit duplicate it.** Symptom: in a function with an EH frame, ours has three full epilogues (`ret N` three times, about fifteen instructions more) where the image has one. Cause: two small arrays (`int a[2], b[2];`) declared INSIDE a case, with `goto fin` or `break` leaving their block: VC8 inlines an epilogue at each exit that crosses the scope. Source: declare them at function scope with initialisers (`int a[2] = { x, y };`: the stores sink to where they are needed); a function-scope declaration with a later ASSIGNMENT brings the three epilogues back; `goto fin` against `break` is byte-inert. An image with block-scope stores AND one epilogue has exits that jump to a shared tail, which then depends on the held-zero and EH-state registers. An epilogue-count difference is otherwise a SYMPTOM (a register race, an x87 schedule, the frame size, a cross-jump, a missing EH frame or cookie, a missing inline): over about 300 parked ports this was the only source-fixable cause. **partial** *(321 -> 127 lines with the two rules below)*.
- **`sub eax,K1; je; sub eax,K2; jne` is a sparse `switch`**, with the case reached by the second test falling through first; a port written as an if/else chain with several returns has neither the dispatch nor the single exit. With named field pointers inside one case and the owner pointer at function scope it halved a handler (239 -> 126 lines). The reverse is not progress: an if-chain that LOWERS the masked diff of a function whose image has the `sub` chain (297 -> 287) moves away from the image; keep the faithful form.
- **Idioms whose image form is the library spelling**: the inline length loop (`lea reg,[eax+1]`, the byte loop, `sub eax,reg`) followed by a test of the result is `if (!strlen(s))`, not the byte test `if (!*s)` (192 -> 188); three floats moved as integers are the record assignment of this section ('Three floats copied with integer moves'; 156 -> 150 on a parked port: the rule keeps paying). `fabs`, `fistp` against the conversion helper and `malloc` against `new` are equally real, but on parked ports each was a minor residue under an allocator wall, and the faithful spelling CASCADED (the exact `fld; fabs; fstp` form moved a held constant to another register, 297 -> 303; a named int conversion brought the helper and 68 frame bytes): apply one at a time and keep the diff.
- **A zero loop becomes the `memset` call only with a LOOP-INVARIANT count.** A loop whose bound is re-read from memory in its condition (`for (i = 0; i < v->n; i++) v->p[i] = 0;`) stays a byte loop; where the image calls the library routine write the call (`memset(v->p, 0, v->n);`) (24 lines on one port).
- **A zero-initialiser the image lacks is DEAD when an out-parameter call defines the local before its first read.** Symptom: two strict bytes or one `fldz; fstp [local]` (or `mov [local],0`) more than the image. Source: `float v;` not `float v = 0.0f;` before `Get(id, &v);  if (v < 1.0f)`. This is not the uninitialised-read case (the original never reads the slot undefined), so no deterministic value is owed. Rare: a scan for the extra store found two ports. **held** *(match)*.
- **Small orderings in handlers**: the default result stored BEFORE the base-class call, and a read-only `__restrict` receiver for the notification call, each moved a handler by four lines; a `__forceinline` member holding the whole inner `switch` is byte-inert (caller-against-inlinee structure is not the residue there).

**Frame kind: the EBP frame, the cookie, the EH prologue and the state width**

Each rule micro-proved in the exact harness, one function per cell. **held** *(micro)*.

- **(F1) An EBP or 8-aligned frame comes ONLY from `__declspec(align(8))` or `__m64` locals, `_alloca` and `setjmp`.** `align(8)` gives `push ebp; mov ebp,esp; and esp,-8` whether or not its address is taken; `__m64` the same plus a cookie; `_alloca` an EBP frame plus a cookie; `setjmp` a plain EBP frame. NOT a trigger: `double` or `__int64` locals (address-taken or not), a union holding an `__int64`, an EH frame, a cookie, eight integers live across calls, a by-value record argument, a jump-table `switch`, a simple `__asm` statement naming a local, the qword temporary of a float-to-unsigned `fistp`, two doubles live across a call: all keep `sub esp,N`. The one plain EBP frame seen with none of the four was a function whose body is assembly. One port's aligned frame was reproduced by none of six further batches (by-value short-record parameters, doubles, a projection loop, the exact signature): the list is complete for the micro-tests and **open** for that family; the aligned frame is also an INDEPENDENT residue, which an EH prologue added by another fix can drop (379 -> 457, reverted). A later lead for that family: a real non-trivial element constructor of an array `new` brought the aligned prologue back together with the EH prologue and the cookie ('Emulated objects' below).
- **(G1) A `/GS` cookie appears if and only if a local array has 1- or 2-byte ELEMENTS and a TOTAL size of at least 5 bytes.** `char[4]` and `short[2]` have none; `char[5]`, `short[3]`, `bool[5]`, a three-element `wchar_t` array have one; `int`, `float`, `double` and pointer arrays NEVER, at any size. A record INHERITS the cookie from such a member (`struct { char c[5]; }`); a record of ints has none. The array must survive optimisation (a buffer replaced by scalars leaves no cookie); an initialiser does not remove it. This completes the three-short entry of this section. Read it both ways: a cookie only in the image means a char or short array of five bytes or more exists in the original; a cookie only in ours means the buffer was int or float words (or under five bytes).
- **(G1, the converse lever) Int words instead of a char array REMOVE a cookie the image lacks.** A local record `{ char c[32]; }` became `int c[8]` with the same field offsets and the same record name (so the by-value pass and the decorated names are unchanged), cast at the string call (`strncpy((char *)n.c, ...)`): 600 -> 564; a local whose TYPE had a `char pad[12]` member, as `int pad[3]`: 154 -> 144. A cookie-mismatch scan over all parked ports exhausts this band in one pass. Enlarging a char buffer to explain an equal-total frame whose locals sit 32 bytes off is wrong (278 -> 357 and worse): a shifted local block is not a missing buffer. The same lever on a 2-byte-element array: a local `unsigned short c[3]` (six bytes of 2-byte elements) as `int c[3]`, the read truncating back to the same values, removed the cookie (324 -> 286). Once the scan has been run the lever is EXHAUSTED: two ports in the whole parked set ever had an extra cookie.
- **(G2) A REAL local class object with a virtual destructor brings the EH prologue AND a cookie**, with no char array anywhere; an int-array emulation of the same object brings neither. The EH prologue (`push -1; push handler; mov eax,fs:0; ...`) comes from any local object with an out-of-line destructor, one funclet per destroyed object; EH alone brings no cookie.
- **(G3) A real `new T()` brings the EH prologue and the cookie for HEAP objects too**: with an inline constructor (a short-array clear and an out-of-line call), the new-expression emits its allocation-cleanup states (the funclet calls `operator delete`) and the cookie, without any local object or virtual destructor. `operator new(N)` followed by member stores emits neither.
- **(E1) The width of the EH state stores is not the count** (refined by rule N6 below: the width is decided PER STORE, by whether the upper bytes of the state slot are known zero, not by a fixed class of state; the figures here are the common case). Unwind states of stack objects and of by-value members in a constructor are stored as a BYTE (`mov BYTE PTR [state],N`); allocation-cleanup states of `new` expressions as a DWORD; the first and the closing store are DWORD in both. 130 stack states stay BYTE, 30 `new` states stay DWORD; function size, a nothrow allocator, thiscall and the null check change nothing. An image with BYTE states where ours has DWORD has members destroyed where ours has `new` cleanups: a constructor that visibly cannot throw leaves no allocation-cleanup state at all (rule N3 below).
- **The byte-relevant EH signals are three**: a MISSING PROLOGUE, the state COUNT and the state WIDTH. The count is byte-relevant only through the state STORES of the body: a count difference whose stores already agree is a dead entry (rule N7 below). What a funclet DOES is byte-irrelevant: funclets are separate functions in their own section and the destructor or delete call in the body is a masked relocation. Over 132 parked ports every byte-relevant difference sat in a large port with an allocator wall; no cheap port had one. A scanner for the unwind tables must accept this compiler's FuncInfo magic `0x19930522` (requiring `0x19930520` finds nothing; the `mov eax, FuncInfo` is about 36 bytes into the handler). The width lever interacts with the allocator (inline against out-of-line destructor), and a byte-store detector on the image side can pick a store that is not a state.
- **The `/GS` shadow copy of a pointer parameter is decided by USE ORDER**: a pointer parameter used AFTER a local buffer has been written is copied to a slot at entry, one used only before is not; the type alone does not decide (a 12-byte frame gap was three missing copies). The shadow copy of `this` appears only when `this` is used after a buffer write AND no callee-saved register is free for it; with a free register it is simply moved there.
- **Frame-size composition**, for reading a `sub esp,N` gap: N = the frame cells (locals, spills, temporaries) + 4 bytes of array pad when there is a cookie + 8 bytes of EH record when there is an EH frame; the cookie and the EH prologue themselves are pushes, outside N; an aligned frame adds `and esp,-8` and restores through EBP. Fix the frame KIND first, the size second: a wrong size shifts every offset of the diff.

**Unwind-state numbering (N1-N7)**

A matrix of about seventy cells in the exact harness (locals, scopes, conditionals, loops, temporaries, `new`, statics, inline expansion, `try`), each parsed into its unwind map (state, `toState`, funclet) and its body state stores with their width. They complete 'The unwind state count is a scan' earlier in this section and rule E1 above. **held** *(micro, two spellings per rule)*.

- **(N1) States are numbered in CONSTRUCTION order** (the order the front end enters the regions), not by declaration, scope depth or frame position. Every local object, every temporary that lives across a call, every static-initialisation guard, every allocation cleanup of a `new` and every `try` region takes the next number: `D a; g(h()); D b;` gives a = 0, the temporary = 1, b = 2, although the temporary is gone before `b` exists; the two arms of an `if` are numbered in source order.
- **(N2) `toState` is the innermost state still LIVE when the object is constructed**, by live range, not by lexical block: overlapping lifetimes chain (`D a; D b;`: 1 -> 0), disjoint ones are siblings that both return to -1 (`{ D a; } { D b; }`, and `if (c) { D a; } D b;`); a guard whose initialiser ran before a local was born is its sibling, one that runs after it is nested in it.
- **(N3) A state exists if and only if a THROWING point lies in the live range.** A `new` whose constructor visibly cannot throw has no cleanup state; a discarded temporary with no call between its construction and its destruction has none; a static with a constant or non-throwing initialiser has no guard state. Read both ways: a state the image has and ours lacks means a call inside that lifetime (or a constructor the original compiler could not see through), not just an object.
- **(N4) Inline expansion adds one state per EXPANSION**, numbered in order, even for a static whose guard word is shared by all expansions. A function-local static blocks plain `inline` expansion (the call stays; `__forceinline` is needed). The member-unwind states of an object with an out-of-line constructor belong to that constructor and destructor: the caller has ONE state for the whole object.
- **(N5) An entry with no funclet is a `try` region** (explicit or compiler-made); statics give guard-reset funclets and `new` gives delete funclets, never empty entries.
- **(N6) A state store is a BYTE if and only if the upper bytes of the slot are known zero**, that is, no -1 has been stored since the last full-width store of a small value. Nested states in one block: DWORD 0, then BYTE 1, BYTE 2. Sibling scopes, `if` arms and loop iterations store -1 between regions, so every store there is a DWORD. The first store is always a DWORD; a guard state may be stored from `al` (the guard mask reused as the number). An image with BYTE stores where ours has DWORD (or the reverse) therefore differs in the NESTING of its lifetimes, not only in the kind of object.
- **(N7) Dead entries are metadata.** A state can be allocated in the map and never stored in the body, and two states can share one funclet (one frame cell reused by a temporary in two regions with different enclosing states), which leaves holes in the stored sequence. The map lives in its own data section and the handler's reference to it is a masked relocation.
- **Split every state-count difference before chasing it: compare the SEQUENCE of body state stores, not `maxState`.** Stores equal: a dead-entry port; the count is byte-inert and the residue is somewhere else (on one such port a 149-line diff contained no state store at all, only register and frame-cell hunks): do not chase it. Stores different: a class-view wall (a base or member with a real destructor, lists, static guards, a real `new` where the port calls the allocator by hand); each missing state names a missing object, and the fix is entangled with the frame and register allocation of the whole function, and with every other user of that class view. It is entangled, not unreachable: two such walls closed once the WHOLE view was rebuilt ('Class views rebuilt from what the image constructs' below).

**Emulated objects**

- **Replace emulations by the real types: the frame kind comes back with them.** A port written from a decompilation stands in for objects with what the listing shows: an `int v[8]` whose first word is stored with a vtable address, byte-offset casts, an `int` parameter that is really a pointer, `operator new(N)` plus stores, a hand-written construct or cleanup loop. Each one loses whatever the compiler attaches to the real construct. A real local object with file-local inline constructors, in place of the int-array-with-vtable form, recovered the EH prologue and the cookie together (961 -> 877); an `int` parameter retyped to the `const char *` that the matched callee's decorated name shows (callers cast, the symbol row renamed) took 250 -> 179; `new char[N]` plus a count word plus a zero loop, rewritten as `p = new T[256];` and `delete[] p;` with the constructor and destructor only DECLARED in the class view, recovered the prologue, the cookie and both vector-iterator calls byte for byte modulo registers (846 -> 664). To rank candidates, count the emulation sites INSIDE each parked function's body (per-file counts over-attribute, and address-of casts in layout assertions are noise) and weight by the diff and by frame-kind flags (EH, cookie, EBP: image against ours).
- **A declare-only constructor and destructor keep the array `new` clean for duplicate checks**: the compiler emits no constructor or destructor body in the caller's object, only the vector-deleting-destructor COMDAT, which a comparison that keeps the best emission of a COMDAT counts once. An inline destructor would add a second emission of a function matched elsewhere. The cleanup funclet of an array `new` is `operator delete[]` on the raw allocation temporary. Callers of the image's vector iterators are few (four parked ports): scan for them once.
- **Substituting real types at emulation sites is BYTE-INERT where the residue is register allocation.** VC8 scalar-replaces a small struct and an int array of equal layout identically (`int p[2]` against `struct { int x, y; }`, an invented four-int box against the real rectangle type), and loads a member by name and through its raw-offset cast identically (`*(int *)((char *)this + 44)` against `this->e.x`): five large ports rewritten that way kept their diffs to the line. The real-type lever pays ONLY where the residue is frame kind (a missing EH prologue, a cookie, an aligned frame, vector-iterator calls). Classify the residue first (prologue and frame figures against register roles); substitute for fidelity if wanted, not as a matching attempt. Two traps of the rewrite: a parameter named like the member it now spells (`int e` against `this->e`) shadows it, and a macro parameter collides with a member name in its expansion.
- **An array `new` uses the vector-constructor iterator only for a NON-TRIVIAL element constructor.** An empty user constructor (`T() {}`) is treated as trivial: no iterator call, no EH. The real constructor (`T() { y = 0; x = 0; }`) on `new T[n]` recovered a base-pointer, 8-aligned prologue with the EH registration and the cookie exactly as in the image, on two ports of one painter family whose aligned frame no micro-test had reproduced; its zero stores are dead-store-eliminated, so the body shows no trace of it. Both ports then got WORSE (1245 -> 1362 and 1660 -> 1801): the frame came out about eight bytes too large and the register web of the whole body shifted. That wall is therefore two residues: a RECOVERABLE PROLOGUE and a body-allocator residue; the constructor is adopted only together with a re-alignment of the body's frame and registers, never alone. A constructor the image calls BARE (no null test, no state) at one site while the port inlines it needs per-site inlining control; a real out-of-line one, with `throw()` too, lost there (+84).
- **A BUILD THAT LINKS IS NOT A BUILD THAT RUNS: the byte match, the second build's link and the identical-set check do NOT see a runtime crash.** The array-`new` rewrite above passed all three and SEGFAULTED at start-up in every scenario of the second (native) build. **Run the in-game smoke before adopting ANY change that adds EH structure, an array `new`, constructor/destructor calls or a library call the port did not have.** This is the same gate as 'a byte-identical function can still break the native run' earlier in this section, for a different cause.
- **Diagnose such a crash under a debugger; do not guess the cause from the change.** The first reading blamed the new construct (the array iterators of the runtime, the constructor's alias row) and the rewrite was reverted. Wrong: run the SMALLEST failing scenario under `gdb` and read the backtrace. It showed one frame repeated more than 260,000 times, in a runtime wrapper, called from start-up code that had nothing to do with the rewritten function. The array `new` was sound (the other compiler emits direct constructor and destructor loops over the counted allocation) and was re-adopted unchanged once the wrapper was fixed. Check that a remaining scenario difference is PRE-EXISTING (the old source with the same fix gives the same frames) before charging it to the change.
- **THE NATIVE-RUNTIME WRAPPER TRAP: a wrapper that calls the plain symbol it replaces can be bound back to itself.** Where the second build binds the original's CRT symbols to its own wrappers through a symbol binder (`memset` -> `rt_memset`), a wrapper written `void *rt_memset(void *d, int c, size_t n) { return memset(d, c, n); }` is harmless only while no bound source references the plain symbol. The FIRST reconstructed source that does (here the explicit `memset` call that replaced a zero loop) makes the binder alias the plain symbol to the wrapper, the wrapper's own call resolves to itself, and every caller of the wrapper recurses until the stack is gone, including callers that ran fine the day before. The match, the link and the identical-set gates cannot see it; any start-up scenario does. Fix: the wrapper is SELF-CONTAINED (a byte loop) and never calls a symbol the binder may alias to it. Audit every wrapper of that form once, before the next source change trips another.
- **THE NATIVE BINDER TRAP: an `extern` data declaration that loses its address annotation can be bound to a FUNCTION.** A second 'links but does not run' case. Where the second build binds data symbols to image addresses recorded in a trailing comment on the `extern` declaration (`extern List *g_list_1234; // 0xADDR`) and otherwise falls back to an address-looking SUFFIX of the name, a declaration that is MOVED to another file or rewritten WITHOUT its comment is bound by the suffix: a data name that carries the address of the function it was first seen in gets aliased to that function. The pointer then reads the function's code bytes (a 'pointer' whose four bytes are a prologue, `55 89 e5 53`: `push ebp; mov ebp,esp; push ebx`) and every scenario that touches it crashes at the first use, long after start-up. The match, the link and the identical set cannot see it. After ANY move of a definition between files (the static-callee move below is one), read the binder's report for a data line with the right address for each moved name (an alias line to a function is the bug), carry every address comment with its declaration, and run the in-game smoke on a scenario that reaches the moved code.

**Class views rebuilt from what the image constructs**

Two matches in a row (360 -> 0 and 400 -> 0 lines) on ports where hundreds of spelling-lever attempts were spent and three independent verdicts said 'cannot be matched'. **held** *(two matches)*.

- **A LARGE diff on an 'unmatchable' port may mean the original constructs REAL classes where the port emulates them.** The emulations: a manual allocation plus stores (`operator new(N)` and a stand-in construction function called on the block), a function `Build(this, a, b)` standing in for the constructor itself, a raw pointer where the original has a small holder member, a class the port does not have at all. No spelling lever reaches these, because the EH states, the state widths, the frame cells and the store schedule all come from the constructs. Inventory the image FIRST, before any lever: (1) every unwind funclet's target (a destructor names a base, a member or a local object; `operator delete` names the allocation cleanup of a `new`), (2) every constructor call with the allocation size pushed before it (the size identifies the class: three sizes are three classes, where the port had two and one manual allocation), (3) the state store before each. Then rebuild the view to that inventory: the real BASE class (its destructor is the first unwind entry of a constructor), real MEMBERS with their real constructors (list and buffer members with inline constructors, not zero stores), every allocation as a real `new T(...)` in the ORIGINAL'S CONSTRUCTION ORDER (rule N1: the order fixes every state number, 29 consecutive states on one constructor, byte-width stores included), real inline member objects for small records (a text buffer, a short triple, an integer pair, a pointer holder), and the real CONSTRUCTOR in place of the stand-in function (its symbol row renamed in place to the constructor's decorated name; member-object constructors the image emits out of line get alias rows).
- **Rebuild without touching what is matched.** A class view is shared: prefer the real shared header selected by file-local switches (a macro defined before the include that turns a declare-only constructor or an out-of-line destructor on for this translation unit) to an edit of the header, and never add a member to an invented view of a class that has a real symbol row (a second build leaves it unresolved). Check the IDENTICAL SET, not the count, after EVERY edit, including every header edit (a header edit can lose a match in another file); a checker that substitutes one scratch object into the comparison reports lost and gained addresses without touching the tree. Expect a parked neighbour in the same translation unit to move by a few lines (one went 1501 -> 1514 from the added declarations): record it.
- **The rebuild also removes real defects**: a two-word record of which the emulation initialised one field became a member object whose constructor initialises both, which removed an uninitialised read the port had carried.
- **A pointer-HOLDER member changes store scheduling only when later THROWING members follow it.** Image: the pointer store precedes an x87 load (`mov [this+12],eax; fldz`), ours has them swapped. With a raw pointer member and nothing throwing after it, the raw and the holder form (`struct Holder { T *p; Holder() : p(0) {} };`) compile identically; after two list members whose inline constructors call array `new`, the raw pointer gives `fldz; mov [this+12],eax` and the inline holder the image's order (2 -> 0). A context-dependent live-range rule, not a pointer-type rule: a micro-test without the following members shows nothing. **held** *(micro, match)*.
- **Initialising an argument record INSIDE a `new` constructor argument moves its stores after the allocation.** `p.Init(0x32); q = new Thing(p);` stores the pair before `operator new`; `q = new Thing(p.Init(0x32));` (with `Init` returning the record) stores it after, inside the allocation-cleanup state, as the image does (46 -> 42). **held** *(micro)*.
- **An unsigned-int-to-float conversion is the ORDINARY CAST.** `fild`, a sign test and a conditional DWORD `fadd` of 2^32 is `(float)(unsigned int)n`; a hand-written correction (`long double v = (int)n; if ((int)n < 0) v += 4294967296.0;`) gives a QWORD `fadd` and a different compare at equal instruction counts (309 -> 304). Before emulating a conversion sequence by hand, compile the plain cast.

**A static custom convention, lifetime intervals and switch order**

- **Arguments in TWO REGISTERS with CALLER cleanup and a plain `ret` is a `static` function of the CALLER'S translation unit, not `__fastcall`.** `__fastcall` is callee-cleaned (`ret 4` with one stack argument); here the caller does `add esp,4` after `push a; mov ecx,c; lea edx,[b]; call f` and the callee ends in a bare `ret`: the compiler chose its own convention for a function with internal linkage whose callers it all sees. Source: MOVE the definition into the caller's file as a `static` function (the standalone file is deleted, with its objects and listings: section 14, 'The stale-object trap'); no annotation reproduces it from another file. **The register homes follow the FORMAL parameter order**: the LAST pointer formal goes to `ecx`, the second-last to `edx`, the first to the stack, so `ecx` = c, `edx` = b, stack = a is `static void f(A *a, B *b, C *c)`; the reverse order gives the wrong permutation with everything else equal (104 -> 98 lines at the entry stores alone). **The convention follows the BODY'S allocation, not only the signature**: on a static callee that is itself still far from the image, a formal reorder made one caller better and the other worse (6 -> 4 and 6 -> 8) with inconsistent homes; the callee must close first, its callers after it. A port recorded as 'a custom calling convention, not reachable' is this case until the move has been tried. **held** *(match)*.
- **THE NESTED-BLOCK LIFETIME LEVER: end a block early and the frame-cell allocator pairs the cell with a later local.** Symptom: one frame cell more than the image (five 32-byte cells against four), and the image shares one cell between a local of the first loop and a local of the second. Cause: the first local is declared at the top of its loop body and so lives to the end of the body, overlapping the temporaries that the image's cell-mate is paired with instead. Source: wrap the declaration and its LAST use in a nested block that closes before the rest of the loop body (`for (...) { int kind = 0; { char name[32]; Read(name); if (!Cmp(name, "A")) kind = 2; } switch (kind) { ... } }`): the lifetime interval shrinks and the allocator ('The frame cell allocator': the most recently created cell whose occupants are all dead) reuses the cell for the later local. 98 -> 6 lines in one step. Declaration order, combined declarations, function scope, an extra enclosing block, and direct against copy initialisation were ALL inert on the same port (fourteen variants): **the axis is the lifetime INTERVAL, not the position of the declaration**. Read the image's cell sharing first (which two objects use one offset) and shape the blocks so their lifetimes are disjoint. **held** *(match)*.
- **A buffer used by two sibling loops at ONE offset is one function-scope local**, re-initialised in each loop (`char tok[32]; ... tok[0] = 0;`), not a block local per loop: two block locals cost two cells (frame 0xe0 -> 0xc0, 267 -> 134). The converse of 'two locals at one offset mean sibling blocks': there the two objects had different types or sizes; here the image's uses are one object.
- **`char c[N] = "";` zeroes all N bytes; `c[0] = 0;` stores one.** An image with a single byte store to a buffer has the explicit assignment (the initialiser form is the run of dword, word and byte stores recorded in the previous subsection, plus a zero register).
- **A small dense `switch` dispatched by a `sub reg,1; je` chain lays its bodies out in REVERSE source order and numbers its unwind states in SOURCE order.** Image: the tests run 1, 2, 3, the bodies sit 3, 2, 1, and the allocation-cleanup states of the `new` in each case are 3, 4, 5 for cases 1, 2, 3. Source: the cases written ASCENDING give both the numbering (rule N1) and the reversed layout; written descending to 'match the layout' the numbers come out reversed and neither residue closes (6 -> 0 with the ascending form). Include a case the image tests although no path selects it: a dead case is still a test, a body and a state. This is the chain form only: a jump-table `switch` keeps 'write the cases in address order' (the previous subsection), where the body addresses give the source order directly (decoded from a byte-index table on one port: 1784 -> 1721).
- **Two sibling branches that test the SAME function-static are one `||` condition.** `if (A) { static int id = Find(..); Send(id); } else if (B) { static int id = Find(..); Send(id); }` has two guard bits and two states; the image has one guard word and one state for both: `if (A || B) { static int id = Find(..); Send(id); }` (the largest single lever on that port, about 150 lines). Count the guard bits and words of the image against ours before anything else in such a function (rule N4: an inline helper expanded at three sites accounts for three).

**Declaration birth order**

- **Independent stores are scheduled in the BIRTH ORDER of their locals' declarations.** Symptom: the prologue's store sequence (or the first call's push sequence) is the image's in a different order, everything else equal. Source: reorder the DECLARATIONS to the image's store order (`char kind = 64;` before `Queue *s = ...;`), never the statements: statement reorders cascade through the register web (6 -> 10 and 2 -> 110 lines on two ports). 14 -> 4 strict bytes and 79 -> 77 lines on the two ports that had it. Mechanised (the image's prologue store and first-push sequences against ours, verdict SAME / ORDER / DIFF): 4 of 325 parked ports had a pure ORDER difference, two of them large multi-residue ports; more than half already agree; no constructor has it. Run the scan once; the band is small. **partial.**
- **A declaration-order gain is not automatically this rule.** Declaring the high half of a split word before the low half gained four lines on one handler; the same two functions in a micro-test compiled INSTRUCTION-IDENTICAL. That gain is an allocation effect of the port, to keep but not to generalise (see Method).
- **DECLARATION INTERLEAVING: declare a computed local just before its first use, where the image computes it.** Symptom: the image computes each derived value just in time (`x0, y0, A, x1, B, y1, C`) and the port declares them all up front (`float x0 = (float)(x - 10.0), y0 = ..., x1 = ..., y1 = ...;`), with frame cells missing or surplus. Source: move each declaration down to the statement before its first use (`A = P(m, x0, y0, z); float x1 = (float)(x + 5.0); B = P(m, x1, y0, z);`). Two ports of one family: 806 -> 725 and 744 -> 727 lines, the frame from 16 bytes off to the image's size class. It differs from the birth-order rule above: that one orders independent STORES, this one places COMPUTATIONS and the cells they spill to. Measured around it (micro and ports): named float offset locals that feed an inlined projection used at several sites make VC8 spill the partial products to QWORD temporaries; the width does not depend on a double literal in the offset or on the interleaving; writing the offsets as inline cast arguments removes the qword cells in the micro-test and shrinks the frame TOO FAR on the ports (751 and 750: worse than interleaving, the second worse than its start); naming a shared product over-CSEs (842); reordering the terms of the sum diverges the whole x87 schedule (781 to 965). A qword temporary spill does not cause an 8-aligned frame (rule F1 holds: one image spills a double to a qword cell in a plain frame). **It is a lever of that one family only.** Mechanised over every parked port (a scan for locals declared with an initialiser three or more statements before their first use, joined to a fresh survey: about 200 candidates), it paid nowhere else: the wins were multi-site projection webs with NAMED float offsets fed to a call. In a deep x87 arithmetic web the declaration order IS the schedule: the image computes the values in a fixed interleaved order, and moving one declaration re-times the whole web (14 -> 30, 201 -> 253); in register-role ports the move is inert, or shrinks the frame toward the image while the schedule cascades (435 -> 473). The 'our frame is larger' trigger is usually NOT this lever either: of 29 such ports the cheap ones all had the surplus cell from a POINTER the image keeps in a register (or computes inline into a reused one) where ours spills a separate local, from an index-against-pointer loop form, or from an EH or aligned frame kind. False positives of the scan: loop-invariant values, static guards, constant-zero initialisers. **partial** *(two ports)*; **no effect** elsewhere.

**By-value records: arguments built in place, returns consumed in place**

Micro-tested in the exact harness; applied on about a dozen ports, no match. They refine the two by-value entries of the previous subsections ('A by-value record argument built IN PLACE...', 'A by-value return chained into a member call...').

- **A DEDICATED DERIVED TYPE with only a CONVERTING constructor builds a by-value argument in place.** Symptom: the image reserves the record in the outgoing argument area and stores its members through the stack pointer (`sub esp,12; mov eax,esp; fstp [eax]; fstp [eax+4]; fstp [eax+8]`), ours builds a temporary and copies it as integer words. Source: `struct VArg : V { VArg(const V &o) : V(o.x, o.y, o.z) {} };`, the callee's view declared `F(VArg, int)`, and the call written with the BASE expression, `F(v, n)` or `F(V(x, y, z), n)`: the IMPLICIT base-to-derived conversion constructs the record directly in the argument area with member-wise stores, and no copy constructor exists anywhere. The explicit spelling `F(VArg(x, y, z))` on such a type does NOT work (two temporary cells, integer copies). Never put a user copy constructor on a SHARED type for this (the hazard of the earlier entry); on the dedicated type a copy constructor works too but is not needed. Measured around it: it works for larger records (a 32-byte name record); a direct temporary can beat a named derived local by removing the local's lifetime from the schedule (81 -> 37 on one port, not predicted by the micro-test); retyping locals that ALREADY exist improves (877 -> 533, 443 -> 339), adding new derived locals enlarges the frame (247 -> 478); a nested base temporary can still move the frame of a large function; the gain is per port, read from the image (eight ports improved, four got worse or did not move and were restored). **held** *(micro)*; **partial** *(eight ports)*.
- **No compiler-specific guards in reconstructed sources: when a by-value form seems to need one, find the guard-free form.** A record view whose user copy constructor had to be hidden from the second compiler (`#ifndef __clang__`) was refused under that standing rule although it improved its port (520 -> 492); the converting-constructor form above then gave more with no guard (520 -> 360, together with the fixes it exposed). A guard marks a form whose semantics differ between the two builds: treat it as a sign that the source form is not yet the right one.
- **When a callee's parameter type changes, rename its symbol row IN PLACE.** The decorated name carries the parameter type, so the view row of the callee's address gets the new name (not a second row beside the old view); the row of the real definition stays, and another caller's view of the same address is an additional alias row. Choose the new type's name against the layout checker: a name that another file already defines differently is flagged as a class defined twice.
- **MEASURE the second build's ABI for every such call; do not assume it.** From the second build's disassembly of the callee and of each new caller: the size of the call area, the offset of every argument, the slot of the hidden return pointer, that the pointer comes back in the accumulator, and the callee-pops count (`ret N`). Measured here: clang in the Microsoft ABI passes a record with a user copy constructor in the outgoing stack memory, as VC8 does (the callee read a 12-byte record and the following integer at the same offsets and ended in `ret 0x10`); a record-returning member took the hidden destination as its first stack argument, returned it in `eax` and popped it with the others. Conditions for adopting a view: the new type is not used by any platform-facing or other-compiler-facing function, the call area and offsets are the same before and after, the second build's alias and duplicate lists are unchanged, and the scenarios run frame-identical. Adding a copy constructor is never the proof by itself.
- **An inline `this` accessor on the returned temporary keeps the hidden-return pointer in `eax`.** Symptom: after a call that returns a record by value the image reads the fields through the returned pointer (`fld [eax+4]; fld [eax]`), ours through the frame (`fld [esp+N]`). Source: `V *d = r->F(v).P();` with the class-inline `V *P() { return this; }`; a named result (`V d = r->F(v);`) gives the frame-relative loads. It replaces the address of a temporary, which is not valid source. Only for a record WITHOUT a destructor (with one, the temporary is destroyed at the end of the statement and the pointer dangles): use the several-reference-argument form below. Check the accessor's name against the file's macros (a function-like macro of the same name breaks the call). **held** *(micro)*; **partial** *(469 -> 446, 193 -> 191)*; the by-value spelling against an out-pointer plus a member call on it is neutral where the image's `mov ecx,eax; call` is all there is to see.
- **A returned record used inside a MACRO argument may need a name before the macro.** A macro that expands to more than its argument (a guarded static initialisation, a second call) evaluates the argument where the expansion puts it: chaining the record-returning call inside it ran the macro's static initialisation BEFORE the call (469 -> 741); `V d = a.Minus(b); SET(d);` restored the image's order (call, static, use: 448).
- **A record is returned DIRECTLY into a member through an inline containing constructor.** `struct W { int r; V v; W(R *q, int i, int *e) : v(q->F(i, e)) {} };`: VC8 computes the member's address (`lea edx,[esi+4]`), passes it as `F`'s hidden return pointer and emits no copy afterwards; the caller reads `w.v.x` in place. Use it where the image has ONE object that holds both a result flag and the returned record: a separate result local assigned into the wrapper copies through a temporary (245 -> 251), separate directly-initialised locals leave a 4-byte frame gap (245), the containing constructor gives 242, and reading the call's own argument inside the constructor (`v(q->F(q->index, e))`) 234. A plain `V d = q->F(i, e);` also returns into its own cell: the containing form matters when the containing object supplies the frame cell or the lifetime. **held** *(micro)*; **partial** *(one port)*.
- **Which register is pushed for `&v` right after the call that filled `v` follows a PHASE, and named locals reach both forms.** `{ B v = s.Value("X"); F(&v); }`: the image pushes `eax` (still the hidden-return pointer, equal to `&v`) at some sites and re-derives the address (`lea ecx,[v]; push ecx`) at others. Six independent named-local blocks in a row give `lea, eax, lea, lea, eax, lea` (the second and fifth reuse the accumulator), with every unwind state intact; declaring several results before their calls shifts the phase by one site; reusing ONE named local through assignment gives the address load at every site; copy- and direct-initialisation are identical; the same for 8-, 16- and 32-byte records with virtual destructors; sibling temporaries share frame cells where their lifetimes allow. **A micro-test with ONE site always starts with the address load**, which is why this was first recorded as an allocator wall: test such forms with several sites in a row. An image `push eax` can therefore be the original's named local; it needs no temporary. **held** *(micro)*.
- **A full-expression temporary does NOT delete or renumber unwind states** (correction: an earlier round recorded that passing the temporary itself 'deletes the named local's EH state and shifts every later number'). In the matrix the named local and the temporary bound to a reference get the SAME state before the consumer and are destroyed after it; on the real site the state and the whole unwind table were identical before and after the rewrite. The larger diff that was blamed on EH (1535 -> 1554) was a SCRATCH-ROTATION CASCADE through the calls that follow. Never reject a direct temporary on EH grounds: measure it per site and per subset.
- **One inline call with SEVERAL REFERENCE ARGUMENTS holds the returned pointers in registers AND destroys the temporaries after all uses** (correction: this was recorded as not expressible for two compilers). Image: the returned pointers of several record-returning calls are kept in registers (`mov esi,eax`), used for member calls, and the temporaries are destroyed afterwards, one unwind state each. Source: `static __forceinline void Read3(P *p, B &a, B &b, B &c) { p->x = a.Number(); p->y = b.Number(); p->z = c.Number(); }` called as `Read3(p, REF(B, s.Item(0)), REF(B, s.Item(1)), REF(B, s.Item(2)));`, where `REF` is the temporary-reference macro (a cast of the temporary to a non-const reference in a spelling both compilers accept). VC8 evaluates the arguments RIGHT TO LEFT (constructing items 2, 1, 0), keeps each returned pointer in a register, makes all the member calls and then destroys the temporaries with states 0, 1, 2. It is valid for the second compiler with no guard, because every temporary lives until the end of the consuming full expression. To get a given CONSTRUCTION order, reverse the helper's formal order (for the second value constructed first: `Read2(p, B &second, B &first)` called with the two in that order). The forms that still fail are the earlier ones: named records (frame cells and `lea`), an inline `this` accessor or a free pointer function on ONE temporary used in a later statement (the destructor runs at the end of the statement: dangling), a non-const reference variable bound to the temporary (rejected by the second compiler), the cast alone with the use in a later statement (no lifetime extension). **held** *(micro)*; **partial** *(278 -> 165 on one port, with the direct forms below)*.
- **Direct single-use temporaries are SUBSET levers, not a universal form.** A named record used once (`B t = s.Item(0); p->n = t.Number();`) against the direct temporary (`p->n = REF(B, s.Item(0)).Number();`), a direct copy-out (`REF(B, s.Value("X")).CopyTo(d)`) and the reference-argument reader above are all safe in a second build and keep lifetime and unwind states; whether each PAYS is decided by the allocation phase that follows it. Measured: replacing one named single-use record by the direct temporary was the largest single gain of two rounds (165 -> 127, instruction counts unchanged), while a record used several times must stay named; replacing every site wholesale was worse than the selected subsets on each port; on one reader every direct or nested group was worse; on another the image wanted the OPPOSITE direction (four named locals in place of direct calls, 1799 -> 1785). Named temporaries where the image has none add a frame cell and an unwind state each. Choose by subset search (Method, below), not by preference for one spelling. **partial** *(about ten ports)*.
- **The bounds-checked accessor form that pays is a named count local plus a PRESET result.** `f = (i < p->n) ? p->data[i] : 0;` against `{ unsigned m = p->n; f = 0; if (i < m) f = p->data[i]; }`: the named count forces the image's register compare (`cmp ecx,edx`, not `cmp ecx,[eax+12]`; rule SH2 of `vc8-regalloc.md`) and the preset beats the ternary and the if/else forms (8 -> 6 lines); a named temporary for the ELEMENT is catastrophic (79 to 135 lines: it adds frame and, with a destructor, EH).


**Two walls turned into tables**

- **The parameter-home rule, complete (P1-P3).** A parameter's home is reusable as a local's frame cell if and only if (P1) the parameter is READ in the final code (a home whose reads were all eliminated is not offered), (P2) its first use is UNCONDITIONAL (a guard test counts as a use), and (P3) its last MEMORY use is at or before the birth of the first stack local (a parameter promoted to a register at entry frees its home at once). A single local takes the LOWEST reusable home; constant locals fill homes that are dead at entry in reverse birth order, lowest first. This closes the 'Open: parameter homes' clause of the frame cell allocator and refines 'The parameter-home rule' of the previous subsection (its 'not first used inside a conditional region' is P2). **held** *(micro)*.
- **x87 operand order (X1-X6).** (X1) Operands in aggregates or globals are loaded in DESCENDING address order; (X2) scalar stack homes in ASCENDING order; (X3) the term order of a commutative sum equals the LOAD order, whatever the source order; (X4) subtraction keeps the source order; (X5) a member copied to a local, or a `double` local, is inert in isolation; (X6) all six spellings tried of a dot-product helper compile IDENTICALLY. A port's product order is therefore set by the function's global schedule: helper-form and named-local levers are inert when VC8 forwards them, and only a change that moves the loads (an owner re-read, a reload across a call, the node counter of the earlier section) moves the order. **held** *(micro)*.
- **The held-constant table** (when a zero or another constant gets a register, by use count, member offset, first use, competing constants and pressure) is in `vc8-regalloc.md`, 'The held-constant table'. It replaces the flat 'four uses across calls' threshold, and like the rules before it predicts the image without being a lever.
- **The register-role table** (which callee-saved register each held value takes: surviving uses across calls plus a bonus of 2 for an independent entry-load local and 1 for `this`, assigned in the order `esi`, `edi`, `ebx`, `ebp`; ties, coalescing, thresholds, the loop counter first) is in `vc8-regalloc.md`, 'The register-role table'. Its dials (an entry load promotes, a compute-local or late load demotes) are the first allocation levers with a stated reach; they closed none of six role-swap ports.
- **The cheap-band shape rules** (the load order of compare operands, the scratch rotation across calls, where a shared exit tail is hosted) are in `vc8-regalloc.md`, 'Shape rules of the cheap band': predictive only.
- **x87, continued (X7-X11; a matrix of about 320 generated float functions).** (X7) Fold against pop is a STACK-DEPTH decision: `H - m` with `H` already on the stack is `fsub [m]` if and only if `H` is `st(0)` at that point, otherwise `fld [m]; fsubp st(N)`. (X8) The GENUINE storage class of the operands selects the form: member operands fold the second operand (`fld; fsub [m]; fmul [m]; fadd [m]`), named-local and global operands load both and pop (`fsubp`, `fmulp`, with `fxch`); a forwarded copy does not change the class. (X9) NAMING a subexpression gives it a memory home (`float q = a * b;` then `fstp [q]` and `fadd [q]`); named locals never create stack pressure, at nine live values either: only unnamed temporaries live on the x87 stack. (X10) A CALL empties the stack: a shared subexpression is re-read from its home across a call and kept on the stack without one, and two identical expressions are computed once even unnamed. (X11) A non-commutative chain associates left to right in source order; swapping the two PRODUCTS of a difference is a sign flip, not a lever (a sweep that 'found' the fold that way was unusable). Limits: 64 isolated variants of a difference of products compiled identically and the interleaved schedule of the image never appeared in isolation; cast placement (all 15 parenthesisations of `(double)`) and factor order are inert. A residue of one fold against one pop is the whole function's live-value schedule: **no effect** from any local spelling. The storage-class rule X8 is NOT an independent matching axis in full functions: a comparator over 316 parked ports (171 with at least four x87 instructions in the image, 25 with an explicit fold-against-pop mismatch) found NO port whose fold/pop difference stands alone; every one carries sibling spill, order, `fxch` or frame hunks, and re-spelling the storage class on three of them was inert or worse (232 -> 267, 9 -> 14: the image there holds unnamed terms on the stack across a float-to-int helper call). A fold the image takes from a spilled cell (`fadd [cell]`) where ours takes it from the stack (`fadd st,st(1)`) stayed at five lines under four spellings. Ports in which the image builds a by-value record of floats directly in the ARGUMENT area belong to the user-copy-constructor family: flipping the parameter type there is unsafe for a second build that shares the type, unless a dedicated derived type carries the constructor.

**Method: what the diff cannot see**

- **The comparison masks every call target, so a callee view with the WRONG NAME and the right signature is invisible to it.** Replacing a port-invented helper by the real image function is then fidelity only (the right callee for the link and for later matching): five such fixes changed no diff. A fix cuts the diff ONLY when the real signature or convention differs (parameter count or type, a by-value against a pointer return, thiscall against cdecl, a base-class `this` adjustment). The safe form is a minimal file-local `class RealName { public: ... };` when the real header is not in the translation unit. Scan: parked ports that call an external symbol with no symbol-table row (124 of them), joined with a fresh diff, ascending.
- **Do not 'fix' an invented view to an arbitrary row's signature.** Over those ports, more than half had ZERO call-setup differences, and the whole 2-32 line band had identical setup (pushes, `sub esp`, `ecx`, the hidden return pointer, a chained `mov ecx,eax`): the invented views were chosen so their call code already matches, and the signature recorded for the address often does not. Resolve the real function from its MATCHED CALLERS, especially at heavily aliased addresses. Push-count and `ecx` signals of a call scan are false positives for ternary arguments, nested-call arguments and virtual calls through a register.
- **A wrong view that changes the PRECISION: an inline helper in double where the image calls one real function in float.** Symptom: several files each define a file-local `static void Project(Cam *c, double x, double y, double z, Pt *o)` that does the arithmetic inline in double (`fld qword`, `fmul qword`, many float-to-int helper calls per function), while the image at those sites pushes a small record and CALLS one out-of-line function that is already matched elsewhere and computes in float. Here the masked call target is not the whole difference: the argument build and the missing arithmetic are. Source: keep the helper's name and make its body a wrapper that builds the argument record field by field and calls the real function through a cast between two views of the same layout (`V3 v; v.x = (float)x; v.y = (float)y; v.z = (float)z; ((RealView *)c)->Project(&v, (int *)o);`). Three ports: 1577 -> 1520, 1115 -> 988, 1076 -> 1062. Apply it to the WHOLE family of files that duplicate the helper, then check each site: one painter of that family inlines the straight-line projections (in float, through a local inline helper) and calls out of line only in its loop, so the wrapper was right for two files and partly wrong for the third (1809 -> 1660 with the inline float helper restored). The same family held a second inline: a matched one-line forwarder (`void Notify(...) { if (flags & 1) owner->Handle(this, ...); }`) that the image inlines at each site, which shows as a call to the INNER callee with one argument more than the port's call; writing it inline raises the diff alone and pays only with the surrounding structure.
- **Most file-local helpers DUPLICATED across files are correct views: the image inlines them too.** A duplicate scan (every static or inline function, local-class method and expression macro, comments stripped, tokenised, clustered by token equality and then by similarity) found 2,404 helpers and 199 clusters spanning two or more files; outside the one family above, NONE was a call the image makes. Decide per cluster from the image's CALL TARGETS in the functions that use the helper: runtime math and allocation helpers only (the float-to-int helper, `cos`/`sin` intrinsics, a `realloc`) means the image inlines the helper and 'restoring the call' would ADD a call it lacks; a real out-of-line address at those sites means a wrong view. The trivial clusters (zeroing constructors, an iterator's validity test, a colour pack) are everywhere and byte-inert.
- **A masked diff of 0 is not a match, for a second reason**: the masked diff hides JUMP targets too (one port: masked 0, raw 4: identical instructions, two different branch targets). Measure raw as well; raw figures are comparable only at equal instruction counts. The cause on that port was the extent: its last `ret N` spans the address taken for the next function's start, so 'end = next start' overshoots into padding and the auto-cut truncates at an earlier `ret`. Give the raw comparison the hand-found end; the two displacements left were a cross-jump hosting difference (`vc8-regalloc.md`, 'Shape rules of the cheap band').
- **Extent bleed.** If the function-start list is incomplete, the image range of a port runs into the NEXT function: the diff and every count taken from it (epilogues, calls) are inflated (a '199' that is really 5; 202 strict bytes that are 31 real lines). Rank by an AUTO-CUT diff (cut the image at the length of our function), not by strict bytes, whenever our object is longer than the image function; the auto-cut in turn UNDER-counts functions whose case bodies lie past an early `ret`. Neither figure alone ranks a worklist. A STRUCTURAL-GAP survey (image instruction count minus ours, at least six) has the same contamination from the other side: jump tables and EH data disassembled as code inflate the image count (a '+55' that is 41 against 40, a '+21' that is 143 against 142): re-measure every candidate with the auto-cut before opening one. The auto-cut also mis-ranks an EH port in which the image has a large helper inlined (a '255' between about 2,900 instructions and 172).
- **First-row poisoning in per-address surveys.** A survey that takes the FIRST row of an address, or marks an address as seen after one name, measures the wrong object when several names share the address; a map from addresses to source files built by grepping address comments picks the wrong file. Every survey before the correction had an 87-address hole. Build the emitted-name-to-object map from the `PROC` lines of the listings, and try EVERY row name of an address before calling it unported.
- **False-park scan**: strip the park marker from every parked row and run the strict comparison over the exact object set of the build. It finds parked rows that already match (aliases, mostly) in one pass; here 0 false parks among addresses, 20 matching aliases. About 70 parked rows were aliases of an address already matched under another name, and five 'functions' were the incremental-link jump table: neither is work.
- **Stale alias rows stay parked when another build needs definitions.** Rows whose alias no object emits, at addresses already matched under another name, look droppable: the match count does not change. A second build that generates its bindings from the table treats every unparked row as a name that must be DEFINED and fails to link. Unparking is not neutral; leave them.
- **An epilogue counter must count our main-body `ret 0`.** EH funclets are emitted inside the function's `PROC..ENDP` and end in `ret 0`, so counting every `ret` over-counts; excluding every `ret 0` under-counts, because OUR no-argument epilogue is listed as `ret 0` where the image has a bare `ret`. Cut the body at the first funclet or handler label and count all returns before it (the image's funclets are in their own section).
- **Instruction-multiset surplus against diff lines** (registers and displacements masked): for most cheap ports the multiset difference is far smaller than the diff, so the residue is a register PERMUTATION or a schedule, not extra instructions. A multiset distance of 0 does not prove the block or case order, and a uniform register rename also gives 0.
- **An instruction-count win can be a diff loss**: removing two stores the image lacks reallocated a function (10 -> 52 lines). Count and diff are two measures; keep the diff.
- **Force-inlining a helper moves the instruction COUNT toward the image while the DIFF grows, unless the inline copy's SHAPE is rewritten.** Most top hits of an inline-gap scan were SIBLING algorithms with identical call sets, not inline copies (907 -> 962 and 1723 -> 1784 lines; 1203 -> 1435 with the count at 802 instructions against the image's 795: the image shares the x87 stack across two expansions). Run the call comparison first: equal call sets mean a sibling, not a missing inline.
- **THE CALL-GAP RULE: before restoring a 'missing' by-value call, check whether the image's arithmetic is INLINE at that site.** A call restored on one function of a class does not transfer to its siblings: two of them had no such call in the image at all (their three-float subtractions are inline), a third's 'missing call' was the callee's body inlined in the image, and an inline distance body had been labelled with the name of a similar function that has a null test the site lacks. Read the image's call list for the function first; forcing a helper inline to cover an inline-against-call difference the other way was worse again (1245 -> 1272).
- **By-value record scanners: compare the KIND, resolve the views, distrust the flags.** (1) A scan of record arguments must compare the construction KIND at each site (float stores, integer copies, a converting-constructor call), not the count of `sub esp` hits: an integer-copied record and the image's float-built one gave equal counts. (2) A scan of record-returning or record-taking calls must demangle OUR actual callee names: a census by the symbol table alone miscounted file-local views, and its 'gaps' on seven ports were unresolved views or aliases; several names at one address still need the site read by hand. (3) A tagger of the first consumption of the returned pointer (`mov reg,eax`, `push eax`, `[eax+N]`, a stored pointer, a re-derived `lea`, a frame read; with `eax` clobbers tracked) found 342 sites in 46 ports: the genuine mismatches sat in ONE family (the pushed-address phase and the reference-argument reader of 'By-value records' above), and every flag on a cheap port was an artefact (a scratch `lea`, the next call's setup) over a register, frame-cell-order, load-order or alignment residue.
- **SUBSET SEARCH: levers are superadditive and phase-dependent, so search subsets of independent toggles mechanically.** A site rejected on one allocation phase can pay after another subset is adopted, and two levers that each gain little can gain much together (25 lines for the pair against 14 and 9 alone; on another port no single named scalar improved and one alone lost 37 lines, while a four-site composition gained 61). Hand sweeps of one lever at a time miss these. The tool: each toggle is an independent `(before, after)` patch that must occur exactly once in its scope (the function, the file, or a scratch copy of a header), with `requires` closures for dependent ones and overlapping spans skipped; all 2^N subsets for N up to about 12, beyond that a measured greedy forward selection, backward elimination and a pair sweep; every variant compiled with the EXACT harness flags after deleting the old object and listing, a failed compile rejected and never scored; scores cached by input hash so a run resumes; the best chosen by lowest diff, then smallest instruction-count gap, then fewest edits; an interaction table (per toggle the mean, minimum and maximum marginal change over all measured pairs). Two safety parts: a PRIVATE scratch copy of the sources (the search never edits the tree, or restores and rebuilds its starting source on exit) and ONE compiler at a time through a lock directory with a stale-lock check, so parallel workers do not corrupt each other's objects. A proposer can generate toggles (if-inversions, compare flips that stay NaN-safe, ternary against if/else stores, per-use copies of a read-only parameter); keep the raw (unmasked) diff available for cross-jump residues. **After adopting a subset, RE-TEST the neighbouring sites on the new phase**: late re-tests gained again on two ports, and old winners turned into losers.
- **Single-lever ledger verdicts go STALE.** A shared-exit `goto` recorded as an improvement (20 -> 12) measured 19 in the current tree, and reverting it to a plain `return 0;` gave 13: the lever's SIGN flipped under later header and class changes. A recorded verdict holds for the tree it was measured on; re-run the subset search over a port's recorded levers after large header or class-view changes, before building on them.
- **SKIP-LIST verdicts go stale too: re-check every exclusion against the identical set before believing it.** Twenty ports that the ledgers excluded as 'not reachable' (register arguments of a static callee, a parameter in a callee-saved register, a callee that keeps `ecx`) were ALREADY in the identical set: later work on static callees and callee visibility had closed them, and nobody had struck them from the list. One port carried three independent verdicts from three rounds ('a custom calling convention', 'not reachable without a rename', 'a class-view fix breaks matches'); all three were wrong and it matched. An exclusion records the levers known when it was written: when a new lever class appears (same-file static callees, real class views, lifetime blocks), re-open the list, first mechanically (is the address matched now?), then with the new levers. The re-check also confirms: about two dozen other exclusions (compiler folds, register ties, scheduler walls, reads of uninitialised slots on a live path, assembly bodies) were re-tried with every later lever and stand.
- **Same-translation-unit effects: nothing added to a file is neutral.** An UNUSED forced-inline helper added to a translation unit moved a neighbouring port's baseline by a line, and an edit to one function changed the score of another in the same file whose source was untouched. Remove helpers that end up unused, and re-measure every parked neighbour in the file after each adoption (the identical set alone does not show a parked port getting worse).
- **Spelling-subset search exhausts the cheap band.** About 46 cheap and mid ports searched mechanically gave two small improvements; every other lever and combination was inert or worse. The search pays on ports with many repeated sites of one family (a reader with dozens of record-returning calls), where the phase of each site depends on the others; cheap residues (register roles, scratch rotation, addressing folds, load order, the held-constant race, x87 spills, cross-jump hosting) are not reachable by it.
- **Saved structural models in old ledgers are worth re-adapting.** A model an earlier round saved but could not merge (real member constructors and an ordinary initialising call, in place of a placement-new loop with its null test and EH scaffolding) was adapted to the current headers and took a constructor from 220 lines to 36, with the exact frame and instruction count; both improvements a later round kept were of this kind. Audit the unmerged bests whenever the shared headers have been consolidated. The audit, done once over about 260 saved variants after such a consolidation: SIXTEEN improvements and no match, one function from 971 lines to 3, the rest from a few lines to more than half. Its two cautions: (1) adopting a WHOLE model can lose unrelated matches through the translation unit: one model scored 128 against 159 and broke two matched functions LATER in the file, because it added an empty constructor to a shared class; without that constructor both came back and the target returned to 159, and the one piece that was safe gave 150. Compare the identical SET, not the count, after every adoption, and ablate the model piece by piece. (2) A saved score that a micro-harness cannot reproduce is not a result: an apparent two-line gain was a precedence bug in the variant generator.
- **Translation-unit order is not a broad lever.** Over about 2,200 caller-callee edges of the image, placing the callee's definition above the caller fires only when the callee is an OUTPUT WRITER, stable in that translation unit, and has a SINGLE caller. With several callers the image may need it visible to one and opaque to another: one file cannot serve both, so that is a file SPLIT; and a matched callee can lose its own match when moved. Trailing declaration comments fool position scans (several 'defined below' flags were false). The split was then tried: a new source file holding the callee ABOVE the first caller, an `extern` declaration in the other file, the matched callers held: two lines gained, exactly one `mov eax,ecx` (the compiler reuses `ecx` across a call it can see: direct evidence of same-file visibility in the original). It was NOT merged: it adds one more emission of an already duplicated constructor for two lines. Of the 2,200 edges six were multi-caller conflicts and this was the only clean one. Moving a function to its sibling's file with nothing else changed is byte-inert.
- **A micro-test in the exact harness decides whether a gain is a rule.** Same flags, old objects and listings deleted first, the two spellings as two functions: identical output means the gain was a port-specific allocation effect (keep it, do not sweep it); different output is a rule worth a scan. Declaration and statement reorders, helper forms and named locals all cascade or cancel inside a real port's register web, so a port is never the experiment.
- **Derive allocator rules from a generated MATRIX, not from hand variants.** Three tables (held constants, frame kind, register roles) and the x87 rules X7-X11 came from this method after dozens of rounds of hand variants on ports had found none of them. The parts: (1) a GENERATOR that emits hundreds to thousands of micro-functions over the axes in question (value kind, use count, use kind, order, what lies between: a call, a store, a loop), each function one cell; make every use OBSERVABLE (a store to its own global sink, so the operand of that store names the register or immediate); (2) the EXACT compiler harness (the build's flags, one translation unit, objects and listings deleted first); (3) a LISTING PARSER that reduces each function to the property tabulated (the holder of each value, the frame kind, the operand kinds of the x87 sequence); (4) a SECOND SPELLING per cell, and targeted controls for every anomaly, before a row is written down: a rule is a row confirmed on two spellings. Then check the table against a known function before using it. The tables come out predictive first; a lever is a row whose inputs a source change can move.
- **A naive intrinsic or idiom scan LIES in five ways** (about 197 ports flagged, 56 real; every cheap port's hit was an artefact): (1) the disassembler prints immediates in hex, the listing in decimal; (2) CRT helper names differ in CASE between the image's symbols and the listing (a whole 'math helper family' was this); (3) allocator ALIASES: the array forms of `new` and `delete` resolve to the same routines as `malloc` and `free`, the scalar forms are distinct; (4) OUR listing puts EH funclets inside the function's `PROC..ENDP`, the image has them out of line (a whole 'operator delete family' was this); (5) the disassembler renders string instructions as `rep movs DWORD PTR ...` and `repz cmps BYTE PTR ...`, the listing as `rep movsd` and `repe cmpsb`. Normalise all five before comparing mnemonic signatures.
- **Lever detectors over a fresh survey mostly re-find one wall.** A per-lever detector fired on 66 of 325 parked ports: the 4-byte frame surplus and the memory null test were ONE residue (the held-zero race); re-read signals on cheap ports were offset shifts from a differing frame (a genuine owner re-read needs equal frames and a same-offset load delta); a placement-new detector matched `operator new(` declarations. A detector is a lead only after the frame figures agree. A register-ROLE detector (prologue pushes and the first definition of each callee-saved register, the source normalised to `this`, parameter N, constant, member load or global; verdict SAME, SWAP or SETDIFF) classified 330 parked rows: 122 the same assignment, 138 a different held SET (a promotion difference), 23 the same set in a different assignment, 47 with no object (data symbols, import thunks, stale alias rows). The 23 are the only worklist for the role dials, and none closed (`vc8-regalloc.md`, 'The register-role table'). Its survey also re-proved 'a masked diff of 0 is not a match': a row reading 0 differed by two strict bytes (one branch hosted the other way); verify every zero row with the strict comparison before unparking it.
- **`__restrict` on a pointer PARAMETER changes the decorated name** (`PBD` becomes `PIBD`), so the function no longer matches its symbol row; a local restrict alias keeps the signature.
- **Comparative scans beat one-sided ones.** A scan of the image alone (a cold block after `ret`, an aligned loop head) flags ports that already have the feature; the comparative version (the image has it, OUR object lacks it) cut 42 and 77 hits to 19 and 14, all large or already worked. The absolute-data scan's hits are mostly existing statics and globals, not missing static arrays.
- **What is left**, by the agents' own estimates: three to eight source-reachable ports, through a new wrong view, class or EH reconstruction (the route of the two latest matches: rank the large EH ports by missing constructor and destructor targets, not by diff size); the cheap band is allocator and scheduler tie-breaks (scratch-rotation phase, load and store scheduling, an addressing fold tied to the register web, frame-cell permutations, a cross-jumped tail, the held-constant race), now also exhausted for mechanical spelling-subset search; state-count differences with matching body state stores are not work (rule N7).

## 14. Running external agents (headless)

**held** *(the Gemini rounds G1-G3, the Qwen rounds Q1-Q2 and the merges of the Kimi and Opus worktrees)*. These are process facts about headless external CLI agents (`agy`, `qwen`) driving the harness, not compiler facts.

- **The first denied tool call ends a headless run** (no prompt is possible); `--dangerously-skip-permissions` may be refused by the host's permission classifier. Allow rules take BARE command names (`command(make)`); a path-scoped `command(cp /abs/...)`, `command(make *)` and `command(make:*)` never match; file tools are `read_file(*)`, `write_file(...)`. Pipes and `&&` are matched per command; a command that BEGINS with a variable assignment, `for`, `if` or `(` matches no rule and ends the run: put such code in a script file and run it with `bash` or `python3`.
- **Quota resets end a run too** (a subscription quota killed one run after about 2.5 hours; a denied command killed another after 2): expect to wait the limit out and restart from the ledger.
- **Merging an external worker's round: three-way, never copy.** A worktree whose base is behind master must be merged with `git merge-file` (current master, the worker's base, the worker's file); copying the files whole deleted a newer `symbols.tsv` row and overwrote newer improvements from other rounds (the native build then reported an unresolved name). Re-run the byte match and the native build after the merge.
- **The agent's own notes may be empty: read its diff.** Review its output before merging: two agents named one record type differently in two files and the layout checker flagged a class defined twice with different member types; the coordinator renamed one. Run the byte match in the main tree, never trust a worktree's.
- **The `qwen` CLI runs headless with `--approval-mode auto-edit` plus `--allowed-tools`**: the list takes the tool names (`read_file`, `write_file`, `edit`, `glob`, `grep_search`, `list_directory`) and shell commands as `run_shell_command(PREFIX)`, one entry per command prefix. A shell command outside the list prints 'requires user approval but cannot execute in non-interactive mode' and the run CONTINUES (unlike `agy`, where the first denied call ends it), so a refused build or compare does not stop the agent: read the log for that warning before trusting a round that reports 'no improvement'. The model id is the one named under `modelProviders` in `~/.qwen/settings.json`.
- **The three-way merge is per file, against the worker's recorded base commit**: note the base when the worktree is handed out, merge every file the worker touched with `git merge-file`, and apply symbol-table rows by content. Several rounds merged this way over a master that had moved by five to seven other rounds with no loss; a worktree whose base IS current master can be copied.
- **The stale-object trap: objects of removed sources stay in the build directory and inflate anything that globs the objects.** A source file that was deleted or renamed leaves its `.obj` behind; the byte match reads the link, but a status script that globs `build/*.obj` counted 20 extra identical names from 15 such objects. The same holds for listing files and for EVERY build directory (the matching build and the second build each keep their own): when a source file is deleted or merged into another, delete its objects and listings in all of them in the same change. Delete every object that has no source (or build into an empty directory) before computing a status or comparing name sets, and never report a match count taken from a glob over a long-lived build directory.
