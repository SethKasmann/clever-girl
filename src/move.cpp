#include "move.h"

/*
Move::Move(int src,
		   int dst,
		   int prop,
		   int score)
  : move_(src | dst << 6 | prop << 12), score_(score)
{}

void Move::set_src(const U32 i)
{
	move_ = move_ & 0xFFC0 | i;
}

void Move::set_dst(const U32 i)
{
	move_ = move_ & 0xF03F | (i << 6);
}

void Move::set_prop(const U32 i)
{
	move_ = move_ & 0xFFF | (i << 12);
}

int Move::get_score() const
{
	return score_;
}

std::ostream & operator << (std::ostream & o, const Move & e)
{
	o << "====================\n";
	o << "From Square: " << std::dec << e.get_src() << '\n';
	o << "To Square: " << std::dec << e.get_dst() << '\n';
	o << "Property: ";

	int i = e.get_prop();
	switch (i)
	{
		case 0:
			o << "QUIET\n";
			break;
		case 1:
			o << "ATTACK\n";
			break;
		case 2:
			o << "DBL_PUSH\n";
			break;
		case 5:
			o << "KING_CAST\n";
			break;
		case 6:
			o << "QUEEN_CAST\n";
			break;
		case 7:
			o << "QUEEN_PROMO\n";
			break;
		case 8:
			o << "KNIGHT_PROMO\n";
			break;
		case 9:
			o << "ROOK_PROMO\n";
			break;
		case 10:
			o << "BISHOP_PROMO\n";
			break;
		case 11:
			o << "EN_PASSANT\n";
			break;
		default:
			o << "ERROR!!!\n";
			break;
	}
	o << "Score: " << e.score_ << '\n';

	o << "====================\n";

    return o;
}
*/