#include "line_element.h"

namespace cgirl {

line_element::line_element() 
: mMove(No_move), mKey(0) 
{}

line_element::line_element(Move pMove, U64 pKey)
: mMove(pMove), mKey(pKey)
{}

line_element::line_element(const line_element& pLineElement)
: mMove(pLineElement.mMove), mKey(pLineElement.mKey)
{}

line_element::line_element(line_element && pLineElement)
{
	mMove = pLineElement.mMove;
	mKey = pLineElement.mKey;
}

void line_element::operator=(const line_element& pLineElement)
{
	mMove = pLineElement.mMove;
	mKey = pLineElement.mKey;
}

Move line_element::get_move() const 
{ 
	return mMove; 
}

U64 line_element::get_key() const 
{ 
	return mKey; 
}

void line_element::set_move(Move pMove)
{ 
	mMove = pMove; 
}

void line_element::set_key(U64 pKey)
{ 
	mKey = pKey; 
}

}