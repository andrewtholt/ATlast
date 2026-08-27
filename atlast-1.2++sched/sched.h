/*

    SCHED.H  --  Cooperative round-robin task scheduler for ATLAST.

    Classic Forth-style multitasker: each task owns its own data
    stack and return stack.  Tasks explicitly give up control by
    calling YIELD -- there is no timer/signal-driven preemption.

    This is public domain, in the spirit of the rest of ATLAST.

*/

#ifndef __ATL_SCHED
#define __ATL_SCHED

#include "atldef.h"

#define TASK_DORMANT 0	  /* Not in the ready rotation */
#define TASK_READY   1	  /* In the ready rotation */

typedef struct task_ctl {
    char t_name[32];		  /* Filled in lazily for diagnostics */

    stackitem *t_stack;	  /* Base of this task's data stack */
    stackitem *t_stk;		  /* Saved data stack pointer */
    stackitem *t_stacktop;	  /* End of this task's data stack */

    dictword ***t_rstack;	  /* Base of this task's return stack */
    dictword ***t_rstk;	  /* Saved return stack pointer */
    dictword ***t_rstacktop;	  /* End of this task's return stack */

    dictword **t_ip;		  /* Saved instruction pointer */

    int t_state;		  /* TASK_DORMANT / TASK_READY */

    struct task_ctl *t_next;	  /* Next task in the round-robin ring */
} task_ctl;

void schedLoad();

#endif /* __ATL_SCHED */
