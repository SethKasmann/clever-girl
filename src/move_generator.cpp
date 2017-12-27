
#include "move_generator.h"

// ----------------------------------------------------------------------------
// Initialize magicmState.
// ----------------------------------------------------------------------------

void mg_init()
{
    initmagicmoves();
}

MoveList::MoveList(const State& pState, Move pBest, History* pHistory, int pPly, bool pQSearch)
: mState(pState), mValid(Full), mBest(pBest), mQSearch(pQSearch), mKiller1(nullMove), mKiller2(nullMove)
, mSize(0), mHistory(pHistory), mPly(pPly)
{
    if (pHistory)
    {
        mKiller1 = mHistory->getKiller(pPly).first;
        mKiller2 = mHistory->getKiller(pPly).second;
    }

    if (mState.inCheck())
    {
        if (mState.inDoubleCheck())
            mStage = mQSearch ? qKingEvadeBestMove : nKingEvadeBestMove;
        else
        {
            Square checker = get_lsb(mState.getCheckersBB());
            Square king = mState.getKingSquare(mState.getOurColor());
            mValid = between_dia[king][checker] | between_hor[king][checker]
                   | mState.getCheckersBB();
            mStage = nBestMove;
        }
    }
    else
        mStage = mQSearch ? qBestMove : nBestMove;
}

MoveList::MoveList(const State& pState)
: mState(pState), mValid(Full), mQSearch(false), mBest(nullMove)
, mSize(0), mHistory(nullptr), mPly(0)
{
    if (pop_count(mState.getCheckersBB()) == 1)
    {
        Square checker = get_lsb(mState.getCheckersBB());
        Square king = mState.getKingSquare(mState.getOurColor());
        mValid = between_dia[king][checker] | between_hor[king][checker]
               | mState.getCheckersBB();
    }
    generateAllMoves();
    mStage = allLegal;
}

std::size_t MoveList::size() const
{
    return mSize;
}

void MoveList::push(Move pMove)
{
    mList[mSize++].move = pMove;
}

Move MoveList::pop()
{
    return mList[--mSize].move;
}

