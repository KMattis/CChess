#include <stdio.h> //For ASSERT
#include "position.h"
#include "bitboards.h"
#include "zobrist.h"
#include "material.h"

const char *SQUARE_NAMES_[64] = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8"
};

void print_move_(Move move)
{
    Square from = from_square(move);
    Square to = to_square(move);
    const char *promotions = "nbrq";
    char promotion_char = is_promotion(move) ? promotions[promoted_piece(move) - KNIGHT] : ' ';
    printf("%s%s%c", SQUARE_NAMES_[from], SQUARE_NAMES_[to], promotion_char);
}

bool Position::is_pseudo_legal(Move move)
{
    Square from = from_square(move);
    Square to = to_square(move);
    Piece moved = this->board[from];

    if(color_of(moved) != color_to_move) return false;

    //The moved piece must be the one from the move
    if(moved != moved_piece(move)) return false;
    if(captured_piece(move) != this->board[to]) return false; //This also checks that no piece of us blocks the target square

    if(moved == WHITE_KING || moved == BLACK_KING)
    {
        //Target square is not attacked. We need not check if the king move is pseudolegal, this is guaranteeed by move generation, and does not depend on the position
        return !get_square(this->current_state->attack_bitboards[~this->color_to_move], to);
    }

    if(this->current_state->in_double_check) return false;
    if(!get_square(this->current_state->checker_bitboard, to)) return false;

    if(moved == WHITE_BISHOP || moved == BLACK_BISHOP || moved == WHITE_ROOK || moved == BLACK_ROOK || moved == WHITE_QUEEN || moved == BLACK_QUEEN)
    {
        return (ray_bitboards[from][to] & this->current_state->blocker_bitboard & ~(1ull << to)) == 0;
    }
    else if(moved == WHITE_PAWN || moved == BLACK_PAWN)
    {
        if(is_en_passent(move) && this->current_state->en_passent != to) return false;
        //No capture pawn moves
        if(is_double_pawn(move))
        {   
            if(color_to_move == white && get_square(current_state->blocker_bitboard, from + N)) return false;
            if(color_to_move == black && get_square(current_state->blocker_bitboard, from - N)) return false;
        }
    }
    else if (moved == WHITE_KNIGHT || moved == BLACK_KNIGHT)
    {
        if(get_square(current_state->pinner_bitboard, from)) return false;
    }

    return true;
}

bool Position::is_legal(Move move)
{
    Square start = from_square(move);
    Square target = to_square(move);

    ASSERT(a1 <= start && start <= h8);
    ASSERT(a1 <= target && target <= h8);

    if(is_casteling(move))
    {
        if(this->current_state->in_check) return false;
        if(target == g1){
            if (!(this->current_state->casteling_rights & WHITE_KINGSIDE_CASTELING)) return false;
            Bitboard f1_g1 = (1ull << f1) | (1ull << g1);
            return (f1_g1 & this->current_state->attack_bitboards[~this->color_to_move]) == 0 && (f1_g1 & this->current_state->blocker_bitboard) == 0;
        }
        else if (target == c1)
        {
            if (!(this->current_state->casteling_rights & WHITE_QUEENSIDE_CASTELING)) return false;
            Bitboard c1_d1 = (1ull << c1) | (1ull << d1);
            Bitboard b1_c1_d1 = (1ull << b1) | c1_d1;
            return (c1_d1 & this->current_state->attack_bitboards[~this->color_to_move]) == 0 && (b1_c1_d1 & this->current_state->blocker_bitboard) == 0;
        }
        else if(target == g8)
        {
            if (!(this->current_state->casteling_rights & BLACK_KINGSIDE_CASTELING)) return false;
            Bitboard f8_g8 = (1ull << f8) | (1ull << g8);
            return (f8_g8 & this->current_state->attack_bitboards[~this->color_to_move]) == 0 && (f8_g8 & this->current_state->blocker_bitboard) == 0;
        }
        else if(target == c8)
        {
            if (!(this->current_state->casteling_rights & BLACK_QUEENSIDE_CASTELING)) return false;
            Bitboard c8_d8 = (1ull << c8) | (1ull << d8);
            Bitboard b8_c8_d8 = (1ull << b8) | c8_d8;
            return (c8_d8 & this->current_state->attack_bitboards[~this->color_to_move]) == 0 && (b8_c8_d8 & this->current_state->blocker_bitboard) == 0;
            
        }
        else 
           return false;
    }
    
    if(get_square(this->current_state->pinner_bitboard, start))
    {
        //We are not a king or a knight (this is already checked in generate)
        //We need to check that we are moving along the ray to the king
        //So we get the ray bitboard from the king square through our starting  square and intersect it with the target
        Square king_square = lsb(this->piece_bitboard[make_piece(KING, this->color_to_move)]);
        Bitboard ray = extended_ray_bitboards[king_square][start];
        return get_square(ray, target);
    }
    else
    {
        //Non-pinned pieces might move as they wish (we already checked, that the moves evade checks)
        return true;
    }
}

