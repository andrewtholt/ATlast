/*

    SCHED.CPP  --  Cooperative round-robin task scheduler for ATLAST.

    ---------------------------------------------------------------
    Why this works without real coroutines
    ---------------------------------------------------------------

    ATLAST's inner interpreter, exword(), is not C-level recursive
    for the common case of calling a colon definition.  Entering a
    colon word (P_nest) just pushes the caller's ip onto the return
    stack and points ip at the new word's body; exword()'s own
    while(ip != NULL) loop then just keeps walking whatever ip
    happens to reference.  Every bit of "call stack" a running
    definition needs is therefore represented by data sitting on
    the Forth data stack, return stack and the ip register -- three
    values, all reachable through global variables (stack/stk/
    stacktop, rstack/rstk/rstacktop, ip).

    That means a "task switch" is nothing more than saving those
    three values into a per-task struct and loading a different
    task's saved values back into the same globals.  Whichever
    exword() while-loop happens to be running physically on the C
    stack at that moment will, on the very next iteration, simply
    start executing the other task's code -- because it only ever
    looks at the current ip.  No setjmp/longjmp, no ucontext, no
    separate C stacks per task are required.  This is exactly how
    classic Forth multitaskers (PAUSE/co-op round robin) have
    always worked, and ATLAST's design happens to already have the
    right shape for it.

    The one thing a task must never do is let its own ip actually
    reach NULL while it is "borrowing" someone else's exword() call
    frame -- that would unwind the wrong C frame.  So newly
    ACTIVATEd tasks are not given a NULL return address; they are
    given the address of a tiny internal trampoline program whose
    only word is (TASK-DONE), which retires the task and hands off
    to the next ready task instead of ever letting ip go NULL.

    ---------------------------------------------------------------
    Words provided
    ---------------------------------------------------------------

    dsize rsize TASK <name>   ( -- )
        Defining word.  Allocates a data stack of dsize cells and a
        return stack of rsize cells for a new, initially DORMANT
        task, and creates <name> as an ordinary CONSTANT-like word
        which, when executed, pushes the address of the task's
        control block.

    xt task ACTIVATE          ( -- )
        Point the task at the entry point of xt (obtained via ',
        i.e. TICK) and insert it into the round-robin ready ring.
        The task does not start running until some task calls
        YIELD (or the task doing the ACTIVATE reaches one).

    YIELD                     ( -- )
        The explicit pre-emption point.  Saves the calling task's
        context and switches to the next READY task in the ring.
        If no other task is ready, this is a no-op.

    .TASKS                    ( -- )
        Diagnostic: list all known tasks and their state.

    CURTASK                   ( -- task )
        Push the control block address of the currently running
        task.

    ---------------------------------------------------------------
    What is, and is not, per-task
    ---------------------------------------------------------------

    Per task:   data stack, return stack, ip (== full call chain
                and all local/loop-index state, since DO...LOOP
                indices live on the return stack).

    Shared:     dictionary, heap (and hence HERE, all VARIABLEs,
                VALUEs, PAD, STATE), and any C-level global state
                belonging to other extension modules (e.g. MQTT).

    This mirrors classic Forth multitasker practice, where the
    dictionary/heap are shared and only the stacks are private.
    If you need genuinely private task-local variables, allocate
    them out of a small array you index by CURTASK, or extend
    task_ctl with a user area.

    Known caveat: if a task YIELDs from underneath a nested
    EXECUTE (recursive exword() call), the C stack frame stays
    pinned until that particular nested call completes.  This is
    harmless but means C stack depth is shared across all tasks;
    prefer to YIELD from a task's outermost loop where possible.

    This file is public domain, in the spirit of the rest of
    ATLAST.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sched.h"

static task_ctl main_task;		/* Wraps the original console stacks */
static task_ctl *cur_task = NULL;	/* Currently live task, or NULL if
					   the scheduler hasn't started */

