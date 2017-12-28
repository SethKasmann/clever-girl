#ifndef TRANSPOSITIONTABLE_H
#define TRANSPOSITIONTABLE_H

#include "bitboard.h"
#include "move.h"
#include <cstring>
#include <vector>

const int Default_size = 1024;

struct TableEntry
{
    U64 key;
    Move best;
    NodeType type;
    int depth;  
    int score;
    bool ancient;
};

class TranspositionTable
{
public:
    TranspositionTable()
    {
        table.resize(Default_size);
    }
    ~TranspositionTable() {}
    const TableEntry* probe(U64 key) const
    {
        return table.data() + key % table.size();
    }
    TableEntry& operator[](U64 key)
    {
        return table[key % table.size()];
    }
    void clear()
    {
        for (TableEntry& tableEntry : table)
        {
            tableEntry.key     = 0;
            tableEntry.best    = nullMove;
            tableEntry.type    = pv;
            tableEntry.depth   = 0;
            tableEntry.score   = 0;
            tableEntry.ancient = true;
        }
    }
    void setAncient()
    {
        for (TableEntry& tableEntry : table)
            tableEntry.ancient = true;
    }
    std::size_t size() const
    {
        return table.size();
    }
    void resize(int size_mb)
    {
        table.clear();
        table.resize(size_mb * 1000 * 1000 / sizeof(TableEntry));
        clear();
    }
    void store(U64 key, Move best, NodeType type, int depth, int score)
    {
        int i = key % table.size();
        if (table[i].depth < depth ||
            table[i].ancient)
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