inline void remove_piece(Position *pos, Square square, Key *position_key, unsigned int *material_key)
{
    ASSERT(square != NO_SQUARE);

    Piece piece = pos->board[square];
    pos->board[square] = NO_PIECE;
    pop_square(pos->color_bitboard[color_of(piece)], square);
    pop_square(pos->piece_bitboard[piece], square);
    zobrist::change_piece(position_key, piece, square);
    material::material_key_remove_piece(material_key, piece);
    pos->material[piece]--;
}

inline void add_piece(Position *pos, Square square, Piece piece, Key *position_key, unsigned int *material_key)
{
    ASSERT(square != NO_SQUARE);
    ASSERT(pos->board[square] == NO_PIECE)

    pos->board[square] = piece;
    set_square(pos->color_bitboard[color_of(piece)], square);
    set_square(pos->piece_bitboard[piece], square);
    zobrist::change_piece(position_key, piece, square);
    material::material_key_add_piece(material_key, piece);
    pos->material[piece]++;
}

void create_attack_bitboard(Position *pos, Color color)
{
    Piece opponent_king = make_piece(KING, ~color);
    Square opponent_king_square = lsb(pos->piece_bitboard[opponent_king]);
    
    Piece our_king = make_piece(KING, color);
    Square our_king_square = lsb(pos->piece_bitboard[our_king]);

    Bitboard blockers_without_king = pos->current_state->blocker_bitboard & ~(1ull << opponent_king_square);

    Bitboard knights = pos->piece_bitboard[make_piece(KNIGHT, color)];
    Bitboard bishops = pos->piece_bitboard[make_piece(BISHOP, color)]
                     | pos->piece_bitboard[make_piece( QUEEN, color)];
    Bitboard rooks   = pos->piece_bitboard[make_piece(  ROOK, color)]
                     | pos->piece_bitboard[make_piece( QUEEN, color)];
    Bitboard pawns   = pos->piece_bitboard[make_piece(  PAWN, color)];

    pos->current_state->attack_bitboards[color] = 0ull;
    while(knights)
    {
        Square knight_square = pop_lsb(&knights);
        pos->current_state->attack_bitboards[color] |= knight_attack_bb(knight_square);
    }

    while(bishops)
    {
        Square bishop_square = pop_lsb(&bishops);
        pos->current_state->attack_bitboards[color] |= bishop_attack_bb(bishop_square, blockers_without_king);
    }

    while(rooks)
    {
        Square rook_square = pop_lsb(&rooks);
        pos->current_state->attack_bitboards[color] |= rook_attack_bb(rook_square, blockers_without_king);
    }

    pos->current_state->pawn_attack_bitboards[color] = color == white ? shift<NE>(pawns) | shift<NW>(pawns) : shift<SE>(pawns) | shift<SW>(pawns);
    pos->current_state->attack_bitboards[color] |= pos->current_state->pawn_attack_bitboards[color];

    pos->current_state->attack_bitboards[color] |= king_attack_bb(our_king_square);
}