static dictword *taskdone_word = NULL;	/* (TASK-DONE) dictionary entry */
static dictword *task_exit_prog[1];	/* One-word "program": just calls
					   (TASK-DONE) when a task's own
					   code runs off the end */

/* Functions with external linkage inside atlast.c that we reuse
   rather than reimplementing: P_create() sets up a new dictionary
   header and arranges for the next token read to become its name;
   P_con() is the "push the one cell stored in my body" action,
   already used by CONSTANT and VALUE. */

extern void P_create();
extern void P_con();

/*  SAVE_CTX / LOAD_CTX  --  Move the live stack/rstack/ip globals
    to and from a task's saved copies.  */

static void save_ctx( task_ctl *t)
{
    t->t_stk  = stk;
    t->t_rstk = rstk;
    t->t_ip   = ip;
}

static void load_ctx( task_ctl *t)
{
    stack     = t->t_stack;
    stackbot  = t->t_stack;
    stacktop  = t->t_stacktop;
    stk       = t->t_stk;

    rstack    = t->t_rstack;
    rstackbot = t->t_rstack;
    rstacktop = t->t_rstacktop;
    rstk      = t->t_rstk;

    ip = t->t_ip;
}

/*  NEXT_READY  --  Find the next READY task after "from" in the
    ring, not counting "from" itself unless nothing else is ready,
    in which case "from" is returned so the caller can keep running
    it.  Returns NULL only if "from" itself is not READY and
    nothing else is either (shouldn't normally happen).  */

static task_ctl *next_ready( task_ctl *from)
{
    task_ctl *t = from->t_next;

    while (t != from) {
        if (t->t_state == TASK_READY)
            return t;
        t = t->t_next;
    }
    return (from->t_state == TASK_READY) ? from : NULL;
}

/*  P_yield  --  YIELD.  Explicit, programmer-placed pre-emption
    point.  */

prim P_yield()
{
    task_ctl *next;

    if (cur_task == NULL)	      /* Scheduler never started; a
					 bare YIELD outside any task
					 is simply a no-op. */
        return;

    save_ctx(cur_task);
    next = next_ready(cur_task);
    if (next == NULL)		      /* Should be unreachable -- cur_task
					 is always READY while running --
					 but fail safe rather than crash. */
        next = cur_task;

    cur_task = next;
    load_ctx(cur_task);
}

/*  P_taskdone  --  Entered automatically when a task's own code
    runs off the end (falls through its final EXIT).  Retires the
    task and switches to whatever is next.  */

prim P_taskdone()
{
    task_ctl *dead = cur_task;
    task_ctl *next;

    dead->t_state = TASK_DORMANT;

    next = next_ready(dead);
    if (next == NULL) {		      /* Nothing left ready anywhere --
					 fall back to MAIN so the system
					 doesn't wedge. */
        next = &main_task;
        next->t_state = TASK_READY;
    }

    cur_task = next;
    load_ctx(cur_task);
}

/*  P_task  --  "dsize rsize TASK <name>"  Defining word.  */

prim P_task()
{
    long dsize, rsize;
    task_ctl *t;

    Sl(2);
    rsize = (long) S0; Pop;
    dsize = (long) S0; Pop;

    if (dsize < 16) dsize = 16;
    if (rsize < 16) rsize = 16;

    t = (task_ctl *) calloc(1, sizeof(task_ctl));
    if (t == NULL) {
        printf("\nTASK: out of memory allocating control block.\n");
        return;
    }

    t->t_stack = (stackitem *) malloc(((size_t) dsize) * sizeof(stackitem));
    t->t_stacktop = t->t_stack + dsize;
    t->t_stk = t->t_stack;

    t->t_rstack = (dictword ***) malloc(((size_t) rsize) * sizeof(dictword **));
    t->t_rstacktop = t->t_rstack + rsize;
    t->t_rstk = t->t_rstack;

    if (t->t_stack == NULL || t->t_rstack == NULL) {
        printf("\nTASK: out of memory allocating stacks.\n");
        free(t->t_stack);
        free(t->t_rstack);
        free(t);
        return;
    }

    t->t_ip = NULL;
    t->t_state = TASK_DORMANT;
    t->t_next = NULL;
    t->t_name[0] = '\0';

    So(1);
    P_create();		      /* Reads the following token as this
				 word's name; leaves createword set. */
    createword->wcode = P_con;	      /* Executing <name> pushes the one
					 cell stored in its body. */
    Ho(1);
    Hstore = (stackitem) t;	      /* That cell is the task_ctl pointer. */
}

