#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "quantik.h"

// loop constants
#define FLAG_EXIT 0         
#define FLAG_RUN 1          
#define BUF_SIZE 16         // input buffer size
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

// opening book constants
#define SYMM_BYTES 2048     // symmetry total size
#define SYMM_CHUNK_SIZE 16  // symmetry chunk size
#define EVAL_MAX_DEPTH 3    // maximum evaluation depth
#define EVAL_BYTES 8908     // evaluation data total size
#define EVAL_CHUNK_SIZE 68  // evaluation data chunk size
#define EVAL_STACK_SIZE 4   // eval-chunk action stack bytes
#define EVAL_TABLE_SIZE 64  // eval-chunk table bytes



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
    SolverStatus status;
    uint8_t _depth;
    uint8_t* _opening_book;
    uint8_t* _symmetries;
    uint8_t* _eval_table;
} Solver;

// main functions
Solver* initialize_solver();
uint8_t load_opening_book(Solver* sv);
void cleanup_solver(Solver* sv);
uint8_t process_user_input(char* buffer, Solver* sv);

// menu option functions
void print_help();
void forward_step(Solver* sv);
void undo_step(Solver* sv);
void print_action_history(Solver *sv);
void print_valid_actions(Solver* sv);
void print_board(Solver* sv);
void print_shape_count(Solver* sv);
void print_action_evaluations(Solver* sv);
void print_legend();

// helper functions
void evaluate_valid_actions(Solver* sv);
void _evaluate_via_book(Solver* sv);
void _evaluate_via_minimax(Solver *sv);
void _condition_prints(char c, Solver *sv, void (*f)(Solver*));



int main() {
    Solver* solver = initialize_solver();
    if (!load_opening_book(solver)) {
        cleanup_solver(solver);
        return EXIT_FAILURE;
    }

    char buffer[BUF_SIZE];
    uint8_t running;

    do {
        memset(&buffer, 0, BUF_SIZE);
        printf("Please enter an input option ('h' to list): ");
        running = process_user_input(fgets(buffer, BUF_SIZE, stdin), solver);
        clearerr(stdin);
    } while (running);

    cleanup_solver(solver); 
    return EXIT_SUCCESS;
}



//
// main functions
//

Solver* initialize_solver() {
    Solver* sv = malloc(sizeof(Solver));
    QT_initialize_state(&(sv->state));
    sv->status = ACTIVE; 
    sv->_depth = 0;
    sv->_opening_book = NULL;
    return sv; 
}

