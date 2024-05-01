/* Quantik game interface

An implementation of Quantik for fast minimax search.

Additional notes
================

Board representation
--------------------

Board tiles, occupied or not, are represented by a uint8_t. The four 
least significant bits define the shape (numbers 1 to 4). If this number
is zero, this means the board spot is empty. The four most significant 
bits define player ownership (0 or 1), i.e. colour.

Looping through actions
-----------------------

There are at most 64 actions for both players. To iterate through each
action with a single loop with uint8_t numbers 0 to 63, use the
__QT_decode_action() function.

*/
#ifndef _QUANTIK_H_
#define _QUANTIK_H_

#define QT_N_ROWS          4 // board rows
#define QT_N_COLS          4 // board columns
#define QT_B_SIZE         16 // board size
#define QT_N_CLR_SHAPES    8 // number of coloured shapes
#define QT_N_SHAPES        4 // number of shapes
#define QT_N_ACTIONS      64 // number of actions
#define QT_N_SHAPE_REPS    2 // number of each coloured shape

#include <stdint.h>

// structs
typedef struct QT_State {
    uint8_t player;
    uint8_t board[QT_N_ROWS][QT_N_COLS];
    uint8_t shapes[QT_N_CLR_SHAPES];
    uint8_t depth;
} QT_State;

typedef struct QT_Action {
    uint8_t x;
    uint8_t y;
    uint8_t shape;
} QT_Action;

// game functions
extern void QT_initialize_state(QT_State *s);
extern uint8_t QT_check_valid_move(const QT_State *s, QT_Action a);
extern void QT_forward_step(QT_State *s, QT_Action a);
extern void QT_backward_step(QT_State *s, QT_Action a);
extern uint8_t QT_check_completion_win(const QT_State *s, QT_Action a);

// other
extern uint8_t QT_minimax(QT_State *s, uint8_t player);
extern void QT_print_state(const QT_State *s);

// helper functions
extern QT_Action __QT_decode_action(uint8_t e);
extern uint8_t __QT_encode_action(QT_Action a);
extern uint8_t __QT_get_opp_player(uint8_t player);
extern uint8_t __QT_encode_tile(uint8_t player, uint8_t shape);
extern uint8_t __QT_decode_player_from_tile(uint8_t tile);
extern uint8_t __QT_decode_shape_from_tile(uint8_t tile);
extern uint8_t __QT_check_tiles_for_win(uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4);

#endif
