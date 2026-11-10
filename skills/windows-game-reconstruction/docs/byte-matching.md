# Byte-matching ideas, with what survived

Source shapes and methods that brought a reconstruction closer to the bytes the
original compiler emitted, collected from project history. Each entry carries the
compiler it was measured on and whether it lasted.

**Tags.** `[vc6]` is Visual C++ 6.0, compiler build 12.00.8168, `/O2 /GX`, on
ArmyMen2 (github.com/borancar/ArmyMen2), which went from 399 to 1,639 of 1,646
functions byte-identical between 2026-09-27 and 2026-10-31. VC8 (`[vc8]`) idioms
from Commandos are in the skill's `SKILL.md`, one directory up; add new compilers here under
their own tag. An idea measured on one compiler is a hypothesis on another.

**Status** says what happened to the idea afterwards:

- **held**: still in the tree with the function byte-identical, and no record
  of it being replaced. Checked against ArmyMen2 `20c901fe`.
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

The figures `A -> B` are the per-function edit distance of the relinked bytes
against the image (0 is identical; a replaced byte costs 2).

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
- **"The ceiling."** `[vc6]` **reverted.** At 1,507 of 1,645 the remaining 138
  were triaged as "register allocation and loop rotation the source cannot
  steer" and declared the practical limit. The cause was stub structs reached
  through `base + OFFSET` casts (section 5); typed members took the count to
  1,639. A band of 5 to 13-byte near misses written off as "the allocator wall"
  mostly closed too. *Rule: do not declare a wall until typed structs, real
  classes, the frame layout and a from-scratch derivation have been tried.*
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

## 2. Method

- **Score the relinked bytes, nothing softer.** `[vc6]` **held.** Compile every
  unit with the original compiler, place each function at its original address,
  resolve every relocation to the address the original linker wrote, compare to
  the image with nothing about reconstructed targets wildcarded. Masked "shape"
  scores hide a swapped global (`H*W` against `W*H`), a wrong jump target and a
  wrong register; they are for reading a diff, never for scoring one.
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
- **A dead `cmp` with no branch after it** is two identical if/else arms the
  compiler merged. `[vc6]` **held.**
- **Small shared tails reached by `goto` get duplicated; do the work in each
  branch and `break`** to get one block (VC6 cross-jumps identical tails).
  `[vc6]` **held.**
- **`xor r,r; test; jle; mov r,X` is `x = 0; if (c) x = X;`**; a ternary gives
  `mov; jmp; xor`. `sub 3; neg; sbb; and K; add B` is the two-constant ternary
  `v == 3 ? B : B + K`. `[vc6]` **held.**

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
- **Dead argument slots are spill slots.** `[vc6]` **held.** VC6 parks byte or
  address-taken locals in a parameter's home once the parameter lives in a
  register. `mov [esp+N],cl; mov edx,[esp+N]; and edx,0xff` is a `uint8_t`
  local, not a parameter.
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
- **Runs of `mov [esp+N], imm`** are a local array initialised from constants;
  declaration order decides the order of the store groups. `[vc6]` **held.**
- **Chained assignment.** `[vc6]` **held.** One load stored to two fields is
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
- **Callee definition order.** `[vc6]` **held.** VC6 infers "cannot throw" only
  for callees defined earlier in the unit, so keep definitions in original
  address order.
- **An EH state store around a constructor call is a real `new T(...)`**, not an
  allocation plus placement construction (MpPanelScreen 339 -> 260). Stores
  before the vptr store are member initialisers. `[vc6]` **held.**
- **Write the classes.** `[vc6]` **held.** Constructor/destructor pairs become
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
- **Enumerators compile exactly like literals.** `[vc6]` **held.** An out-of-range
  comparison constant spelled as an `enum` silences a host-compiler warning with
  no byte change; the same constant in range, or passed through an inline helper,
  changes the bytes.

## 7. Register allocation: the measured model

Full detail is in ArmyMen2's `docs/vc6-regalloc.md`. The parts that decide most
stuck functions:

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

## 8. Reading the image

Tells that say what the source was, before any compile:

- A plain `ret` with stack arguments is `__cdecl`; `ret N` is `__stdcall` or
  thiscall. `[vc6]` **held.**
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

## 10. Searches, and what never paid

- **Adjacent statement swaps**, keeping the best that worsens nothing, repeated.
  `[vc6]` **held.** Store order is the lever it finds: eleven functions closer in
  two passes, three of 49 on a later run. Filter for independence and read every
  winner.
- **Copy substitution** (use a copy's source instead of the copy, or the
  reverse) was the only mechanical family that produced matches late
  (1,552 -> 1,555). `[vc6]` **held.**
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
- **Compiler-flag sweeps**: no per-file flag beat `/O2 /GX`; `/Oa` and `/Ow`
  explode the diffs. `[vc6]` **no effect.**
- **Retyping each integer local across the fixed widths**: 17 closer, none
  matched. **Introducing a parameter copy everywhere**: 3 closer, none matched.
  `[vc6]` **no effect.**
- **decomp-permuter**: found one match in two minutes and then cost more memory
  than it returned; dropped for single-process sweeps. `[vc6]` **superseded.**

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
