#include <stdio.h>
#include "quantik.h"

// game functions

void QT_initialize_state(QT_State *s) {
    // Initialize the state with default values.
    //
    s->player = 0;
    
    // set board to zero
    for (uint8_t i = 0; i < QT_N_ROWS; i++) {
        for (uint8_t j = 0; j < QT_N_COLS; j++) {
            s->board[i][j] = 0;
        }
    }
    
    // set number of coloured shapes available to each player
    for (uint8_t i = 0; i < QT_N_CLR_SHAPES; i++) {
        s->shapes[i] = QT_N_SHAPE_REPS;
    }
}

uint8_t QT_check_valid_move(const QT_State *s, QT_Action a) {
    // Check if a move is legal to play.
    //
    uint8_t opp_tile = __QT_encode_tile(__QT_get_opp_player(s->player), a.shape);
    
    // check if shape is available 
    if (s->shapes[(s->player)*QT_N_SHAPES + a.shape - 1] == 0) {
        return 0;
    }

    // check if current position is occupied
    if (s->board[a.x][a.y] != 0) {
        return 0;
    }

    // check column placement rule
    for (uint8_t k = 0; k < QT_N_ROWS; k++) {
        if (s->board[k][a.y] == opp_tile) {
            return 0;
        }
    }

    // check row placement rule
    for (uint8_t k = 0; k < QT_N_COLS; k++) {
        if (s->board[a.x][k] == opp_tile) {
            return 0;
        }
    }

    // check quadrant placement rule
    if (a.x < 2 && a.y < 2) {
        if (s->board[0][0] == opp_tile || s->board[0][1] == opp_tile || 
            s->board[1][0] == opp_tile || s->board[1][1] == opp_tile) {
            return 0;
        }
    }
    else if (a.x < 2 && a.y > 1) {
        if (s->board[0][2] == opp_tile || s->board[0][3] == opp_tile || 
            s->board[1][2] == opp_tile || s->board[1][3] == opp_tile) {
            return 0;
        }
    }
    else if (a.x > 1 && a.y < 2) {
        if (s->board[2][0] == opp_tile || s->board[2][1] == opp_tile || 
            s->board[3][0] == opp_tile || s->board[3][1] == opp_tile) {
            return 0;
        }
    }
    else { // a.x > 1 && a.y > 1
        if (s->board[2][2] == opp_tile || s->board[2][3] == opp_tile || 
            s->board[3][2] == opp_tile || s->board[3][3] == opp_tile) {
            return 0;
        }
    }

    return 1;
}

void QT_forward_step(QT_State *s, QT_Action a) {
    // Play a move.
    //
    s->board[a.x][a.y] = __QT_encode_tile(s->player, a.shape);
    s->shapes[QT_N_SHAPES*(s->player) + a.shape - 1] -= 1;
    s->player = __QT_get_opp_player(s->player); 
}

void QT_backward_step(QT_State *s, QT_Action a) {
    // Undo a move.
    //
    s->player = __QT_get_opp_player(s->player);
    s->shapes[QT_N_SHAPES*(s->player) + a.shape - 1] += 1;
    s->board[a.x][a.y] = 0;
}

uint8_t QT_check_completion_win(const QT_State *s, QT_Action a) {
    // Checks for a region completion win subject to the previous move,
    // which should be tracked externally after the forward step. Losing
    // by having no legal moves available should be checked separately.
    //
    uint8_t result;

    // check row completion 
    result = __QT_check_tiles_for_win(
        s->board[a.x][0],
        s->board[a.x][1],
        s->board[a.x][2],
        s->board[a.x][3]);

    if (result) { return 1; }
    
    // check column completion
    result = __QT_check_tiles_for_win(
        s->board[0][a.y],
        s->board[1][a.y],
        s->board[2][a.y],
        s->board[3][a.y]);

    if (result) { return 1; }

    // check quadrant completion
    if (a.x < 2 && a.y < 2) {
        result = __QT_check_tiles_for_win(
            s->board[0][0], s->board[0][1],
            s->board[1][0], s->board[1][1]);
    }
    else if (a.x < 2 && a.y > 1) {
        result = __QT_check_tiles_for_win(
            s->board[0][2], s->board[0][3], 
            s->board[1][2], s->board[1][3]);
    }
    else if (a.x > 1 && a.y < 2) {
        result = __QT_check_tiles_for_win(
            s->board[2][0], s->board[2][1], 
            s->board[3][0], s->board[3][1]);
    }
    else { // a.x > 1 && a.y > 1
        result = __QT_check_tiles_for_win(
            s->board[2][2], s->board[2][3],
            s->board[3][2], s->board[3][3]);
    }

    if (result) { return 1; }

    return 0;
}

