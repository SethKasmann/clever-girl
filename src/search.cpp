#include "search.h"

int search_nodes = 0;
int table_hits = 0;

static PV pvlist[Max_ply];
static Move killers[Max_ply][Killer_size];
GameList glist;

int qsearch(State& s, int d, int alpha, int beta)
{
    int qscore = evaluate(s);

    // If a beta cutoff is found, return the qscore.
    if (qscore >= beta)
        return beta;

    // Update alpha.
    alpha = std::max(alpha, qscore);

    // Generate moves and create the movelist.
    MoveList mlist;
    push_moves(s, &mlist);
    mlist.sort();

    int val;
    Move m;
    State c;

    while (mlist.size() > 0)
    {
        m = mlist.pop();                         // Get the next move.
        // TODO: remove "4" constant.
        if (get_score(m) <= 5)                   // Break if a quiet move is found.
            break;
        std::memmove(&c, &s, sizeof s);          // Copy current state.
        c.make(m);                               // Make move.
        val = -qsearch(c, d - 1, -beta, -alpha); // Recursive call to qsearch.
        if (val >= beta)                         // Alpha-Beta pruning.
            return beta;
        alpha = std::max(alpha, val);
    }
    return alpha;                                // Fail-Hard alpha beta score.
}

template<NodeType NT>
int negamax(State & s, int d, int alpha, int beta)
{
    search_nodes += 1;
    int a = alpha;
    int b = beta;
    Move pv_move = No_move;
    int i;

    // Check for draw.
    if (glist.repeat() || s.fmr > 99)
        return Draw;

    // Get a reference to the correct transposition table location.
    const TableEntry* test = ttable.probe(s.key);

    // Check if table entry is valid and matches the position key.
    if (test->key == s.key && test->depth >= d)
    {
        table_hits++;
        if (test->type == pv)             // PV Node, return the score.
            return test->score;
        else if (test->type == cut)       // Cut Node, adjust alpha.
            a = std::max(a, test->score);
        else                            // All Node, adjust beta.
            b = std::min(b, test->score);

        // If an early cutoff is caused by the entry, return it's score.
        if (a >= b)
            return test->score;

        pv_move = test->best;
    }

    // Evaluate leaf nodes.
    if (d == 0)
        return qsearch(s, d, a, b);

    // Generate moves and create the movelist.
    MoveList mlist;
    push_moves(s, &mlist);

    // Check if the position is checkmate/stalemate.
    if (mlist.size() == 0)
        return s.check() ? Checkmate : Stalemate;

    if (NT == pv && d > 1)
    {
        if (pvlist[d - 1].key == s.key)
            pv_move = pvlist[d - 1].move;
        else
            ;
            // Internal Iterative Deepening
    }

    // Extract the best move to the front and sort the remaining moves.
    mlist.extract(pv_move);
    mlist.sort();

    // Confirm a killer move has been stored at this ply.
    if (killers[glist.ply()][0] != No_move) 
        mlist.order_killer(killers[glist.ply()]); 

    int best = Neg_inf;
    int val;
    Move m, best_move;
    State c;

    while (mlist.size() > 0)
    {
        std::memmove(&c, &s, sizeof s);    // Copy current state.
        m = mlist.pop();                   // Get the next move.
        c.make(m);                         // Make move.
        glist.push(m, c.key);              // Add move to gamelist.

        // Scout alrogithm. Search pv_move with a full window.
        if (m == pv_move && NT == pv)
            val = -negamax<pv>(c, d - 1, -b, -a);
        else
        {
            // Search all other nodes with a full window.
            val = NT == cut ? -negamax<all>(c, d - 1, -(a + 1), -a)
                            : -negamax<cut>(c, d - 1, -(a + 1), -a);

            // If an alpha improvement caused fail high, research using
            // a full window.
            if (a < val && b > val)
                val = -negamax<pv>(c, d - 1, -b, -a);
        }

        --glist;                           // Remove move from gamelist.
        if (val > best)
        {
            best = val;                    // Store the best value.
            best_move = m;                 // Store the best move.
        }
        a = std::max(a, best);
        if (a >= b)                        // Alpha-Beta pruning.
        {   
            a = b;
            // If a quiet move caused a beta cut off, store as a killer move.
            if (is_quiet(m) && m != killers[glist.ply()][0])
            {
                killers[glist.ply()][1] = killers[glist.ply()][0];
                killers[glist.ply()][0] = m;
            }
            break;
        }
    }

    // Store in transposition table, using depth first replacement.
    ttable.store(s.key, best_move, a <= alpha ? all : a >= b ? cut : pv, d, a);

    // Store principle variation if alpha was improved.
    if (a > alpha && a < b)
        pvlist[d] = PV(best_move, s.key);

    return a;                           // Fail-Hard alpha beta score.
}

Move search(State & s)
{
    const int depth = 6; // Depth to search. Will adjust this later.
    int a, d, i;

    MoveList mlist;
    push_moves(s, &mlist);

    if (mlist.size() == 0)
    {
        std::cout << "0000" << '\n';
        return No_move;
    }

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
            glist.push(it->move, c.key);

            //it->score = -negamax<pv>(c, d - 1, Neg_inf, -a);
            if (it->move == candidates.back().move)
                it->score = -negamax<pv>(c, d - 1, Neg_inf, -a);
            else
            {
                it->score = -negamax<cut>(c, d - 1, -(a + 1), -a);
                if (it->score > a)
                    it->score = -negamax<pv>(c, d - 1, Neg_inf, -a);
            }

            --glist;
            a = std::max(a, it->score);
        }
        std::stable_sort(candidates.begin(), candidates.end());
    }

    s.make(candidates.back().move);
    glist.push_root(candidates.back().move, s.key);
    std::cout << search_nodes << '\n';
    std::cout << table_hits << '\n';
    return candidates.back().move;
}

void search_init()
{
    int i, j;

    for (i = 0; i < Max_ply; ++i)
    {
        for (j = 0; j < Killer_size; ++j)
        {
            killers[i][j] = No_move;
        }
    }
}