#include "search.h"

int search_nodes = 0;
int table_hits = 0;

static PV pvlist[Max_ply];

int negamax(State & s, int d, int alpha, int beta)
{
    search_nodes += 1;
    int a = alpha;
    int b = beta;
    bool entry_flag = false;
    Move best_move = No_move;

    // Get a reference to the correct transposition table location.
    TableEntry& tte = ttable.get(s.key);

    // Check if table entry is valid and matches the position key.
    if (tte.key == s.key && tte.depth >= d)
    {
        table_hits++;
        if (tte.type == pv)             // PV Node, return the score.
            return tte.score;
        else if (tte.type == cut)       // Cut Node, adjust alpha.
            a = std::max(a, tte.score);
        else                            // All Node, adjust beta.
            b = std::min(b, tte.score);

        // If an early cutoff is caused by the entry, return it's score.
        if (a >= b)
            return tte.score;

        best_move = tte.best;
    }
    // If no transposition is found, look for a principle variation.
    else if (d > 1 && pvlist[d - 1].key == s.key)
        best_move = pvlist[d - 1].move;

    // Evaluate leaf nodes.
    if (d == 0)
        return evaluate(s);

    // Generate moves and create the movelist.
    MoveList mlist;
    push_moves(s, &mlist);

    // Check if the position is checkmate/stalemate.
    if (mlist.size() == 0)
        return s.check() ? Checkmate : Stalemate;

    // Extract the best move to the front and sort the remaining moves.
    mlist.extract(best_move);
    mlist.sort();

    int best = Neg_inf;
    int val;
    Move m;
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
        if (a >= b)                        // Alpha-Beta pruning.
            break;
    }

    // Store information to the TableEntry reference.
    // Current method is to always replace.
    tte.best  = best_move;
    tte.score = best;
    tte.type  = best <= a ? all : best >= b ? cut : pv;
    tte.depth = d;
    tte.key   = s.key;

    // Store principle variation in the pvlist.
    if (tte.type == pv)
        pvlist[d] = PV(best_move, s.key);

    return best;                           // Fail-Soft alpha beta score.
}

Move search(State & s)
{
    const int depth = 6; // Depth to search. Will adjust this later.
    int a, d, i;

    MoveList mlist;
    push_moves(s, &mlist);
    mlist.c = mlist.e;
    mlist.sort();

    std::vector<Candidate> candidates;
    std::vector<Candidate>::iterator it;

    while (mlist.size() > 0)
        candidates.insert(candidates.begin() ,Candidate(mlist.pop(), 0));

    State c;
    Move m;
    // Need to check and return a null move if it's checkmate/stalemate.

    // Iterative deepening.
    for (d = 1; d <= depth; ++d)
    {
        a = Neg_inf;
        for (it = candidates.end() - 1; it >= candidates.begin(); --it)
        {
            std::memmove(&c, &s, sizeof s);
            c.make(it->move);
            it->score = -negamax(c, d - 1, Neg_inf, -a);
            a = std::max(a, it->score);
        }
        std::stable_sort(candidates.begin(), candidates.end());
    }
    std::cout << search_nodes << '\n';
    std::cout << table_hits << '\n';
    return candidates.back().move;
}