bool MoveList::contains(Move move) const
{
    return std::find(mList.begin(), mList.begin() + mSize, move)
            != mList.begin() + mSize;
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Add quiet checks to the move list, used only in Q-Search. When Q-Search    //
// is called we only want to evaluate quiet positions, where neither side can //
// capture or give check. This function only pushes quiet checks to the move  //
// list - moves that give check but do NOT capture a piece.                   //
//                                                                            //
// -------------------------------------------------------------------------- //
void MoveList::pushQuietChecks()
{
    Square dst;
    U64 m, ray, promo, pawnChecks, discovered;
    int dir;
    Color us, them;

// ---------------------------------------------------------------------------//
//                                                                            //
// Some initialization. Based on the color to move, store the colors of the   //
// players, a mask of the promotion rank, and the direction of a pawn push.   //
// This is only done to make the remaining code more readable.                //
//                                                                            //
// More importantly, we initialize discovered checks and pawn checks.         //
// Discovered checks are pieces which are pinned to the enemy king by their   //
// own slider. This means if they make a quiet move that is not directly      //
// towards are away from the king (on the same diagonal/horizontal), then the //
// move will give discovered check.                                           //
//                                                                            //
// Also initialize the bitboard for pawn checks - which is simply where a     //
// pawn needs to be to give check. We can then attempt to push pawns AND      //
// bitwise AND them with this bitboard to confirm whether or not they give    //
// check.                                                                     //
//                                                                            //
// -------------------------------------------------------------------------- //    
    discovered = mState.getDiscoveredChecks(mState.getOurColor());

    if (mState.getOurColor() == white)
    {
        us = white;
        them = black;
        promo = Rank_8;
        dir = 8;
    }
    else
    {
        us = black;
        them = white;
        promo = Rank_1;
        dir = -8;
    }

    pawnChecks = mState.getAttackBB<pawn>(mState.getKingSquare(them), them);

// ---------------------------------------------------------------------------//
//                                                                            //
// Iterate through the pawn piece list. We're looking for three cases:        //
//   1. Pawn pushes that attack the enemy king.                               //
//   2. Pawn knight promotions that check the enemy king.                     //
//   3. Pawn pushes that cause discovered check from a slider.                //
//                                                                            //
// The third scenario is the most tricky. Since we already have a bitboard    //
// containing the pieces which could give a discovered check, all we need to  //
// do is bitwise AND the source square with the discovered check bitboard.    //
// If the result is not zero, then check to see if the pawn is moving         //
// towards the enemy king. To do this, bitwise AND coplanar[src][dst] with    //
// the location of the enemy king. If the result is zero then the pawn did    //
// not move towards the enemy king and there was a discovered check.          //
//                                                                            //
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<pawn>(us))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & promo)
        {
            if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(them)
              & mState.getAttackBB<knight>(mState.getKingSquare(them)))
                push(makeMove(src, dst+1, knight));
            if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(them)
              & mState.getAttackBB<knight>(mState.getKingSquare(them)))
                push(makeMove(src, dst-1, knight));
            if (square_bb[dst] & mState.getEmptyBB() & mState.getAttackBB<knight>(mState.getKingSquare(them)))
                push(makeMove(src, dst, knight));
        }
        else
        {
            if (square_bb[dst] & mState.getEmptyBB())
            {
                if (square_bb[dst])
                {
                    if (square_bb[dst] & pawnChecks)
                        push(makeMove(src, dst));
                    else if (square_bb[src] & discovered 
                          && !(coplanar[src][dst] & mState.getPieceBB<king>(them)))
                        push(makeMove(src, dst));
                }
                if (pawn_dbl_push[us][src] & square_bb[dst+dir] & mState.getEmptyBB())
                {
                    if (square_bb[dst+dir])
                    {
                        if (square_bb[dst+dir] & pawnChecks)
                            push(makeMove(src, dst+dir));
                        else if (square_bb[src] & discovered 
                              && !(coplanar[src][dst+dir] & mState.getPieceBB<king>(them)))
                            push(makeMove(src, dst+dir));
                    }
                }
            }
        }
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Iterate through the knight piece list. Check for knight moves to empty     //
// squares that check the enemy king. For each possible knight move, if the   //
// bitboard of knight moves from the destination will bitwise AND with the    //
// opponents king, then the move will give check. If a knight is a discovered //
// candidate, then any move will cause discovered check.                      //
//                                                                            //
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<knight>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<knight>(src) & mState.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            if (mState.getAttackBB<knight>(dst) & mState.getPieceBB<king>(them))
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered)
                push(makeMove(src, dst));
        }
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Iterate through the bishop piece list. Generate only moves that move to    //
// empty squares. If a move forms a diagonal ray with the enemy king, and     //
// the squares between the destination and the king square are empty, this    //
// move gives check. If the bishop is a discovered checker, and does not move //
// directly towards or away from the enemy king (on the same ray), then the   //
// move gives discovered check.                                               //
//                                                                            //
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<bishop>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<bishop>(src) & mState.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            ray = between_dia[dst][mState.getKingSquare(them)];
            if (bishopMoves[dst] & mState.getPieceBB<king>(them)
             && pop_count(ray & mState.getOccupancyBB()) == 0)
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered 
                  && !(coplanar[src][dst] & mState.getPieceBB<king>(them)))
                push(makeMove(src, dst));
        }
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Iterate through the rook piece list. Generate only moves that move to     //
// empty squares. If a move forms a horizontal ray with the enemy king, and   //
// the squares between the destination and the king square are empty, this    //
// move gives check. If the rook is a discovered checker, and does not move   //
// directly towards or away from the enemy king (on the same ray), then the   //
// move gives discovered check.                                               //
//                                                                            //
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<rook>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<rook>(src) & mState.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            ray = between_hor[dst][mState.getKingSquare(them)];
            if (rookMoves[dst] & mState.getPieceBB<king>(them)
             && pop_count(ray & mState.getOccupancyBB()) == 0)
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered 
                  && !(coplanar[src][dst] & mState.getPieceBB<king>(them)))
                push(makeMove(src, dst));
        }
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Iterate through the queen piece list. Generate only moves that move to     //
// empty squares. If a move forms a horizontal or diagonal ray with the enemy //
// king, and the squares between the destination and the king square are      //
// empty, this move gives check. If the rook is a discovered checker, and     //
// does not move directly towards or away from the enemy king (on the same    //
// ray), then the move gives discovered check.                                //
//                                                                            //
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<queen>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<queen>(src) & mState.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            ray = between_hor[dst][mState.getKingSquare(them)]
                | between_dia[dst][mState.getKingSquare(them)];
            if ((rookMoves[dst]|bishopMoves[dst]) & mState.getPieceBB<king>(them)
             && pop_count(ray & mState.getOccupancyBB()) == 0)
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered 
                  && !(coplanar[src][dst] & mState.getPieceBB<king>(them)))
                push(makeMove(src, dst));
        }
    }
}


