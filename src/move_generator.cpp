
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
        Square k = mState.getKingSquare(mState.getOurColor());
        Square c = get_lsb(mState.getCheckersBB());
        mValid = between[k][c] | mState.getCheckersBB();
        mStage = nEvadeBestMove;
    }
    else
        mStage = mQSearch ? qBestMove : nBestMove;
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

template<MoveType T>
void MoveList::pushPromotion(Square src, Square dst)
{
    const Color C = mState.getOurColor();
    if (square_bb[dst] & Not_a_file && square_bb[dst+1] & mState.getOccupancyBB(!C))
    {
        if (T == MoveType::Attacks || T == MoveType::All)
            push(makeMove(src, dst+1, queen));

        if (T == MoveType::Quiets || T == MoveType::All)
        {
            push(makeMove(src, dst+1, knight));
            push(makeMove(src, dst+1, rook));
            push(makeMove(src, dst+1, bishop));
        }

        if (T == MoveType::Evasions)
        {
            if (square_bb[dst+1] & mValid)
            {
                push(makeMove(src, dst+1, queen));
                push(makeMove(src, dst+1, knight));
                push(makeMove(src, dst+1, rook));
                push(makeMove(src, dst+1, bishop));
            }
        }

        if (T == MoveType::QuietChecks)
        {
            if (mState.getCheckSquaresBB(knight) & square_bb[dst+1])
                push(makeMove(src, dst+1, knight));
        }
    }

    if (square_bb[dst] & Not_h_file && square_bb[dst-1] & mState.getOccupancyBB(!C))
    {
        if (T == MoveType::Attacks || T == MoveType::All)
            push(makeMove(src, dst-1, queen));

        if (T == MoveType::Quiets || T == MoveType::All)
        {
            push(makeMove(src, dst-1, knight));
            push(makeMove(src, dst-1, rook));
            push(makeMove(src, dst-1, bishop));
        }

        if (T == MoveType::Evasions)
        {
            if (square_bb[dst-1] & mValid)
            {
                push(makeMove(src, dst-1, queen));
                push(makeMove(src, dst-1, knight));
                push(makeMove(src, dst-1, rook));
                push(makeMove(src, dst-1, bishop));
            }
        }

        if (T == MoveType::QuietChecks)
        {
            if (mState.getCheckSquaresBB(knight) & square_bb[dst-1])
                push(makeMove(src, dst-1, knight));
        }
    }

    if (square_bb[dst] & mState.getEmptyBB())
    {
        if (T == MoveType::Attacks || T == MoveType::All)
            push(makeMove(src, dst, queen));

        if (T == MoveType::Quiets || T == MoveType::All)
        {
            push(makeMove(src, dst, knight));
            push(makeMove(src, dst, rook));
            push(makeMove(src, dst, bishop));
        }

        if (T == MoveType::Evasions)
        {
            if (square_bb[dst] & mValid)
            {
                push(makeMove(src, dst, queen));
                push(makeMove(src, dst, knight));
                push(makeMove(src, dst, rook));
                push(makeMove(src, dst, bishop));
            }
        }

        if (T == MoveType::QuietChecks)
        {
            if (mState.getCheckSquaresBB(knight) & square_bb[dst])
                push(makeMove(src, dst, knight));
        }
    }
}

template<MoveType T, PieceType P>
void MoveList::pushMoves()
{
    U64 m;
    Square dst;
    Color c = mState.getOurColor();

    for (Square src : mState.getPieceList<P>(c))
    {
        if (src == no_sq)
            break;

        m = mState.getAttackBB<P>(src) & mValid;

        if (T == MoveType::QuietChecks)
        {
            if (square_bb[src] & mDiscover)
            {
                if (P == king)
                    m &= ~coplanar[src][mState.getKingSquare(!c)];
            }
            else
                m &= mState.getCheckSquaresBB(P);
        }

        while (m)
            push(makeMove(src, pop_lsb(m)));

        if (P == king && T != MoveType::Attacks && T != MoveType::Evasions)
            pushCastle<T>();
    }
}

