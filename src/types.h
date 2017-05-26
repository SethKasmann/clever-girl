#ifndef TYPES_H
#define TYPES_H

#include <string>


#define NDEBUG
#include <assert.h>

// ----------------------------------------------------------------------------
// Bitboard Typedef
// ----------------------------------------------------------------------------

typedef unsigned long long U64;
#define C64(constantU64) constantU64##ULL;

// ----------------------------------------------------------------------------
// Board Types
// ----------------------------------------------------------------------------

static const int BOARD_SIZE  = 64;
static const int TYPES_SIZE  = 6;
static const int PLAYER_SIZE = 2;
static const int PIECE_MAX   = 10;

static const int CASTLE_RIGHTS[BOARD_SIZE] = 
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

enum Color 
{ 
	WHITE, 
	BLACK 
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
	FIRST_SQ = 0, LAST_SQ = 63
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

enum CR
{
	W_KING_CASTLE = 1,
	W_QUEEN_CASTLE = 2,
	B_KING_CASTLE = 4,
	B_QUEEN_CASTLE = 8
};

// ----------------------------------------------------------------------------
// Move Types
//
// Moves are stored in 32 bits:
// bits 0-5	  : source location
// bits 6-11  : destination location
// bits 12-15 : move property
// bits 16-22 : move score
// ----------------------------------------------------------------------------

typedef unsigned int Move;

static const int SCORE[TYPES_SIZE][TYPES_SIZE] = 
{
	{ 25, 29, 30, 32, 35, 0 },  
    { 19, 24, 26, 28, 34, 0 },  
    { 18, 20, 23, 27, 33, 0 },  
    { 15, 16, 17, 22, 31, 0 },  
	{ 11, 12, 13, 14, 21, 0 },  
    { 5,  6,  7,  8,  9,  0 } 
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

enum CheckType
{
	NO_CHECK,
	SINGLE_CHECK,
	DOUBLE_CHECK
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

inline Square get_src(Move m) { return Square(m & 0x3F); }
inline Square get_dst(Move m) { return Square((m & 0xFC0) >> 6); }
inline Prop get_prop(Move m)  { return Prop((m & 0xF000) >> 12); }

// ----------------------------------------------------------------------------
// Search Types
// ----------------------------------------------------------------------------

const int NEG_INF   = -1001;
const int POS_INF   = 1001;
const int KILLER_SZ = 2;
const int MAX_PLY   = 50;

enum NodeType
{
	PV, CUT, ALL
};

// ----------------------------------------------------------------------------
// Operators
// ----------------------------------------------------------------------------

template<typename T>
inline T & operator++(T & t) { return t = T(int(t) + 1); }
template<typename T>
inline T & operator--(T & t) { return t = T(int(t) - 1); }
template<typename T>
inline T & operator+(const T t0, const T t1) { return T(int(t0) + int(t1)); }
template<typename T>
inline T operator+(const T t0, const int i) { return T(int(t0) + i); }
template<typename T>
inline T & operator-(const T t0, const T t1) { return T(int(t0) - int(t1)); }
template<typename T>
inline T operator-(const T t0, const int i) { return T(int(t0) - i); }
template<typename T>
inline T & operator +=(T & t0, const T t1) { return t0 = t0 + t1; }
template<typename T>
inline T & operator -=(T & t0, const T t1) { return t0 = t0 - t1; }
template<typename T>
inline T operator!(const T t) { return T(!bool(t)); }
template<typename T>
inline U64 operator&(const T t, const U64 u) { return u & (1ULL << t); }

#endif