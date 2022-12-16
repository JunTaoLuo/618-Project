#!/usr/bin/env python3

import os
import random
import re

connectivity = 0.05

script_dir = os.path.dirname(__file__)
graph_dir = os.path.join(script_dir, "graphs")
ref_executable = os.path.join(script_dir, "dijk-release")
runtime_ref_path = os.path.join(graph_dir, "runtime-ref.txt")

graph_sizes = [64, 256, 1024, 4096, 8191]
runtime_ref = []

for graph_size in graph_sizes:
    possible_edges = (graph_size*(graph_size-1)//2)
    num_edges = (int)(possible_edges * connectivity)
    edges = [0] * possible_edges
    for i in range(num_edges):
        edges[i] = 1
    random.shuffle(edges)
    edge_index = 0

    graph_path = os.path.join(graph_dir, f"bench-{graph_size}-init.txt")
    # print(f"Generating graph of size {graph_size} at {graph_path}")
    # with open(graph_path, "w") as f:
    #     f.write(f"{graph_size}\n")
    #     for i in range(graph_size-1):
    #         for j in range(i+1, graph_size):
    #             if (edges[edge_index] == 1):
    #                 f.write(f"{i} {j} {random.randint(1, 64)}\n")
    #             edge_index += 1

    ref_path = os.path.join(graph_dir, f"bench-{graph_size}-ref.txt")
    log_path = f'logs/bench-{graph_size}.log'
    print(f"Generating reference at {ref_path}")
    cmd = f'{ref_executable} -in {graph_path} -o {ref_path} > {log_path}'
    print(f'Command: {cmd}')
    ret = os.system(cmd)
    t = float(re.findall(
        r'total time: (.*?)s', open(log_path).read())[0])
    print(f'total time: {t:.6f}s')
    runtime_ref.append(t)

with open(runtime_ref_path, "w") as f:
    for i in range(len(graph_sizes)):
        f.write(f"bench-{graph_sizes[i]} {runtime_ref[i]:.6f}\n")