// other

uint8_t QT_minimax(QT_State *s, QT_MM_Results *results, uint8_t depth, uint8_t player) {
    QT_Action a;
    uint8_t value;

    results->nodes_visited += 1;
    
    // check if there is a winning move this turn
    if (depth >= 3) {
        uint8_t game_over = 0, num_invalid_moves = 0;
        
        // loop through all actions 
        for (uint8_t n = 0; n < QT_N_ACTIONS; n++) {
            a = __QT_decode_action(n);
            
            if (QT_check_valid_move(s,a)) {
                QT_forward_step(s,a);
                game_over = QT_check_completion_win(s,a);
                QT_backward_step(s,a);
            }
            else {
                num_invalid_moves += 1;
            }

            if (game_over) {
                return player; // 1 if maximizing, 0 o.w.
            }
        }

        if (num_invalid_moves == QT_N_ACTIONS) {
            return !player; // 0 if maximizing, 1 o.w.
        } 
    }

    value = (player) ? 0 : 1; // -inf if maximizing, +inf o.w.
    
    // otherwise recursively search next states
    for (uint8_t n = 0; n < QT_N_ACTIONS; n++) {
        a = __QT_decode_action(n);

        if (QT_check_valid_move(s,a)) {
            QT_forward_step(s,a);

            if (player) {
                value = QT_minimax(s, results, depth+1, 0);
            }
            else {
                value = QT_minimax(s, results, depth+1, 1);
            }
            
            QT_backward_step(s,a);

            if ((player && value == 1) || 
                (!player && value == 0)) {
                break;
            }
        }
    }

    return value;
}

void QT_print_state(const QT_State *s) {
    // Print a text representation of the current board to a char array.
    // The array should have length >= 28.
    //
    uint8_t k = 0;
    uint8_t player, shape;
    char str[32];

    for (uint8_t i = 0; i < QT_N_ROWS; i++) {
        str[k] = '[';
        k++;
 
        for (uint8_t j = 0; j < QT_N_COLS; j++) {
            player = __QT_decode_player_from_tile(s->board[i][j]); 
            shape = __QT_decode_shape_from_tile(s->board[i][j]);

            switch (shape) {
                case 0:
                    str[k] = '.';
                    break;
                case 1:
                    str[k] = (player) ? 'A' : 'a';
                    break;
                case 2:
                    str[k] = (player) ? 'E' : 'e';
                    break;
                case 3:
                    str[k] = (player) ? 'O' : 'o';
                    break;
                case 4:
                    str[k] = (player) ? 'U' : 'u';
                    break;
            }
            
            k++;
        }

        str[k] = ']';
        k++;
        str[k] = '\n';
        k++;
    }
    
    str[k] = '\0';

    printf("%s", str);
}

// helper functions

QT_Action __QT_decode_action(uint8_t e) {
    return (QT_Action) { .x = (e & 12) >> 2, .y = e & 3, .shape = ((e & 48) >> 4) + 1 };
}

uint8_t __QT_get_opp_player(uint8_t player) {
    return (player == 0) ? 1 : 0;
}

uint8_t __QT_encode_tile(uint8_t player, uint8_t shape) {
    return (player << 4) | shape;
}

uint8_t __QT_decode_player_from_tile(uint8_t tile) {
    return tile >> 4;
}

uint8_t __QT_decode_shape_from_tile(uint8_t tile) {
    return tile & 15;
}

uint8_t __QT_check_tiles_for_win(uint8_t t1, uint8_t t2, uint8_t t3, uint8_t t4) {
    if (t1 == 0 || t2 == 0 || t3 == 0 || t4 == 0) {
        return 0;
    }

    uint8_t sh1 = __QT_decode_shape_from_tile(t1);
    uint8_t sh2 = __QT_decode_shape_from_tile(t2);
    uint8_t sh3 = __QT_decode_shape_from_tile(t3);
    uint8_t sh4 = __QT_decode_shape_from_tile(t4);

    uint8_t shape_count[QT_N_SHAPES] = {};
    
    shape_count[sh1 - 1] += 1;
    shape_count[sh2 - 1] += 1;
    shape_count[sh3 - 1] += 1;
    shape_count[sh4 - 1] += 1;

    for (uint8_t k = 0; k < QT_N_SHAPES; k++) {
        if (shape_count[k] != 1) {
            return 0;
        }
    }

    return 1;
}