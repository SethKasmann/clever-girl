#ifndef MOVE_H
#define MOVE_H

#include <string>
#include "types.h"

typedef uint16_t Move;

struct MoveEntry
{
	Move move;
	int score;
};

inline bool noScore(const MoveEntry & entry)
{
	return entry.score == 0;
}

inline bool operator==(const MoveEntry& pEntry, const Move pMove)
{
	return pEntry.move == pMove;
}

inline bool operator<(const MoveEntry& pEntry1, const MoveEntry& pEntry2)
{
	return pEntry1.score < pEntry2.score;
}

static const Move nullMove = 0;
static const Move srcMask           = 0x003F;
static const Move dstMask           = 0x0FC0;
static const Move piecePromoMask    = 0x7000;
static const Move castleFlag        = 0x8000;

inline Move makeMove(Square src, Square dst)
{
	return static_cast<Move>(src) | static_cast<Move>(dst) << 6;
}

inline Move makeMove(Square src, Square dst, PieceType promo)
{
	return static_cast<Move>(src) 
	     | static_cast<Move>(dst)   << 6 
	     | static_cast<Move>(promo) << 12;
}

inline Move makeCastle(Square src, Square dst)
{
	return static_cast<Move>(src) 
	     | static_cast<Move>(dst) << 6
	     | castleFlag;
}

inline Square getSrc(Move m)
{
	return static_cast<Square>(m & srcMask);
}

inline Square getDst(Move m)
{
	return static_cast<Square>((m & dstMask) >> 6);
}

inline PieceType getPiecePromo(Move m)
{
	return static_cast<PieceType>((m & piecePromoMask) >> 12);
}

inline bool isCastle(Move m)
{
	return m & castleFlag;
}

inline std::string toString(Move m)
{
	std::string ret;
	Square src, dst;

	src = getSrc(m);
	dst = getDst(m);

	ret = SQ[src] + SQ[dst];

	switch (getPiecePromo(m))
	{
		case knight:
			ret += "n";
			break;
		case bishop:
			ret += "b";
			break;
		case rook:
			ret += "r";
			break;
		case queen:
			ret += "q";
			break;
		default:
			break;
	}

	return ret;
}

#endif