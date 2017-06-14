#include "search.h"

int search_nodes = 0;
int table_hits = 0;

int negamax(State & s, int d, int alpha, int beta)
{
    search_nodes += 1;
    int a = alpha;
    int b = beta;

    bool entry_flag = false;

    // Get a reference to the correct transposition table location.
    TableEntry& tte = ttable.get(s.key);
    // Check if table entry is valid and matches the position key.
    if (tte.key == s.key && tte.depth >= d)
    {
        table_hits++;
        if (tte.type == pv)
            return tte.score;
        else if (tte.type == cut)
            a = std::max(a, tte.score);
        else
            b = std::min(b, tte.score);

        // If an early cutoff is caused by the entry, return it's score.
        if (a >= b)
            return tte.score;

        entry_flag = true;
    }

    // Evaluate leaf nodes.
    if (d == 0)
        return evaluate(s);

    // Generate moves and create the movelist.
    MoveList mlist;
    push_moves(s, &mlist);

    // Check if the position is checkmate/stalemate.
    if (mlist.size() == 0)
        return s.check() ? Checkmate : Stalemate;

    int best = Neg_inf;
    int val;
    Move m, best_move;
    // If an entry is found, sort the movelist with the pv move at the front.
    // If no entry is found, sort normally.
    if (entry_flag)
        mlist.sort_pv(tte.best);
    else
        mlist.sort();

    State c;
    while (mlist.size() > 0)
    {
        std::memmove(&c, &s, sizeof s);    // Copy current state.
        m = mlist.pop();                   // Get the next move.
        c.make(m);                         // Make move.
        val = -negamax(c, d - 1, -b, -a);  // Recursive search call.
        if (val > best)
        {
            best = val;                    // Store the best value.
            best_move = m;                 // Store the best move.
        }
        a = std::max(a, best);
        if (a >= b)                        // Alpha beta pruning.
            break;
    }

    // Store information to the TableEntry reference.
    // Current method is to always replace.
    tte.best  = best_move;
    tte.score = best;
    tte.type  = best <= a ? all : best >= b ? cut : pv;
    tte.depth = d;
    tte.key   = s.key;

    return best;
}

Move search(State & s)
{
    const int d = 7; // Depth to search. Will adjust this later.
    int a = Neg_inf;

    MoveList mlist;
    push_moves(s, &mlist);

    State c;
    Move m;
    std::vector<Candidate> candidates;
    // Need to check and return a null move if it's checkmate/stalemate.
    while (mlist.size() > 0)
    {
        std::memmove(&c, &s, sizeof s);
        m = mlist.pop();
        c.make(m);
        candidates.push_back(Candidate(m, negamax(c, d - 1, Neg_inf, -a)));
        a = std::max(a, candidates.back().score);
    }
    std::stable_sort(candidates.begin(), candidates.end());
    std::cout << search_nodes << '\n';
    std::cout << table_hits << '\n';
    return candidates.front().move;
}