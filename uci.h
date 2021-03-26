#ifndef UCI_H
#define UCI_H

#include "types.h"
#include "search.h"

namespace uci 
{
    void send_best_move(Move move);
    
    void send_depth_info(SearchResult *res, int nps);

    void send_move_info(int move_num, Move move, Depth depth);

    void send_hashtable_info(float percentage);

    void init();

    void loop();
}


#endif