void Position::compute_bitboards()
{
    Color them = ~this->color_to_move;
    Piece our_king = make_piece(KING, this->color_to_move);
    Square our_king_square = lsb(this->piece_bitboard[our_king]);

    this->current_state->in_check = 0;
    this->current_state->in_double_check = 0;
    this->current_state->checker_bitboard = 0ull;
    this->current_state->pinner_bitboard = 0ull;
    this->current_state->free = ~(this->color_bitboard[white] | this->color_bitboard[black]);
    this->current_state->blocker_bitboard = ~this->current_state->free;

    create_attack_bitboard(this, white);
    create_attack_bitboard(this, black);

    if(this->current_state->attack_bitboards[them] & this->piece_bitboard[our_king]){
        //We are in check, calculate checkers
        
        Bitboard knight_checks = knight_attack_bb(our_king_square) & this->piece_bitboard[make_piece(KNIGHT, them)];
        if(knight_checks)
        {
            this->current_state->in_check = 1;
            this->current_state->checker_bitboard |= knight_checks;
        }

        Bitboard pawn_checks = pawn_attack_bb(this->color_to_move, our_king_square) & this->piece_bitboard[make_piece(PAWN, them)];
        if(pawn_checks)
        {
            this->current_state->in_double_check = this->current_state->in_check;
            this->current_state->in_check = 1;
            this->current_state->checker_bitboard |= pawn_checks;
        }

        if(this->current_state->in_double_check)
            return; //We need no pinners or checkers in double check mode
    }
    else 
    {
        //No checks means every move avoids check
        this->current_state->checker_bitboard = ~0ull;
    }

    
    //Compute ray checks and pins
    Bitboard king_ray_diag_bitboard = bishop_attack_bb(our_king_square, this->color_bitboard[them]);
    Bitboard possible_diag_king_attackers = king_ray_diag_bitboard & this->color_bitboard[them];

    while(possible_diag_king_attackers)
    {
        Square attacker_square = pop_lsb(&possible_diag_king_attackers);
        Piece attacker = this->board[attacker_square];
        if(attacker == make_piece(BISHOP, them) || attacker == make_piece(QUEEN, them))
        {
            int num_pinned = popcount(ray_bitboards[our_king_square][attacker_square] & this->color_bitboard[this->color_to_move]);
            if(num_pinned == 1)
            {
                //Pin this ray if there is exactly one of our pieces, and the enemy attacks along it
                this->current_state->pinner_bitboard |= ray_bitboards[our_king_square][attacker_square];
            }
            else if(num_pinned == 0)
            {
                //This is a check
                this->current_state->in_double_check = this->current_state->in_check;
                this->current_state->in_check = 1;
                this->current_state->checker_bitboard |= ray_bitboards[our_king_square][attacker_square];
                if(this->current_state->in_double_check)
                    return;
            }
        }

    }

    Bitboard king_ray_ortho_bitboard = rook_attack_bb(our_king_square, this->color_bitboard[them]);
    Bitboard possible_ortho_king_attackers = king_ray_ortho_bitboard & this->color_bitboard[them];

    while(possible_ortho_king_attackers)
    {
        Square attacker_square = pop_lsb(&possible_ortho_king_attackers);
        Piece attacker = this->board[attacker_square];
        if(attacker == make_piece(ROOK, them) || attacker == make_piece(QUEEN, them))
        {   
            int num_pinned = popcount(ray_bitboards[our_king_square][attacker_square] & this->color_bitboard[this->color_to_move]);
            if(num_pinned == 1)
            {
                //Pin this ray if there is exactly one of our pieces, and the enemy attacks along it
                this->current_state->pinner_bitboard |= ray_bitboards[our_king_square][attacker_square];
            }
            else if(num_pinned == 0)
            {
                //This is a check
                this->current_state->in_double_check = this->current_state->in_check;
                this->current_state->in_check = 1;
                this->current_state->checker_bitboard |= ray_bitboards[our_king_square][attacker_square];
                if(this->current_state->in_double_check)
                    return;
            }
        }
    }
}

