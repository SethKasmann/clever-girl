#ifndef MOVE_H
#define MOVE_H

#include <iostream>
#include "bitboard.h"
#include "types.h"

const int MAX_SIZE = 256;

class MoveList
{
public:
	MoveList() : e(_m), c(_m) 
	{}
	~MoveList() 
	{}
	int size();
	void push(int src, int dst, int p, int s);
	Move pop();
private:
	Move _m[MAX_SIZE];
public:
	Move * c;
	Move * e;
};

inline int MoveList::size()
{
	return e - _m;
}

inline void MoveList::push(int src, int dst, int p, int s)
{
	*(e++) = src | dst << 6 | p << 12 | s << 16;
}

inline Move MoveList::pop()
{
	return *(--e); 
}

inline
Move make_move(int src, int dst, int p, int s)
{
	return src | dst << 6 | p << 12 | s << 16;
}

inline
Square get_src(Move m)
{
	return Square(m & 0x3F);
}

inline
Square get_dst(Move m)
{
	return Square((m & 0xFC0) >> 6);
}

inline
Prop get_prop(Move m)
{
	return Prop((m & 0xF000) >> 12);
}

#endif