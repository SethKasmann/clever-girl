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
	const TableEntry* probe(U64 key) const
	{
		return table.data() + key % table.capacity();
	}
	TableEntry& operator[](U64 key)
	{
		return table[key % table.capacity()];
	}
	void clear()
	{
		table.clear();
	}
	int size() const
	{
		return table.size();
	}
	void resize(int size_mb)
	{
		table.clear();
		table.resize(size_mb * 1000 * 1000 / sizeof(TableEntry));
	}
	void store(U64 key, Move best, NodeType type, int depth, int score)
	{
		int i = key % table.capacity();
		if (table[i].depth < depth)
		{
			table[i].key   = key;
			table[i].best  = best;
			table[i].type  = type;
			table[i].depth = depth;
			table[i].score = score;
		}
	}
private:
	std::vector<TableEntry> table;
};

extern TranspositionTable ttable;

#endif