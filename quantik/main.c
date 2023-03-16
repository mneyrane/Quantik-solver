#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quantik.h"

#define BUF_SIZE 16

#define FLAG_EXIT 0
#define FLAG_RUN 1

#define SYMM_BYTES 2048
#define SYMM_CHUNK_SIZE 16

#define EVAL_MAX_DEPTH 3
#define EVAL_BYTES 8908
#define EVAL_CHUNK_SIZE 68
#define EVAL_STACK_SIZE 4
#define EVAL_DATA_SIZE 64

// solver main definitions and functions

typedef enum SolverStatus {
    ACTIVE = 0,
    END_COMPLETION = 1,
    END_NO_ACTIONS = 2
} SolverStatus;

typedef struct Solver {
    QT_State state;
    QT_Action action_stack[QT_B_SIZE];
    uint8_t valid_actions[QT_N_ACTIONS];
    uint8_t action_evaluations[QT_N_ACTIONS];
    uint8_t _depth;
    uint8_t* _opening_book;
    uint8_t* _symmetries;
    uint8_t* _eval_table;
    SolverStatus status;
} Solver;

Solver* initialize_solver();
uint8_t load_opening_book(Solver* sv);
void cleanup_solver(Solver* sv);
uint8_t process_user_input(char* buffer, Solver* sv);

// menu option functions
void print_help();
void forward_step(Solver* sv);
void undo_step(Solver* sv);
void print_valid_actions(Solver* sv);
void print_board(Solver* sv);
void print_shape_count(Solver* sv);
void print_summary(Solver* sv);
void print_action_evaluations(Solver* sv);
void print_win_ratios(Solver* sv);
void print_legend();

// helper functions
void evaluate_valid_actions(Solver* sv);
void _evaluate_via_book(Solver* sv);
void _evaluate_via_minimax(Solver *sv);
void evaluate_win_ratios(Solver* sv);


int main() {
    // create and configure solver
    Solver* solver = initialize_solver();
    if (! load_opening_book(solver)) {
        return EXIT_FAILURE;
    }

    char buffer[BUF_SIZE];
    uint8_t running;

    do {
        memset(&buffer, 0, BUF_SIZE);
        printf("Please enter an input option ('h' to list): ");
        fgets(buffer, BUF_SIZE, stdin);
        running = process_user_input(buffer, solver);
    } while (running);

    cleanup_solver(solver); 
    return EXIT_SUCCESS;
}



//
// solver main functions
//

Solver* initialize_solver() {
    Solver* sv = malloc(sizeof(Solver));
    QT_initialize_state(&(sv->state));
    sv->status = ACTIVE; 
    sv->_depth = 0;
    return sv; 
}

uint8_t load_opening_book(Solver* sv) {
    const char* bookfile = "opening_book.bin";
    FILE* fp = fopen(bookfile, "r");
    
    if (!fp) {
        perror("Failed to read opening book. Terminating.\n");
        return 0;
    }
    
    // allocate memory for opening book 
    uint32_t num_bytes = 0;
    while (fgetc(fp) != EOF) {
        num_bytes++;
    }
    rewind(fp);
    
    printf("Opening book consists of %d bytes.\n", num_bytes);
    sv->_opening_book = malloc(num_bytes * sizeof(uint8_t));
    
    // load opening book to memory 
    uint8_t b;
    uint32_t i = 0;
    for (uint32_t i = 0; i < num_bytes; i++) {
        sv->_opening_book[i] = fgetc(fp);
    }
    

    fclose(fp);
    
    // assign pointers to specific addresses in opening book 
    sv->_symmetries = &(sv->_opening_book[0]);
    sv->_eval_table = &(sv->_opening_book[SYMM_BYTES]);

    return 1;
}

void cleanup_solver(Solver* sv) {
    free(sv->_opening_book);
    free(sv);
    sv = NULL;
}

uint8_t process_user_input(char* buffer, Solver* sv) {
    char c = buffer[0];
    
    switch (c) {
        case 'h':
            print_help();
            break;
        case 'q':
            printf("Quitting.\n");
            return FLAG_EXIT;
        case 'f':
            forward_step(sv);
            break;
        case 'u':
            undo_step(sv);
            break;
        case 'a':
            print_valid_actions(sv);
            break;
        case 'b':
            print_board(sv); 
            break;
        case 's':
            print_shape_count(sv);
            break;
        case 'p':
            print_summary(sv);
            break;
        case 'e':
            print_action_evaluations(sv);
            break;
        case 'w':
            print_win_ratios(sv);
            break;
        case 'l':
            print_legend();
            break;
        default: // invalid input
            if (!(isblank(c) || c == '\n')) {
                printf("'%c' is not a valid input. Enter 'h' to list all options.\n", c);
            }
    }

    return FLAG_RUN;
}



//
// menu option functions
//

void print_help() {
    printf("QUANTIK SOLVER INPUT OPTIONS\n"
        "  Navigation:\n"
        "    h - print this prompt\n"
        "    q - quit the application\n"
        "    f - input an action\n"
        "    u - undo previous action\n"
        "  Printing:\n"
        "    a - print valid actions\n"
        "    b - print Quantik board\n"
        "    s - print player shape counts\n"
        "    p - print summary of current state\n"
        "    w - print win ratios for each action\n"
        "    e - print action evaluations\n"
        "    l - print evaluation legend\n"
    );
}

