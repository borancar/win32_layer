# Typing the structs a byte-matched port still reaches by offset

A port written from the disassembly reaches its objects as `*(T *)((char *)p +
0xNN)`, `CAMPO(T, 0xNN)` macros and `char padNN[..]` stand-ins in partial class views.
This file is the method that turned about 4,000 such accesses into typed members on
Commandos (`[vc8]`, 12,300 functions byte-identical) **without a match lost and
without a match gained**: the typing is a pure refactoring, and the work is in the
layout engine and in not breaking the layout. Do it AFTER the matching has
slowed down, and in an isolated git worktree (a matcher keeps committing on the main
checkout), merging by fast-forward after verification.

## The rule that makes it safe

**Nothing is committed below the match baseline.** Temporary drops in the working tree
are fine (a typed access can change what the compiler schedules: references to casts
alias so loads stay put, real members let the loads move up, a typed pointer
member can change the register allocation), but a state is committed only when the
identical NAME SET contains the baseline set (compare names, not counts) and `different`
is 0, the native build is clean, the one-definition check (`odrcheck`) reports nothing
in BOTH object orders, the layout check reports no under-sized allocation and no
new size/offset/polymorphic entry, and the guard count is unchanged. When a function drops
after typing, re-match it with the byte-matching idioms: do not keep a raw cast to
protect a match, except where the match needs the cast (see below).

## Method, in the order that worked

1. **A survey tool** counts raw-offset lines, macro uses and filler members per class
   (the enclosing `Class::Method`, or the declared pointee type, else the variable name),
   so that progress is a number (3,902 -> 1,610 raw-offset lines in four rounds).
2. **A layout engine** computes each class's member offsets from its definition and
   splits a filler `char c[END - START]` to put a typed `T fNN` at the offset an access
   uses, or appends members to a method-only view. Every added member carries a
   compile-time guard, because C++03 has no `static_assert`:
   `typedef char layout_C_m[(int)&((C *)0)->m == 0xNN ? 1 : -1];` (and one for the
   size where known). A failing guard excludes the class (an exclude file), it does not
   get patched around.
3. **Base-class sizes from the compiler**, not from the tool's own arithmetic:
   dump `sizeof` of every class with clang per source file; a class with an unknown
   base had its members start at 0, which is why most early exclusions failed their
   guards (103 -> 39 excluded classes after this one change). An empty class is 1 byte,
   not 0.
4. **Rewrite accesses** on `this`, on `char *t = (char *)this` aliases and on variables
   declared as a class pointer; resolve `extern Class *g;` globals and cast macros
   (`#define V ((Class *)expr)`); expand raw-access macros (`I(o)`, `F(o)`) at literal
   offsets first so the pass can type them; for a variable declared `char *`/`void *`
   that is really an object, a small retyping tool changes the declaration to the
   class pointer and fixes the arithmetic.
5. **Verify every batch** with the rule above.

## Pitfalls that cost a drop or a bug

- **A typed pointer plus an offset scales.** `actor + 0x50` on a `ClassBicho *` is
  `0x50 * sizeof(ClassBicho)`: keep `(char *)actor + 0x50`.
- **A member named like a local is shadowed.** A local `int fase` hid the member
  `fase` and the typed rewrite wrote the local: write `this->name` where the function
  declares the name.
- **Giving a base class members moves every member of a derived view** that assumed
  the base to be a lone vptr; skip classes used as bases or by value in the same
  file or in the includers of the header until their derived views are typed first.
- **One class name, several layouts.** A name given to a code range (`ClassElementos`,
  `ClassDC2D` at 0x28 bytes in 54 sources and 0x4e4 in others, symbols that spread one
  class over ranges) is not one class: merging the views is the native ODR bug (the
  linker keeps one `sizeof`); padding the same-size views, renaming (the mangled names
  carry the class name, so a rename changes names the image's addresses need) and
  un-excluding all fail the layout check: leave such names excluded.
- **A member read as bits by an int user**: when members become `float` (`x, y, z` of
  an actor that every user reads as float), the one port that copies the three words
  as an int needs `*(int *)&p->x` (a bit copy, not a conversion). Compile every source
  with the implicit-float-conversion warning both ways: identical warning sets, but the
  check does not catch an int written INTO the members.
- **A matched function can need the raw cast**: a named pointer to an embedded record
  formed by `(char *)actor + 0x194` gives `add eax,0x194`, where the typed member
  `&actor->suscrito` folds into a direct store and moves the schedule. Keep such casts
  in matched functions and list them. Likewise, do not type through a one-off
  stand-in struct that only one port uses: it changed codegen.
- **A rename that changes a mangled name** (a class rename, a signature change)
  breaks every name table that spells the old one (the symbol table, the native
  binding table, the exercised-port list).
- `make` stops at the first failing file: use `make -k` to see all failing layout guards.

## Naming fields (offset-named members -> real names)

The typed members come out named by offset (`f1c`, `c198`, `campo0c`). Naming them is
cheap and safe where a rename is AST-exact and a rename cannot change object code, but
the evidence is the bottleneck: name a member only from what its class's own methods do with
it, the message ids and strings around its uses, and its neighbours (adjacent same-typed
members used together are a point or a rectangle); a name without clear evidence stays an
offset name (a wrong name misleads more than an offset). Tools: dump the compiler's JSON
syntax tree of every source with the native flags; a survey per member (access kinds:
read, written, compared with, called through, used in float arithmetic, and the
functions that use it); a rename that rewrites the declaration, every use (an implicit use
becomes `this->name`), constructor initialisers, `using Base::m;` lines and the layout
guards, identifying a member by its DECLARATION SITE (a header read through a macro or
a `using` shows several class names), then re-scans to check that every use still
resolves. Yield: 52 members named and 6 retyped (3 int -> pointer, 3 int -> unsigned,
none dropped a match) from 645 scalar offset-named members in headers; most of the rest
are scratch fields reused for several purposes (a union), flags set in one function and
read in another, or have no use at all, and need the owning class's methods read first.
Keep a table (class, offset, old, new, type, evidence) so a wrong name can be audited.
