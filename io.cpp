
#include <stdio.h>
#include <string.h>

#include "io.h"

namespace io 
{
    char *move_to_string(Move move) {
        static char mv_str[6]; // = (char *) malloc(6);
        char pchar;

        Square from = from_square(move);
        Square to = to_square(move);
        int ff = from % 8;
        int rf = from / 8;
        int ft = to % 8;
        int rt = to / 8;


        if(is_promotion(move)) {
           int promoted = promoted_piece(move);
            if(promoted == QUEEN) pchar = 'q';
            if(promoted == ROOK) pchar = 'r';
            if(promoted == BISHOP) pchar = 'b';
            if(promoted == KNIGHT) pchar = 'n';
            sprintf(mv_str, "%c%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt), pchar);
        } else {
            sprintf(mv_str, "%c%c%c%c", ('a' + ff), ('1' + rf), ('a' + ft), ('1' + rt));
        }

        return mv_str;
    }
}