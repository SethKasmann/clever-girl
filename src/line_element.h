#ifndef LINE_ELEMENT_H
#define LINE_ELEMENT_H

#include "types.h"

namespace cgirl {

class line_element
{
public:
	line_element();
	line_element(Move pMove, U64 pKey);
	line_element(const line_element& pLineElement);
	line_element(line_element && pLineElement);
	void operator=(const line_element& pLineElement);
	Move get_move() const;
	U64 get_key() const;
	void set_move(Move pMove);
	void set_key(U64 pKey);
private:
	Move mMove;
	U64 mKey;
};

inline std::ostream& operator<<(std::ostream& os, const line_element& pLineElement)
{
	os << to_string(pLineElement.get_move());
	return os;
}

}

#endif