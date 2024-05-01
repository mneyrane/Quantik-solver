#include <stdio.h>
#include <assert.h>
#include "quantik.h"

void test_QT_initialize_state();
void test_QT_check_valid_move();
void test_QT_forward_step();
void test_QT_backward_step();
void test_QT_check_completion_win();

void test___QT_decode_action();
void test___QT_get_opp_player();
void test___QT_encode_tile();
void test___QT_decode_player_from_tile();
void test___QT_decode_shape_from_tile();
void test___QT_check_tiles_for_win();

uint8_t compare_states(QT_State *s1, QT_State *s2);



int main() {
    test_QT_initialize_state();
    test_QT_check_valid_move();
    test_QT_forward_step();
    test_QT_backward_step();
    test_QT_check_completion_win();

    test___QT_decode_action();
    test___QT_get_opp_player();
    test___QT_encode_tile();
    test___QT_decode_player_from_tile();
    test___QT_decode_shape_from_tile();
    test___QT_check_tiles_for_win();
    
    printf("Tests succeeded!\n");
    return 0;
}



void test_QT_initialize_state() {
    QT_State s;

    QT_initialize_state(&s);

    assert(s.player == 0);

    for (uint8_t i = 0; i < QT_N_ROWS; i++) {
        for (uint8_t j = 0; j < QT_N_COLS; j++) {
            assert(s.board[i][j] == 0);
        }
    }

    for (uint8_t k = 0; k < QT_N_CLR_SHAPES; k++) {
        assert(s.shapes[k] == 2);
    }
}

void test_QT_check_valid_move() {
    uint8_t is_valid;
    QT_Action a;
    QT_State s = {
        0,
        {{   0,   0,   0,16+3},
         {   0,   0,   2,   0},
         {16+1,   0,   4,   0},
         {   0,   4,   0,16+2}},
        {2,1,2,0,
         1,1,1,2},
        0
    };

    uint8_t valid_moves[QT_N_ACTIONS] = {
        0,1,1,0, 
        0,1,0,1, 
        0,0,0,0, 
        0,0,1,0,

        1,1,1,0,
        1,1,0,0,
        0,1,0,0,
        0,0,0,0,

        0,0,0,0,
        1,1,0,0,
        0,1,0,0,
        1,0,1,0,

        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0};

    for (uint8_t n = 0; n < QT_N_ACTIONS; n++) {
        a = __QT_decode_action(n);
        is_valid = QT_check_valid_move(&s,a);
        assert((is_valid && valid_moves[n]) || !(is_valid || valid_moves[n]));
    }
}

void test_QT_forward_step() {
    QT_State s, target;
    QT_Action a;

    QT_initialize_state(&s);

    // move 1
    a = (QT_Action) {.x = 1, .y = 2, .shape = 2};
    target = (QT_State) {
        1,
        {{0,0,0,0},
         {0,0,2,0},
         {0,0,0,0},
         {0,0,0,0}},
        {2,1,2,2, 
         2,2,2,2},
        0
    };

    QT_forward_step(&s,a);
    assert(compare_states(&s, &target));

    // move 2
    a = (QT_Action) {.x = 2, .y = 0, .shape = 1};
    target = (QT_State) {
        0,
        {{0,0,0,0},
         {0,0,2,0},
         {16+1,0,0,0},
         {0,0,0,0}},
        {2,1,2,2, 
         1,2,2,2},
        0
    };

    QT_forward_step(&s,a);
    assert(compare_states(&s, &target));
}

void test_QT_backward_step() {
    QT_Action a;
    QT_State target;
    QT_State s = {
        0,
        {{0,0,0,0},
         {0,0,0,3},
         {0,0,4,16+1},
         {0,0,0,16+2}},
        {2,2,1,1, 
         1,1,2,2},
        0
    };

    // undo a move
    a = (QT_Action) {.x = 2, .y = 3, .shape = 1};
    target = (QT_State) {
        1,
        {{0,0,0,0},
         {0,0,0,3},
         {0,0,4,0},
         {0,0,0,16+2}},
        {2,2,1,1, 
         2,1,2,2},
        0
    };

    QT_backward_step(&s,a);
    assert(compare_states(&s, &target));

    // undo a second move
    a = (QT_Action) {.x = 1, .y = 3, .shape = 3};
    target = (QT_State) {
        0,
        {{0,0,0,0},
         {0,0,0,0},
         {0,0,4,0},
         {0,0,0,16+2}},
        {2,2,2,1, 
         2,1,2,2},
        0
    };

    QT_backward_step(&s,a);
    assert(compare_states(&s, &target));
}

