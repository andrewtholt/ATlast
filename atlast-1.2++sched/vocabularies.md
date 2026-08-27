# Vocabularies and Search Order in ATLAST

ATLAST includes support for **Forth-83 Vocabularies** and the standard **Search Order** word set (Forth-83 Experimental Extension / ANSI Forth).

Vocabularies allow you to partition the dictionary into distinct namespaces. This prevents naming collisions, organizes large modular codebases, and gives you fine-grained control over:
1. **Search Order (`CONTEXT`)**: Which vocabularies are searched when compiling or interpreting words.
2. **Definition Target (`CURRENT`)**: Which vocabulary newly compiled words (`:`, `VARIABLE`, `CONSTANT`, `CREATE`, etc.) are linked into.

---

## Wordset Summary

| Word | Stack | Description |
| :--- | :--- | :--- |
| `VOCABULARY <name>` | `( -- )` | Defining word. Creates a new vocabulary named `<name>`. When `<name>` is executed at runtime, it replaces the top of the search order (`order[0]`). |
| `FORTH` | `( -- )` | Standard root vocabulary. When executed, replaces the top of the search order with `FORTH`. |
| `CONTEXT` | `( -- addr )` | Pushes the address of the variable pointing to the first vocabulary searched. `CONTEXT @` returns the top wordlist ID (`wid`). |
| `CURRENT` | `( -- addr )` | Pushes the address of the variable pointing to the compilation target vocabulary. `CURRENT @` returns the compilation `wid`. |
| `DEFINITIONS` | `( -- )` | Sets `CURRENT` to match the active `CONTEXT` vocabulary (`CONTEXT @ CURRENT !`). |
| `ALSO` | `( -- )` | Duplicates the top vocabulary on the search order stack, expanding the search depth. |
| `PREVIOUS` | `( -- )` | Drops the topmost vocabulary from the search order stack. |
| `ONLY` | `( -- )` | Resets the search order stack to contain only `FORTH`. |
| `ORDER` | `( -- )` | Displays the current search order (`Context: ...`) and compilation target (`Current: ...`). |
| `VOCS` | `( -- )` | Lists all defined vocabularies in the system. |
| `SEAL` | `( -- )` | Seals the search order to contain only the active context vocabulary. |
| `WORDS` | `( -- )` | Lists all words contained in the active `CONTEXT` vocabulary. |
| `GET-ORDER` | `( -- wid_n ... wid_1 n )` | (ANSI) Pushes all vocabulary IDs from the search order and the count `n`. |
| `SET-ORDER` | `( wid_n ... wid_1 n -- )` | (ANSI) Sets search order to `n` vocabularies (`-1` restores default `[FORTH]`). |
| `GET-CURRENT` | `( -- wid )` | (ANSI) Returns the compilation vocabulary `wid`. |
| `SET-CURRENT` | `( wid -- )` | (ANSI) Sets the compilation vocabulary to `wid`. |
| `WORDLIST` | `( -- wid )` | (ANSI) Allocates an anonymous wordlist descriptor on the heap and returns its `wid`. |

---

## Standard Usage Patterns

### 1. Creating and Populating a Vocabulary

```forth
\ 1. Define a new vocabulary
VOCABULARY EDITOR

\ 2. Push EDITOR to search order and set it as compilation target
ALSO EDITOR DEFINITIONS

\ 3. Define words in EDITOR
: OPEN-FILE   "Opening editor file..." type CR ;
: CLOSE-FILE  "Closing editor file..." type CR ;
: HELP        "Editor Help: commands are OPEN-FILE, CLOSE-FILE" type CR ;

\ 4. Return compilation and search order to FORTH
ONLY FORTH DEFINITIONS
```

### 2. Calling Words in a Vocabulary

```forth
\ When in FORTH, OPEN-FILE is isolated and not visible:
\ OPEN-FILE  -> Error: 'OPEN-FILE' undefined

\ Add EDITOR to search order:
ALSO EDITOR
OPEN-FILE    \ Executes OPEN-FILE from EDITOR
HELP         \ Executes EDITOR's HELP (shadowing FORTH's HELP if present)

\ Remove EDITOR when done:
PREVIOUS
```

