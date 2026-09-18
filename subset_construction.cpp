// subset_construction.cpp
//
// SI2002 - Formal Languages, Assignment 2
// NFA -> DFA conversion via the Subset Construction algorithm described
// in Kozen (1997), Automata and Computability, Lecture 6.
//
// -------------------------------------------------------------------
// Algorithm summary
// -------------------------------------------------------------------
// Given an NFA N = (Q, Sigma, Delta, S, F), where S subset Q is a SET
// of start states and Delta : Q x Sigma -> 2^Q, the subset construction
// builds an equivalent DFA M = (Q', Sigma, delta', s', F') where:
//
//   Q'    = subsets of Q that are reachable from S
//   s'    = S                       (the NFA's whole start set, as one
//                                     DFA state)
//   delta'(T, a) = union of Delta(q, a) for all q in T
//   F'    = { T in Q' : T intersect F is nonempty }
//
// Rather than materializing all of 2^Q (which is exponential and
// mostly unreachable), we build only the REACHABLE subsets with a
// worklist/BFS starting from S: each time delta'(T, a) produces a
// subset we haven't seen before, we enqueue it. This terminates
// because there are at most 2^|Q| distinct subsets.
//
// The resulting DFA states are renamed to small integers 0, 1, 2, ...
// in discovery order (state 0 is always the initial state), since the
// assignment explicitly allows renaming states for readability instead
// of printing the literal subsets.
//
// -------------------------------------------------------------------
// Input format (as specified in the assignment)
// -------------------------------------------------------------------
// line 1:                c              (number of test cases)
// for each case:
//   line:                n              (number of NFA states, 1..n)
//   line:                s1 s2 ...      (initial states S, plain numbers)
//   line:                a1 a2 ... ak   (the alphabet, k symbols)
//   line:                f1 f2 ...      (final states F, plain numbers,
//                                        possibly none)
//   n lines (state 1..n, in order):
//                         id T1 T2 ... Tk
//                         where "id" is the state's own number and each
//                         Ti is either "0" (empty set) or a brace-
//                         delimited set like "{1 5}" giving Delta(id, ai).
//
// -------------------------------------------------------------------
// Output format
// -------------------------------------------------------------------
// The assignment leaves the exact table layout to us ("you may rename
// the states... indicate the initial state and final states"). We use
// a format that mirrors the DFA representation from Assignment 1, for
// consistency and easy re-use:
//
//   line: k                     (number of states in the resulting DFA)
//   line: a1 a2 ... ak          (alphabet, same order as the input)
//   line: s'                    (the DFA's initial state, always 0)
//   line: f1 f2 ...             (the DFA's final states, ascending,
//                                space-separated; empty line if none)
//   k lines: id t1 t2 ... tk    (one row per DFA state, 0..k-1, giving
//                                the transition target for each symbol,
//                                in alphabet order)

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>

using namespace std;

// ---- low-level line tokenizer that respects "{ ... }" groups ----
// Returns raw tokens: either a brace-delimited group (including the
// braces, e.g. "{1 5}") or a maximal run of non-space characters
// (e.g. a bare state id or "0").
static vector<string> tokenizeRow(const string &line) {
    vector<string> tokens;
    size_t i = 0, n = line.size();
    while (i < n) {
        while (i < n && isspace(static_cast<unsigned char>(line[i]))) ++i;
        if (i >= n) break;
        if (line[i] == '{') {
            size_t start = i;
            while (i < n && line[i] != '}') ++i;
            if (i < n) ++i; // consume '}'
            tokens.push_back(line.substr(start, i - start));
        } else {
            size_t start = i;
            while (i < n && !isspace(static_cast<unsigned char>(line[i]))) ++i;
            tokens.push_back(line.substr(start, i - start));
        }
    }
    return tokens;
}