// ---------------------------------------------------------------------------//
//                                                                            //
// Add only captures to the move list. This function will cover captures      //
// for:                                                                       //
//                                                                            //
//   Knights                                                                  //
//   Bishop                                                                   //
//   Rooks                                                                    //
//   Queens                                                                   //
//                                                                            //
// Since king and pawn moves are trickier, they need their own                //
// specialization.                                                            //
//                                                                            //
// -------------------------------------------------------------------------- //
template <PieceType P>
void MoveList::pushAttackMoves()
{
    U64 m;
    for (Square src : mState.getPieceList<P>(mState.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<P>(src) & mValid & mState.getOccupancyBB(mState.getTheirColor());;
        while (m)
            push(makeMove(src, pop_lsb(m)));
    }
}


// ---------------------------------------------------------------------------//
//                                                                            //
// Specialization to add only pawn capture moves. Moves that promote to a     //
// queen are generated as well - even if they do not capture a piece.         //
//                                                                            //
// -------------------------------------------------------------------------- //
template<>
void MoveList::pushAttackMoves<pawn>()
{
    U64 promo, attack;
    Square dst;
    int dir;
    Color us, them;

// ---------------------------------------------------------------------------//
//                                                                            //
// Some initialization to make the code more readable. Store the colors of    //
// the players, the promotion rank (as a mask), and the direction of a single //
// pawn push.                                                                 //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (mState.getOurColor() == white)
    {
        us = white;
        them = black;
        promo = Rank_8;
        dir = 8;
    }
    else
    {
        us = black;
        them = white;
        promo = Rank_1;
        dir = -8;
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Iterate through the pawn move list. Since we are only generating captures  //
// and queen promotions, there's only a few cases to cover:                   //
//   1. Queen promotions that attack diagonally left, diagonally right, or    //
//      push.                                                                 //
//   2. Pawn attacks diagonally left, and right.                              //
//   3. The trickiest - en-passant.                                           //
//                                                                            //
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<pawn>(us))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
// ---------------------------------------------------------------------------//
//                                                                            //
// Queen promotions:                                                          //
// If the destination of a pawn push bitwise AND with the promotion mask,     //
// check for queen promotions. First bitwise AND the destination square with  //
// the notAFile or notHFile mask (depending on the direction). This mask will //
// check the pawn can legally capture a direction without sliding off the     //
// board. If this is true, bitwise AND the destination square (+/- one), the  //
// occupancy of the opponent, and the bitboard of valid moves. If the         //
// operation is successful, then the move is legal.                           //
//                                                                            //
// -------------------------------------------------------------------------- //
        if (dst & promo)
        {
            if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst+1, queen));
            if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst-1, queen));
            if (dst & mState.getEmptyBB() & mValid)
                push(makeMove(src, dst, queen));
        }