void forward_step(Solver* sv) {
    printf("Enter an action (format: [row][col][shape], 0 <= [*] < 4): ");
    
    uint8_t digits;
    char buffer[BUF_SIZE];
    memset(&buffer, 0, BUF_SIZE);
    fgets(buffer, BUF_SIZE, stdin);
    
    // check for correct number of digits 
    digits = 0;
    for (uint8_t i = 0; i < BUF_SIZE; i++) {
        if (isdigit(buffer[i])) {
            digits++;
        }
    }

    if (digits != 3) {
        printf("Invalid action input; expected 3 digits.\n");
        return;
    }
    
    // check for correct digit range
    int8_t idxs[3] = {-1,-1,-1}; // set default values to invalid input
    
    digits = 0; 
    for (uint8_t i = 0; i < BUF_SIZE; i++) {
        if (isdigit(buffer[i])) {
            idxs[digits] = buffer[i] - '0';
            digits++;
        }
        if (digits == 3) { break; }
    }

    if (!(idxs[0] >= 0 && idxs[0] < 4 && 
          idxs[1] >= 0 && idxs[1] < 4 && 
          idxs[2] >= 0 && idxs[2] < 4)) {
        printf("Invalid action input; digits should be 0, 1, 2, or 3.\n");
        return;
    }

    // process valid input
    QT_Action a = { .x = idxs[0], .y = idxs[1], .shape = idxs[2]+1 };
    if (QT_check_valid_move(&(sv->state), a)) {
        QT_forward_step(&(sv->state), a);
        sv->action_stack[sv->_depth] = a;
        sv->_depth += 1;
    }
    else {
        printf("Not a legal move.\n");
    }
}

void undo_step(Solver* sv) {
    if (sv->_depth == 0) {
        printf("No actions to undo.\n");
    }
    else {
        sv->_depth -= 1;
        QT_Action a = sv->action_stack[sv->_depth];
        QT_backward_step(&(sv->state), a);
    }
} 

void print_valid_actions(Solver* sv) {
    QT_Action a;
    uint8_t enc;

    for (uint8_t e = 0; e < QT_N_ACTIONS; e++) {
        a = __QT_decode_action(e);
        sv->valid_actions[e] = QT_check_valid_move(&(sv->state), a);
    }
    
    for (uint8_t x = 0; x < QT_N_ROWS; x++) { 
        for (uint8_t shape = 1; shape <= QT_N_SHAPES; shape++) {
            printf("[");
            for (uint8_t y = 0; y < QT_N_COLS; y++) {
                a = (QT_Action) { .x = x, .y = y, .shape = shape };
                enc = __QT_encode_action(a);
                sv->valid_actions[enc] ? printf("+") : printf(".");
            }
            printf("] ");
        }
        printf("\n");
    }
}

void print_board(Solver* sv) {
    QT_print_state(&(sv->state));
}

void print_shape_count(Solver* sv) {
    printf("{");
    for (uint8_t i = 0; i < QT_N_CLR_SHAPES; i++) {
        printf("%d, ", sv->state.shapes[i]);
    }
    printf("}\n");
}

void print_summary(Solver* sv) {
    // TO DO ...
    printf("Not implemented.\n");
}

void print_action_evaluations(Solver* sv) {
    evaluate_valid_actions(sv);
    
    uint8_t eval, enc; 
    QT_Action a;

    for (uint8_t x = 0; x < QT_N_ROWS; x++) { 
        for (uint8_t shape = 1; shape <= QT_N_SHAPES; shape++) {
            printf("[");
            for (uint8_t y = 0; y < QT_N_COLS; y++) {
                a = (QT_Action) { .x = x, .y = y, .shape = shape };
                enc = __QT_encode_action(a);
                eval = sv->action_evaluations[enc];
                if (eval  == 255) {
                    printf(".");
                    continue;
                }

                if (eval == sv->state.player) {
                    printf("w");
                }
                else {
                    printf("l");
                }
            }
            printf("] ");
        }
        printf("\n");
    }   
}

void print_win_ratios(Solver* sv) {
    // TO DO ...
    printf("Not implemented.\n");
}

void print_legend() {
    // TO DO ...
    printf("Not implemented.\n");
}



//
// helper functions
//

void evaluate_valid_actions(Solver* sv) {
    if (sv->_depth <= EVAL_MAX_DEPTH) {
        _evaluate_via_book(sv);
    }
    else {
        _evaluate_via_minimax(sv);
    }
}

