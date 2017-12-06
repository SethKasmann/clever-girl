#ifndef LINE_ELEMENT_H
#define LINE_ELEMENT_H

#include "types.h"
#include "move.h"

namespace cgirl {

class line_element
{
public:
	line_element();
	line_element(Move_t pMove, U64 pKey);
	line_element(const line_element& pLineElement);
	line_element(line_element && pLineElement);
	void operator=(const line_element& pLineElement);
	Move_t get_move() const;
	U64 get_key() const;
	void set_move(Move_t pMove);
	void set_key(U64 pKey);
private:
	Move_t mMove;
	U64 mKey;
};

inline std::ostream& operator<<(std::ostream& os, const line_element& pLineElement)
{
	os << toString(pLineElement.get_move());
	return os;
}

}

#endif