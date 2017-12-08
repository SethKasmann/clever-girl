#ifndef MOVE_H
#define MOVE_H

#include <string>
#include "types.h"

typedef uint16_t Move_t;

struct MoveEntry
{
	Move_t move;
	int score;
};

inline bool operator==(const MoveEntry& pEntry, const Move_t pMove)
{
	return pEntry.move == pMove;
}

inline bool operator<(const MoveEntry& pEntry1, const MoveEntry& pEntry2)
{
	return pEntry1.score < pEntry2.score;
}

static const Move_t nullMove = 0;
static const Move_t srcMask           = 0x003F;
static const Move_t dstMask           = 0x0FC0;
static const Move_t piecePromoMask    = 0x7000;
static const Move_t castleFlag        = 0x8000;

inline Move_t makeMove(Square src, Square dst)
{
	return static_cast<Move_t>(src) | static_cast<Move_t>(dst) << 6;
}

inline Move_t makeMove(Square src, Square dst, PieceType promo)
{
	return static_cast<Move_t>(src) 
	     | static_cast<Move_t>(dst)   << 6 
	     | static_cast<Move_t>(promo) << 12;
}

inline Move_t makeCastle(Square src, Square dst)
{
	return static_cast<Move_t>(src) 
	     | static_cast<Move_t>(dst) << 6
	     | castleFlag;
}

inline Square getSrc(Move_t m)
{
	return static_cast<Square>(m & srcMask);
}

inline Square getDst(Move_t m)
{
	return static_cast<Square>((m & dstMask) >> 6);
}

inline PieceType getPiecePromo(Move_t m)
{
	return static_cast<PieceType>((m & piecePromoMask) >> 12);
}

inline bool isCastle(Move_t m)
{
	return m & castleFlag;
}

inline std::string toString(Move_t m)
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