/*  P_activate  --  "xt task ACTIVATE"  */

prim P_activate()
{
    dictword *xt;
    task_ctl *t;

    Sl(2);
    t  = (task_ctl *) S0; Pop;
    xt = (dictword *) S0; Pop;

    if (t == NULL || xt == NULL) {
        printf("\nACTIVATE: bad xt or task.\n");
        return;
    }

    if (t->t_name[0] == '\0' && xt->wname != NULL) {
        strncpy(t->t_name, xt->wname + 1, sizeof(t->t_name) - 1);
    }

    /* Point ip at the xt's body, exactly as P_nest does when
       ordinary code calls a colon definition. */
    t->t_ip = ((dictword **) xt) + Dictwordl;

    /* Pre-load the one and only return address this task will
       ever need: our completion trampoline. */
    t->t_rstk = t->t_rstack;
    *(t->t_rstk) = (dictword **) task_exit_prog;
    t->t_rstk++;

    t->t_state = TASK_READY;

    if (cur_task == NULL) {	      /* Scheduler not started yet: make
					 this the whole ring for now. */
        cur_task = t;
        t->t_next = t;
    } else if (t->t_next == NULL) {  /* Not already linked: splice in
					 right after the current task. */
        t->t_next = cur_task->t_next;
        cur_task->t_next = t;
    }
}

/*  P_curtask  --  CURTASK ( -- task )  */

prim P_curtask()
{
    So(1);
    Push = (stackitem) cur_task;
}

/*  P_dottasks  --  .TASKS ( -- )  Diagnostic listing.  */

prim P_dottasks()
{
    task_ctl *t;

    if (cur_task == NULL) {
        printf("\nScheduler not started (no YIELD/ACTIVATE yet).\n");
        return;
    }

    printf("\n");
    t = cur_task;
    do {
        printf("  %-16s %-8s%s\n",
               t->t_name[0] ? t->t_name : "(unnamed)",
               t->t_state == TASK_READY ? "READY" : "DORMANT",
               t == cur_task ? "  <- current" : "");
        t = t->t_next;
    } while (t != cur_task);
}

static struct primfcn sched_prims[] = {
    {"0TASK",     P_task},
    {"0ACTIVATE", P_activate},
    {"0YIELD",    P_yield},
    {"0CURTASK",  P_curtask},
    {"0.TASKS",   P_dottasks},
    {"0(TASK-DONE)", P_taskdone},   /* Internal; not meant to be
					called by name from user code. */
    {NULL, (codeptr) 0}
};

void schedLoad()
{
    atl_primdef(sched_prims);

    taskdone_word = atl_lookup((char *) "(TASK-DONE)");
    task_exit_prog[0] = taskdone_word;

    /* MAIN wraps whatever stacks atl_init() already set up for the
       console/foreground -- it is task 0 in the ring, and always
       exists so YIELD has something sane to fall back to even
       before any ACTIVATE has happened. */
    strcpy(main_task.t_name, "MAIN");
    main_task.t_stack = stack;
    main_task.t_stacktop = stacktop;
    main_task.t_stk = stk;
    main_task.t_rstack = rstack;
    main_task.t_rstacktop = rstacktop;
    main_task.t_rstk = rstk;
    main_task.t_ip = NULL;
    main_task.t_state = TASK_READY;
    main_task.t_next = &main_task;

    cur_task = &main_task;
}
