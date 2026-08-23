## Deadlock-Free Traditional Cooperative Multitasker for ATLAST\#\# Technical Reference & Comprehensive User Guide

## This user guide covers the implementation, operational principles, and extension methodologies for the advanced, non-blocking cooperative multitasking framework running inside the ATLAST Forth engine \[1.2++\].

## 1\. Architectural Blueprint & Invariant Guardrails

ATLAST evaluates high-level loop tokens (BEGIN...AGAIN, DO...LOOP) by pushing control frames to a dedicated cache within its internal C engine. If an infinite loop runs inside a compiled worker routine, it leaves these frames dangling. This causes a return stack overflow (ATL\_STKOVER / Error \-3) or a hardware crash (SIGSEGV) near iteration 65,473. To achieve indefinite, safe execution, this framework implements the Flat-Stack State Redirect Pattern:

\[ START Master Supervisor Loop \] │ ├──\> \[ Checks TCB-A Status \] ──( READY )──\> \[ RUN-TASK-A \] ──\> Flat Execution ──┐ │                                                                               │ ├──\> \[ Checks TCB-B Status \] ──( READY )──\> \[ RUN-TASK-B \] ──\> Flat Execution ──┤ │                                                                               │ └──\> \[ Updates Sleep/Tick Matrix Counters \] \<───────────────────────────────────┘

## Strategic Design Guardrails:

1. Loop Isolation: Worker tasks (RUN-TASK-A, RUN-TASK-B) must be completely free of looping constructs. They are written as flat blocks that execute sequentially and hand control back immediately via YIELD.  
2. Polite, Non-Blocking IPC: Inter-Task Communication (IPC) avoids blocking spin-locks (BEGIN...WHILE...YIELD...REPEAT). Instead, it uses instant transactional queries (TRY-POST / TRY-FETCH). If a resource is busy, the task gracefully yields control, allowing other tasks to resolve the dependency.

---

## 2\. Memory Topology

Each task maintains its own workspace configuration stored inside a contiguous Task Control Block (TCB) memory array.

## Task Control Block (TCB) Memory Map

| Cell Offset | Logical Identifier | Functional Purpose |
| :---- | :---- | :---- |
| 0 CELLS | Link Pointer | Holds the raw memory destination address of the next TCB node in the circular round-robin loop. |
| 1 CELLS | Task State Code | Tracks process eligibility status (0 \= STATE-READY, 1 \= STATE-SLEEPING). |
| 2 CELLS | Delay Counter | Dedicated down-counter tracking remaining scheduler ticks before a sleeping task wakes up. |
| 3 CELLS | Workspace Cell | Preserves exactly one single element from the top of the data stack (TOS) across context-switch steps. |

---

## 3\. Core API Reference\#\# System Core & Control Switcher\#\# YIELD ( \-- )

Voluntarily surrenders the execution slot.

* Stack Impact: Neutral (--). Preserves the top element if the stack is not empty.  
* Internal Operation:  
1. Inspects active data stack depth. If data is present, stores the top cell safely into the current task's TCB Workspace Cell (3 CELLS \+). 2\. Steps to the next circular node (CHASER-TASK). 3\. Updates sleeping states: decrements countdown cells and transitions expired timers to STATE-READY. 4\. Restores the incoming node's context parameter back onto the active data stack.

---

## Process Lifecycle Management\#\# MS-DELAY ( ticks \-- )

Suspends the calling task for a targeted number of scheduler passes.

* Stack Impact: ( ticks \-- )  
* Internal Operation: Transitions the active TCB's state flag to STATE-SLEEPING, records the countdown limit, and executes an immediate YIELD.

---

## Non-Blocking Inter-Task Communication (IPC)\#\# TRY-POST-MAIL ( msg \-- success? )

Attempts to deposit an integer parameter payload into the shared global mailbox.