void test_QT_check_completion_win() {
    QT_State s;
    QT_Action a;

    // check row completion
    s = (QT_State) {
        0,
        {{0,0,0,0},
         {2,16+3,16+4,1},
         {0,0,0,0},
         {0,0,0,0}},
        {1,1,2,2,
         2,2,1,1},
        0
    };

    a = (QT_Action) {.x = 1, .y = 2, .shape = 4};
    assert(QT_check_completion_win(&s, a));

    // check column completion
    s = (QT_State) {
        0,
        {{0,0,0,1},
         {0,0,0,4},
         {0,0,0,16+3},
         {0,0,0,16+2}},
        {1,2,2,1,
         2,1,1,2},
        0
    };

    a = (QT_Action) {.x = 2, .y = 3, .shape = 3};
    assert(QT_check_completion_win(&s, a));

    // check quadrant completions
    // top left
    s = (QT_State) {
        0,
        {{1,16+2,0,0},
         {3,16+4,0,0},
         {0,0,0,0},
         {0,0,0,0}},
        {1,2,1,2,
         2,1,2,1},
        0
    };

    a = (QT_Action) {.x = 0, .y = 1, .shape = 2};
    assert(QT_check_completion_win(&s, a));

    // top right
    s = (QT_State) {
        0,
        {{0,0,1,16+2},
         {0,0,3,16+4},
         {0,0,0,0},
         {0,0,0,0}},
        {1,2,1,2,
         2,1,2,1},
        0
    };

    a = (QT_Action) {.x = 0, .y = 3, .shape = 2};
    assert(QT_check_completion_win(&s, a));

    // bottom left
    s = (QT_State) {
        0,
        {{0,0,0,0},
         {0,0,0,0},
         {1,16+2,0,0},
         {3,16+4,0,0}},
        {1,2,1,2,
         2,1,2,1},
        0
    };

    a = (QT_Action) {.x = 2, .y = 1, .shape = 2};
    assert(QT_check_completion_win(&s, a));

    // bottom right
    s = (QT_State) {
        0,
        {{0,0,0,0},
         {0,0,0,0},
         {0,0,1,16+2},
         {0,0,3,16+4}},
        {1,2,1,2,
         2,1,2,1},
        0
    };

    a = (QT_Action) {.x = 2, .y = 3, .shape = 2};
    assert(QT_check_completion_win(&s, a));
}



void test___QT_decode_action() {
    QT_Action a;

    a = __QT_decode_action(0);
    assert(a.x == 0 && a.y == 0 && a.shape == 1);

    a = __QT_decode_action(1);
    assert(a.x == 0 && a.y == 1 && a.shape == 1);

    a = __QT_decode_action(4);
    assert(a.x == 1 && a.y == 0 && a.shape == 1);

    a = __QT_decode_action(16);
    assert(a.x == 0 && a.y == 0 && a.shape == 2);

    a = __QT_decode_action(17);
    assert(a.x == 0 && a.y == 1 && a.shape == 2);

    a = __QT_decode_action(20);
    assert(a.x == 1 && a.y == 0 && a.shape == 2);

    a = __QT_decode_action(41);
    assert(a.x == 2 && a.y == 1 && a.shape == 3);

    a = __QT_decode_action(62);
    assert(a.x == 3 && a.y == 2 && a.shape == 4);
}

void test___QT_get_opp_player() {
    assert(__QT_get_opp_player(0) == 1);
    assert(__QT_get_opp_player(1) == 0);
}

// tile encoding and decoding:
//
// 4 LSBs is a shape or empty (0 to 4)
// 4 MSBs is a player (0 or 1)

void test___QT_encode_tile() {
    uint8_t player, shape;

    player = 0;
    shape = 2;

    assert(__QT_encode_tile(player, shape) == 2) ;

    player = 1;
    shape = 4;

    assert(__QT_encode_tile(player, shape) == 20);
}

void test___QT_decode_player_from_tile() {
    uint8_t tile;

    tile = 2;

    assert(__QT_decode_player_from_tile(tile) == 0);

    tile = 20;

    assert(__QT_decode_player_from_tile(tile) == 1);
}

void test___QT_decode_shape_from_tile() {
    uint8_t tile;

    tile = 2;

    assert(__QT_decode_shape_from_tile(tile) == 2);

    tile = 20;

    assert(__QT_decode_shape_from_tile(tile) == 4);
}

void test___QT_check_tiles_for_win() {
    // true cases
    assert(__QT_check_tiles_for_win(   1,   2,   3,   4));
    assert(__QT_check_tiles_for_win(   2,   4,   1,   3));
    assert(__QT_check_tiles_for_win(16+1,16+2,   3,   4));
    assert(__QT_check_tiles_for_win(   1,16+2,16+4,16+3));
    assert(__QT_check_tiles_for_win(16+2,   1,   3,   4));
    assert(__QT_check_tiles_for_win(16+4,16+3,16+2,16+1));

    // false cases
    assert(!__QT_check_tiles_for_win(   0,   0,   0,   0));
    assert(!__QT_check_tiles_for_win(   0,   3,   2,   4));
    assert(!__QT_check_tiles_for_win(   1,   1,   2,   3));
    assert(!__QT_check_tiles_for_win(16+1,   1,   2,   4));
    assert(!__QT_check_tiles_for_win(16+4,16+3,16+2,   0));
    assert(!__QT_check_tiles_for_win(   2,16+2,   4,16+4));
}

uint8_t compare_states(QT_State *s1, QT_State *s2) {
    if (s1->player != s2->player) {
        return 0;
    }

    for (uint8_t i = 0; i < QT_N_ROWS; i++) {
        for (uint8_t j = 0; j < QT_N_COLS; j++) {
            if (s1->board[i][j] != s2->board[i][j]) {
                return 0;
            }
        }
    }

    for (uint8_t k = 0; k < QT_N_CLR_SHAPES; k++) {
        if (s1->shapes[k] != s2->shapes[k]) {
            return 0;
        }
    }

    return 1;
}
