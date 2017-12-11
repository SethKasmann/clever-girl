
#include "move_generator.h"

// ----------------------------------------------------------------------------
// Initialize magicmState.
// ----------------------------------------------------------------------------

void mg_init()
{
    initmagicmoves();
}

// ----------------------------------------------------------------------------
// Push all legal pawn moves and pawn attacks, including promotions,
// double pawn pushes, and en-passant.
// ----------------------------------------------------------------------------

void MoveList::pushQuietChecks()
{
    Square dst;
    U64 m, ray, promo, pawnChecks, discovered;
    int dir;
    Color us, them;

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
// -------------------------------------------------------------------------- //
    for (Square src : mState.getPieceList<pawn>(us))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & promo)
        {
            if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(them) & mValid 
              & mState.getAttackBB<knight>(mState.getKingSquare(them)))
                push(makeMove(src, dst+1, knight));
            if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(them) & mValid
              & mState.getAttackBB<knight>(mState.getKingSquare(them)))
                push(makeMove(src, dst-1, knight));
            if (square_bb[dst] & mState.getEmptyBB() & mValid & mState.getAttackBB<knight>(mState.getKingSquare(them)))
                push(makeMove(src, dst, knight));
        }
        else
        {
            if (square_bb[dst] & mState.getEmptyBB())
            {
                if (square_bb[dst] & mValid)
                {
                    if (square_bb[dst] & pawnChecks)
                        push(makeMove(src, dst));
                    else if (square_bb[src] & discovered 
                          && !(coplanar[src][dst] & mState.getPieceBB<king>(them)))
                        push(makeMove(src, dst));
                }
                if (pawn_dbl_push[us][src] & square_bb[dst+dir] & mState.getEmptyBB())
                {
                    if (square_bb[dst+dir] & mValid)
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

    for (Square src : mState.getPieceList<knight>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<knight>(src) & mValid & mState.getEmptyBB();
        while (m)
        {
            dst = pop_lsb(m);
            if (mState.getAttackBB<knight>(dst) & mState.getPieceBB<king>(them))
                push(makeMove(src, dst));
            else if (square_bb[src] & discovered)
                push(makeMove(src, dst));
        }
    }
    for (Square src : mState.getPieceList<bishop>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<bishop>(src) & mValid & mState.getEmptyBB();
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
    for (Square src : mState.getPieceList<rook>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<rook>(src) & mValid & mState.getEmptyBB();
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
    for (Square src : mState.getPieceList<queen>(us))
    {
        if (src == no_sq)
            break;
        m  = mState.getAttackBB<queen>(src) & mValid & mState.getEmptyBB();
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

// ----------------------------------------------------------------------------
// Push all pseudo-legal moves and attacks for Knights, Bishops, Rooks, and 
// QueenmState.
// ----------------------------------------------------------------------------

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

template<>
void MoveList::pushAttackMoves<pawn>()
{
    U64 promo, attack;
    Square dst;
    int dir;
    Color us, them;

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

    for (Square src : mState.getPieceList<pawn>(us))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
        if (dst & promo)
        {
            if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst+1, queen));
            if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst-1, queen));
            if (dst & mState.getEmptyBB() & mValid)
                push(makeMove(src, dst, queen));
        }
        else
        {
            if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst+1));
            if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(them) & mValid)
                push(makeMove(src, dst-1));
        }
    }

    // En passant.
    if (mState.getEnPassantBB() && mValid & pawn_push[them][get_lsb(mState.getEnPassantBB())])
    {
        dst = get_lsb(mState.getEnPassantBB());
        attack = pawn_attacks[them][dst] & mState.getPieceBB<pawn>(us);
        while (attack)
        {
            if (mState.check(pawn_push[them][dst] | get_lsb_bb(attack)))
                return;
            push(makeMove(pop_lsb(attack), dst));
        }
    }
}

template<>
void MoveList::pushAttackMoves<king>()
{
    U64 m;
    Square k, dst;

    k = mState.getKingSquare(mState.getOurColor());

    m = mValidKingMoves & mState.getOccupancyBB(mState.getTheirColor());

    while (m) 
        push(makeMove(k, pop_lsb(m)));
}

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