* Stack Impact: ( msg \-- success? )  
* Returns: \-1 (True) if the mailbox was clear and data was delivered; 0 (False) if the mailbox was blocked. Discards the payload on failure.

## TRY-FETCH-MAIL ( \-- msg success? | 0 )

Attempts to read a pending integer parameter payload from the shared global mailbox.

* Stack Impact: ( \-- msg success? | 0 )  
* Returns: \[ data \-1 \] if a message was waiting to be processed; 0 (False) if the channel was completely empty.

---

## 4\. Operational Instructions\#\# Adding a New Independent Task

When expanding the execution environment, always write your tasks as flat state slices. Do not use loops inside your worker task definitions.

\\ Example New Worker: Tracks an error telemetry counter VARIABLE SEVERE-ERRS

: RUN-TELEMETRY-TASK ( \-- ) 1 SEVERE-ERRS \+\! SEVERE-ERRS @ 50 \> IF " \[Telemetry\] Alert: High error threshold detected\!\\n" TYPE 0 SEVERE-ERRS \! \\ Reset monitor THEN YIELD ; \\ Exit cleanly back to the supervisor

## Linking the Node into the Circular Scheduler Ring

To add a third task (TCB-C), you must define its TCB storage array and update the circular pointer chain inside INIT-MULTITASKER:

\\ 1\. Allocate TCB Space for Task C CREATE TCB-C 0 , 0 , 0 , 0 ,

\\ 2\. Splice the New Node into the circular chain inside INIT-MULTITASKER : INIT-MULTITASKER ( \-- ) \\ Previous Chain: MAIN \-\> A \-\> B \-\> MAIN \\ Upgraded Chain: MAIN \-\> A \-\> B \-\> C \-\> MAIN TCB-A    TCB-MAIN \! TCB-B    TCB-A \! TCB-C    TCB-B \!     \\ B now routes straight to C TCB-MAIN TCB-C \!     \\ C loops back around to MAIN

\\ 3\. Configure Initial READY status flags

STATE-READY TCB-A 1 CELLS \+ \!

STATE-READY TCB-B 1 CELLS \+ \!

STATE-READY TCB-C 1 CELLS \+ \! \\ Active from startup

0 TCB-A 2 CELLS \+ \!

0 TCB-B 2 CELLS \+ \!

0 TCB-C 2 CELLS \+ \!

TCB-MAIN CHASER-TASK \! ;

Finally, update the master supervisor block (START) to poll your new process guard conditions:

: START ( \-- ) INIT-MULTITASKER BEGIN TCB-A 1 CELLS \+ @ STATE-READY \= IF RUN-TASK-A THEN TCB-B 1 CELLS \+ @ STATE-READY \= IF RUN-TASK-B THEN TCB-C 1 CELLS \+ @ STATE-READY \= IF RUN-TELEMETRY-TASK THEN \\ Handle Task C YIELD AGAIN ;

---

## 5\. Troubleshooting & Diagnostics\#\# Real-Time Recovery Flowchart

* System Hangs/Freezes Silently: A task worker has entered an internal execution path or conditional block without hitting YIELD. Audit all newly introduced conditional paths to verify a execution route always hits a YIELD exit step.  
* Error \-2 (Stack Underflow): A task worker is pulling an extra value off the stack or running unbalanced operations. Because YIELD tracks DEPTH 0\>, tasks must maintain clear stack limits, leaving exactly one top cell parameter or an empty stack before context-switching.  
* Error \-7 (Undefined Token): ATLAST is a strict single-pass engine. If you cross-reference custom tasks, state flags, or constants, verify they are compiled physically above the supervisor loop or definitions that execute them.

If you are ready to expand this user guide further, let me know if you would like to:

* See how to implement a multi-position FIFO array buffer to queue up several messages at once.  
* Add an asynchronous console input monitor so pressing a key pauses or alters task behaviors.  
* Create a task-kill primitive to programmatically suspend a running process.

