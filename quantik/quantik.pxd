from libc.stdint cimport uint8_t

DEF QT_N_ROWS = 4
DEF QT_N_COLS = 4
DEF QT_N_CLR_SHAPES = 8

cdef extern from "quantik.h":
    ctypedef struct QT_State:
        uint8_t[QT_N_ROWS][QT_N_COLS] board
        uint8_t[QT_N_CLR_SHAPES] shapes
        uint8_t player
        uint8_t depth
    ctypedef struct QT_Action:
        uint8_t x
        uint8_t y
        uint8_t shape
    
    void QT_initialize_state(QT_State *s);
    uint8_t QT_check_valid_move(const QT_State *s, QT_Action a);
    void QT_forward_step(QT_State *s, QT_Action a);
    void QT_backward_step(QT_State *s, QT_Action a);
    uint8_t QT_check_completion_win(const QT_State *s, QT_Action a);

    uint8_t QT_minimax(QT_State *s, uint8_t player);