uint8_t load_opening_book(Solver* sv) {
    const char* bookfile = "opening_book.bin";
    FILE* fp = fopen(bookfile, "r");
    
    if (!fp) {
        perror("Failed to open 'opening_book.bin'");
        return 0;
    }
    
    // allocate memory for opening book 
    uint32_t num_bytes = 0;
    while (fgetc(fp) != EOF) {
        num_bytes++;
    }
    rewind(fp);
    
    sv->_opening_book = malloc(num_bytes * sizeof(uint8_t));

    if (!(sv->_opening_book)) {
        eprintf("Failed to allocate memory for opening book.\n");
        return 0;
    }
    
    // load opening book to memory 
    for (uint32_t i = 0; i < num_bytes; i++) {
        sv->_opening_book[i] = fgetc(fp);
    }

    fclose(fp);
    
    // assign pointers to specific byte offsets
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
    if (!buffer) {
        printf("\n");
        return FLAG_RUN;
    }

    char c;
    c = (buffer) ? buffer[0] : '\n';
    
    switch (c) {
        case 'h':
            print_help();
            break;
        case 'q':
            printf("Quitting.\n");
            return FLAG_EXIT;
        case 'f':
            _condition_prints(c, sv, forward_step);
            break;
        case 'u':
            undo_step(sv);
            break;
        case 'a':
            print_action_history(sv);
            break;
        case 'v':
            _condition_prints(c, sv, print_valid_actions);
            break;
        case 'b':
            print_board(sv); 
            break;
        case 's':
            print_shape_count(sv);
            break;
        case 'p':
            print_legend();
            break;
        case 'e':
            _condition_prints(c, sv, print_action_evaluations);
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
    printf("Navigation:"
        "\n    h - print this prompt"
        "\n    q - quit the application"
        "\n    f - input an action"
        "\n    u - undo previous action"
        "\nPrinting:"
        "\n    a - print action history"
        "\n    v - print valid actions"
        "\n    b - print Quantik board"
        "\n    s - print player shape counts"
        "\n    e - print action evaluations"
        "\n    p - print output legend"
        "\n"
    );
}

void forward_step(Solver* sv) {
    printf("Enter an action (format: [row][col][shape], 0 <= [*] < 4): ");
    
    uint8_t digits;
    char buffer[BUF_SIZE];
    memset(&buffer, 0, BUF_SIZE);
    
    if (!fgets(buffer, BUF_SIZE, stdin)) {
        printf("\n");
        return;
    }
    
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

        if (QT_check_completion_win(&(sv->state), a)) {
            printf("Game ended - player %d wins by region completion!\n", !(sv->state.player));
            sv->status = END_COMPLETION;
        }
        else {
            uint8_t num_invalid_moves = 0;
            QT_Action b;
            for (uint8_t e = 0; e < QT_N_ACTIONS; e++) {
                b = __QT_decode_action(e);
                if (!QT_check_valid_move(&(sv->state), b)) {
                    num_invalid_moves++;
                }
            }
            if (num_invalid_moves == QT_N_ACTIONS) {
                printf("Game ended - player %d has no legal moves!\n", sv->state.player);
                sv->status = END_NO_ACTIONS;
            }
        }
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
        sv->status = ACTIVE;
        QT_Action a = sv->action_stack[sv->_depth];
        QT_backward_step(&(sv->state), a);
    }
}