void Position::do_move(Move move)
{
    Square to = to_square(move);
    Square from = from_square(move);
    Piece moved = moved_piece(move);
    Piece captured = captured_piece(move);

    ASSERT(from != NO_SQUARE);
    ASSERT(to != NO_SQUARE);
    ASSERT(moved != NO_PIECE);
    //ASSERT(captured != BLACK_KING);
    //ASSERT(captured != WHITE_KING);

    State *state = new State();
    state->last_state = this->current_state;
    state->move = move;
    state->ply = this->current_state->ply + 1;
    state->casteling_rights = this->current_state->casteling_rights;
    state->position_key = this->current_state->position_key;
    state->material_key = this->current_state->material_key;
    state->is_standard_material_config = this->current_state->is_standard_material_config;

    zobrist::change_casteling(&state->position_key, this->current_state->casteling_rights);
    if(this->current_state->en_passent != NO_SQUARE)
        zobrist::change_en_passent(&state->position_key, this->current_state->en_passent);

    if(captured || piece_type_of(moved) == PAWN) 
    {
        //We need to reset the 50 moves counter, because the move was a capture or a pawn move
        state->fifty_moves = 0;
    }
    else
    {
        //If the move is not a capture or a pawn move, increase the 50 moves counter
        state->fifty_moves = this->current_state->fifty_moves + 1;
    }

    //Remove piece from starting square
    remove_piece(this, from, &state->position_key, &state->material_key);
    
    if(captured)
    {
        //If this is a capture, remove the captured piece
        remove_piece(this, to, &state->position_key, &state->material_key);
    }
    
    if(is_promotion(move))
    {
        //Add promoted piece to target
        PieceType promoted = promoted_piece(move);
        Piece promoted_piece = make_piece(promoted, this->color_to_move);
        add_piece(this, to, promoted_piece, &state->position_key, &state->material_key);

        //This can lead to a non-standard material config
        if((promoted == QUEEN && material[promoted_piece] >= 2) || material[promoted_piece] >= 3)
            state->is_standard_material_config = false;
    }
    else
    {
        //Add moved piece to target
        add_piece(this, to, moved, &state->position_key, &state->material_key);
    }

    if(is_double_pawn(move))
    {
        //We need to set the en_passent square to the square behind the pawn
        state->en_passent = this->color_to_move == white ? from + N : from - N;
        zobrist::change_en_passent(&state->position_key, state->en_passent);
    }
    else
    {
        //No en_passent possible next move
        state->en_passent = NO_SQUARE;
    }
    
    if(is_en_passent(move))
    {
        //Remove the pawn from the square in front of the en_passent square
        Square capture_square = (Square)(8 * (from / 8) + (to % 8)); //this->color_to_move == white ? to - N : to + N;
        ASSERT((3 <= capture_square / 8) && (capture_square / 8 <= 4));
        remove_piece(this, capture_square, &state->position_key, &state->material_key);
        en_passent_moves++;
    }

    //Revoke casteling rights
    if(piece_type_of(moved) == KING)
    {
        if(this->color_to_move == white)
            state->casteling_rights &= BLACK_CASTELING;
        else
            state->casteling_rights &= WHITE_CASTELING;
    }
    else if(piece_type_of(moved) == ROOK)
    {
        if(from == a1)
            state->casteling_rights &= ~WHITE_QUEENSIDE_CASTELING;
        else if(from == h1)
            state->casteling_rights &= ~WHITE_KINGSIDE_CASTELING;
        else if(from == a8)
            state->casteling_rights &= ~BLACK_QUEENSIDE_CASTELING;
        else if(from == h8)
            state->casteling_rights &= ~BLACK_KINGSIDE_CASTELING; 
    }

    if(piece_type_of(captured) == ROOK)
    {
        if(to == a1)
            state->casteling_rights &= ~WHITE_QUEENSIDE_CASTELING;
        else if(to == h1)
            state->casteling_rights &= ~WHITE_KINGSIDE_CASTELING;
        else if(to == a8)
            state->casteling_rights &= ~BLACK_QUEENSIDE_CASTELING;
        else if(to == h8)
            state->casteling_rights &= ~BLACK_KINGSIDE_CASTELING;
    }
    
    if(is_casteling(move))
    {
        Square rook_from = NO_SQUARE;
        Square rook_to = NO_SQUARE;
        switch(to)
        {
            case c1: rook_from = a1; rook_to = d1; break;
            case g1: rook_from = h1; rook_to = f1; break;
            case c8: rook_from = a8; rook_to = d8; break;
            case g8: rook_from = h8; rook_to = f8; break;
            default: rook_from = NO_SQUARE; rook_to = NO_SQUARE; break;
        }
        
        ASSERT(rook_from != NO_SQUARE);
        ASSERT(rook_to != NO_SQUARE);

        Piece rook = this->board[rook_from];

        ASSERT(piece_type_of(rook) == ROOK);

        remove_piece(this, rook_from, &state->position_key, &state->material_key);
        add_piece(this, rook_to, rook, &state->position_key, &state->material_key);
    }

    ASSERT(this->piece_bitboard[WHITE_KING]);
    ASSERT(this->piece_bitboard[BLACK_KING]);

    zobrist::change_casteling(&state->position_key, state->casteling_rights);
    zobrist::change_color_to_move(&state->position_key);
    this->color_to_move = ~this->color_to_move;
    this->current_state = state;

    //Setup attack/checker/blocker bitboards
    this->compute_bitboards();
}

