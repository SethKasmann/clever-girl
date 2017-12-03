#ifndef PV_H
#define PV_H

#include <array>
#include <algorithm>
#include <iterator>
#include "state.h"
#include "move_generator.h"
#include "line_element.h"
#include "types.h"

namespace cgirl {

static const int pv_max_size = ((Max_ply*Max_ply) + Max_ply) / 2;

inline int triangular_index(int ply)
{
	return ply * (2 * Max_ply + 1 - ply) / 2;
}

class line_manager
{
public:
	line_manager()
	: mMatingLine(false), mSize(0)
	{}
	void push_to_pv(Move pMove, U64 pKey, int pPly, int pScore)
	{
		// Check if this is a mating line.
		mMatingLine = std::abs(pScore) + pPly >= Checkmate ? true : false;

		// Find the indicies using the triangular forumla.
		int copyTo = triangular_index(pPly);
		int copyFromStart = triangular_index(pPly + 1);
		int copyFromEnd = copyFromStart + Max_ply - pPly - 1;

		// Store the current move.
		mPv[copyTo++] = cgirl::line_element(pMove, pKey);

		// Copy from the previous iteration.
		std::copy(std::make_move_iterator(mPv.begin() + copyFromStart), 
			      std::make_move_iterator(mPv.begin() + copyFromEnd), 
			      mPv.begin() + copyTo);
	}
	U64 get_pv_key(int pPly=0) const
	{
		return mPv[pPly].get_key();
	}
	Move get_pv_move(int pPly=0) const
	{
		return mPv[pPly].get_move();
	}
	bool is_mate() const
	{
		return mMatingLine;
	}
	int get_mate_in_n() const
	{
		return mSize / 2.0 + 0.5;
	}
	void clear_pv()
	{
		mSize = 0;
		std::fill(mPv.begin(), mPv.begin() + Max_ply, cgirl::line_element(0, 0));
	}
	void check_pv(State& pState)
	{
		State c;
		MoveList moveList;
		Move nextMove;
		std::memmove(&c, &pState, sizeof(pState));
		for (int i = 0; i < Max_ply; ++i)
		{
			nextMove = mPv[i].get_move();
			if (nextMove == No_move)
			{
				mSize = i;
				break;
			}
			// Push moves to the move list.
			moveList.clear();
			push_moves(c, &moveList);

			// If the next pv move is in the move list, make the move.
			if (moveList.contains(nextMove))
				c.make(nextMove);
			// If the next pv move is not found, break the loop.
			else
			{
				mSize = i;
				break;
			}
		}
	}
	void print_pv()
	{
		std::cout << " pv ";
		std::copy(mPv.begin(), 
			      mPv.begin() + mSize, 
			      std::ostream_iterator<cgirl::line_element>(std::cout, " "));
	}
private:
	std::array<cgirl::line_element, pv_max_size> mPv;
	bool mMatingLine;
	std::size_t mSize;
};
}

#endif