#include <stdio.h>
#include "quantik.h"

int main() {
    QT_State s;
    QT_MM_Results r;
    QT_Action a;
    uint8_t result;

    r.nodes_visited = 0;
    
    QT_initialize_state(&s);

    // opening move
    a = (QT_Action) { .x = 0, .y = 0, .shape = 1 };
    QT_forward_step(&s,a);
    QT_print_state(&s);
    
    /*
    a = (QT_Action) { .x = 0, .y = 1, .shape = 2 };
    QT_forward_step(&s,a);
    QT_print_state(&s);

    a = (QT_Action) { .x = 3, .y = 3, .shape = 3 };
    QT_forward_step(&s,a);
    QT_print_state(&s);
    */
    
    result = QT_minimax(&s, &r, 1, s.player);
    
    printf("result: %i\n", result);
    printf("nodes visited: %lu\n", r.nodes_visited);
}