void Position::undo_move()
{
    State *current_state = this->current_state;
    Move move = current_state->move;

    Square from = from_square(move);
    Square to = to_square(move);
    Piece moved = moved_piece(move);
    Piece captured = captured_piece(move);

    //Add moved piece to starting square
    add_piece(this, from, moved, &current_state->position_key, &current_state->material_key);

    //Remove the piece from the target square (this will also take care of promotions)
    remove_piece(this, to, &current_state->position_key, &current_state->material_key);

    if(captured)
    {
        //If this is a capture, add the captured piece to the target square
        add_piece(this, to, captured, &current_state->position_key, &current_state->material_key);
    }
    else if(is_en_passent(move))
    {
        //Add the pawn to the square in front of the en_passent square
        Square capture_square = this->color_to_move == white ? to + N : to - N;
        add_piece(this, capture_square, make_piece(PAWN, this->color_to_move), &current_state->position_key, &current_state->material_key);
    }
    else if(is_casteling(move))
    {
        Square rook_from = NO_SQUARE;
        Square rook_to = NO_SQUARE;
        switch(to)
        {
            case c1: rook_from = a1; rook_to = d1; break;
            case g1: rook_from = h1; rook_to = f1; break;
            case c8: rook_from = a8; rook_to = d8; break;
            case g8: rook_from = h8; rook_to = f8; break;
            default: rook_from = NO_SQUARE; rook_to = NO_SQUARE; break;
        }

        ASSERT(rook_from != NO_SQUARE);
        ASSERT(rook_to != NO_SQUARE);


        Piece rook = this->board[rook_to];
        remove_piece(this, rook_to, &current_state->position_key, &current_state->material_key);
        add_piece(this, rook_from, rook, &current_state->position_key, &current_state->material_key);
    }

    zobrist::change_color_to_move(&current_state->position_key);

    //Check that the position key is the same as before
    //Does not work right now as we do not care about e-paasent or casteling
    //ASSERT(current_state->position_key == current_state->last_state->position_key);

    this->color_to_move = ~this->color_to_move;
    this->current_state = current_state->last_state;
    delete current_state;
}

