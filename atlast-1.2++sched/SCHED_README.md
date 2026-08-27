# Cooperative scheduler for ATLAST

## New files
- `sched.h`, `sched.cpp` -- the scheduler itself.
- `sched.mk` -- build profile (`./build.sh -p sched && make`, see caveat below).
- `sched_demo.atl` -- a worked example (two tasks round-robining via YIELD,
  driven by a DO LOOP in the console/MAIN task, with clean automatic
  retirement once both finish). This is the exact script used to verify
  the implementation below.

## Modified files
- `atlmain.c` -- added a `#ifdef SCHED ... schedLoad(); #endif` hook,
  in the same place and style as the existing EXTRAS/MQTT hooks.
- `Makefile` -- added a `sched.o` build rule (same pattern as
  `ringBuffer.o`/`mqtt.o`).
- `atlast.c` -- two small, targeted fixes, described below. Both are
  pre-existing bugs, independent of the scheduler; the scheduler
  wouldn't boot without them but they affect the whole interpreter.

## Design: why YIELD doesn't need real coroutines

ATLAST's inner interpreter, `exword()`, is not C-recursive for the
common case of calling a colon definition: entering a colon word just
pushes the caller's `ip` onto the return stack and points `ip` at the
new word's body (see `P_call`, the wcode for defined words); `exword`'s
own `while (ip != NULL)` loop then just keeps walking whatever `ip`
currently references. Every bit of state a running definition needs --
its whole "call stack", including DO-LOOP indices -- lives in three
global values: the data stack (`stack`/`stk`/`stacktop`), the return
stack (`rstack`/`rstk`/`rstacktop`), and `ip` itself.

That means a task switch is just: save those three into a per-task
struct, and load a different task's saved copies back into the same
globals. Whichever `exword()` loop happens to be physically running on
the C stack at that moment doesn't need to know anything changed -- on
its next iteration it just starts executing the other task's code,
because it only ever looks at the live `ip`. No `setjmp`/`longjmp`, no
`ucontext`, no separate C stacks. This is exactly how classic Forth
multitaskers (`PAUSE`) have always worked, and ATLAST's architecture
already has the right shape for it.

The one thing a task must never do is let its own `ip` reach NULL while
it's "borrowing" someone else's `exword()` C stack frame -- that would
unwind the wrong frame. So a newly `ACTIVATE`d task isn't given a NULL
return address; it's given a tiny internal one-word "program" whose
only word is `(TASK-DONE)`, so a task that runs off the end of its own
code retires itself and hands off to the next ready task instead of
ever letting `ip` go NULL underneath somebody else's frame.

## Words added

    dsize rsize TASK <name>   ( -- )
        Defining word. Allocates a private data stack (dsize cells)
        and return stack (rsize cells) for a new task, initially
        DORMANT. Executing <name> afterwards pushes the address of
        its control block (exactly like CONSTANT pushes a value).

    xt task ACTIVATE          ( -- )
        Point `task` at the entry point of `xt` (get xt via ', i.e.
        TICK) and splice it into the round-robin ready ring. The task
        doesn't actually run until something calls YIELD.

    YIELD                     ( -- )
        The explicit, programmer-placed pre-emption point. Saves the
        calling task's context and switches to the next READY task
        in the ring. If no other task is ready, it's a no-op.

    CURTASK                   ( -- task )
        Push the control block address of the task currently running.

    .TASKS                    ( -- )
        Diagnostic: lists every task in the ring and its state.

    (TASK-DONE)
        Internal completion trampoline -- not meant to be called by
        name from user code.

## What's per-task vs shared

Per task:  data stack, return stack, ip (hence the full call chain
           and all DO-LOOP index state, which lives on the return
           stack).

Shared:    dictionary, heap (HERE, VARIABLEs, VALUEs, PAD, STATE),
           and any C-level global state belonging to other extension
           modules (e.g. MQTT). This mirrors classic Forth
           multitasker practice. If you need genuinely private
           task-local data, index a small array by CURTASK, or add
           a user-area field to task_ctl.

## Known caveats

- If a task YIELDs from underneath a nested EXECUTE (a recursive
  exword() call), the C stack frame it's inside stays pinned until
  that nested call eventually completes. Harmless, but C stack depth
  is shared across all tasks -- prefer YIELDing from a task's
  outermost loop where practical.
- The WALKBACK debug-trace stack (if that build option is enabled)
  is a single shared global, not per-task; interleaved walkbacks
  across tasks would be misleading. Not touched here -- flagging it
  as a follow-on if you build with WALKBACK enabled together with
  SCHED.