void _evaluate_via_book(Solver* sv) {
    // obtain shape permutation
    uint8_t shape_perm[QT_N_SHAPES] = {0,1,2,3};
    uint8_t distinct_shapes[QT_N_SHAPES] = {0,0,0,0}; 
    
    uint8_t max_shape_idx = 0;

    for (uint8_t i = 0; i < sv->_depth; i++) {
        uint8_t idx = (sv->action_stack[i]).shape - 1;
        if (distinct_shapes[idx] == 0) {
            distinct_shapes[idx] = 1;

            shape_perm[idx] = max_shape_idx;
            max_shape_idx++;
        }
    }

    for (uint8_t i = 0; i < QT_N_SHAPES; i++) {
        if (distinct_shapes[i] == 0) {
            shape_perm[i] = max_shape_idx;
            max_shape_idx++;
        }
    }

    //printf("Shape perm: %d,%d,%d,%d\n", shape_perm[0], shape_perm[1], shape_perm[2], shape_perm[3]);

    // search for board permutation
    // _sp : shape permuted, _bp : board permuted
    QT_Action m_sp[EVAL_MAX_DEPTH], m_bp[EVAL_MAX_DEPTH];
    
    memcpy(m_sp, sv->action_stack, EVAL_MAX_DEPTH * sizeof(QT_Action));
    
    //printf("Shape seq before: %d,%d,%d\n", m_sp[0].shape, m_sp[1].shape, m_sp[2].shape);
    for (uint8_t i = 0; i < sv->_depth; i++) {
        m_sp[i].shape = shape_perm[m_sp[i].shape-1] + 1;
    }
    //printf("Shape seq after: %d,%d,%d\n", m_sp[0].shape, m_sp[1].shape, m_sp[2].shape);

    memcpy(m_bp, m_sp, EVAL_MAX_DEPTH * sizeof(QT_Action));


    for (uint32_t b = 0; b < EVAL_BYTES; b += EVAL_CHUNK_SIZE) {
        //for (uint8_t k = 0; k < EVAL_CHUNK_SIZE; k++) {
        //    printf("%d,", sv->_eval_table[b+k]);
        //}
        //printf("\n");

        // check if queried table depth matches the current depth
        uint8_t table_depth = 0;
        for (uint8_t d = 0; d < EVAL_MAX_DEPTH; d++) {
            if (sv->_eval_table[b+d] == 255) {
                break;
            }
            table_depth++;
        }

        if (table_depth != sv->_depth) {
            //printf("Table depth failed\n");
            goto end_current_eval;
        }
        
        // check if queried table shapes match our permuted action stack shapes
        for (uint8_t d = 0; d < sv->_depth; d++) {
            QT_Action a = __QT_decode_action(sv->_eval_table[b+d]);
            if (a.shape != m_sp[d].shape) {
                //printf("Shape permutation failed\n");
                goto end_current_eval;
            }
        }
        
        for (uint32_t p = 0; p < SYMM_BYTES; p += SYMM_CHUNK_SIZE) {
            //printf("p %d\n", p);
            for (uint8_t d = 0; d < sv->_depth; d++) {
                uint8_t z0, zp, xp, yp;
                QT_Action a;

                z0 = QT_N_COLS*m_sp[d].x + m_sp[d].y;
                zp = sv->_symmetries[p+z0];
                xp = zp / QT_N_COLS;
                yp = zp % QT_N_ROWS;

                a = __QT_decode_action(sv->_eval_table[b+d]);
                
                if (!(a.x == xp && a.y == yp)) {
                    //printf("Mismatch of permuted shape: a = (%d,%d), perm = (%d,%d)\n", a.x, a.y, xp, yp);
                    goto end_perm_loop;
                }
            }

            // no errors encountered, we have found a permutation
            uint8_t inv_shape_perm[QT_N_SHAPES];
            uint8_t inv_board_perm[QT_B_SIZE];
            
            for (uint8_t i = 0; i < QT_N_SHAPES; i++) {
                inv_shape_perm[shape_perm[i]] = i;
            }

            for (uint8_t j = 0; j < QT_B_SIZE; j++) {
                inv_board_perm[sv->_symmetries[p+j]] = j;
            }

            for (uint8_t e = 0; e < QT_N_ACTIONS; e++) {
                uint8_t inv_idx = QT_B_SIZE*inv_shape_perm[e/QT_B_SIZE] + inv_board_perm[e%QT_B_SIZE];
                sv->action_evaluations[inv_idx] = sv->_eval_table[b+EVAL_STACK_SIZE+e];
            }

            printf("Succeded search\n");
            
            goto end_search;

            end_perm_loop:
        }

        end_current_eval:

    }

    printf("ERROR--- there is a bug; could not find table to query.\n");
    
    end_search:
}

void _evaluate_via_minimax(Solver* sv) {
    QT_State s = sv->state;
    QT_Action a;
    for (uint8_t e = 0; e < QT_N_ACTIONS; e++) {
        a = __QT_decode_action(e);
        if (QT_check_valid_move(&s, a)) {
            QT_forward_step(&s,a);
            if (QT_check_completion_win(&s, a)) {
                printf("Helo\n");
                sv->action_evaluations[e] = !(s.player);
            }
            else { 
                QT_MM_Results r = {0};
                sv->action_evaluations[e] = QT_minimax(&s, &r, sv->_depth, s.player);
            }
            QT_backward_step(&s,a);
        }
        else {
            sv->action_evaluations[e] = 255;            
        }
    }
}

void evaluate_win_ratios(Solver* sv) {

}