template<MoveType T, Color C>
void MoveList::pushPawnMoves()
{
    constexpr int U = C == white ? 8 : -8;
    constexpr int L = C == white ? 9 : -7;
    constexpr int R = C == white ? 7 : -9;

    U64 promo, pawns;
    Square dst;

    pawns = mState.getPieceBB<pawn>(C);

    promo = (C == white ? pawns << 8 : pawns >> 8) & RankPromo;

    while (promo)
    {
        dst = pop_lsb(promo);
        pushPromotion<T>(dst - U, dst);
    }

    if (T == MoveType::Attacks || T == MoveType::Evasions || T == MoveType::All)
    {
        U64 occ = ((T == MoveType::Evasions ? mState.getOccupancyBB(!C) & mValid
                                            : mState.getOccupancyBB(!C))
                  | mState.getEnPassantBB()) & ~RankPromo;

        U64 left  = C == white ? (pawns & Not_a_file) << 9 & occ 
                               : (pawns & Not_a_file) >> 7 & occ;
        U64 right = C == white ? (pawns & Not_h_file) << 7 & occ
                               : (pawns & Not_h_file) >> 9 & occ;

        while (left)
        {
            dst = pop_lsb(left);
            push(makeMove(dst - L, dst));
        }

        while (right)
        {
            dst = pop_lsb(right);
            push(makeMove(dst - R, dst));
        }
    }

    if (T != MoveType::Attacks)
    {
        U64 empty = mState.getEmptyBB() & ~RankPromo;

        U64 up  = (C == white ? pawns << 8 : pawns >> 8) & empty;
        U64 dbl = (C == white ? (up & Rank_3) << 8 
                              : (up & Rank_6) >> 8) & empty;

        if (T == MoveType::Evasions)
        {
            up  &= mValid;
            dbl &= mValid;
        }

        if (T == MoveType::QuietChecks)
        {
            U64 dis;

            up  &= mState.getCheckSquaresBB(pawn);
            dbl &= mState.getCheckSquaresBB(pawn);

            dis = mDiscover & pawns & ~file_bb[mState.getKingSquare(!C)];
            dis = (C == white ? dis << 8 : dis >> 8) & empty;
            up  |= dis;
            dbl |= (C == white ? (dis & Rank_3) << 8 
                               : (dis & Rank_6) >> 8) & empty;
        }

        while (up)
        {
            dst = pop_lsb(up);
            push(makeMove(dst - U, dst));
        }

        while (dbl)
        {
            dst = pop_lsb(dbl);
            push(makeMove(dst - U - U, dst));
        }
    }
}

