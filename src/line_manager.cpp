#include "line_manager.h"
#include "evaluation.h"

LineManager::LineManager()
: mMatingLine(false), mSize(0)
{}

void LineManager::pushToPv(Move pMove, U64 pKey, int pPly, int pScore)
{
    // Check if this is a mating line.
    mMatingLine = std::abs(pScore) + Max_ply >= Checkmate ? true : false;

    // Find the indicies using the triangular forumla.
    int copyTo = triangularIndex(pPly);
    int copyFromStart = triangularIndex(pPly + 1);
    int copyFromEnd = copyFromStart + Max_ply - pPly - 1;

    // Store the current move.
    mPv[copyTo++] = std::make_pair(pMove, pKey);

    // Copy from the previous iteration.
    std::copy(std::make_move_iterator(mPv.begin() + copyFromStart), 
              std::make_move_iterator(mPv.begin() + copyFromEnd), 
              mPv.begin() + copyTo);
}

U64 LineManager::getPvKey(int pPly) const
{
    return mPv[pPly].second;
}

Move LineManager::getPvMove(int pPly) const
{
    return mPv[pPly].first;
}

bool LineManager::isMate() const
{
    return mMatingLine;
}

int LineManager::getMateInN() const
{
    return mSize / 2.0 + 0.5;
}

void LineManager::clearPv()
{
    mSize = 0;
    std::fill(mPv.begin(), mPv.begin() + Max_ply, std::make_pair(0, 0));
}

void LineManager::checkPv(State& pState)
{
    State c;
    Move nextMove;
    std::memmove(&c, &pState, sizeof(pState));
    for (int i = 0; i < Max_ply; ++i)
    {
        nextMove = mPv[i].first;
        if (nextMove == nullMove)
        {
            mSize = i;
            break;
        }
        // Push moves to the move list.
        MoveList moveList(c);

        // If the next pv move is in the move list, make the move.
        if (moveList.contains(nextMove))
            c.make_t(nextMove);
        // If the next pv move is not found, break the loop.
        else
        {
            mSize = i;
            break;
        }
    }
}

void LineManager::printPv()
{
    std::cout << " pv ";
    for (auto it = mPv.begin(); it != mPv.begin() + mSize; ++it)
        std::cout << toString(it->first) << " ";
}