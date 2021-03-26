#ifndef SEARCH_H
#define SEARCH_H

#include "position.h"

const int INFINITY = 30000;
const int CHECKMATE = 29000;
const int DRAW = 0;

const int MAX_PLY = 1024;
const int MAX_PV_LENGTH = 32;

struct SearchResult
{
    long int fh;
    long int fhf;
    long int nodes;
    int score;
    Depth search_depth;
    int start_ply;
    int CutoffHistory[64][64];
    Move killers[2][MAX_PLY];
    Move pv[MAX_PV_LENGTH];
    int pv_length;
    bool aborted;
};

enum NodeType {PV = 0, Cut = 1, All=-1 };

void do_search(Depth min_depth, Depth max_depth, Position *pos, int timeleft);

//Alpha-beta search
template<NodeType T>
int search(Depth depth, int alpha, int beta, Position *pos, SearchResult *res);

//Quiescence search
int qsearch(int alpha, int beta, Position *pos, SearchResult *res);

#endif //!SEARCH_H