\ scheduler.fth - Simple round-robin scheduler for Atlast Forth

\ Define the number of tasks
: NUM-TASKS 4 ;

\ Define the task structure
: TASK-STRUCT
    2 CELLS ALLOT
    CELL+ 2 CELLS ALLOT
    CELL+ 2 CELLS ALLOT
;

\ Define the task array
VARIABLE TASKS
NUM-TASKS TASK-STRUCT * ALLOT

\ Initialize the task array
: INIT-TASKS
    0 DO
        I TASKS + 0 SWAP !
        I TASKS + 1 CELLS + 0 SWAP !
        I TASKS + 2 CELLS + 0 SWAP !
    LOOP
;

\ Define the current task index
VARIABLE CURRENT-TASK 0 !

\ Define the next task word
: NEXT-TASK
    CURRENT-TASK @ 1+ NUM-TASKS MOD CURRENT-TASK !
;

\ Define the run task word
: RUN-TASK
    CURRENT-TASK @ TASKS + @
    IF
        @
        EXECUTE
    ELSE
        ." No task to run" CR
    THEN
;

\ Define the scheduler word
: SCHEDULER
    BEGIN
        RUN-TASK
        NEXT-TASK
        1000 MS
    AGAIN
;

\ Initialize the tasks
INIT-TASKS

\ Load the scheduler
SCHEDULER
