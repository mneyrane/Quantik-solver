import json
import pyquantik as qt
import networkx as nx
from pathlib import Path
from sympy.combinatorics import Permutation, PermutationGroup

# create output directory

outputdir = Path(__file__).parent / '01-output'
outputdir.mkdir(exist_ok=True)

# constants

N_ROWS = 4
N_COLS = 4
SHAPE_F_IDX = 1
SHAPE_L_IDX = 5

# define symmetries

rotation = Permutation(0,12,15,3)(1,8,14,7)(2,4,13,11)(5,9,10,6)
ht_flip = Permutation(0,4)(1,5)(2,6)(3,7)
hb_flip = Permutation(8,12)(9,13)(10,14)(11,15)
h_swap = Permutation(0,8)(1,9)(2,10)(3,11)(4,12)(5,13)(6,14)(7,15)
vl_flip = Permutation(0,1)(4,5)(8,9)(12,13)
vr_flip = Permutation(2,3)(6,7)(10,11)(14,15)
v_swap = Permutation(0,2)(4,6)(8,10)(12,14)(1,3)(5,7)(9,11)(13,15)

P = PermutationGroup(rotation, ht_flip, hb_flip, h_swap, vl_flip, vr_flip, v_swap)

# helper functions

def get_max_shape_idx_from_tuple(t):
    return max([tile if tile < 16 else tile-16 for tile in t])

def build_graph(state, graph, node, depth):
    if depth == 3: # stop descending at depth 3
        if 'winner' in graph.nodes[node]:
            return # do nothing, already evaluated this node
        
        print(node)
        winning_moves = state.get_winning_moves()
        
        #print("Node:", node)
        #print("^ Node's Winning moves:", winning_moves)
        
        graph.nodes[node]['moves'] = tuple(winning_moves)

        for a in winning_moves:
            if a == state.get_player():
                graph.nodes[node]['winner'] = a
                return
        
        # otherwise, no winning moves
        graph.nodes[node]['winner'] = int(not state.get_player())
        return
    
    max_shape_value = get_max_shape_idx_from_tuple(G.nodes[node]['board']) + 1
    
    node_board = G.nodes[node]['board']
    nonempty_idxs = [i for i in range(len(node_board)) if node_board[i] != 0]
    
    P_stab = P.pointwise_stabilizer(nonempty_idxs)
    orbits = [tuple(sorted(o)) for o in P_stab.orbits()]
    
    print('max_shape_value:', max_shape_value)
    print('nonempty_idxs:', nonempty_idxs)
    print('orbits:', orbits)
    
    for shape in range(SHAPE_F_IDX,SHAPE_L_IDX):
        if (shape > max_shape_value):
            continue
        
        for o in orbits:
            x, y = o[0] // N_ROWS, o[0] % N_COLS
            
            if not state.check_valid_move(x,y,shape):
                continue
            
            state.forward_step(x,y,shape)
            
            #next_node = tuple(state.get_board()) # changing to move stack
            next_node = node + (x,y,shape)
            graph.add_edge(node, next_node, orbit=o, x=x, y=y, shape=shape)
            graph.nodes[next_node]['player'] = state.get_player()
            graph.nodes[next_node]['board'] = tuple(state.get_board())
            graph.nodes[next_node]['depth'] = depth + 1

            print("Recursing with orbit", o, "with", (x,y,shape))
            
            build_graph(state, graph, next_node, depth + 1)
            
            state.backward_step(x,y,shape)

    # construct winning move table
    winning_moves = bytearray(64)

    for shape in range(SHAPE_F_IDX,SHAPE_L_IDX):
        if (shape > max_shape_value):
            winning_moves[16*(shape-1):16*shape] = winning_moves[16*(max_shape_value-1):16*max_shape_value]
            continue

        for o in orbits:
            x0, y0 = o[0] // N_ROWS, o[0] % N_COLS
            
            if state.check_valid_move(x0,y0,shape):
                state.forward_step(x0,y0,shape)
                #child = tuple(state.get_board())
                child = node + (x0,y0,shape)
                state.backward_step(x0,y0,shape)
            
                edge = graph.edges[node, child] # the child node should have been reached

                assert(edge['orbit'] == o and edge['shape'] == shape)
                
                for pos in o:
                    winning_moves[16*(shape-1) + pos] = graph.nodes[child]['winner']
            
            else:
                for pos in o:
                    winning_moves[16*(shape-1) + pos] = 255

    graph.nodes[node]['moves'] = tuple(winning_moves)

    for a in winning_moves:
        if a == state.get_player():
            graph.nodes[node]['winner'] = a
            return
    
    # otherwise, no winning moves
    graph.nodes[node]['winner'] = int(not state.get_player())
    return

# create opening book graph

G = nx.DiGraph()

# construct opening book

s = qt.State()
s.initialize()

#root = tuple(s.get_board())
root = tuple()
G.add_node(root, player=s.get_player(), board=tuple(s.get_board()), depth=0)

build_graph(s, G, root, 0)

# save graph and symmetries

separators = (',',':')
indent = '  '

with open(outputdir / "graph.json", mode='w') as datafile:
    G_json = nx.tree_data(G, root)
    json.dump(G_json, datafile, separators=separators, indent=indent)

with open(outputdir / "symmetries.json", mode='w') as datafile:
    P_json = [g.array_form for g in P.generate()]
    json.dump(P_json, datafile, separators=separators)