### 3. Multiple Vocabularies & Layering

```forth
VOCABULARY MATH-TOOLS
ALSO MATH-TOOLS DEFINITIONS
: SQUARE  DUP * ;
: CUBE    DUP SQUARE * ;

ONLY FORTH DEFINITIONS

VOCABULARY GRAPHICS
ALSO GRAPHICS DEFINITIONS
: DRAW-BOX  "Box drawn" type CR ;

ONLY FORTH DEFINITIONS

\ Combine vocabularies in search order:
ALSO MATH-TOOLS
ALSO GRAPHICS
ORDER
\ Context: GRAPHICS MATH-TOOLS FORTH
\ Current: FORTH

5 SQUARE . CR     \ Found in MATH-TOOLS
DRAW-BOX          \ Found in GRAPHICS
```

---

## Architectural Implementation in ATLAST

### Word Header & Vocabulary Structure

Each word in ATLAST is represented by a `dictword` header:

```c
typedef struct dw {
    struct dw *wnext;   /* Pointer to previous word in this vocabulary */
    char *wname;        /* Word name with flags byte (IMMEDIATE, VOCABULARY, etc.) */
    codeptr wcode;      /* Machine code function pointer */
} dictword;
```

Each vocabulary descriptor is defined as:

```c
typedef struct atl_vocab_struct {
    dictword *v_head;                 /* Head of wordlist chain */
    struct atl_vocab_struct *v_next;  /* Linked list of all vocabularies */
    dictword *v_word;                 /* dictword representing the vocabulary */
    char v_name[32];                  /* Vocabulary name */
} atl_vocab;
```

* **Heap Safety**: `forth_vocab`, `context_order` (`atl_vocab **`), and `current_vocab_ptr` are allocated on the ATLAST heap during `atl_init()`. This ensures that pointer verification (`Hpc`) passes cleanly when `@` and `!` are used with `CONTEXT` and `CURRENT`.
* **Search Resolution (`lookup`)**: `lookup()` iterates through `context_order[0]`, `context_order[1]`, ..., `context_order[context_depth - 1]`, traversing each vocabulary's `v_head` chain until a matching unhidden token is found.
* **Heap Pruning (`FORGET` / `atl_unwind`)**: When `FORGET <word>` or `atl_unwind()` is called, `forget_words_above(limit)` traverses all vocabularies in `vocabs` and prunes all definitions allocated at or above `limit` on the heap, and automatically removes forgotten vocabularies from `vocabs` and `context_order`.

---

## ATLAST Specifics and Deviations from Classic Forth-83

1. **Independent Wordlists vs. Hierarchical Chaining**:
   Classic Forth-83 without the Search Order extension linked new vocabularies to a static parent vocabulary at definition time. ATLAST implements the **Search Order model** (ANSI / Forth-83 Search Order extension), where each vocabulary is an independent word list, and search precedence is controlled dynamically via `ALSO`, `PREVIOUS`, `ONLY`, `SEAL`, and `SET-ORDER`.
   * *Best Practice*: Always use `ALSO <VOCAB> DEFINITIONS` when defining words in a vocabulary so that standard Forth primitives (like `:`, `;`, `+`, `DUP`) remain visible during compilation.
2. **Case Insensitivity**:
   ATLAST converts tokens to uppercase by default during lookup and definition. Vocabulary names and word lookups are case-insensitive.
3. **Strings and I/O**:
   In ATLAST, string literals in definitions can use quoted tokens like `"Hello" type` or `." "Hello"`.
4. **`WORDS` Scope**:
   In ATLAST, `WORDS` displays definitions in the **active `CONTEXT` vocabulary** (`context_order[0]`). To view all vocabularies, use `VOCS`. To inspect the active search order, use `ORDER`.
