#include "search.h"
#include <fstream>

static Move_t killers[Max_ply][Killer_size];
cgirl::line_manager lineManager;
GameList glist;

bool interrupt(SearchInfo& si)
{
    // Check to see if we have run out of time.
    if (system_time() - si.start_time >= si.move_time && !si.infinite)
    {
        si.quit = true;
        return true;
    }

    if (input_waiting())
    {
        std::string command(get_input());
        if (command == "quit" || command == "stop")
        {
            si.quit = true;
            return true;
        }
    }

    // The GUI should be able to send quit commands as well.
    // I may need multiple threads for this.
    return false;
}

int qsearch(State& s, SearchInfo& si, int d, int alpha, int beta)
{
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
    mlist.sort(s);

    int val;
    Move_t m;
    State c;

    while (mlist.size() > 0)
    {
        m = mlist.pop();                         // Get the next move.
        // TODO: remove "4" constant.
        if (get_score(m) <= C)                   // Break if a quiet move is found.
            break;
        std::memmove(&c, &s, sizeof s);          // Copy current state.
        c.make_t(m);                               // Make move.
        val = -qsearch(c, si, d - 1, -beta, -alpha); // Recursive call to qsearch.
        if (val >= beta)                         // Alpha-Beta pruning.
            return beta;
        alpha = std::max(alpha, val);
    }
    return alpha;                                // Fail-Hard alpha beta score.
}

int scout_search(State& s, SearchInfo& si, int depth, int ply, int alpha, int beta)
{
    Move_t best_move = nullMove;

    if (si.quit || (si.nodes % 3000 == 0 && interrupt(si)))
        return 0;

    si.nodes++;
    // Check for draw.
    if (glist.repeat() || s.getFiftyMoveRule() > 99)
        return Draw;

    // Evaluate leaf nodes.
    if (depth == 0)
        return qsearch(s, si, depth, alpha, beta);

    // Get a pointer to the correction transposition table location.
    const TableEntry* table_entry = ttable.probe(s.getKey());

    // Check if table entry is valid and matches the position key.
    if (table_entry->key == s.getKey() && table_entry->depth >= depth)
    {
        if (table_entry->type == pv)             // PV Node, return the score.
            return table_entry->score;
        else if (table_entry->type == cut)       // Cut Node, adjust alpha.
        {
            if (table_entry->score >= beta)
                return beta;
        }
        else
        {
            if (table_entry->score <= alpha)
                return alpha;
        }

        best_move = table_entry->best;
    }

    // Generate moves and create the movelist.
    MoveList mlist;
    push_moves(s, &mlist);

    // Check if the position is checkmate/stalemate.
    if (mlist.size() == 0)
        return s.check() ? -Checkmate : Stalemate;

    // Check if we are at the PV line.
    if (lineManager.get_pv_key(ply) == s.getKey())
    {
        best_move = lineManager.get_pv_move(ply);
    }

    // Internal Iterative Deepening. If no best move was found, do a small
    // search to determine which move to search first.
    if (best_move == nullMove && depth > 5)
    {
        // Using depth calculation from Stockfish.
        int d = 3 * depth / 4 - 2;
        scout_search(s, si, d, ply, alpha, beta);
        table_entry = ttable.probe(s.getKey());
        if (table_entry->key == s.getKey())
            best_move = table_entry->best;
    }

    // Extract the best move to the front and sort the remaining moves.
    mlist.extract(best_move);
    mlist.sort(s);

    // Confirm a killer move has been stored at this ply.
    if (killers[ply][0] != nullMove && mlist.size() > Killer_size) 
        mlist.order_killer(killers[ply]);

    int a = alpha;
    int b = beta;
    int val;
    Move_t m;
    State c;
    bool first = true;

    while (mlist.size() > 0)
    {
        std::memmove(&c, &s, sizeof s);    // Copy current state.
        m = mlist.pop();                   // Get the next move.
        c.make_t(m);                         // Make move.
        glist.push(m, c.getKey());              // Add move to gamelist.

        // Scout alrogithm. Search the first node with a full window.
        if (first)
        {
            // Set the best move to the first move just in case no move
            // improves alpha.
            best_move = m;
            val = -scout_search(c, si, depth - 1, ply + 1, -b, -a);
            first = false;
        }       
        else
        {
            val = -scout_search(c, si, depth - 1, ply + 1, -(a + 1), -a);

            // If an alpha improvement caused fail high, research using a full window.
            if (a < val && b > val)
                val = -scout_search(c, si, depth - 1, ply + 1, -b, -a);
        }

        --glist;                           // Remove move from gamelist.

        // If alpha was improved, update alpha and store the best move. Also
        // update the pv array.
        if (val > a)
        {
            a = val;
            best_move = m;
        }

        // Alpha-Beta pruning. If the move that caused the Beta cutoff is a
        // quiet move, store it as a killer.
        if (a >= b)
        {   
            if (is_quiet(best_move) && best_move != killers[ply][0])
            {
                killers[ply][1] = killers[ply][0];
                killers[ply][0] = best_move;
            }
            break;
        }
    }

    if (a > alpha && a < b && !si.quit)
    {
        lineManager.push_to_pv(best_move, s.getKey(), ply, a);
    }

    // Store in transposition table, using depth first replacement.
    ttable.store(s.getKey(), best_move, a <= alpha ? all : a >= b ? cut : pv, depth, a);

    // Fail-Hard alpha beta score.
    return a;

}

void iterative_deepening(State& s, SearchInfo& si)
{
    int score;

    // Iterative deepening.
    for (int d = 1; /*d < 4*/!si.quit; ++d)
    {
        score = scout_search(s, si, d, 0, Neg_inf, Pos_inf);

        if (lineManager.get_pv_move() == nullMove)
        {
            std::cout << "bestmove 0000" << std::endl;
            return;
        }

        if (si.quit)
            break;

        // Confirm all the pv moves are legal.
        lineManager.check_pv(s);

        // Print info to gui.
        std::cout << "info "
                  << "depth " << d;

        if (lineManager.is_mate())
        {
            int n = lineManager.get_mate_in_n();
            std::cout << " score mate " << (score > 0 ? n : -n);
        }
        else
        {
            std::cout << " score cp " << score;
        }

        std::cout << " time " << system_time() - si.start_time
                  << " nodes " << si.nodes
                  << " nps " << si.nodes / (system_time() - si.start_time + 1) * 1000;   
        lineManager.print_pv();
        std::cout << std::endl;

        // Reset node count.
        si.nodes = 0;
    }

    s.make_t(lineManager.get_pv_move());
    glist.push_root(lineManager.get_pv_move(), s.getKey());
    std::cout << "bestmove " << toString(lineManager.get_pv_move()) << std::endl;
}

void setup_search(State& s, SearchInfo& si)
{
    ttable.clear();
    lineManager.clear_pv();
    search_init();
    iterative_deepening(s, si);
}

void search_init()
{
    int i, j;

    for (i = 0; i < Max_ply; ++i)
    {
        for (j = 0; j < Killer_size; ++j)
        {
            killers[i][j] = nullMove;
        }
    }
}