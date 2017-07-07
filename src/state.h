//=============================================================================
// A struct to keep track of the game state
//=============================================================================

#ifndef STATE_H
#define STATE_H

#include <iostream>
#include <string>
#include <cmath>
#include <cstring>
#include <algorithm>
#include "bitboard.h"
#include "types.h"

struct State
{
    State() {};
    State(const std::string &);
    State(const State & s);
    void operator=(const State &);

    // EDP string for database query.
    const char * get_EPD() const;

    // Access piece bitboards.
    template<PieceType P> U64 piece_bb(Color c) const;
    template<PieceType P> const Square* piece(Color c) const;
    Square king_sq(Color c) const;

    // Board occupancy
    U64 occ() const;
    U64 occ(Color c) const;
    U64 empty() const;
    PieceType on_square(const Square s, const Color c) const;
    PieceType on_square(const Square s) const;

    // Castle rights.
    bool k_castle() const;
    bool q_castle() const;

    // Functions involved in making a move.
    void make(Move m);
    void add_piece(int, PieceType);
    void add_piece(Color c, PieceType p, Square s);
    void move_piece(int, int);
    void del_piece(bool, int);
    void swap_turn();

    // Valid king moves and pins.
    U64 valid_king_moves() const;
    U64 get_pins() const;

    // Check and attack information.
    bool attacked(Square s) const;
    bool defended(Square s, Color c) const;
    bool check() const;
    bool check(U64 change) const;
    bool check(U64 change, Color c) const;
    U64 attackers(Square s) const;
    U64 checkers() const;
    template<PieceType> U64 attack_bb(Square s) const;

    // Print
    friend std::ostream & operator << (std::ostream & o, const State & state);

    int fmr;
    int castle;
    PieceType board[Player_size][Board_size];
    int piece_count[Player_size][Types_size];
    Square piece_list[Player_size][Types_size][Piece_max];
    int piece_index[Board_size];
    U64 pieces[Player_size][Types_size];
    U64 occupancy[Player_size];
    U64 ep;
    U64 key;
    Color us;
    Color them;
};

inline void State::add_piece(Color c, PieceType p, Square s)
{
    set_bit(pieces[c][p], s);
    set_bit(occupancy[c], s);
    board[c][s] = p;
    piece_index[s] = piece_count[c][p];
    piece_list[c][p][piece_index[s]] = s;
    piece_count[c][p]++;
}

inline void State::add_piece(int dst, PieceType p)
{
    if (board[them][dst] != none)
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
    board[us][src] = none;
    piece_index[dst] = piece_index[src];
    piece_list[us][p][piece_index[dst]] = Square(dst);
}

inline void State::del_piece(bool c, int dst)
{
    PieceType p = board[c][dst];
    piece_count[c][p]--;
    clear_bit(pieces[c][p], dst);
    clear_bit(occupancy[c], dst);
    board[c][dst] = none;
    Square swap = piece_list[c][p][piece_count[c][p]];
    piece_index[swap] = piece_index[dst];
    piece_list[c][p][piece_index[swap]] = swap;
    piece_list[c][p][piece_count[c][p]] = no_sq;
}

inline
void State::swap_turn()
{
    them =  us;
    us   = !us;
}

template<PieceType P>
inline const Square* State::piece(Color c) const
{
    return piece_list[c][P];
}

template<PieceType P>
inline U64 State::piece_bb(Color c) const
{
    return pieces[c][P];
}

inline
Square State::king_sq(Color c) const
{
    return piece_list[c][king][0];
}

inline
PieceType State::on_square(const Square s) const
{
    return board[us][s] != none ? board[us][s] : board[them][s];
}

inline
PieceType State::on_square(const Square s, const Color c) const
{
    return board[c][s];
} 

inline
U64 State::occ() const
{
    return occupancy[white] | occupancy[black];
}

inline
U64 State::occ(Color c) const
{
    return occupancy[c];
}

inline
U64 State::empty() const
{
    return ~(occupancy[white] | occupancy[black]);
}

inline 
bool State::k_castle() const
{
    return us ? castle & b_king_castle : castle & w_king_castle;
}

inline
bool State::q_castle() const
{
    return us ? castle & b_queen_castle : castle & w_queen_castle;
}

template<>
inline U64 State::attack_bb<pawn>(Square s) const
{
    return pawn_attacks[us][s];
}

template<>
inline U64 State::attack_bb<knight>(Square s) const
{
    return Knight_moves[s];
}

template<>
inline U64 State::attack_bb<bishop>(Square s) const
{
    return Bmagic(s, occ());
}

template<>
inline U64 State::attack_bb<rook>(Square s) const
{
    return Rmagic(s, occ());
}

template<>
inline U64 State::attack_bb<queen>(Square s) const
{
    return Qmagic(s, occ());
}

inline
bool State::defended(Square s, Color c) const
{
    return pawn_attacks[!c][s]  &  piece_bb< pawn >(c)
        || attack_bb<knight>(s) &  piece_bb<knight>(c)
        || attack_bb<bishop>(s) & (piece_bb<bishop>(c) | piece_bb<queen>(c))
        || attack_bb< rook >(s) & (piece_bb< rook >(c) | piece_bb<queen>(c));
}

inline
bool State::attacked(Square s) const
{
    return attack_bb< pawn >(s) &  piece_bb< pawn >(them)
        || attack_bb<knight>(s) &  piece_bb<knight>(them)
        || attack_bb<bishop>(s) & (piece_bb<bishop>(them) | piece_bb<queen>(them))
        || attack_bb< rook >(s) & (piece_bb< rook >(them) | piece_bb<queen>(them));
}

inline
bool State::check() const
{
    return attacked(king_sq(us));
}

inline
U64 State::attackers(Square s) const
{
    return (attack_bb< pawn >(s) &  piece_bb< pawn >(them)
         | attack_bb<knight>(s) &  piece_bb<knight>(them)
         | attack_bb<bishop>(s) & (piece_bb<bishop>(them) | piece_bb<queen>(them))
         | attack_bb< rook >(s) & (piece_bb< rook >(them) | piece_bb<queen>(them)));
}

inline
U64 State::checkers() const
{
    return attackers(king_sq(us));
}

inline
bool State::check(U64 change) const
{
    return (Bmagic(king_sq(us), occ() ^ change) & (piece_bb<bishop>(them) | piece_bb<queen>(them)))
        || (Rmagic(king_sq(us), occ() ^ change) & (piece_bb< rook >(them) | piece_bb<queen>(them)));
}

inline
bool State::check(U64 change, Color c) const
{
    return (Bmagic(king_sq(c), occ() ^ change) & (piece_bb<bishop>(!c) | piece_bb<queen>(!c)))
        || (Rmagic(king_sq(c), occ() ^ change) & (piece_bb< rook >(!c) | piece_bb<queen>(!c)));
}


#endif