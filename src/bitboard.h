//=============================================================================
// A file to keep basic functions and algorithms
// regarding bitboards.
//=============================================================================

#ifndef BITBOARD_H
#define BITBOARD_H

#include <string>
#include <iostream>
#include <cmath>
#include "types.h"
#include "MagicMoves.hpp"

extern U64 square_bb[Board_size];
extern U64 file_bb[Board_size];
extern U64 rank_bb[Board_size];
extern U64 pawn_attacks[Player_size][Board_size];
extern U64 pawn_push[Player_size][Board_size];
extern U64 pawn_dbl_push[Player_size][Board_size];
extern U64 between_dia[Board_size][Board_size];
extern U64 between_hor[Board_size][Board_size];
extern U64 coplanar[Board_size][Board_size];
extern U64 adj_files[Board_size];
extern U64 in_front[Player_size][Board_size];
extern U64 king_net_bb[Player_size][Board_size];
extern const U64 Knight_moves[Board_size];
extern const U64 King_moves[Board_size];

static const U64 Rank_1 = 0x00000000000000FFULL;
static const U64 Rank_2 = 0x000000000000FF00ULL;
static const U64 Rank_3 = 0x0000000000FF0000ULL;
static const U64 Rank_4 = 0x00000000FF000000ULL;
static const U64 Rank_5 = 0x000000FF00000000ULL;
static const U64 Rank_6 = 0x0000FF0000000000ULL;
static const U64 Rank_7 = 0x00FF000000000000ULL;
static const U64 Rank_8 = 0xFF00000000000000ULL;

static const U64 File_a = 0x8080808080808080ULL;
static const U64 File_b = 0x4040404040404040ULL;
static const U64 File_c = 0x2020202020202020ULL;
static const U64 File_d = 0x1010101010101010ULL;
static const U64 File_e = 0x0808080808080808ULL;
static const U64 File_f = 0x0404040404040404ULL;
static const U64 File_g = 0x0202020202020202ULL;
static const U64 File_h = 0x0101010101010101ULL;

static const U64 Not_a_file = 0x7F7F7F7F7F7F7F7F;
static const U64 Not_h_file = 0xFEFEFEFEFEFEFEFE;

void bb_init();
//unsigned int pop_count(U64);

// Returns the number of 1-bits.
inline int pop_count(U64 bb)
{
   return __builtin_popcountll(bb); 
}

// Returns the index of the LSB.
inline Square get_lsb(U64 bb)
{
   assert(bb);
   return Square(__builtin_ctzll(bb));
}

inline U64 get_lsb_bb(U64 bb)
{
   assert(bb);
   return 1ULL << get_lsb(bb);
}

// Clears the LSB and returns the index.
inline Square pop_lsb(U64 & bb)
{
   U64 t = bb;
   bb &= (bb - 1);
   return get_lsb(t);
}

// Clears the LSB and returns as a bitboard.
inline U64 pop_lsb_bb(U64 & bb)
{
   assert(bb);
   U64 t = bb;
   bb &= (bb - 1);
   return t ^ bb;
}

// Returns the file of the LSB
inline File get_file(U64 bb)
{
   assert(bb);
	return File(get_lsb(bb) % 8);
}

inline Rank get_rank(U64 bb)
{
   assert(bb);
   return Rank(get_lsb(bb) / 8);
}

inline
U64 shift_up(U64 bb, const Dir D)
{
   return D == N  ? bb                << 8 
        : D == NE ? (bb & Not_h_file) << 9
        : D == NW ? (bb & Not_a_file) << 7
        : 0;
}

inline
U64 shift(U64 bb, const Dir D)
{
   return D == N ? bb << 8 : bb >> 8;
}

inline
U64 shift_e(U64 bb, const Dir D)
{
   return D == NE ? (bb & Not_h_file) << 7 : (bb & Not_h_file) >> 9;
}

inline
U64 shift_w(U64 bb, const Dir D)
{
   return D == NW ? (bb & Not_a_file) << 9 : (bb & Not_a_file) >> 7;
}

inline
U64 shift_ep(U64 bb, const Dir D)
{
   return D == E ? (bb & Not_h_file) >> 1 : (bb & Not_a_file) << 1;
}

inline
void clear_bit(U64 & bb, int dst)
{
   bb &= ~(1ULL << dst);
}

inline
void set_bit(U64 & bb, int dst)
{
   bb |= 1ULL << dst;
}

inline 
void move_bit(U64 & bb, int src, int dst)
{
   bb ^= (1ULL << src) | (1ULL << dst);
}

// Prints a bitboard for debugging purposes.
void print_bb(U64);

// Currently unused fill functions;
inline
U64 north_fill(U64 gen) 
{
   gen |= (gen <<  8);
   gen |= (gen << 16);
   gen |= (gen << 32);
   return gen;
}

inline
U64 south_fill(U64 gen) 
{
   gen |= (gen >>  8);
   gen |= (gen >> 16);
   gen |= (gen >> 32);
   return gen;
}

inline
U64 east_fill(U64 gen) 
{
   const U64 pr0 = Not_h_file;
   const U64 pr1 = pr0 & (pr0 << 1);
   const U64 pr2 = pr1 & (pr1 << 2);
   gen |= pr0 & (gen  << 1);
   gen |= pr1 & (gen  << 2);
   gen |= pr2 & (gen  << 4);
   return gen;
}

inline
U64 north_east_fill(U64 gen) 
{
   const U64 pr0 = Not_h_file;
   const U64 pr1 = pr0 & (pr0 <<  9);
   const U64 pr2 = pr1 & (pr1 << 18);
   gen |= pr0 & (gen <<  9);
   gen |= pr1 & (gen << 18);
   gen |= pr2 & (gen << 36);
   return gen;
}

inline
U64 south_east_fill(U64 gen) 
{
   const U64 pr0 = Not_h_file;
   const U64 pr1 = pr0 & (pr0 >>  7);
   const U64 pr2 = pr1 & (pr1 >> 14);
   gen |= pr0 & (gen >>  7);
   gen |= pr1 & (gen >> 14);
   gen |= pr2 & (gen >> 28);
   return gen;
}

inline
U64 west_fill(U64 gen) 
{
   const U64 pr0 = Not_a_file;
   const U64 pr1 = pr0 & (pr0 >> 1);
   const U64 pr2 = pr1 & (pr1 >> 2);
   gen |= pr0 & (gen >> 1);
   gen |= pr1 & (gen >> 2);
   gen |= pr2 & (gen >> 4);
   return gen;
}

inline
U64 south_west_fill(U64 gen) 
{
   const U64 pr0 = Not_a_file;
   const U64 pr1 = pr0 & (pr0 >>  9);
   const U64 pr2 = pr1 & (pr1 >> 18);
   gen |= pr0 & (gen >>  9);
   gen |= pr1 & (gen >> 18);
   gen |= pr2 & (gen >> 36);
   return gen;
}

inline
U64 north_west_fill(U64 gen) 
{
   const U64 pr0 = Not_a_file;
   const U64 pr1 = pr0 & (pr0 <<  7);
   const U64 pr2 = pr1 & (pr1 << 14);
   gen |= pr0 & (gen <<  7);
   gen |= pr1 & (gen << 14);
   gen |= pr2 & (gen << 28);
   return gen;
}

#endif