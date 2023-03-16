import json
import pyquantik as qt
import networkx as nx
from pathlib import Path


# define directories

maindir = Path(__file__).parent
inputdir = maindir / "01-output"
outputdir = maindir / "02-output"
outputdir.mkdir(exist_ok=True)


# adjust JSON graph data so that "id" contains a hashable type

def convert_to_hashable_nodes(data):
    data['id'] = tuple(data['id'])
    try:
        for child in data['children']:
            convert_to_hashable_nodes(child)
    except KeyError:
        return # we reached a leaf node


# load data

with open(inputdir / "symmetries.json", mode='r') as symfile:
    symdata = json.load(symfile)

with open(inputdir / "graph.json", mode='r') as graphfile:
    graphdata = json.load(graphfile)
    convert_to_hashable_nodes(graphdata)
    G = nx.tree_graph(graphdata)


# print for debugging

"""
print("Number of nodes", len(G.nodes))
print("Number of edges", len(G.edges))

for i, e in enumerate(G.edges(data=True)):
    print("Edge", i)
    print("Parent:", e[0])
    print("Child :", e[1])
    print("Data  :", e[2])

print("===== PRINTING NODES =====")

for i, v in enumerate(G.nodes(data=True)):
    print("Node", i, "-", v)
"""

# save data in binary format

with open(outputdir / "opening_book.bin", mode='wb') as binfile:
    # write symmetries (128*8 = 2048 bytes)
    for g in symdata: 
        binfile.write(bytes(g))
    
    # write move stack + win/loss table
    root_nodes = list(filter(lambda v : G.in_degree(v) == 0, G.nodes))
    assert(len(root_nodes) == 1)
    root = root_nodes[0]

    for layer in nx.bfs_layers(G, root): # use BFS order
        for node in layer:
            i = 0
            # get action by 3-element chunks from node
            action_iter = zip(*[iter(node)]*3, strict=True)
            for action in action_iter:
                e = action[1] | (action[0] << 2) | ((action[2]-1) << 4)
                binfile.write(e.to_bytes(1, byteorder='big'))
                i += 1

            while i < 4:
                binfile.write(b'\xff')
                i += 1

            binfile.write(bytes(G.nodes[node]['moves']))
