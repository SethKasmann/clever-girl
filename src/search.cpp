#include "search.h" 

static PV pvlist[Max_ply];
static Move killers[Max_ply][Killer_size];
GameList glist;

bool interrupt(SearchInfo& si)
{
    // Check to see if we have run out of time.
    if (system_time() - si.start_time >= si.move_time)
    {
        std::cout << "quitting...\n";
        si.quit = true;
        return true;
    }

    // The GUI should be able to send quit commands as well.
    // I may need multiple threads for this.
    return false;
}

int qsearch(State& s, SearchInfo& si, int d, int alpha, int beta)
{
    if (si.quit || (si.nodes % 3000 == 0 && interrupt(si)))
        return 0;

    si.nodes++;

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
        val = -qsearch(c, si, d - 1, -beta, -alpha); // Recursive call to qsearch.
        if (val >= beta)                         // Alpha-Beta pruning.
            return beta;
        alpha = std::max(alpha, val);
    }
    return alpha;                                // Fail-Hard alpha beta score.
}

template<NodeType NT>
int negamax(State & s, SearchInfo& si, int d, int alpha, int beta)
{
    int a = alpha;
    int b = beta;
    Move pv_move = No_move;
    int i;

    if (si.quit || (si.nodes % 3000 == 0 && interrupt(si)))
        return 0;

    si.nodes++;
    // Check for draw.
    if (glist.repeat() || s.fmr > 99)
    {
        //std::cout << "found a draw\n";
        return Draw;
    }

    // Get a reference to the correct transposition table location.
    const TableEntry* test = ttable.probe(s.key);

    // Check if table entry is valid and matches the position key.
    if (test->key == s.key && test->depth >= d)
    {
        std::cout << "TT hit " << test->key << " " << s.key << " " << test->depth << '\n';
        if (test->type == pv)             // PV Node, return the score.
        {
            std::cout << "returning PV...\n";
            return test->score;
        }
        else if (test->type == cut)       // Cut Node, adjust alpha.
            a = std::max(a, test->score);
        else                            // All Node, adjust beta.
            b = std::min(b, test->score);

        // If an early cutoff is caused by the entry, return it's score.
        if (a >= b)
        {
            std::cout << "TT cutoff\n";
            return test->score;
        }

        pv_move = test->best;
    }

    // Evaluate leaf nodes.
    if (d == 0)
        return qsearch(s, si, d, a, b);

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
            val = -negamax<pv>(c, si, d - 1, -b, -a);
        else
        {
            // Search all other nodes with a full window.
            val = NT == cut ? -negamax<all>(c, si, d - 1, -(a + 1), -a)
                            : -negamax<cut>(c, si, d - 1, -(a + 1), -a);

            // If an alpha improvement caused fail high, research using
            // a full window.
            if (a < val && b > val)
                val = -negamax<pv>(c, si, d - 1, -b, -a);
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

Move search(State& s, SearchInfo& si, std::vector<RootMove>& rmoves)
{
    std::vector<RootMove>::iterator it;
    State c;
    RootMove best;
    int a, d;

    if (rmoves.empty())
    {
        std::cout << "0000" << '\n';
        return No_move;
    }

    // Iterative deepening.
    for (d = 1; !si.quit; ++d)
    {
        a = Neg_inf;
        // Reverse sort to bring the best moves to the front.
        std::stable_sort(rmoves.begin(), rmoves.end(), std::greater<RootMove>());
        for (it = rmoves.begin(); it != rmoves.end(); ++it)
        {
            std::memmove(&c, &s, sizeof s); // Copy current state.
            c.make(it->move);               // Make move.
            glist.push(it->move, c.key);    // Push new move to the game list.

            it->prev_score = it->score;
            // Search PV with a full window.
            if (it == rmoves.begin())
                it->score = -negamax<pv>(c, si, d - 1, Neg_inf, -a);
            else
            {
                // Search other nodes with a null window.
                it->score = -negamax<cut>(c, si, d - 1, -(a + 1), -a);
                // Perform a research on fail high.
                if (it->score > a)
                    it->score = -negamax<pv>(c, si, d - 1, Neg_inf, -a);
            }
            --glist;
            if (si.quit)
                break;
            a = std::max(a, it->score);     // Update alpha.
        }
        if (si.quit)
            break;
        best = *std::max_element(rmoves.begin(), rmoves.end());
        if (si.quit)
            std::cout << "si.quit got changed to true...\n";

        // Print info to gui.
        std::cout << "info "
                  << "depth " << d
                  << " time " << system_time() - si.start_time
                  << " nodes " << si.nodes
                  << " nps " << si.nodes / (system_time() - si.start_time + 1) * 1000
                  << " score cp " << best.score
                  << " pv ";
        std::cout << to_string(best.move) << " ";
        for (int i = 1; i < d; ++i)
        {
            std::cout << to_string(pvlist[i].move) << " ";
        }
        std::cout << '\n';

        // Reset node count.
        si.nodes = 0;
    }

    s.make(best.move);
    glist.push_root(best.move, s.key);
    std::cout << s;
    std::cout << to_string(best.move) << '\n';
    return best.move;
}

void setup_search(State& s, SearchInfo& si)
{
    std::vector<RootMove> rmoves;
    RootMove r;
    MoveList mlist;

    // Initialize only certain root moves if specified by the uci.
    if (!si.sm.empty())
    {
        for (std::vector<Move>::iterator it = si.sm.begin(); it != si.sm.end(); ++it)
        {
            r.move  = *it;
            r.score = get_score(*it);
            rmoves.push_back(r);
        }
    }
    // Initialize all possible root moves.
    else
    {
        push_moves(s, &mlist);
        while (mlist.size() > 0)
        {
            r.move  = mlist.pop();
            r.score = get_score(r.move);
            rmoves.push_back(r);
        }
    }
    ttable.resize(ttable.size());
    search(s, si, rmoves);
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