#!/usr/bin/env python3

from argparse import ArgumentParser
import sys
import os
import re

script_dir = os.path.dirname(__file__)
graph_dir = os.path.join(script_dir, "graphs")
runtime_ref_path = os.path.join(graph_dir, "runtime-ref.txt")

# Usage: ./checker.py prog
parser = ArgumentParser()
parser.add_argument("prog", type=str, nargs="?", default="pardijk-release")
args = parser.parse_args()
prog = args.prog
if os.environ["HOME"].startswith("/afs/andrew.cmu.edu"):
    workers = [4, 8]
else:
    workers = [4, 8, 16, 32, 64, 128]

ref_perfs = {}

with open(runtime_ref_path, "r") as f:
    lines = f.readlines()

for line in lines:
    scene, runtime = line.split(" ")
    ref_perfs[scene] = float(runtime)

perfs = [[None] * len(workers) for _ in range(len(ref_perfs))]

def compare(actual, ref):
    threshold = 1.0 if 'repeat' in actual else 0.1
    actual = open(actual).readlines()
    ref = open(ref).readlines()
    assert len(actual) == len(ref), \
        f'ERROR -- number of verticies is {len(actual)}, should be {len(ref)}'
    for i, (l1, l2) in enumerate(zip(actual, ref)):
        l1 = [int(x) for x in l1.split()]
        l2 = [int(x) for x in l2.split()]
        assert len(l1) == len(l2),\
            f'ERROR -- invalid format at line {i}, should contain {len(l2)} ints'

        for j, (x, y) in enumerate(zip(l1, l2)):
            assert abs(x - y) < threshold, f'ERROR -- incorrect result at line {i} value {j}, expected value {y} actual value {x}'

def compute_score(actual, ref):
    return ref/actual

os.system('mkdir -p logs')
os.system('rm -rf logs/*')
for i, scene_name in enumerate(ref_perfs.keys()):
    for j, worker in enumerate(workers):
        print(f'--- running {scene_name} on {worker} workers ---')
        init_file = f'graphs/{scene_name}-init.txt'
        output_file = f'logs/{scene_name}-{worker}.txt'
        log_file = f'logs/{scene_name}-{worker}.log'
        cmd = f'OMP_NUM_THREADS={worker} ./{prog} -in {init_file} -o {output_file} > {log_file}'
        print(f'Command: {cmd}')
        ret = os.system(cmd)
        assert ret == 0, 'ERROR -- nbody exited with errors'
        compare(output_file, f'graphs/{scene_name}-ref.txt')
        t = float(re.findall(
            r'total time: (.*?)s', open(log_file).read())[0])
        print(f'total time: {t:.6f}s')
        perfs[i][j] = t

print('\n-- Performance Table ---')
header = '|'.join(f' {x:<15} ' for x in ['Scene Name'] + workers)
print(header)
print('-' * len(header))
for scene, perf in zip(ref_perfs.keys(), perfs):
    print('|'.join(f' {x:<15} ' for x in [scene] + perf))

score = 0.0
print('\n-- Speedup Table ---')
print(header)
print('-' * len(header))
for i, (scene, perf) in enumerate(zip(ref_perfs.items(), perfs)):
    scores = [compute_score(p, scene[1])
              for p in perf]
    scores_str = [f"{s:.6f}" for s in scores]
    score += sum(scores)
    print('|'.join(f' {x:<15} ' for x in [scene[0]] + scores_str))
print(f'average speedup: {score/(len(workers) * len(ref_perfs))}')