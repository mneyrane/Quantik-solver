#include <stdio.h>
#include <stdint.h>
#include "quantik.h"

uint8_t depth_minimax(QT_State* s, uint8_t player);


int main() {
    QT_State s;
    QT_Action a;
    uint8_t result;

    QT_initialize_state(&s);

    // opening move
    
    a = (QT_Action) { .x = 0, .y = 0, .shape = 1 };
    QT_forward_step(&s,a);
    
    a = (QT_Action) { .x = 0, .y = 1, .shape = 2 };
    QT_forward_step(&s,a);
    
    a = (QT_Action) { .x = 3, .y = 3, .shape = 3 };
    QT_forward_step(&s,a);

    QT_print_state(&s);
    
    result = depth_minimax(&s, s.player);
    
    printf("result: %i\n", result);
}


uint8_t depth_minimax(QT_State* s, uint8_t player) {
    QT_Action a;
    uint8_t value, new_value, mm_value;
    uint8_t game_over = 0;

    if (player == 0) {
        // check if there are no valid moves for player 0
        if (s->depth >= 3) {
            uint8_t num_invalid_moves = 0;
            
            // loop through all actions 
            for (uint8_t n = 0; n < QT_N_ACTIONS; n++) {
                a = __QT_decode_action(n);                
                if (! QT_check_valid_move(s,a)) {
                    num_invalid_moves += 1;
                }
            }

            if (num_invalid_moves == QT_N_ACTIONS) {
                // technically, this should just be 'depth', but this would show if
                // player 0 can force the game to end by no legal moves, or if player
                // 1 can always squeeze out a win by region completion

                //return depth;
                return s->depth; 
            } 
        }

        // otherwise recursively search next states
        value = 0;
    
        for (uint8_t n = 0; n < QT_N_ACTIONS; n++) {
            a = __QT_decode_action(n);

            if (QT_check_valid_move(s,a)) {
                QT_forward_step(s,a);

                new_value = depth_minimax(s, 1);
                
                QT_backward_step(s,a);

                if (new_value > value) {
                    value = new_value;
                }
            }
        }

        if (s->depth == 4) { // DEBUG
            printf("Value for player %i at depth %i: %i\n", player, s->depth, value);
        }

        return value;
    }

    else { // player == 1 o.w.
        value = 16;

        // check if one of the next moves ends the game; this is optimal for player 1
        for (uint8_t n = 0; n < QT_N_ACTIONS; n++) {
            a = __QT_decode_action(n);

            if (QT_check_valid_move(s,a)) {
                QT_forward_step(s,a);
                game_over = QT_check_completion_win(s,a);
                QT_backward_step(s,a);
            }

            if (game_over) {
                return s->depth + 1;
            }
        }

        // otherwise, recurse
        for (uint8_t n = 0; n < QT_N_ACTIONS; n++) {
            a = __QT_decode_action(n);

            if (QT_check_valid_move(s,a)) {
                QT_forward_step(s,a);
                mm_value = QT_minimax(s, 0);
                QT_backward_step(s,a);

                if (mm_value != player) {
                    continue;
                }

                QT_forward_step(s,a);
                new_value = depth_minimax(s, 0);
                QT_backward_step(s,a);

                if (new_value < value) {
                    value = new_value;
                }                
            }
        }

        return value;
    }
}