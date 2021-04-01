#ifndef MOVEGEN_H
#define MOVEGEN_H

#include <cstddef>

#include "types.h"
#include "position.h"

struct MoveExt
{
    MoveExt() {}
    MoveExt(Move move, int score) { this->move = move; this->score = score; }
    Move move = 0;
    int score = 0;
};

//Returns the pointer to the last move in the list
MoveExt *generate_moves(Position *pos, MoveExt *moveList, bool only_captures);

struct MoveList{
    MoveList(Position *pos, bool only_captures) {
        MoveExt *last = generate_moves(pos, this->moveList, only_captures); 
        this->size = last - this->moveList; 
    }
    int size;
    MoveExt moveList[256];
};

#endif //!MOVEGEN_H