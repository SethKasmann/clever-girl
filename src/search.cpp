#include "search.h"
#include "misc.h"
#include <fstream>

LineManager lineManager;
History history;

bool interrupt(SearchInfo& si)
{
    // Check to see if we have run out of time.
    if (si.clock.elapsed<std::chrono::milliseconds>() >= si.moveTime
        && !si.infinite)
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

    if (history.isThreefoldRepetition(s) || 
        s.getFiftyMoveRule() > 99)
        return Draw;

    Evaluate evaluate(s);
    /*
    std::cout << evaluate;
    int z;
    std::cin >> z;
    */
    int qscore = evaluate.getScore();


    // If a beta cutoff is found, return the qscore.
    if (qscore >= beta)
        return beta;

    // Update alpha.
    alpha = std::max(alpha, qscore);

    // Generate moves and create the movelist.
    MoveList mlist(s, nullMove, &history, ply, true);

    int bestScore = Neg_inf;
    int score;
    Move m;
    State c;

    while (m = mlist.getBestMove())
    {
// ---------------------------------------------------------------------------//
//                                                                            //
// Futility pruning. Do not search subtrees that are unlikely to improve      //
// alpha. To avoid pruning away tactical positions, there are a few things    //
// we need to check:                                                          //
//   1. Side to move is not in check.                                         //
//   2. Move itself doesn't give check.                                       //
//   3. Move is not en passant. I want to be sure to calculate the fScore     //
//      properly, but en passant could throw a wrench by adding a 0 piece     //
//      value. Insead of a separate check to cover en passant moves, I am     //
//      currently just not cosidering them.                                   //
//                                                                            //
// ---------------------------------------------------------------------------//
        if (!s.inCheck() &&
            !s.givesCheck(m) &&
            !s.isEnPassant(m) &&
            !isPromotion(m))
        {
            // TODO:
            // Can I simply include en passant in this calculation?
            // Add a "gives check" function, so I dont have to make the move.
            int fScore = qscore + 100 + getPieceValue(s.onSquare(getDst(m)));
            if (fScore <= alpha)
            {
                bestScore = std::max(bestScore, fScore);
                continue;
            }
        }

        std::memmove(&c, &s, sizeof s);
        c.make_t(m);

        history.push(std::make_pair(m, c.getKey()));
        score = -qsearch(c, si, ply + 1, -beta, -alpha);
        history.pop();
        if (score >= bestScore)
            bestScore = score;

        alpha = std::max(alpha, bestScore);

        if (alpha >= beta)
            return beta;
    }

    if (bestScore == Neg_inf && s.check())
        return -Checkmate + ply;

    return alpha;                                // Fail-Hard alpha beta score.
}

