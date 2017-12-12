#include "search.h"
#include <fstream>

static Move killers[Max_ply][Killer_size];
cgirl::line_manager lineManager;
GameList glist;
History history;

bool interrupt(SearchInfo& si)
{
    // Check to see if we have run out of time.
    if (system_time() - si.start_time >= si.moveTime && !si.infinite)
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

int qsearch(State& s, SearchInfo& si, int ply, int alpha, int beta)
{
    si.nodes++;
    assert(ply < Max_ply);

    int qscore = evaluate(s);
    /*
    std::cout << " Ply: " << ply << '\n';
    std::cout << "Alpha: " << alpha << " Beta: " << beta << '\n';
    std::cout << "BEGINNING Q SEARCH.\n";
    std::cout << "QSCORE: " << qscore << '\n';
    std::cout << s;
    int z;
    std::cin >> z;
    */


    // If a beta cutoff is found, return the qscore.
    if (qscore >= beta)
    {
        //std::cout << "QSCORE >= BETA, returning: " << beta << '\n';
        return beta;
    }

    // Update alpha.
    alpha = std::max(alpha, qscore);

    // Generate moves and create the movelist.
    MoveList mlist(s, nullMove, &history, ply, true);

    int val;
    Move m;
    State c;

    while (m = mlist.getBestMove())
    {
        std::memmove(&c, &s, sizeof s);          // Copy current state.
        c.make_t(m);                               // Make move.
        val = -qsearch(c, si, ply + 1, -beta, -alpha); // Recursive call to qsearch.
        if (val >= beta)                         // Alpha-Beta pruning.
        { 
            /*
            std::cout << " Ply: " << ply << '\n';
            std::cout << "Alpha: " << alpha << " Beta: " << beta << '\n';
            std::cout << "Val: " << val << '\n';
            std::cout << "BETA CUTOFF, returning: " << beta << '\n';
            std::cout << s;
            std::cin >> z;*/
            return beta;
        }
/*
        alpha = std::max(alpha, val);
        std::cout << " Ply: " << ply << '\n';
        std::cout << "Alpha: " << alpha << " Beta: " << beta << '\n';
        std::cout << "CONTINUING Q SEARCH.\n";
        std::cout << s;
        int z;
        std::cin >> z;*/
    }
    /*
    std::cout << " Ply: " << ply << '\n';
    std::cout << "Alpha: " << alpha << " Beta: " << beta << '\n';
    std::cout << "Q SEARCH DONE. Returning: " << alpha << '\n';
    std::cout << s;
    std::cin >> z;*/
    return alpha;                                // Fail-Hard alpha beta score.
}

int scout_search(State& s, SearchInfo& si, int depth, int ply, int alpha, int beta)
{
    Move best_move = nullMove;
/*
    std::cout << "Depth: " << depth << " Ply: " << ply << '\n';
    std::cout << "Alpha: " << alpha << " Beta: " << beta << '\n';
    std::cout << "BEGINNING REGULAR SEARCH.\n";
    std::cout << s;
    int z;
    std::cin >> z;*/

    if (si.quit || (si.nodes % 3000 == 0 && interrupt(si)))
    {
        //std::cout << "Interrupt detected\n";
        return 0;
    }

    si.nodes++;
    // Check for draw.
    if (history.isThreefoldRepetition(s) || s.getFiftyMoveRule() > 99)
    {
        //std::cout << "Threefold or 50MR\n";
        return Draw;
    }

    // Evaluate leaf nodes.
    if (depth == 0)
        return qsearch(s, si, ply, alpha, beta);

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
        //std::cout << "Begin IDD\n";
        scout_search(s, si, d, ply, alpha, beta);
        table_entry = ttable.probe(s.getKey());
        //std::cout << "End IDD\n";
        if (table_entry->key == s.getKey())
            best_move = table_entry->best;
    }

    // Generate moves and create the movelist.
    MoveList mlist(s, best_move, &history, ply);

    int a = alpha;
    int b = beta;
    int d;
    int score;
    int bestScore = Neg_inf;
    Move m;
    State c;
    bool first = true;

    while (m = mlist.getBestMove())
    {
        d = depth - 1;
        std::memmove(&c, &s, sizeof s);              // Copy current state.
        c.make_t(m);                                 // Make move.
        history.push(std::make_pair(m, c.getKey())); // Add move to gamelist.

        if (c.inCheck() && (depth == 1 || s.see(m) > -50))
        {
            std::cout << c;
            std::cout << s.see(m) << '\n';
            int z;
            std::cin >> z;
            d++;
        }

        // Scout alrogithm. Search the first node with a full window.
        if (first)
        {
            // Set the best move to the first move just in case no move
            // improves alpha.
            //std::cout << "Full Search\n";
            best_move = m;
            score = -scout_search(c, si, d, ply + 1, -b, -a);
            first = false;
        }       
        else
        {
            //std::cout << "Scout Search\n";
            score = -scout_search(c, si, d, ply + 1, -(a + 1), -a);

            // If an alpha improvement caused fail high, research using a full window.
            if (a < score && b > score)
            {
                //std::cout << "Full research\n";
                score = -scout_search(c, si, d, ply + 1, -b, -a);
            }
        }

        history.pop();                           // Remove move from gamelist.

        if (score > bestScore)
        {
            best_move = m;
            bestScore = score;
        }

        a = std::max(a, bestScore);

        // Alpha-Beta pruning. If the move that caused the Beta cutoff is a
        // quiet move, store it as a killer.
        if (a >= b)
        {
            a = b;
            if (s.isQuiet(m))
            {
                
                //std::cout << "Storing a killer!: " << toString(m) << '\n';
                //std::cin >> z;
                history.update(m, depth, ply, true);
            }
            break;
        }
        else
        {
            if (s.isQuiet(m))
                history.update(m, depth, ply, false);
        }
        /*
        std::cout << "Depth: " << depth << " Ply: " << ply << '\n';
        std::cout << "Alpha: " << a << " Beta: " << b << '\n';
        std::cout << "CONTINUING REGULAR SEARCH.\n";
        std::cout << s;
        int z;
        std::cin >> z;*/
    }

    if (bestScore == Neg_inf)
        return s.check() ? -Checkmate : Stalemate;

    if (a > alpha && a < b && !si.quit)
    {
        lineManager.push_to_pv(best_move, s.getKey(), ply, a);
    }

    // Store in transposition table, using depth first replacement.
    ttable.store(s.getKey(), best_move, a <= alpha ? all : a >= b ? cut : pv, depth, a);
/*
    std::cout << "Depth: " << depth << " Ply: " << ply << '\n';
    std::cout << "Alpha: " << a << " Beta: " << b << '\n';
    std::cout << "ALL CHILDREN SEARCHED.\n";
    std::cout << "REG-SCORE: " << a << '\n';
    std::cout << s;
    std::cin >> z;*/

    // Fail-Hard alpha beta score.
    return a;

}

void iterative_deepening(State& s, SearchInfo& si)
{
    int score;

    // Iterative deepening.
    for (int d = 1; !si.quit; ++d)
    {
        //std::cout << "SEARCH CALL BEGIN D = " << d << '\n';
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
    history.push(std::make_pair(lineManager.get_pv_move(), s.getKey()));
    std::cout << "bestmove " << toString(lineManager.get_pv_move()) << std::endl;
}

void setup_search(State& s, SearchInfo& si)
{
    ttable.clear();
    lineManager.clear_pv();
    search_init();
    iterative_deepening(s, si);
    init_eval();
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