// Splits a line into plain whitespace-separated tokens (no braces
// expected — used for the S and F lines).
static vector<string> tokenizePlain(const string &line) {
    vector<string> tokens;
    istringstream iss(line);
    string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

// Parses a transition-set token ("0" or "{a b c}") into a set of ints.
static set<int> parseSet(const string &tok) {
    set<int> result;
    if (tok.empty()) return result;
    if (tok[0] == '{') {
        string inner = tok.substr(1, tok.size() >= 2 ? tok.size() - 2 : 0);
        istringstream iss(inner);
        int x;
        while (iss >> x) result.insert(x);
    }
    // a bare token here is always "0" (empty set) per the spec, so
    // nothing to insert in that case.
    return result;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string line;
    if (!getline(cin, line)) return 0;
    int numCases = stoi(tokenizePlain(line)[0]);

    ostringstream out;

    for (int caseIndex = 0; caseIndex < numCases; ++caseIndex) {
        // --- number of NFA states ---
        getline(cin, line);
        int n = stoi(tokenizePlain(line)[0]);

        // --- initial states S ---
        getline(cin, line);
        vector<string> sTokens = tokenizePlain(line);
        set<int> S;
        for (const string &t : sTokens) S.insert(stoi(t));

        // --- alphabet ---
        getline(cin, line);
        vector<string> alphabet = tokenizePlain(line);
        int k = static_cast<int>(alphabet.size());

        // --- final states F ---
        getline(cin, line);
        set<int> F;
        for (const string &t : tokenizePlain(line)) F.insert(stoi(t));

        // --- NFA transition table: delta[state-1][symbolIndex] = set of targets ---
        vector<vector<set<int>>> delta(n, vector<set<int>>(k));
        for (int i = 0; i < n; ++i) {
            getline(cin, line);
            vector<string> row = tokenizeRow(line);
            // row[0] is the state's own id; row[1..k] are the
            // transition sets for a1..ak, in that order.
            for (int a = 0; a < k; ++a) {
                delta[i][a] = parseSet(row[a + 1]);
            }
        }

        // --- subset construction (BFS over reachable subsets) ---
        map<set<int>, int> stateId;   // subset -> renamed DFA state id
        vector<set<int>> discovered;  // renamed id -> subset (for reference)
        queue<set<int>> worklist;

        stateId[S] = 0;
        discovered.push_back(S);
        worklist.push(S);

        vector<vector<int>> dfaTrans; // dfaTrans[dfaState][symbolIdx] = target dfaState
        dfaTrans.push_back(vector<int>(k, -1));

        while (!worklist.empty()) {
            set<int> cur = worklist.front();
            worklist.pop();
            int curId = stateId[cur];

            for (int a = 0; a < k; ++a) {
                set<int> next;
                for (int q : cur) {
                    // q is a 1-indexed NFA state; delta is 0-indexed
                    for (int t : delta[q - 1][a]) next.insert(t);
                }
                auto it = stateId.find(next);
                int nextId;
                if (it == stateId.end()) {
                    nextId = static_cast<int>(discovered.size());
                    stateId[next] = nextId;
                    discovered.push_back(next);
                    dfaTrans.push_back(vector<int>(k, -1));
                    worklist.push(next);
                } else {
                    nextId = it->second;
                }
                dfaTrans[curId][a] = nextId;
            }
        }

        int kStates = static_cast<int>(discovered.size());

        // final DFA states: any discovered subset intersecting F
        vector<int> finalStates;
        for (int id = 0; id < kStates; ++id) {
            const set<int> &subset = discovered[id];
            bool isFinal = false;
            for (int q : subset) {
                if (F.count(q)) { isFinal = true; break; }
            }
            if (isFinal) finalStates.push_back(id);
        }

        // --- print this case's output ---
        out << kStates << "\n";
        for (int a = 0; a < k; ++a) {
            if (a > 0) out << " ";
            out << alphabet[a];
        }
        out << "\n";
        out << 0 << "\n"; // initial state is always renamed id 0
        for (size_t idx = 0; idx < finalStates.size(); ++idx) {
            if (idx > 0) out << " ";
            out << finalStates[idx];
        }
        out << "\n";
        for (int id = 0; id < kStates; ++id) {
            out << id;
            for (int a = 0; a < k; ++a) out << " " << dfaTrans[id][a];
            out << "\n";
        }
    }

    cout << out.str();
    return 0;
}