template<MoveType T>
void MoveList::pushCastle()
{
    Square k = mState.getKingSquare(mState.getOurColor());

    if (mState.canCastleKingside() &&
        !(between_hor[k][k-3] & mState.getOccupancyBB()) &&
        !mState.attacked(k-1) && 
        !mState.attacked(k-2)) 
    {
        if (T == MoveType::QuietChecks)
        {
            if (square_bb[k-1] & mState.getCheckSquaresBB(rook))
                push(makeCastle(k, k-2));
        }
        else
            push(makeCastle(k, k-2));
    }

    if (mState.canCastleQueenside() && 
        !(between_hor[k][k+4] & mState.getOccupancyBB()) && 
        !mState.attacked(k+1) && 
        !mState.attacked(k+2))
    {
        if (T == MoveType::QuietChecks)
        {
            if (square_bb[k+1] & mState.getCheckSquaresBB(rook))
                push(makeCastle(k, k+2));
        }
        else
            push(makeCastle(k, k+2));
    }
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

template<MoveType T>
void MoveList::generateMoves()
{
    assert(!mState.inCheck());

    if (T == MoveType::QuietChecks)
        mDiscover = mState.getDiscoveredChecks(mState.getOurColor());

    mValid = T == MoveType::Attacks 
                ? mState.getOccupancyBB(mState.getTheirColor())
                : mState.getEmptyBB();

    mState.getOurColor() == white ? pushPawnMoves<T, white>() 
                                  : pushPawnMoves<T, black>();
    pushMoves<T, knight>();
    pushMoves<T, bishop>();
    pushMoves<T, rook>();
    pushMoves<T, queen>();
    pushMoves<T, king>();
}

template<>
void MoveList::generateMoves<MoveType::Evasions>()
{
    assert(mState.inCheck());

    Square k, c;

    mValid = mState.getOccupancyBB(mState.getTheirColor()) | mState.getEmptyBB();
    pushMoves<MoveType::Evasions, king>();

    if (mState.inDoubleCheck())
        return;

    k = mState.getKingSquare(mState.getOurColor());
    c = get_lsb(mState.getCheckersBB());
    mValid = between[k][c] | mState.getCheckersBB();

    mState.getOurColor() == white ? pushPawnMoves<MoveType::Evasions, white>() 
                                  : pushPawnMoves<MoveType::Evasions, black>();
    pushMoves<MoveType::Evasions, knight>();
    pushMoves<MoveType::Evasions, bishop>();
    pushMoves<MoveType::Evasions, rook>();
    pushMoves<MoveType::Evasions, queen>();
}

template<>
void MoveList::generateMoves<MoveType::All>()
{
    if (mState.inCheck())
    {
        generateMoves<MoveType::Evasions>();
        return;
    }

    mValid = mState.getOccupancyBB(mState.getTheirColor()) | mState.getEmptyBB();

    mState.getOurColor() == white ? pushPawnMoves<MoveType::All, white>() 
                                  : pushPawnMoves<MoveType::All, black>();
    pushMoves<MoveType::All, knight>();
    pushMoves<MoveType::All, bishop>();
    pushMoves<MoveType::All, rook>();
    pushMoves<MoveType::All, queen>();
    pushMoves<MoveType::All, king>();
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
            if (mState.isValid(mBest, Full)
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
            generateMoves<MoveType::Attacks>();
            for (int i = 0; i < mSize; ++i)
            {
                int see = mState.see(mList[i].move);
                if (see >= 0)
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
            if ((mState.isValid(mKiller1, Full)
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
            if ((mState.isValid(mKiller2, Full))
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
            generateMoves<MoveType::Quiets>();
            for (int i = 0; i < mSize; ++i)
                mList[i].score = mHistory->getHistoryScore(mList[i].move);
            auto it2 = 
                std::partition(mList.begin(), mList.begin() + mSize, noScore);
            std::stable_sort(it2, mList.begin() + mSize);
            auto it1 = mList.begin();
            for (auto it1 = mList.begin(); it1 != it2; ++it1)
            {
                Square src = getSrc(it1->move);
                Square dst = getDst(it1->move);
                PieceType toMove = mState.onSquare(src);
                it1->score = PieceSquareTable::iScore(mState.getGamePhase(), 
                                                      toMove,
                                                      mState.getOurColor(), 
                                                      dst) -
                             PieceSquareTable::iScore(mState.getGamePhase(), 
                                                      toMove,
                                                      mState.getOurColor(), 
                                                      src);
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
            if (mState.isValid(mBest, Full)
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
            generateMoves<MoveType::Attacks>();
            for (int i = 0; i < mSize; ++i)
            {
                mList[i].score = mState.onSquare(getDst(mList[i].move))
                               - mState.onSquare(getSrc(mList[i].move));
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
        {
            for (int i = 0; i < mSize; ++i)
                mList[i].score = mHistory->getHistoryScore(mList[i].move);
            mStage++;
        }
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
            break;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q search (double check) - best move                                        //
// Before generating any moves, first check if the move given by the pv list  //
// or the transposition table is valid. If so, return that move and pray for  //
// a beta cutoff.                                                             //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nEvadeBestMove:
            mStage++;
            if (mState.isValid(mBest, mValid)
                && mState.isLegal(mBest)
                && !mState.inDoubleCheck())
                return mBest;
// ---------------------------------------------------------------------------//
//                                                                            //
// Q search (double check) - generate captures                                //
// Generate king captures only. No scoring or sorting is done.                //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nEvadeMovesGen:
        {
            const int CaptureFlag = 0x40000000;
            const int HistoryFlag = 0x20000000;
            generateMoves<MoveType::Evasions>();
            for (int i = 0; i < mSize; ++i)
            {
                if (mState.isCapture(mList[i].move))
                {
                    int see = mState.see(mList[i].move);
                    if (see >= 0)
                        mList[i].score = mState.onSquare(getDst(mList[i].move))
                                       - mState.onSquare(getSrc(mList[i].move));
                    else
                        mList[i].score = see;
                    mList[i].score |= CaptureFlag;
                }
                else
                {
                    int hist = mHistory->getHistoryScore(mList[i].move);
                    if (hist > 0)
                        mList[i].score = hist | HistoryFlag;
                    else
                    {
                        Square src = getSrc(mList[i].move);
                        Square dst = getDst(mList[i].move);
                        PieceType piece = mState.onSquare(src);
                        mList[i].score = PieceSquareTable::iScore(mState.getGamePhase(),
                                                                  piece,
                                                                  mState.getOurColor(),
                                                                  dst) -
                                         PieceSquareTable::iScore(mState.getGamePhase(),
                                                                  piece,
                                                                  mState.getOurColor(),
                                                                  src);
                    }
                }
            }
            std::stable_sort(mList.begin(), mList.begin() + mSize);
            mStage++;
        }
// ---------------------------------------------------------------------------//
//                                                                            //
// Q search (double check) - captures                                         //
// As long as the capture is not the same as the best move, return it.        //
//                                                                            //
// ---------------------------------------------------------------------------//
        case nEvade:
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
            break;
        default:
            assert(false);
    }
    return nullMove;
}

MoveList::MoveList(const State& pState)
: mState(pState), mValid(Full), mQSearch(false), mBest(nullMove)
, mSize(0), mHistory(nullptr), mPly(0)
{
    generateMoves<MoveType::All>();
    mStage = allLegal;
    checkLegal();
}