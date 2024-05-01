cimport quantik
from libc.stdio cimport printf
from libc.stdint cimport uint8_t
from libc.stdlib cimport malloc, free
from collections import namedtuple

# macros
DEF N_ROWS = 4
DEF N_COLS = 4
DEF N_CLR_SHAPES = 8
DEF B_SIZE = 16
DEF N_ACTIONS = 64
DEF SHAPE_RNG_F_IDX = 1
DEF SHAPE_RNG_L_IDX = 5


ActionTuple = namedtuple('ActionTuple', ['x','y','shape'])


def ActionIterator():
    cdef uint8_t x, y, shape 
    
    for shape in range(SHAPE_RNG_F_IDX, SHAPE_RNG_L_IDX):
        for x in range(N_ROWS):
            for y in range(N_COLS):
                yield ActionTuple(x=x, y=y, shape=shape)


cdef class State:
    cdef quantik.QT_State* _state
    
    def __cinit__(self):
        self._state = <quantik.QT_State*> malloc(sizeof(quantik.QT_State))
        if self._state is NULL:
            raise MemoryError()

    def __dealloc__(self):
        if self._state is not NULL:
            free(self._state)

    cpdef void initialize(self):
        quantik.QT_initialize_state(self._state)
    
    cpdef uint8_t check_valid_move(self, uint8_t x, uint8_t y, uint8_t shape):
        cdef quantik.QT_Action a = quantik.QT_Action(x,y,shape)
        return quantik.QT_check_valid_move(self._state, a)

    cpdef void forward_step(self, uint8_t x, uint8_t y, uint8_t shape):
        cdef quantik.QT_Action a = quantik.QT_Action(x,y,shape)
        quantik.QT_forward_step(self._state, a)

    cpdef void backward_step(self, uint8_t x, uint8_t y, uint8_t shape):
        cdef quantik.QT_Action a = quantik.QT_Action(x,y,shape)
        quantik.QT_backward_step(self._state, a)

    cpdef uint8_t check_completion_win(self, uint8_t x, uint8_t y, uint8_t shape):
        cdef quantik.QT_Action a = quantik.QT_Action(x,y,shape)
        return quantik.QT_check_completion_win(self._state, a)

    cpdef uint8_t minimax(self):
        cdef quantik.QT_State s_local
        cdef uint8_t player

        s_local = self._state[0]

        player = self._state[0].player

        return quantik.QT_minimax(&s_local, player)

    def get_board(self):
        board = bytearray(B_SIZE)

        cdef uint8_t i, j
        for i in range(N_ROWS):
            for j in range(N_COLS):
                board[N_ROWS*i+j] = self._state[0].board[i][j]

        return board
 
    def get_shapes(self):
        shapes = bytearray(N_CLR_SHAPES)

        cdef uint8_t k
        for k in range(N_CLR_SHAPES):
            shapes[k] = self._state[0].shapes[k]

        return shapes

    def get_player(self):
        return self._state[0].player

    def get_winning_moves(self):
        cdef uint8_t i
        results = bytearray(N_ACTIONS)
        
        for i, a in enumerate(ActionIterator()):
            if self.check_valid_move(a.x, a.y, a.shape):
                self.forward_step(a.x, a.y, a.shape)
                if self.check_completion_win(a.x, a.y, a.shape):
                    printf("DEBUG: completed a win\n")
                    results[i] = not self._state[0].player
                else:
                    results[i] = self.minimax()
                self.backward_step(a.x, a.y, a.shape)
            else:
                results[i] = 255 # 0xff

        return results
