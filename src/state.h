//=============================================================================
// A struct to keep track of the game state
//=============================================================================

#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include "bitboard.h"
#include "types.h"

struct State
{
    State() {};
    State(std::string &);
    State(const State & s);
    void operator=(const State &);

    // EDP string for database query.
    const char * get_EPD() const;

    // Access piece bitboards.
    U64 p_pawn() const;
    U64 e_pawn() const;
    U64 p_knight() const;
    U64 e_knight() const;
    U64 p_bishop() const;
    U64 e_bishop() const;
    U64 p_rook() const;
    U64 e_rook() const;
    U64 p_queen() const;
    U64 e_queen() const;
    U64 p_king() const;
    U64 e_king() const;

    // Access king square.
    Square p_king_sq() const;
    Square e_king_sq() const;

    // Board occupancy
    U64 occ() const;
    U64 p_occ() const;
    U64 & p_occ();
    U64 e_occ() const;
    U64 & e_occ();
    U64 empty() const;
    PieceType on_square(const Square s, const Color c) const;
    PieceType on_square(const Square s) const;

    // Castle rights.
    bool k_castle() const;
    bool q_castle() const;

    // Piece differentials.
    int pawn_diff() const;
    int knight_diff() const;
    int bishop_diff() const;
    int rook_diff() const;
    int queen_diff() const;

    // Functions involved in making a move.
    void make(Move m);
    void add_piece(int, PieceType);
    void move_piece(int, int);
    void del_piece(bool, int);
    void swap_turn();

    // Valid king moves and pins.
    U64 valid_king_moves() const;
    U64 get_pins() const;

    // Check and attack information.
    template<PieceType> U64 attack_bb(const Square s) const;
    bool is_attacked(const Square s) const;
    bool is_attacked_by_slider(U64 change) const;
    bool in_check();
    U64 checkers() const;

    // Print
    friend std::ostream & operator << (std::ostream & o, const State & state);

    int fmr;
    int castle;
    PieceType board[PLAYER_SIZE][BOARD_SIZE];
    int piece_count[PLAYER_SIZE][TYPES_SIZE];
    Square piece_list[PLAYER_SIZE][TYPES_SIZE][PIECE_MAX];
    int piece_index[BOARD_SIZE];
    U64 pieces[PLAYER_SIZE][TYPES_SIZE];
    U64 occupancy[PLAYER_SIZE];
    U64 en_passant;
    U64 key;
    Color us;
    Color them;
};

inline void State::add_piece(int dst, PieceType p)
{
    if (board[them][dst] != NONE)
    {
        del_piece(them, dst);
    }
    set_bit(pieces[us][p], dst);
    set_bit(occupancy[us], dst);
    board[us][dst] = p;
    piece_index[dst] = piece_count[us][p];
    piece_list[us][p][piece_index[dst]] = Square(dst);
    piece_count[us][p]++;
}

inline void State::move_piece(int src, int dst)
{
    PieceType p = board[us][src];
    move_bit(pieces[us][p], src, dst);
    move_bit(occupancy[us], src, dst);
    board[us][dst] = p;
    board[us][src] = NONE;
    piece_index[dst] = piece_index[src];
    piece_list[us][p][piece_index[dst]] = Square(dst);
}

inline void State::del_piece(bool c, int dst)
{
    PieceType p = board[c][dst];
    piece_count[c][p] -= 1;
    clear_bit(pieces[c][p], dst);
    clear_bit(occupancy[c], dst);
    board[c][dst] = NONE;
    Square swap = piece_list[c][p][piece_count[c][p]];
    piece_index[swap] = piece_index[dst];
    piece_list[c][p][piece_index[swap]] = swap;
    piece_list[c][p][piece_count[c][p]] = NO_SQ;
}

inline
void State::swap_turn()
{
    them =  us;
    us   = !us;
}

inline
U64 State::p_pawn() const
{
    return pieces[us][PAWN];
}

inline
U64 State::e_pawn() const
{
    return pieces[them][PAWN];
}

inline
U64 State::p_knight() const
{
    return pieces[us][KNIGHT];
}

inline
U64 State::e_knight() const
{
    return pieces[them][KNIGHT];
}

inline
U64 State::p_bishop() const
{
    return pieces[us][BISHOP];
}

inline
U64 State::e_bishop() const
{
    return pieces[them][BISHOP];
}

inline
U64 State::p_rook() const
{
    return pieces[us][ROOK];
}

inline
U64 State::e_rook() const
{
    return pieces[them][ROOK];
}

inline
U64 State::p_queen() const
{
    return pieces[us][QUEEN];
}

inline
U64 State::e_queen() const
{
    return pieces[them][QUEEN];
}

inline
U64 State::p_king() const
{
    return pieces[us][KING];
}

inline
U64 State::e_king() const
{
    return pieces[them][KING];
}

inline
Square State::p_king_sq() const
{
    return piece_list[us][KING][0];
}

inline
Square State::e_king_sq() const
{
    return piece_list[them][KING][0];
}

inline
PieceType State::on_square(const Square s) const
{
    return board[us][s] != NONE ? board[us][s] : board[them][s];
}

inline
PieceType State::on_square(const Square s, const Color c) const
{
    return board[c][s];
} 

inline
U64 State::p_occ() const
{
    return occupancy[us];
}

inline
U64 & State::p_occ()
{
    return occupancy[us];
}

inline
U64 State::e_occ() const
{
    return occupancy[them];
}

inline
U64 & State::e_occ()
{
    return occupancy[them];
}

inline
U64 State::occ() const
{
    return occupancy[WHITE] | occupancy[BLACK];
}

inline
U64 State::empty() const
{
    return ~(occupancy[WHITE] | occupancy[BLACK]);
}

inline 
bool State::k_castle() const
{
    return us ? castle & B_KING_CASTLE : castle & W_KING_CASTLE;
}

inline
bool State::q_castle() const
{
    return us ? castle & B_QUEEN_CASTLE : castle & W_QUEEN_CASTLE;
}

template<>
inline U64 State::attack_bb<KNIGHT>(const Square s) const
{
    return knight_moves[s];
}

template<>
inline U64 State::attack_bb<BISHOP>(const Square s) const
{
    return Bmagic(s, occ());
}

template<>
inline U64 State::attack_bb<ROOK>(const Square s) const
{
    return Rmagic(s, occ());
}

template<>
inline U64 State::attack_bb<QUEEN>(const Square s) const
{
    return Qmagic(s, occ());
}

inline
bool State::is_attacked(const Square s) const
{
    return (pawn_attacks[us][s] & e_pawn())
        || (knight_moves[s] & e_knight())
        || (Bmagic(s, occ() ^ p_king()) & (e_bishop() | e_queen()))
        || (Rmagic(s, occ() ^ p_king()) & (e_rook()   | e_queen()));
}

inline
bool State::is_attacked_by_slider(U64 change) const
{
    return (Bmagic(p_king_sq(), occ() ^ change) & (e_bishop() | e_queen()))
         | (Rmagic(p_king_sq(), occ() ^ change) & (e_rook()   | e_queen()));
}

inline
U64 State::checkers() const
{
    return (pawn_attacks[us][p_king_sq()] &  e_pawn())
         | (knight_moves[p_king_sq()]     &  e_knight())
         | (Bmagic(p_king_sq(), occ())    & (e_bishop() | e_queen()))
         | (Rmagic(p_king_sq(), occ())    & (e_rook()   | e_queen()));
}

#endif