// ---------------------------------------------------------------------------//
//                                                                            //
// Pawn captures:                                                             //
// If the move is not a promotion, then check for normal captures. First,     //
// bitwise AND the destination square with the notAFile or notHFile mask      //
// (depending on the direction). This mask will check the pawn can legally    //
// capture a direction without sliding off the board. If this is true,        //
// bitwise AND the destination square (+/- one), the occupancy of the         //
// opponent, and the bitboard of valid moves. If the operation is successful, //
// then the move is legal.                                                    //
//                                                                            //
// -------------------------------------------------------------------------- //
        else
        {
            if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst+1));
            if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst-1));
        }
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// En-passant:                                                                //
// This is the toughest case. First make sure the en-passant bitboard is not  //
// zero. If so, make sure the pawn that we want to capture is a valid move    //
// (for example, if we are in check, make sure capturing this pawn gets us    //
// out of check).                                                             //
// Find the possible attackers - since there are two. Bitwise AND this        //
// bitboard with the bitboard of the side-to-move's pawns. There is a rare    //
// edge case where both of these pawns are pinned to the enemy king - but     //
// wouldn't show up in the bitboard of pinned pieces (since there are TWO     //
// pawns being removed from the same rank).                                   //
// The check(mask) function handles this case. It will simply remove the bits //
// from the occupancy and see if it leaves the king in check. Since           //
// en-passant is so rare I'm not worried about the overhead cost of this      //
// check.                                                                     //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (mState.getEnPassantBB() && mValid & pawn_push[them][get_lsb(mState.getEnPassantBB())])
    {
        dst = get_lsb(mState.getEnPassantBB());
        attack = pawn_attacks[them][dst] & mState.getPieceBB<pawn>(us);
        while (attack)
            push(makeMove(pop_lsb(attack), dst));
    }
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Specialization to add only king capture moves. Valid king moves are given  //
// to the move generator on initialization - so simply bitwise AND this       //
// bitboard with the opponent's pieces.                                       //
//                                                                            //
// -------------------------------------------------------------------------- //
template<>
void MoveList::pushAttackMoves<king>()
{
    U64 m;
    Square k, dst;

    k = mState.getKingSquare(mState.getOurColor());

    m = mState.getAttackBB<king>(k) & mState.getOccupancyBB(mState.getTheirColor());

    while (m) 
        push(makeMove(k, pop_lsb(m)));
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Add only quiet moves (non captures) to the move list. This function will   //
// cover quiet moves for:                                                     //
//                                                                            //
//   Knights                                                                  //
//   Bishop                                                                   //
//   Rooks                                                                    //
//   Queens                                                                   //
//                                                                            //
// Since king and pawn moves are trickier, they need their own                //
// specialization.                                                            //
//                                                                            //
// -------------------------------------------------------------------------- //
template <PieceType P>
void MoveList::pushQuietMoves()
{
    U64 m;
    for (Square src : mState.getPieceList<P>(mState.getOurColor()))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<P>(src) & mValid & mState.getEmptyBB();
        while (m)
            push(makeMove(src, pop_lsb(m)));
    }
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Specialization to add only pawn quiet (non capture) moves. This also       //
// includes under promotions, and underpromotions that capture a piece.       //
//                                                                            //
// -------------------------------------------------------------------------- //
template<>
void MoveList::pushQuietMoves<pawn>()
{
    U64 promo;
    Square dst;
    int dir;
    Color us, them;

// ---------------------------------------------------------------------------//
//                                                                            //
// Some initialization to make the code more readable. Store the colors of    //
// the players, the promotion rank (as a mask), and the direction of a single //
// pawn push.                                                                 //
//                                                                            //
// -------------------------------------------------------------------------- //
    if (mState.getOurColor() == white)
    {
        us = white;
        them = black;
        promo = Rank_8;
        dir = 8;
    }
    else
    {
        us = black;
        them = white;
        promo = Rank_1;
        dir = -8;
    }

// ---------------------------------------------------------------------------//
//                                                                            //
// Iterate through the pawn move list. Since we are only generating quiets    //
// and under promotions, there's only a few cases to cover:                   //
//   1. Under promotions that attack diagonally left, diagonally right, or    //
//      push.                                                                 //
//   2. Single pawn pushes.                                                   //
//   3. Double pawn pushes.                                                   //
//                                                                            //
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<pawn>(us))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
// ---------------------------------------------------------------------------//
//                                                                            //
// Under promotions:                                                          //
// If the destination of a pawn push bitwise AND with the promotion mask,     //
// check for under promotions. First bitwise AND the destination square with  //
// the notAFile or notHFile mask (depending on the direction). This mask will //
// check the pawn can legally capture a direction without sliding off the     //
// board. If this is true, bitwise AND the destination square (+/- one), the  //
// occupancy of the opponent, and the bitboard of valid moves. If the         //
// operation is successful, then the move is legal.                           //
//                                                                            //
// -------------------------------------------------------------------------- //
        if (dst & promo)
        {
            if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(them) & mValid)
            {
                push(makeMove(src, dst+1, knight));
                push(makeMove(src, dst+1, rook));
                push(makeMove(src, dst+1, bishop));
            }
            if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(them) & mValid)
            {
                push(makeMove(src, dst-1, knight));
                push(makeMove(src, dst-1, rook));
                push(makeMove(src, dst-1, bishop));
            }
            if (dst & mState.getEmptyBB() & mValid)
            {
                push(makeMove(src, dst, knight));
                push(makeMove(src, dst, rook));
                push(makeMove(src, dst, bishop));
            }
        }
