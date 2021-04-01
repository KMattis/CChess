#include "types.h"
#include "tt.h"
#include "movegen.h"
#include "search.h"

enum Stage
{
    TT_MOVE, KILLER_MOVE_0, KILLER_MOVE_1, MOVE_LIST_INIT, MOVE_LIST
};

class MovePicker
{
public:
    MovePicker(Position *pos, TranspositionTableEntry *tte, Move killer0, Move killer1, SearchResult *res, bool only_captures);
    ~MovePicker();
    Move next_move();
    int legal_moves() {return this->num_legal_moves;}
    void reset() {stage = TT_MOVE; num_legal_moves = 0;}
private:
    void pick_next_move();
    TranspositionTableEntry *tte;
    SearchResult *res;
    Position *pos;
    Stage stage;
    Move killer0;
    Move killer1;
    MoveList *list = nullptr;
    int move_num = 0;
    int num_legal_moves = 0;
    bool only_captures;
};