int scout_search(State& s, SearchInfo& si, int depth, int ply, int alpha, int beta, bool isPv, bool isNull, bool isRoot)
{
    Move best_move = nullMove;
    
    if (si.quit || (si.nodes % 3000 == 0 && interrupt(si)))
        return 0;

    si.nodes++;
    // Check for draw.
    if ((!isRoot && history.isThreefoldRepetition(s)) || 
        s.getFiftyMoveRule() > 99)
        return Draw;

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
    if (lineManager.getPvKey(ply) == s.getKey())
    {
        best_move = lineManager.getPvMove(ply);
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Static Evaluation. Evaluate the current position statically if the         //
// current node is not a PV node.                                             //
//                                                                            //
// ---------------------------------------------------------------------------//
    int staticEval = 0;
    if (!isPv)
    {
        Evaluate evaluate(s);
        staticEval = evaluate.getScore();
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Reverse Futility Pruning. At pre-frontier and pre-pre-frontier, if the     //
// side to move is doing so well that the static evaluation - a futility      //
// penalty still causes a beta cutoff, return beta.                           //
//                                                                            //
// ---------------------------------------------------------------------------//
    if (!isPv && 
        !isNull &&
        !s.inCheck() &&
        depth <= 2 &&
        s.getNonPawnPieceCount(s.getOurColor()))
    {
        if (staticEval - 100 * depth >= beta)
            return beta;
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Null move pruning. Make a null move and searched to a reduced to check     //
// for fail high under the following conditions:                              //
//   1. The current node is not a PV node.                                    //
//   2. The current state is not in check.                                    //
//   3. The depth is high enough.                                             //
//   4. There are enough non pawn pieces on the board.                        //
//                                                                            //
// ---------------------------------------------------------------------------//
    if (!isPv && !isNull && !s.inCheck() && depth > NullMoveDepth 
        && s.getNonPawnPieceCount(s.getOurColor()) > NullMoveCount)
    {
        State n;
        std::memmove(&n, &s, sizeof s);
        n.makeNull();
        history.push(std::make_pair(nullMove, n.getKey()));
        int nullScore = -scout_search(n, si, depth - 3, ply + 1, -(alpha + 1), -alpha, false, true, false);
        history.pop();
        if (nullScore >= beta)
            return beta;
    }

    // Internal Iterative Deepening. If no best move was found, do a small
    // search to determine which move to search first.
    if (isPv && !isNull && !s.inCheck() && best_move == nullMove && depth > 5)
    {
        // Using depth calculation from Stockfish.
        int d = 3 * depth / 4 - 2;
        scout_search(s, si, d, ply, alpha, beta, isPv, true, false);
        table_entry = ttable.probe(s.getKey());
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
    int count = 0;

    while (m = mlist.getBestMove())
    {
        d = depth - 1;
// ---------------------------------------------------------------------------//
//                                                                            //
// Futility Pruning                                                           //
//                                                                            //
// ---------------------------------------------------------------------------//
        if (!isPv &&
            depth == 1 &&
            !s.inCheck() &&
            !s.givesCheck(m) &&
            s.isQuiet(m) &&
            !isPromotion(m) &&
            staticEval + 300 < a)
            continue;

        std::memmove(&c, &s, sizeof s);              // Copy current state.
        c.make_t(m);                                 // Make move.
        history.push(std::make_pair(m, c.getKey())); // Add move to gamelist.
        count++;

        if (c.inCheck() && depth == 1)
            d++;


        // Scout alrogithm. Search the first node with a full window.
        if (first)
        {
            // Set the best move to the first move just in case no move
            // improves alpha.
            best_move = m;
            score = -scout_search(c, si, d, ply + 1, -b, -a, isPv, isNull, false);
            first = false;
        }       
        else
        {
// ---------------------------------------------------------------------------//
//                                                                            //
// Late move reduction. Perform a reduced depth search (by 1 ply) under the   //
// following conditions:                                                      //
//   1. A number (LmrCount) of moves have already been tried.                 //
//   2. The current node is not a PV node.                                    //
//   3. The side to move is not in check.                                     //
//   4. The move itself does not give check.                                  //
//   5. The move does not promote a piece.                                    //
//   6. The move does not capture a piece.                                    //
//                                                                            //
// ---------------------------------------------------------------------------//
            if (count > LmrCount && depth > LmrDepth && !isPv && !s.inCheck()
                && !c.inCheck() && !s.isCapture(m) && !isPromotion(m))
                score = -scout_search(c, si, d - 1, ply + 1, -(a + 1), -a, false, isNull, false);
            else
                score = a + 1;

            if (score > a)
                score = -scout_search(c, si, d, ply + 1, -(a + 1), -a, false, isNull, false);

            // If an alpha improvement caused fail high, research using a full window.
            if (a < score && b > score)
            {
                score = -scout_search(c, si, d, ply + 1, -b, -a, isPv, isNull, false);
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
                history.update(m, depth, ply, true);
            break;
        }
        else
        {
            if (s.isQuiet(m))
                history.update(m, depth, ply, false);
        }
    }

    if (bestScore == Neg_inf)
        return s.check() ? -Checkmate + ply : Stalemate;

    if (a > alpha && a < b && !si.quit)
    {
        lineManager.pushToPv(best_move, s.getKey(), ply, a);
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
    for (int d = 1; !si.quit; ++d)
    {
        score = scout_search(s, si, d, 0, Neg_inf, Pos_inf, true, false, true);

        if (lineManager.getPvMove() == nullMove)
        {
            std::cout << "bestmove 0000" << std::endl;
            return;
        }

        if (si.quit)
            break;

        // Confirm all the pv moves are legal.
        lineManager.checkPv(s);

        // Print info to gui.
        std::cout << "info "
                  << "depth " << d;

        if (lineManager.isMate())
        {
            int n = lineManager.getMateInN();
            std::cout << " score mate " << (score > 0 ? n : -n);
        }
        else
        {
            std::cout << " score cp " << score;
        }

        std::cout << " time " 
                  << si.clock.elapsed<std::chrono::milliseconds>()
                  << " nodes " 
                  << si.nodes
                  << " nps " 
                  << si.nodes / (si.clock.elapsed<std::chrono::milliseconds>() + 1) * 1000;
        lineManager.printPv();
        std::cout << std::endl;

        // Reset node count.
        si.nodes = 0;
    }

    s.make_t(lineManager.getPvMove());
    history.push(std::make_pair(lineManager.getPvMove(), s.getKey()));
    std::cout << "bestmove " << toString(lineManager.getPvMove()) << std::endl;
}

void setup_search(State& s, SearchInfo& si)
{
    //ttable.setAncient();
    ttable.clear();
    lineManager.clearPv();
    init_eval();
    history.clear();
    iterative_deepening(s, si);
}