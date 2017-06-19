#ifndef TRANSPOSITIONTABLE_H
#define TRANSPOSITIONTABLE_H

#include "bitboard.h"
#include <cstring>
#include <vector>

const int Default_size = 1024;

struct TableEntry
{
	TableEntry() : depth(0), key(0)
	{}
	TableEntry(U64 k, Move b, NodeType t, int d, int s)
	  : key(k), best(b), type(t), depth(d), score(s)
	{}
	U64 key;
	Move best;
	NodeType type;
	int depth;	
	int score;
};

class TranspositionTable
{
public:
	TranspositionTable()
	{
		table.reserve(Default_size);
	}
	~TranspositionTable() {}
	TableEntry& get(U64 key)
	{
		return table[key % table.capacity()];
	}
	void clear()
	{
		table.clear();
	}
	void resize(int size_mb)
	{
		table.clear();
		table.reserve(size_mb * 1000 * 1000 / sizeof(TableEntry));
	}
	void store(U64 key, Move best, NodeType type, int depth, int score)
	{
		TableEntry& at = get(key);
		if (at.depth <= depth)
		{
			at.key   = key;
			at.best  = best;
			at.type  = type;
		    at.depth = depth;
		    at.score = score;
		}
	}
private:
	std::vector<TableEntry> table;
};

extern TranspositionTable ttable;

#endif