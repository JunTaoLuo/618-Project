#!/usr/bin/env python3

import os
import random

connectivity = 0.1

script_dir = os.path.dirname(__file__)
graph_dir = os.path.join(script_dir, "graphs")
ref_executable = os.path.join(script_dir, "dijk-release")

graph_sizes = [16, 64, 256, 1024, 4096]

for graph_size in graph_sizes:
    possible_edges = (graph_size*(graph_size-1)//2)
    num_edges = (int)(possible_edges * connectivity)
    edges = [0] * possible_edges
    for i in range(num_edges):
        edges[i] = 1
    random.shuffle(edges)
    edge_index = 0

    graph_path = os.path.join(graph_dir, f"bench-{graph_size}-init.txt")
    print(f"Generating graph of size {graph_size} at {graph_path}")
    with open(graph_path, "w") as f:
        f.write(f"{graph_size}\n")
        for i in range(graph_size-1):
            for j in range(i+1, graph_size):
                if (edges[edge_index] == 1):
                    f.write(f"{random.randint(1, 99)} ")
                else:
                    f.write(f"0 ")
                edge_index += 1
            f.write(f"\n")

    ref_path = os.path.join(graph_dir, f"bench-{graph_size}-ref.txt")
    print(f"Generating reference at {ref_path}")
    cmd = f'{ref_executable} -in {graph_path} -o {ref_path}'
    print(f'Command: {cmd}')
    ret = os.system(cmd)
