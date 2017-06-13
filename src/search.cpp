#include "search.h"

int search_nodes = 0;

int negamax(State & s, int d, int a, int b)
{
	search_nodes += 1;
	if (d == 0)
		return evaluate(s);

	MoveList mlist;
	push_moves(s, &mlist);

	if (mlist.size() == 0)
		return s.check() ? Checkmate : Stalemate;

	int best = Neg_inf;
	mlist.sort();
	State c;

	while (mlist.size() > 0)
	{
		std::memmove(&c, &s, sizeof s);
		c.make(mlist.pop());
		best = std::max(best, -negamax(c, d - 1, -b, -a));
		a = std::max(a, best);
		if (a >= b)
			break;
	}

	return best;
}

Move search(State & s)
{
	const int d = 6; // Depth to search. Will adjust this later.
	int a = Neg_inf;

	MoveList mlist;
	push_moves(s, &mlist);

	State c;
	Move m;
	std::vector<Candidate> candidates;
	// Need to check and return a null move if it's checkmate/stalemate.
	while (mlist.size() > 0)
	{
		std::memmove(&c, &s, sizeof s);
		m = mlist.pop();
		c.make(m);
		candidates.push_back(Candidate(m, negamax(c, d - 1, Neg_inf, -a)));
		a = std::max(a, candidates.back().score);
	}
	std::stable_sort(candidates.begin(), candidates.end());
	std::cout << search_nodes << '\n';
	return candidates.front().move;
}