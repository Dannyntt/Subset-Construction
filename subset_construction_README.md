# SI2002 - Formal Languages, Assignment 2
## Subset Construction (NFA to DFA Conversion)

**Student's full name:** [TU NOMBRE COMPLETO AQUI]
**Class number:** [TU NUMERO DE CLASE AQUI]

## Environment

- **Operating system:** [e.g. Ubuntu 24.04 LTS / Windows 11 / macOS 15 — replace with what you actually used]
- **Programming language:** C++17
- **Compiler:** g++ (GCC) — tested with 13.3.0
- **Build tool:** none required beyond g++ itself

## How to run

1. Compile:
   ```
   g++ -std=c++17 -O2 -Wall -o subset_construction subset_construction.cpp
   ```
2. Run, feeding the input file through standard input:
   ```
   ./subset_construction < input.txt
   ```
3. The program prints, for each test case, a table representing the equivalent DFA (see "Output format" below).

## Input format assumed

```
c
n
s1 s2 ...
a1 a2 ... ak
f1 f2 ...
id1 T1 T2 ... Tk
id2 T1 T2 ... Tk
...
```
where `c` is the number of test cases; `n` is the number of NFA states (numbered `1..n`); the next line lists the initial states `S` (possibly more than one); the alphabet line lists the `k` input symbols; the next line lists the final states `F` (possibly empty); and each of the following `n` lines gives a state's own id followed by its transition **set** for each alphabet symbol, in the same order the alphabet was given. Each transition set is written either as `0` (the empty set) or as `{x y z}` (a brace-delimited, space-separated list of target states), exactly as specified in the assignment.

## Output format

Since the assignment leaves the exact table layout open ("you may rename the states of M for readability... indicate the initial state and the final states"), the program prints, for each case:

```
k
a1 a2 ... ak
s'
f1 f2 ...
id0 t1 t2 ... tk
id1 t1 t2 ... tk
...
```
where `k` is the number of states in the resulting DFA; the alphabet line is unchanged from the input; `s'` is the DFA's initial state (always renamed to `0`); the next line lists the DFA's final states (space-separated, empty line if none); and the following `k` lines give each DFA state's own renamed id followed by its transition target for each symbol.

This mirrors the DFA table format used in Assignment 1, so the output of this program can be fed directly into that assignment's minimizer if desired.

## Algorithm explanation

The program implements the **Subset Construction** algorithm from Kozen (1997), *Automata and Computability*, Lecture 6, which proves that every NFA has an equivalent DFA.

Given an NFA `N = (Q, Sigma, Delta, S, F)`, where `S subset Q` is a *set* of start states and `Delta : Q x Sigma -> 2^Q` maps a state and a symbol to a *set* of possible next states, the construction builds a DFA `M = (Q', Sigma, delta', s', F')` whose states are subsets of `Q`:

- **States:** `Q' = 2^Q` in principle, but we only ever construct the subsets that are actually **reachable** from `S`, since the rest are irrelevant to the language accepted.
- **Initial state:** `s' = S` — the NFA's entire set of start states becomes a single DFA state.
- **Transition function:** `delta'(T, a) = union of Delta(q, a) for every q in T` — from a DFA state `T` (itself a set of NFA states), symbol `a` leads to the union of everywhere any state in `T` could go on `a`.
- **Final states:** `F' = { T in Q' : T intersect F is nonempty }` — a DFA state is accepting if *any* of the NFA states it represents is accepting, which correctly captures the NFA's nondeterministic "there exists an accepting run" semantics.

### Implementation (worklist / BFS over reachable subsets)

1. Start with `T0 = S` as DFA state `0`, and put it in a worklist.
2. While the worklist is non-empty, take a subset `T`, and for every alphabet symbol `a`, compute `T' = delta'(T, a)` by unioning the NFA transitions of every state in `T`.
3. If `T'` has not been seen before, assign it a new renamed id and add it to the worklist; either way, record the transition `T --a--> T'`.
4. Repeat until the worklist is empty. Because there are at most `2^n` distinct subsets of an `n`-state NFA, this always terminates.
5. A DFA state is marked final if the NFA subset it represents contains at least one state from `F`.

This naturally produces a **complete** DFA (a well-defined transition for every state and symbol), including a "dead state" (the empty set `{}`) whenever some combination of NFA transitions has no valid target — the empty set is itself a valid, non-final DFA state that loops back to itself on every symbol.

States are renamed to small integers `0, 1, 2, ...` in the order they are discovered (state `0` is always the initial state), rather than printed as literal subsets like `{1, 4, 5}`, per the assignment's note that renaming is allowed for readability.

## Notes

- No additional features (e.g., diagram rendering) were implemented; this covers the required specification only.
- The program assumes the input is well-formed, following the exact grammar shown in the assignment's example.