void print_action_history(Solver *sv) {
    if (sv->_depth == 0) {
        printf("No pieces have been played yet.\n");
        return;
    }

    QT_Action a;
    for (uint8_t d = 0; d < sv->_depth; d++) {
        a = sv->action_stack[d];
        printf("%d:%d%d%d", d, a.x, a.y, a.shape - 1);
        (d < sv->_depth - 1) ? printf(" ") : printf("\n");
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
    uint8_t player, shape;

    for (uint8_t i = 0; i < QT_N_ROWS; i++) {
        putchar('[');
        for (uint8_t j = 0; j < QT_N_COLS; j++) {
            player = __QT_decode_player_from_tile(sv->state.board[i][j]); 
            shape = __QT_decode_shape_from_tile(sv->state.board[i][j]);

            (player) ? putchar('*') : putchar(' ');

            switch (shape) {
                case 0:
                    putchar('.');
                    break;
                case 1:
                    putchar('0');
                    break;
                case 2:
                    putchar('1');
                    break;
                case 3:
                    putchar('2');
                    break;
                case 4:
                    putchar('3');
                    break;
            }

            if (j < QT_N_COLS - 1) {
                putchar(' ');
            }
        }
        putchar(']');
        putchar('\n');
    }
}

void print_shape_count(Solver* sv) {
    printf("0 shapes {");
    for (uint8_t i = 0; i < QT_N_SHAPES; i++) {
        printf("%d:%d", i, sv->state.shapes[i]);
        if (i < QT_N_SHAPES - 1) { printf(", "); }
    }
    printf("}\n");
    
    printf("1 shapes {");
    for (uint8_t i = 0; i < QT_N_SHAPES; i++) {
        printf("%d:%d", i, sv->state.shapes[i + QT_N_SHAPES]);
        if (i < QT_N_SHAPES - 1) { printf(", "); }
    }
    printf("}\n");
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

void print_legend() {
    printf("Action history (a):"
    "\n    Format 'm:xys' where m is the move number, x and y are the board"
    "\n    coordinates and s is the shape. The values match that of command 'f',"
    "\n    which are [x][y][s], 0 <= [*] < 4."
    "\n"
    "Board (b):"
    "\n    ' .' : empty tile"
    "\n    ' n' : shape n from player 0"
    "\n    '*n' : shape n from player 1"
    "\n"
    "Player shape counts (s):"
    "\n    Format is 's:c', where s is the shape and c is the count."
    "\n"
    "Valid actions (v) and action evaluations (e):"
    "\n    Here the boards from left to right correspond to shapes 0, 1, etc."
    "\n    and each tile informs placement and win conditions for the respective"
    "\n    shape."
    "\n    '+'  : valid tile"
    "\n    '.'  : invalid tile"
    "\n    'w'  : current player can force a win"
    "\n    'l'  : opposing player can force a win"
    "\n"
    );
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

    // search for board permutation
    // _sp : shape permuted, _bp : board permuted
    QT_Action m_sp[EVAL_MAX_DEPTH], m_bp[EVAL_MAX_DEPTH];
    
    memcpy(m_sp, sv->action_stack, EVAL_MAX_DEPTH * sizeof(QT_Action));
    
    for (uint8_t i = 0; i < sv->_depth; i++) {
        m_sp[i].shape = shape_perm[m_sp[i].shape-1] + 1;
    }

    memcpy(m_bp, m_sp, EVAL_MAX_DEPTH * sizeof(QT_Action));


    for (uint32_t b = 0; b < EVAL_BYTES; b += EVAL_CHUNK_SIZE) {
        // check if queried table depth matches the current depth
        uint8_t table_depth = 0;
        for (uint8_t d = 0; d < EVAL_MAX_DEPTH; d++) {
            if (sv->_eval_table[b+d] == 255) {
                break;
            }
            table_depth++;
        }

        if (table_depth != sv->_depth) {
            goto end_current_eval;
        }
        
        // check if queried table shapes match our permuted action stack shapes
        for (uint8_t d = 0; d < sv->_depth; d++) {
            QT_Action a = __QT_decode_action(sv->_eval_table[b+d]);
            if (a.shape != m_sp[d].shape) {
                goto end_current_eval;
            }
        }
        
        for (uint32_t p = 0; p < SYMM_BYTES; p += SYMM_CHUNK_SIZE) {
            for (uint8_t d = 0; d < sv->_depth; d++) {
                uint8_t z0, zp, xp, yp;
                QT_Action a;

                z0 = QT_N_COLS*m_sp[d].x + m_sp[d].y;
                zp = sv->_symmetries[p+z0];
                xp = zp / QT_N_COLS;
                yp = zp % QT_N_ROWS;

                a = __QT_decode_action(sv->_eval_table[b+d]);
                
                if (!(a.x == xp && a.y == yp)) {
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

            for (uint8_t e = 0; e < EVAL_TABLE_SIZE; e++) {
                uint8_t inv_idx = QT_B_SIZE*inv_shape_perm[e/QT_B_SIZE] + inv_board_perm[e%QT_B_SIZE];
                sv->action_evaluations[inv_idx] = sv->_eval_table[b+EVAL_STACK_SIZE+e];
            }

            // Search succeeded            
            goto end_search;

            end_perm_loop:
        }

        end_current_eval:

    }

    eprintf("ERROR! Could not find book table entry for current state.\n");
    
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

void _condition_prints(char c, Solver *sv, void (*f)(Solver*)) {
    if ((c == 'f' || c == 'e' || c == 'v') && sv->status == END_COMPLETION) {
        printf("Cannot do '%c' since game ended by region completion.\n", c);
    }
    else if ((c == 'f' || c == 'e') && sv->status == END_NO_ACTIONS) {
        printf("Cannot do '%c' since game ended by no legal actions.\n", c);
    }
    else {
        (*f)(sv);
    }
}