template<>
void MoveList::pushQuietMoves<pawn>()
{
    U64 promo;
    Square dst;
    int dir;
    Color us, them;

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

    for (Square src : mState.getPieceList<pawn>(us))
    {
        if (src == no_sq)
            break;

        dst = src + dir;
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

    m = mValidKingMoves & mState.getEmptyBB();

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

// ----------------------------------------------------------------------------
// Check if moves are legal by removing any moves from the moves list
// where a pinned piece moves off it's pin ray.
// ----------------------------------------------------------------------------

void MoveList::checkLegal()
{
    // Run through the move list. Remove any moves where the source location
    // is a pinned piece and the king square is not on the line formed by the
    // source and destination locationmState.
    for (int i = 0; i < mSize; ++i)
    {
        if (!mState.isLegal(mList[i].move))
            mList[i--] = mList[--mSize];
    }
}

// ----------------------------------------------------------------------------
// Push legal moves onto the moves list. The generation technique is to first 
// find the number of checks on our king.
// If checks == 0, add all pseudo-legal movemState.
// If checks == 1, add pseudo-legal moves that block or capture the checking
//     piece.
// If checks == 0, add only legal king movemState.
// Lastly, verify all moves made by pinned pieces stay on their pin ray.
// ----------------------------------------------------------------------------

void MoveList::pushAllLegal()
{
    if (pop_count(mState.getCheckersBB()) == 2)
    {
        pushAttackMoves<king>();
        pushQuietMoves<king>();
        return;
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

void MoveList::generateQuietChecks()
{
    pushQuietChecks();
    checkLegal();
}

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

Move_t MoveList::getBestMove()
{
    Move_t move;

    switch (mStage)
    {
        case nBestMove:
            mStage++;
            if (mState.isValid(mBest, mValidKingMoves, mValid))
                return mBest;
        case nAttacksGen:
            generateAttacks();
            // MVV - LVA Algorithm.
            for (int i = 0; i < mSize; ++i)
            {
                int see = mState.see(mList[i].move);
                if (see > 0)
                    mList[i].score = mState.onSquare(getDst(mList[i].move))
                                   - mState.onSquare(getSrc(mList[i].move));
                else
                    mList[i].score = see;
            }
            mStage++;
        case nAttacks:
            while (mSize)
            {
                std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), 
                               mList.begin() + mSize - 1);
                move = pop();
                if (move != mBest)
                    return move;
            }
            mStage++;
        case nKiller1:
            mStage++;
            if ((mState.isValid(mKiller1, mValidKingMoves, mValid))
                && mKiller1 != mBest)
                return mKiller1;
        case nKiller2:
            mStage++;
            if ((mState.isValid(mKiller2, mValidKingMoves, mValid))
                && mKiller2 != mBest
                && mKiller2 != mKiller1)
                return mKiller2;
        case nQuietsGen:
        {
            generateQuiets();
            for (int i = 0; i < mSize; ++i)
                mList[i].score = mHistory->getHistoryScore(mList[i].move);
            std::array<MoveEntry, Max_size>::iterator it2 = 
                std::partition(mList.begin(), mList.begin() + mSize, noScore);
            std::stable_sort(it2, mList.begin() + mSize);
            std::array<MoveEntry, Max_size>::iterator it1 = mList.begin();
            for (std::array<MoveEntry, Max_size>::iterator it1 = mList.begin(); it1 != it2; ++it1)
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
            /*
            for (int i = 0; i < mSize; ++i)
                std::cout << mList[i].score << " ";
            std::cout << std::endl;
            */
            mStage++;
        }
        case nQuiets:
            while (mSize)
            {
                move = pop();
                if (move != mBest
                    && move != mKiller1
                    && move != mKiller2)
                    return move;
            }
            break;
        case qBestMove:
            mStage++;
            if (mState.isValid(mBest, mValidKingMoves, mValid))
                return mBest;
        case qAttacksGen:
            mStage++;
            generateAttacks();
            // MVV - LVA Algorithm.
            for (int i = 0; i < mSize; ++i)
                mList[i].score = mState.onSquare(getDst(mList[i].move))
                               - mState.onSquare(getSrc(mList[i].move));
        case qAttacks: 
            while (mSize)
            {
                std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), 
                               mList.begin() + mSize - 1);
                move = pop();
                if (move != mBest)
                    return move;
            }
            mStage++;
        case qQuietChecksGen:
            generateQuietChecks();
            for (int i = 0; i < mSize; ++i)
                mList[i].score = mHistory->getHistoryScore(mList[i].move);
            mStage++;
        case qQuietChecks:
            while (mSize)
            {
                std::iter_swap(std::max_element(mList.begin(), mList.begin() + mSize), 
                               mList.begin() + mSize - 1);
                move = pop();
                if (move != mBest)
                    return move;
            }
            break;
        case qKingEvadeBestMove:
            mStage++;
            if (mState.isValid(mBest, mValidKingMoves, mValid))
                return mBest;
        case qKingEvadeAttacksGen:
            pushAttackMoves<king>();
            mStage++;
        case qKingEvadeAttacks:
            while (mSize)
            {
                move = pop();
                if (move != mBest)
                    return move;
            }
            break;
        case nKingEvadeBestMove:
            mStage++;
            if (mState.isValid(mBest, mValidKingMoves, mValid))
                return mBest;
        case nKingEvadeMovesGen:
            pushQuietMoves<king>();
            pushAttackMoves<king>();
            mStage++;
        case nKingEvadeMoves:
            while (mSize)
            {
                move = pop();
                if (move != mBest)
                    return move;
            }
            break;
        case allLegal:
            while (mSize)
            {
                move = pop();
                return move;
            }
    }
    return nullMove;
}