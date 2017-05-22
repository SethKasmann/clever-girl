#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>


#define NDEBUG
#include <assert.h>

typedef unsigned long long U64;
#define C64(constantU64) constantU64##ULL;

//=============================================================================
// Encoded Move
// Moves are stored in 23 bits:
// bits 0-5		: source location
// bits 6-11	: destination location
// bits 12-15	: move property
// bits 16-22	: move score
//=============================================================================
typedef unsigned int Move;

const int NEG_INF = -1001;
const int POS_INF = 1001;

const int BOARD_SZ = 64;
const int OCC_SZ = 2;
const int PIECE_TYPES_SZ = 6;
const int PLAYER_SZ = 2;
const int MAX_PIECE_COUNT = 10;

const int MAX_PLY = 50;
const int KILLER_SZ = 2;

enum Color
{
	WHITE,
	BLACK
};

enum MoveScore
{
	BP = 1,
	RP = 2,
	NP = 3,
	QP = 36,
	Q  = 4,
	C  = 10,
	EP = 25
};

enum PieceType
{
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	NONE
};

enum Castle_Rights
{
    NO_RIGHTS,
    KING_ONLY,
    QUEEN_ONLY,
    ALL_RIGHTS
};

enum CR
{
	W_KING_CASTLE = 1,
	W_QUEEN_CASTLE = 2,
	B_KING_CASTLE = 4,
	B_QUEEN_CASTLE = 8
};

enum Square
{
	H1, G1, F1, E1, D1, C1, B1, A1,
	H2, G2, F2, E2, D2, C2, B2, A2,
	H3, G3, F3, E3, D3, C3, B3, A3,
	H4, G4, F4, E4, D4, C4, B4, A4,
	H5, G5, F5, E5, D5, C5, B5, A5,
	H6, G6, F6, E6, D6, C6, B6, A6,
	H7, G7, F7, E7, D7, C7, B7, A7,
	H8, G8, F8, E8, D8, C8, B8, A8,
	NO_SQ = -1,
};

enum File
{
	A_FILE,
	B_FILE,
	C_FILE,
	D_FILE,
	E_FILE,
	F_FILE,
	G_FILE,
	H_FILE
};

enum Rank
{
	RANK_1,
	RANK_2,
	RANK_3,
	RANK_4,
	RANK_5,
	RANK_6,
	RANK_7,
	RANK_8
};

enum Dir
{
	N  =  8,
	S  = -8,
	E  = -1,
	W  =  1,
	NE =  7,
	NW =  9,
	SE = -9,
	SW = -7
};

const std::string SQ[64] =
{
	"h1", "g1", "f1", "e1", "d1", "c1", "b1", "a1",
	"h2", "g2", "f2", "e2", "d2", "c2", "b2", "a2",
	"h3", "g3", "f3", "e3", "d3", "c3", "b3", "a3",
	"h4", "g4", "f4", "e4", "d4", "c4", "b4", "a4",
	"h5", "g5", "f5", "e5", "d5", "c5", "b5", "a5",
	"h6", "g6", "f6", "e6", "d6", "c6", "b6", "a6",
	"h7", "g7", "f7", "e7", "d7", "c7", "b7", "a7",
	"h8", "g8", "f8", "e8", "d8", "c8", "b8", "a8"
};

enum Prop
{
	QUIET,
	ATTACK,
	DBL_PUSH,
	KING_CAST,
	QUEEN_CAST,
	QUEEN_PROMO,
	KNIGHT_PROMO,
	ROOK_PROMO,
	BISHOP_PROMO,
	EN_PASSANT
};

enum NodeType
{
	PV, CUT, ALL
};

const int EP_VICTIM[64] = 
{
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
	24, 25, 26, 27, 28, 29, 30, 31,
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,
    32, 33, 34, 35, 36, 37, 38, 39,	
	0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0
};

const int CASTLE_RIGHTS[64] = 
{
	14, 15, 15, 12, 15, 15, 15, 13,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	11, 15, 15,  3, 15, 15, 15,  7
};

inline Color operator!(const Color c)
{
	return Color(!bool(c));
}

inline Square & operator++(Square & s)
{
	s = Square(int(s) + 1);
	return s;
}

inline Square & operator--(Square & s)
{
	s = Square(int(s) - 1);
	return s;
}

inline PieceType operator+(Color c, PieceType p)
{
	return PieceType(int(c) + int(p));
}

inline PieceType & operator++(PieceType & p)
{
	return p = PieceType(int(p) + 1);
}

inline File & operator++(File & f)
{
	return f = File(int(f) + 1);
}

#endif