- MARK/UNWIND snapshots (used for e.g. transactional FORGET) aren't
  scheduler-aware; taking a MARK in one task and UNWINDing after other
  tasks have run isn't meaningful. Same category as the above.
- `atlast_sched` needs its stacks reasonably sized for a background
  task's own colon-definition nesting depth; MAIN's stacks are
  whatever `-S`/`-R` set them to, but new tasks made via TASK use
  whatever cell counts you give them explicitly.

## Two pre-existing bugs found and fixed along the way

These aren't scheduler-specific; they'd bite any build, but the
scheduler smoke test is what surfaced them.

1. **atl_init() segfault on modern compilers.** `Cconst(cell, name)`
   and one direct call, `lookup("FORTH")`, passed C string literals
   into `lookup()`, which calls `ucase()` on its argument and mutates
   it in place. String literals live in read-only memory on any
   modern toolchain, so this is a write to `.rodata` -- authors of
   this code clearly relied on older compilers (or `-fwritable-strings`)
   that placed literals in writable memory. Fixed by copying into a
   small local buffer first at both call sites.

2. **New words registered by extension modules were invisible to
   lookup().** `atl_primdef()` (used by every extension: EXTRAS, MQTT,
   and now SCHED) correctly prepends new words onto the global `dict`
   chain, but `lookup()` doesn't search `dict` directly once
   vocabularies are active -- it searches `context_order[v]->v_head`.
   `forth_vocab->v_head` is set *once*, inside `atl_init()`, to
   whatever `dict` was at that moment. Since all extension loaders run
   *after* `atl_init()`, every word any of them registers was silently
   unreachable by name (`'TASK' undefined`, etc.) even though it was
   sitting right there in the dictionary. Fixed by having
   `atl_primdef()` re-sync `forth_vocab->v_head = dict` whenever
   `forth_vocab` already exists (a no-op during the bootstrap calls
   inside `atl_init()` itself, since `forth_vocab` is still NULL then).

   This second one is worth double-checking against whatever your
   normal MQTT/EXTRAS build+run workflow looks like -- it's possible
   it was masked there in some way I didn't reproduce, but as
   reproduced here it affected any word added after `atl_init()`,
   regardless of which module added it.

## Build system fix: extraFunc.o/-lhiredis no longer forced on every profile

Previously the top-level Makefile did, unconditionally, above
`include profile.mk`:

    ATLOBJ += atlast.o atlmain.o extraFunc.o
    LIBRARIES += -lhiredis -lrt

so *every* profile -- basic, linux, small, uclinux, sched, darwin --
required libhiredis installed just to link, even though only
`extraFunc.cpp`'s `cpp_extrasLoad()` (guarded by `-DCPP_EXTRAS`, set
only by `extras.mk` and `mqtt.mk`) actually uses it. The plain-C
`extrasLoad()` (guarded by plain `-DEXTRAS`, set by `linux.mk` too)
lives in `linux.c` and needs nothing extra.

Fixed by:
- Makefile: drop `extraFunc.o` from the base `ATLOBJ +=` and
  `-lhiredis` from the base `LIBRARIES +=` (kept `-lrt`, which several
  profiles' SYSVIPC use genuinely needs and costs nothing extra).
- `extras.mk`: now supplies `extraFunc.o`/`-lhiredis` itself, since it
  sets `-DCPP_EXTRAS`.
- `mqtt.mk`: same -- it also sets `-DCPP_EXTRAS`, so it needs them
  too.
- `pubsub.mk` untouched: it never set `-DCPP_EXTRAS`, so it was never
  part of this coupling. Its existing failure in fails.txt (missing
  `-lsmall`) is a separate, unrelated issue.

Verified through the project's own `./build.sh -p <profile> && make`
flow, not just a manual compile line:

    ./build.sh -p sched  && make   # builds clean, no hiredis, runs correctly
    ./build.sh -p basic  && make   # builds clean, no hiredis (as before)
    ./build.sh -p linux  && make   # builds clean, no hiredis (previously forced it)
    ./build.sh -p extras && make   # still correctly asks for hiredis --
                                    # it's the one profile that legitimately
                                    # needs it, and still declares that itself

`./build.sh -p sched && make` now produces a working `atlast` that
runs `sched_demo.atl` with the exact transcript below.

## Verified test transcript (sched_demo.atl)

    B1
    A1
    B2
    A2
    B3
    A3
    d          <- LOOP-B finished (100 EMIT), retired itself
    c          <- LOOP-A finished (99 EMIT), retired itself

      MAIN             READY     <- current
      LOOP-B           DORMANT
      LOOP-A           DORMANT

    X          <- MAIN's own DO LOOP ran out its remaining YIELDs
                   as harmless no-ops once nothing else was READY