void Position::do_null_move()
{
    State *state = new State();
    state->last_state = this->current_state;
    state->fifty_moves = this->current_state->fifty_moves;
    state->ply = this->current_state->ply + 1;
    state->casteling_rights = this->current_state->casteling_rights;
    state->en_passent = NO_SQUARE;
    state->move = 0;
    state->position_key = this->current_state->position_key;
    state->material_key = this->current_state->material_key;
    state->is_standard_material_config = this->current_state->is_standard_material_config;
    zobrist::change_color_to_move(&state->position_key);
    if(this->current_state->en_passent != NO_SQUARE)
        zobrist::change_en_passent(&state->position_key, this->current_state->en_passent);

    this->color_to_move = ~this->color_to_move;
    this->current_state = state;

    //Setup attack/checker/blocker bitboards
    this->compute_bitboards();
}

void Position::undo_null_move()
{
    State *current_state = this->current_state;

    this->color_to_move = ~this->color_to_move;
    this->current_state = current_state->last_state;
    delete current_state;
}

void Position::init(string &fen)
{
    Key position_key = 0ull;
    unsigned int material_key = 0;
    char token;

    std::istringstream ss(fen);

    ss >> std::noskipws;

    int file = 0, rank = 7;
    while((ss >> token) && !isspace(token))
    {
        ASSERT(0 <= rank && rank <= 7);
        ASSERT(0 <= file && file <= 8);

        if(token == '/')
        {
            rank--;
            file = 0;
        }
        else if(isdigit(token))
            file += (token - '0'); 
        else 
        {
            Piece piece = NO_PIECE;
            switch(token)
            {
                case 'p': piece = BLACK_PAWN;   break; 
                case 'n': piece = BLACK_KNIGHT; break;
                case 'b': piece = BLACK_BISHOP; break;
                case 'r': piece = BLACK_ROOK;   break;
                case 'q': piece = BLACK_QUEEN;  break;
                case 'k': piece = BLACK_KING;   break;
                case 'P': piece = WHITE_PAWN;   break;
                case 'N': piece = WHITE_KNIGHT; break;
                case 'B': piece = WHITE_BISHOP; break;
                case 'R': piece = WHITE_ROOK;   break;
                case 'Q': piece = WHITE_QUEEN;  break;
                case 'K': piece = WHITE_KING;   break;
            }

            add_piece(this, (Square)(rank * 8 + file), piece, &position_key, &material_key);
            
            file++;
        }
    }
    
    ss >> token;
    this->color_to_move = token == 'w' ? white : black;
    ss >> token; //ws

    //Setup the initial state of the position
    this->current_state = new State();
    this->current_state->en_passent = NO_SQUARE;
    this->current_state->move = NO_MOVE;
    this->current_state->fifty_moves = 0;
    this->current_state->ply = 0;
    this->current_state->last_state = nullptr;
    this->current_state->position_key = position_key;
    this->current_state->material_key = material_key;
    this->current_state->is_standard_material_config = 
            material[WHITE_KNIGHT] <= 2
         && material[BLACK_KNIGHT] <= 2
         && material[WHITE_BISHOP] <= 2
         && material[BLACK_BISHOP] <= 2
         && material[WHITE_ROOK]   <= 2
         && material[BLACK_ROOK]   <= 2
         && material[WHITE_QUEEN]  <= 1
         && material[BLACK_QUEEN]  <= 1;

    ASSERT(position_key != 0ull);

    while((ss >> token) && !isspace(token))
    {
        switch(token)
        {
            case 'Q': this->current_state->casteling_rights |= WHITE_QUEENSIDE_CASTELING; break;
            case 'K': this->current_state->casteling_rights |= WHITE_KINGSIDE_CASTELING; break;
            case 'q': this->current_state->casteling_rights |= BLACK_QUEENSIDE_CASTELING; break;
            case 'k': this->current_state->casteling_rights |= BLACK_KINGSIDE_CASTELING; break;
        }
    }

    //TODO en_passent, ply and fullmove number.
    
    this->compute_bitboards();
}