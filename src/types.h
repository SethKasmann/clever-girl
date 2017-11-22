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

static const int Board_size  = 64;
static const int Types_size  = 6;
static const int Player_size = 2;
static const int Piece_max   = 10;

static const int Castle_rights[Board_size] = 
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
    white, 
    black 
};

enum PieceType
{
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king,
    none
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
    no_sq = -1,
    first_sq = 0, last_sq = 63
};

enum File
{
    a_file,
    b_file,
    c_file,
    d_file,
    e_file,
    f_file,
    g_file,
    h_file
};

inline File file(Square s)
{
    return File(s & 0x7);
}

enum Rank
{
    rank_1,
    rank_2,
    rank_3,
    rank_4,
    rank_5,
    rank_6,
    rank_7,
    rank_8
};

inline Rank rank(Square s)
{
    return Rank(s >> 3);
}

enum CR
{
    w_king_castle = 1,
    w_queen_castle = 2,
    b_king_castle = 4,
    b_queen_castle = 8
};

// ----------------------------------------------------------------------------
// Move Types
//
// Moves are stored in 32 bits:
// bits 0-5   : source location
// bits 6-11  : destination location
// bits 12-15 : move property
// bits 16-22 : move score
// ----------------------------------------------------------------------------

typedef unsigned int Move;

static const int No_move = 0;

static const int Score[Types_size][Types_size] = 
{
    { 26, 30, 31, 33, 36, 0 },  
    { 20, 25, 27, 29, 35, 0 },  
    { 19, 21, 24, 28, 34, 0 },  
    { 16, 17, 18, 23, 32, 0 },  
    { 12, 13, 14, 15, 22, 0 },  
    { 7,  8,  9,  10, 11, 0 } 
};

enum MoveScore
{
    BP = 1,
    RP = 2,
    NP = 3,
    QP = 37,
    Q  = 4,
    C  = 5,
    QC = 6,
    EP = 26
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
    quiet,
    attack,
    dbl_push,
    king_cast,
    queen_cast,
    queen_promo,
    knight_promo,
    rook_promo,
    bishop_promo,
    en_passant
};

inline Square get_src(Move m) { return Square(m & 0x3F); }
inline Square get_dst(Move m) { return Square((m & 0xFC0) >> 6); }
inline Prop get_prop(Move m)  { return Prop((m & 0xF000) >> 12); }
inline int get_score(Move m)  { return (m & 0x7F0000) >> 16; }
inline void set_score(Move* m, int s) { *m = (*m ^ (*m & 0x7F0000)) | (s << 16); };
inline bool is_quiet(Move m)
{
    return get_prop(m) == quiet || get_prop(m) == dbl_push;
}

// ----------------------------------------------------------------------------
// Search Types
// ----------------------------------------------------------------------------

const int Neg_inf   = -50000;
const int Pos_inf   = 50000;
const int Killer_size = 2;
const int Max_ply   = 50;

enum NodeType
{
    pv, cut, all
};

// ----------------------------------------------------------------------------
// Operators
// ----------------------------------------------------------------------------
/*
template<typename T>
inline T & operator++(T & t) { return t = T(int(t) + 1); }
template<typename T>
inline T & operator--(T & t) { return t = T(int(t) - 1); }
template<typename T>
inline T operator+(const T t0, const T t1) { return T(int(t0) + int(t1)); }
template<typename T>
inline T operator+(const T t0, const int i) { return T(int(t0) + i); }
template<typename T>
inline T operator-(const T t0, const T t1) { return T(int(t0) - int(t1)); }
template<typename T>
inline T operator-(const T t0, const int i) { return T(int(t0) - i); }
template<typename T>
inline T & operator +=(T & t0, const T t1) { return t0 = t0 + t1; }
template<typename T>
inline T & operator -=(T & t0, const T t1) { return t0 = t0 - t1; }
template<typename T>
inline T operator!(const T t) { return T(!bool(t)); }
*/

inline Square& operator++(Square& s) { return s = static_cast<Square>(static_cast<int>(s) + 1); }
inline PieceType& operator++(PieceType& p) { return p = static_cast<PieceType>(static_cast<int>(p) + 1); }
inline File& operator++(File& f) { return f = static_cast<File>(static_cast<int>(f) + 1); }
inline Color operator!(const Color c) { return static_cast<Color>(!static_cast<bool>(c)); }
inline Square operator+(const Square s, const int i) { return static_cast<Square>(static_cast<int>(s) + i); }
inline Square operator-(const Square s, const int i) { return static_cast<Square>(static_cast<int>(s) - i); }

inline std::string to_string(File f)
{
    return std::string(1, char('h' - f));
}

inline std::string to_string(Rank r)
{
    return std::string(1, char('1' + r));
}

inline std::string to_string(Prop p)
{
    return p == queen_promo  ? "q"
         : p == knight_promo ? "k"
         : p == rook_promo   ? "r"
         : p == bishop_promo ? "b"
         : "";
}

inline std::string to_string(Move m)
{
    return to_string(file(get_src(m))) 
         + to_string(rank(get_src(m)))
         + to_string(file(get_dst(m))) 
         + to_string(rank(get_dst(m)))
         + to_string(get_prop(m));
}   


#endif