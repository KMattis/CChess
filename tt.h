#ifndef TT_H
#define TT_H

#include <cstdlib>
#include <stdio.h>

#include "types.h"
#include "position.h"
#include "bitboards.h"

//transposition table

//Returned by get_score if the lookup failed. This is not a valid evaluation or checkmate score
const int LOOKUP_FAILED = -10000000;

enum TranspositionTableEntryType
{
    Exact, LowerBound, UpperBound
};

struct TranspositionTableEntry
{
    Key key;
    TranspositionTableEntryType type;
    int score;
    Move pv_move;
    Depth depth;
    int insertion_ply;

    int get_score(int alpha, int beta, Depth depth);
};

struct TranspositionTable
{
    TranspositionTable(size_t num_entries)
    {
        if(popcount(num_entries) != 1)
        {
            printf("num_entries must be a power of 2\n");
            throw("num_entries must be a power of 2\n");
        }
        this->used_entries = 0;
        this->num_entries = num_entries;
        ASSERT(this->num_entries > 0);
        this->data = (TranspositionTableEntry *)calloc(this->num_entries, sizeof(TranspositionTableEntry));
        ASSERT(this->data != nullptr);
        printf("initialized table with %llu entries\n", this->num_entries);
    }
    ~TranspositionTable()
    {
        free(this->data);
    }
    TranspositionTableEntry *get_entry(Key key);
    void store(Key key, TranspositionTableEntryType type, int score, Move pv_move, Depth depth, int insertion_ply);
    int find_pv(Position *pos, Move *pv);
    float get_used_percentage() { return used_entries / (float)num_entries; }
    int get_used() {return used_entries; }
    int collissions = 0;
    int hits = 0;
private:
    TranspositionTableEntry *data;
    size_t num_entries;
    size_t used_entries;
};

#endif //!TT_H