// ---------------------------------------------------------------------------//
//                                                                            //
// Pawn single and double push:                                               //
// Confirm the square for a single pawn push is empty. This is a condition    //
// for both a single and double push. If this square is also valid, a single  //
// pawn push is generated.                                                    //
// For double pushes, check the source square with the double push mask.      //
// Bitwise AND This result with the empty squares and the bitboard of valid   //
// moves. If these conditions are true, generate a pawn double push.          //
//                                                                            //
// -------------------------------------------------------------------------- //
        else
        {
            if (square_bb[dst] & mState.getEmptyBB())
            {
                if (square_bb[dst] & mValid)
                    push(makeMove(src, dst));
                if (pawn_dbl_push[mState.getOurColor()][src] & square_bb[dst+dir] & mState.getEmptyBB() & mValid)
                    push(makeMove(src, dst+dir));
            }
        }
    }
}

template<>
void MoveList::pushQuietMoves<king>()
{
    U64 m;
    Square k, dst;

    k = mState.getKingSquare(mState.getOurColor());

    m = mState.getAttackBB<king>(k) & mState.getEmptyBB();

    while (m) 
        push(makeMove(k, pop_lsb(m)));

    if (mState.getCheckersBB()) return;

    if (mState.canCastleKingside()
        && !(between_hor[k][k-3] & mState.getOccupancyBB())
        && !mState.attacked(k-1) 
        && !mState.attacked(k-2))
        push(makeCastle(k, k-2));

    if (mState.canCastleQueenside()
        && !(between_hor[k][k+4] & mState.getOccupancyBB())
        && !mState.attacked(k+1)
        && !mState.attacked(k+2))
        push(makeCastle(k, k+2));    
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Run through the move list. Remove any moves where the source location      //
// is a pinned piece and the king square is not on the line formed by the     //
// source and destination locationmState.                                     //
//                                                                            //
// -------------------------------------------------------------------------- //
void MoveList::checkLegal()
{
    for (int i = 0; i < mSize; ++i)
    {
        if (!mState.isLegal(mList[i].move))
            mList[i--] = mList[--mSize];
    }
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Push legal moves onto the moves list. If the king is in double check, then //
// only generate king moves. Otherwise, generate all attack and quiet moves.  //
// This function is only used for testing (such as PERFT testing).            //
//                                                                            //
// ---------------------------------------------------------------------------//
void MoveList::generateAllMoves()
{
    if (mState.inDoubleCheck())
    {
        pushAttackMoves<king>();
        pushQuietMoves<king>();
    }
    else
    {
        pushAttackMoves<pawn>();
        pushAttackMoves<knight>();
        pushAttackMoves<bishop>();
        pushAttackMoves<rook>();
        pushAttackMoves<queen>();
        pushAttackMoves<king>();

        pushQuietMoves<pawn>();
        pushQuietMoves<knight>();
        pushQuietMoves<bishop>();
        pushQuietMoves<rook>();
        pushQuietMoves<queen>();
        pushQuietMoves<king>();
    }

    checkLegal();
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Generate only quiet checks - used in Q-Search.                             //
//                                                                            //
// ---------------------------------------------------------------------------//
void MoveList::generateQuietChecks()
{
    pushQuietChecks();
    checkLegal();
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Generate all quiet moves.                                                  //
//                                                                            //
// ---------------------------------------------------------------------------//
void MoveList::generateQuiets()
{
    pushQuietMoves<pawn>();
    pushQuietMoves<knight>();
    pushQuietMoves<bishop>();
    pushQuietMoves<rook>();
    pushQuietMoves<queen>();
    pushQuietMoves<king>();
    checkLegal();
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Generate all captures.                                                     //
//                                                                            //
// ---------------------------------------------------------------------------//
void MoveList::generateAttacks()
{
    pushAttackMoves<pawn>();
    pushAttackMoves<knight>();
    pushAttackMoves<bishop>();
    pushAttackMoves<rook>();
    pushAttackMoves<queen>();
    pushAttackMoves<king>();
    checkLegal();
}

// ---------------------------------------------------------------------------//
//                                                                            //
// Return the best move avaliable in the move list. Since not all moves are   //
// generated at once, mStage keeps track of the stage of move ordering that   //
// we are in. This way we can use a big switch case for handling move         //
// generation for both regular and q-search.                                  //
//                                                                            //
// If a case begins with the letter n it refers to normal search, and if it   //
// begins with q it refers to q search.                                       //
//                                                                            //
// ---------------------------------------------------------------------------//
Move MoveList::getBestMove()
{
    Move move;

    switch (mStage)
    {
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal Search - best move.                                                 //
// Before generating any moves, first check if the move given by the pv list  //
// or the transposition table is valid. If so, return that move and pray for  //
// a beta cutoff.                                                             //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nBestMove:
            mStage++;
            if (mState.isValid(mBest, mValid)
                && mState.isLegal(mBest))
                return mBest;
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal Search - captures generation.                                       //
// Since the best move failed, this case handles generating and sorting       //
// capture moves. After generating all captures, check if each capture has    //
// a positive SEE (static exchange evaluation) score. If so, score them by    //
// their LVA-MVV (least valuable attacker, most valuable victim) score. I     //
// just do Victim - Attacker.                                                 //
//                                                                            //
// If the see value is negative, store the see value as the score.            //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nAttacksGen:
            generateAttacks();
            for (int i = 0; i < mSize; ++i)
            {
                int see = mState.see(mList[i].move);
                if (see > 0)
                    mList[i].score = mState.onSquare(getDst(mList[i].move))
                                   - mState.onSquare(getSrc(mList[i].move));
                else
                {
                    mList[i].score = see;
                    badCaptures.push_back(mList[i]);
                    std::swap(mList[i], mList[mSize - 1]);
                    pop();
                }
            }
            mStage++;
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal Search - captures.                                                  //
// Since there are not very many captures there's not need to run a sorting   //
// algorithm on them. Since we pop off the move from the end of the array,    //
// swap the max element in the array with the move in the final position.     //
// Make sure to confirm the move is not the same as the best move we already  //
// tried.                                                                     //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nAttacks:
            while (mSize)
            {
                std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), 
                               mList.begin() + mSize - 1);
                move = pop();
                if (move != mBest
                    && mState.isLegal(move))
                    return move;
            }
            mStage++;
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal Seach - first killer.                                               //
// Check if the first killer stored at this ply is a valid move. If so,       //
// return it before generating any quiet moves. Confirm it is not the same    //
// as the best move.                                                          //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nKiller1:
            mStage++;
            if ((mState.isValid(mKiller1, mValid)
                && mState.isLegal(mKiller1)
                && mState.isQuiet(mKiller1))
                && mKiller1 != mBest)
                return mKiller1;
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal Seach - second killer.                                              //
// Check if the second killer stored at this ply is a valid move. If so,      //
// return it before generating any quiet moves. Confirm it is not the same    //
// as the best move or the first killer.                                      //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nKiller2:
            mStage++;
            if ((mState.isValid(mKiller2, mValid))
                && mState.isLegal(mKiller2)
                && mState.isQuiet(mKiller2)
                && mKiller2 != mBest
                && mKiller2 != mKiller1)
                return mKiller2;
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal Seach - quiets generation                                           //
// Since both killers failed, generate and order quiet moves. Our first pass  //
// through the quiet moves scores them based on their history score. The      //
// history score uses the moves increment from the history heuristic and the  //
// butterfly hueristic.                                                       //
//                                                                            //
// score = history / butterfly                                                //
//                                                                            //
// The history score is increased by += depth*depth if a quiet move causes a  //
// beta cutoff. The butterfly score is increased by += depth if a quiet move  //
// does not improve alpha in search.                                          //
//                                                                            //
// Since move moves will not have a history score (thus the result is zero),  //
// partition the quiet moves so that the ones with the history score are on   //
// the right side since we want to try these first. Sort the right side.      //
//                                                                            //
// The remaining quiets, score based on piece square table values.            //
// (where I'm going - where I'm at). Sort the left side.                      //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nQuietsGen:
        {
            generateQuiets();
            for (int i = 0; i < mSize; ++i)
                mList[i].score = mHistory->getHistoryScore(mList[i].move);
            std::array<MoveEntry, maxSize>::iterator it2 = 
                std::partition(mList.begin(), mList.begin() + mSize, noScore);
            std::stable_sort(it2, mList.begin() + mSize);
            std::array<MoveEntry, maxSize>::iterator it1 = mList.begin();
            for (std::array<MoveEntry, maxSize>::iterator it1 = mList.begin(); it1 != it2; ++it1)
            {
                Square src = getSrc(it1->move);
                Square dst = getDst(it1->move);
                PieceType toMove = mState.onSquare(src);
                it1->score = PieceSquareTable::pst[toMove][middle][mState.getOurColor()][dst]
                           - PieceSquareTable::pst[toMove][middle][mState.getOurColor()][src]
                           + PieceSquareTable::pst[toMove][late][mState.getOurColor()][dst]
                           - PieceSquareTable::pst[toMove][late][mState.getOurColor()][src];
            }
            std::stable_sort(mList.begin(), it2);
            mStage++;
        }
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal Search - quiets.                                                    //
// Confirm each move is not the same as the best move, the first killer, or   //
// the second killer.                                                         //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nQuiets:
            while (mSize)
            {
                move = pop();
                if (move != mBest
                    && move != mKiller1
                    && move != mKiller2
                    && mState.isLegal(move))
                    return move;
            }
            while (!badCaptures.empty())
            {
                move = badCaptures.back().move;
                badCaptures.pop_back();
                if (move != mBest
                    && mState.isLegal(move))
                    return move;
            }
            break;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q Search - best move.                                                      //
// Before generating any moves, first check if the move given by the pv list  //
// or the transposition table is valid. If so, return that move and pray for  //
// a beta cutoff.                                                             //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qBestMove:
            mStage++;
            if (mState.isValid(mBest, mValid)
                && mState.isLegal(mBest))
                return mBest;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q Search - captures generation.                                            //
// Since the best move failed, this case handles generating and sorting       //
// capture moves. After generating all captures, check if each capture has    //
// a positive SEE (static exchange evaluation) score. If so, score them by    //
// their LVA-MVV (least valuable attacker, most valuable victim) score. I     //
// just do Victim - Attacker.                                                 //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qAttacksGen:
            generateAttacks();
            for (int i = 0; i < mSize; ++i)
            {
                mList[i].score = mState.onSquare(getDst(mList[i].move))
                               - mState.onSquare(getSrc(mList[i].move));
                                   /*
                else
                {
                    mList[i].score = see;
                    badCaptures.push_back(mList[i]);
                    std::swap(mList[i], mList[mSize - 1]);
                    pop();
                }
                */
            }
            mStage++;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q Search - captures.                                                       //
// Since there are not very many captures there's not need to run a sorting   //
// algorithm on them. Since we pop off the move from the end of the array,    //
// swap the max element in the array with the move in the final position.     //
// Make sure to confirm the move is not the same as the best move we already  //
// tried.                                                                     //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qAttacks: 
            while (mSize)
            {
                std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), 
                               mList.begin() + mSize - 1);
                move = pop();
                if (mState.see(move) < 0 && !isPromotion(move))
                    continue;
                if (move != mBest
                    && mState.isLegal(move))
                    return move;
            }
            mStage++;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q Search - generate quiet checks.                                          //
// Generate quiet checks and score them based on their history score. The     //
// history score uses the moves increment from the history heuristic and the  //
// butterfly hueristic.                                                       //
//                                                                            //
// score = history / butterfly                                                //
//                                                                            //
// The history score is increased by += depth*depth if a quiet move causes a  //
// beta cutoff. The butterfly score is increased by += depth if a quiet move  //
// does not improve alpha in search.                                          //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qQuietChecksGen:
            generateQuietChecks();
            for (int i = 0; i < mSize; ++i)
                mList[i].score = mHistory->getHistoryScore(mList[i].move);
            mStage++;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q Search - quiet checks.                                                   //
// Since there are not very many quiet checks there's no need to run a        //
// sorting algorithm on them. Since we pop off the move from the end of the   //
// array, swap the max element in the array with the move in the final        //
// position. Make sure to confirm the move is not the same as the best move   //
// we already tried.                                                          //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qQuietChecks:
            while (mSize)
            {
                std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), 
                               mList.begin() + mSize - 1);
                move = pop();
                if (move != mBest
                    && mState.isLegal(move))
                    return move;
            }
            /*
            while (!badCaptures.empty())
            {
                move = badCaptures.back().move;
                badCaptures.pop_back();
                if (move != mBest
                    && mState.isLegal(move))
                    return move;
            }*/
            break;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q search (double check) - best move                                        //
// Before generating any moves, first check if the move given by the pv list  //
// or the transposition table is valid. If so, return that move and pray for  //
// a beta cutoff.                                                             //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qKingEvadeBestMove:
            mStage++;
            if (mState.isValid(mBest, mValid)
                && mState.isLegal(mBest))
                return mBest;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q search (double check) - generate captures                                //
// Generate king captures only. No scoring or sorting is done.                //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qKingEvadeAttacksGen:
            pushAttackMoves<king>();
            mStage++;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q search (double check) - captures                                         //
// As long as the capture is not the same as the best move, return it.        //
//                                                                            //
// ---------------------------------------------------------------------------//
        case qKingEvadeAttacks:
            while (mSize)
            {
                move = pop();
                if (move != mBest
                    && mState.isLegal(move))
                    return move;
            }
            break;
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal search (double check) - best move                                   //
// Before generating any moves, first check if the move given by the pv list  //
// or the transposition table is valid. If so, return that move and pray for  //
// a beta cutoff.                                                             //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nKingEvadeBestMove:
            mStage++;
            if (mState.isValid(mBest, mValid)
                && mState.isLegal(mBest))
                return mBest;
// ---------------------------------------------------------------------------//
//                                                                            //
// Noraml search (double check) - generate evasions                           //
// Generate king evasions only. No scoring or sorting is done.                //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nKingEvadeMovesGen:
            pushQuietMoves<king>();
            pushAttackMoves<king>();
            mStage++;
// ---------------------------------------------------------------------------//
//                                                                            //
// Normal search (double check) - evasions                                    //
// As long as the capture is not the same as the best move, return it.        //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nKingEvadeMoves:
            while (mSize)
            {
                move = pop();
                if (move != mBest
                    && mState.isLegal(move))
                    return move;
            }
            break;
// ---------------------------------------------------------------------------//
//                                                                            //
// All Legal                                                                  //
// Generate all legal moves and return each one. Useful for testing such as   //
// PERFT.                                                                     //
//                                                                            //
// ---------------------------------------------------------------------------//
        case allLegal:
            while (mSize)
            {
                move = pop();
                return move;
            }
    }
    return nullMove;
}