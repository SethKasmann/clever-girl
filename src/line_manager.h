#ifndef PV_H
#define PV_H

#include <array>
#include <algorithm>
#include <iterator>
#include "state.h"
#include "move_generator.h"
#include "types.h"
#include "move.h"

static const int pvMaxSize = ((Max_ply*Max_ply) + Max_ply) / 2;

inline int triangularIndex(int ply)
{
    return ply * (2 * Max_ply + 1 - ply) / 2;
}

class LineManager
{
public:
    LineManager();
    void pushToPv(Move pMove, U64 pKey, int pPly, int pScore);
    U64 getPvKey(int pPly=0) const;
    Move getPvMove(int pPly=0) const;
    bool isMate() const;
    int getMateInN() const;
    void clearPv();
    void checkPv(State& pState);
    void printPv();
private:
    std::array<std::pair<Move, U64>, pvMaxSize> mPv;
    bool mMatingLine;
    std::size_t mSize;
};

#endif