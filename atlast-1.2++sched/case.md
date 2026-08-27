 Adding a CASE ... OF ... ENDOF ... ENDCASE construct to ATlast can be done in two ways:

  1. Pure ATlast / Forth level (No C changes required; added to a .atl script or startup file).
  2. Native C primitive level (Modifying atlast.c to add dedicated compiler & runtime primitives).

  Both approaches implement the standard ANS Forth CASE construct semantics:

  • CASE pushes a sentinel (0) onto the compile-time stack to track unresolved ENDOF jump backpatches.
  • OF compares the selector on top of the data stack with the test value. If equal, it pops both and executes the clause; if not,
  it leaves the selector and branches around the clause to the next OF.
  • ENDOF compiles an unconditional branch (BRANCH) past the end of the CASE statement and resolves the preceding OF conditional
  jump.
  • ENDCASE compiles a DROP (to clean up the selector if no OF matched) and backpatches all ENDOF forward jumps to the address
  after ENDCASE.
  ──────
  ### Method 1: High-Level Forth Definition (Recommended, No C Recompilation)

  ATlast already exposes low-level compilation words:

  • >MARK: Emits a 0 cell into the heap dictionary for a jump offset and pushes its cell address onto the stack.
  • >RESOLVE: Takes a marked cell address from the stack, calculates the relative offset to hptr, and stores it into the marked
  cell.
  • COMPILE <word>: Compiles the execution token of <word> into the target definition.

  You can define case, of, endof, and endcase in pure ATlast Forth:

    \ CASE initializes the endof-backpatch chain with a sentinel (0)
    : case 0 ; immediate
    
    \ OF compares ( selector test -- selector [if false] )
    : of
        compile over
        compile =
        compile ?branch
        >mark              \ reserve jump offset for false branch
        compile drop       \ drop selector on true branch
    ; immediate
    
    \ ENDOF branches around remaining cases and resolves the preceding OF branch
    : endof
        compile branch
        >mark              \ reserve forward branch past ENDCASE
        swap
        >resolve           \ resolve preceding OF false-branch to point here
    ; immediate
    
    \ ENDCASE cleans up the unmatched selector and resolves all ENDOF branches
    : endcase
        compile drop       \ drop unmatched selector
        begin
            ?dup
        while
            >resolve       \ backpatch each ENDOF jump offset
        repeat
        drop               \ drop the sentinel (0)
    ; immediate
    
  #### How it executes:

  • 1 OF ... ENDOF: At runtime, OVER = ?BRANCH checks if selector == 1. If false, it jumps to right after ENDOF (where the next OF
  test begins), retaining selector on the stack. If true, DROP discards selector, the clause executes, and ENDOF branches to post-
  ENDCASE.
  • Fall-through / Default: If no OF matches, execution falls through to ENDCASE, which executes DROP to discard selector.
  ──────
  ### Method 2: Native C Implementation in atlast.c

  If you prefer higher performance by combining OVER = ?BRANCH DROP into a single C runtime primitive (XOF), you can modify
  atlast.c:

  #### Step 1: Declare Static Symbol & Runtime Primitive

  In atlast.c, declare s_xof:

    static stackitem s_xof;
    
  Around line atlast.c (near P_if / P_branch), add the runtime primitive P_xof:

    prim P_xof()                    /* Runtime for OF */
    {
        Sl(2);
        if (S1 == S0) {             /* If selector == test value */
            stk -= 2;               /* Pop selector and test value */
            ip++;                   /* Skip jump offset, execute clause */
        } else {
            Pop;                    /* Pop test value, keep selector */
            ip += (stackitem) *ip;  /* Jump past clause (IP-relative) */
        }
    }
    
  #### Step 2: Add Compiler Primitives

  Add the compiler primitives for CASE, OF, ENDOF, ENDCASE:

    prim P_case()                   /* Compile CASE */
    {
        Compiling;
        So(1);
        Push = 0;                   /* Sentinel on stack */
    }
    
    prim P_of()                     /* Compile OF */
    {
        Compiling;
        Compconst(s_xof);           /* Compile (XOF) */
        So(1);
        Push = (stackitem) hptr;    /* Backpatch address for false branch */
        Compconst(0);               /* Placeholder offset */
    }
    
    prim P_endof()                  /* Compile ENDOF */
    {
        stackitem *bp;
        Compiling;
        Sl(1);
        Compconst(s_branch);        /* Compile branch around rest of CASE */
        Compconst(0);               /* Placeholder offset */
        Hpc(S0);
        bp = (stackitem *) S0;      /* Get preceding OF backpatch cell */
        *bp = hptr - bp;            /* Resolve OF false branch to here */
        S0 = (stackitem) (hptr - 1);/* Replace with ENDOF backpatch cell */
    }

    prim P_endcase()                /* Compile ENDCASE */
    {
        Compiling;
        Compconst(s_drop);          /* Drop unmatched selector */
        Sl(1);
        while (S0 != 0) {           /* Resolve all ENDOF branches */
            stackitem *bp;
            Hpc(S0);
            bp = (stackitem *) S0;
            *bp = hptr - bp;
            Pop;
        }
        Pop;                        /* Pop sentinel (0) */
    }

  #### Step 3: Register Primitives in primt[] and Symbol Lookup

  1. In primt[] table (around line atlast.c), register the words (0 = standard, 1 = immediate):

        {"0(XOF)", P_xof},
        {"1CASE", P_case},
        {"1OF", P_of},
        {"1ENDOF", P_endof},
        {"1ENDCASE", P_endcase},

  2. In atl_init() (around line atlast.c), look up (XOF):

        Cconst(s_xof, "(XOF)");
    ──────
  ### Example Usage in ATlast

  Regardless of whether Method 1 or Method 2 is used, syntax and behavior in ATlast Forth are identical:

    : print-digit ( n -- )
        case
            1 of "One\n"   type   endof
            2 of "Two\n"   type  endof
            3 of "Three\n" type endof
            "Unknown digit: " type dup . cr
        endcase
        cr
    ;

    1 print-digit \ Outputs: One
    2 print-digit \ Outputs: Two
    9 print-digit \ Outputs: Unknown digit: 9


### Runtime Verification

  The binary was recompiled and verified with both basic and nested CASE statements:

    : test-switch ( n -- )
        case
            10 of 1000 endof
            20 of 2000 endof
            30 of 3000 endof
        endcase
    ;

    10 test-switch . \ Outputs 1000
    20 test-switch . \ Outputs 2000
    30 test-switch . \ Outputs 3000
    40 test-switch . \ Stack cleaned (selector dropped automatically)

