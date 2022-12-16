#!/usr/bin/env python3

from argparse import ArgumentParser
import sys
import os
import re

script_dir = os.path.dirname(__file__)

# Usage: ./microbenchmarks.py
if os.environ["HOME"].startswith("/afs/andrew.cmu.edu"):
    workers = [1, 2, 4, 8]
else:
    workers = [1, 2, 4, 8, 16, 32, 64]
scenarios = ["PQGLock Insert", "PQGLock Delete", "PQGLock Mixed", "PQMDList Insert", "PQMDList Delete", "PQMDList Mixed"]

perfs = [[None] * len(workers) for _ in range(len(scenarios))]

os.system('mkdir -p logs')
os.system('rm -rf logs/*')
for j, worker in enumerate(workers):
    print(f'--- running microbenchmarks on {worker} workers ---')
    log_file = f'logs/microbenchmarks-{worker}.log'
    cmd = f'OMP_NUM_THREADS={worker} ./micro > {log_file}'
    print(f'Command: {cmd}')
    ret = os.system(cmd)
    assert ret == 0, 'ERROR -- micro exited with errors'

    with open(log_file, "r") as f:
        log = f.read()
        t_GLI = float(re.findall(r'GLock Insert: (.*?)s', log)[0])
        t_GLD = float(re.findall(r'GLock Delete: (.*?)s', log)[0])
        t_GLM = float(re.findall(r'GLock Mixed: (.*?)s', log)[0])
        t_MDI = float(re.findall(r'MDList Insert: (.*?)s', log)[0])
        t_MDD = float(re.findall(r'MDList Delete: (.*?)s', log)[0])
        t_MDM = float(re.findall(r'MDList Mixed: (.*?)s', log)[0])
        perfs[0][j] = t_GLI
        perfs[1][j] = t_GLD
        perfs[2][j] = t_GLM
        perfs[3][j] = t_MDI
        perfs[4][j] = t_MDD
        perfs[5][j] = t_MDM

print('\n-- Performance Table ---')
header = '|'.join(f' {x:<15} ' for x in ['Scene Name'] + workers)
print(header)
print('-' * len(header))
for scene, perf in zip(scenarios, perfs):
    print('|'.join(f' {x:<15} ' for x in [scene] + perf))

score = 0.0
print('\n-- Ops/s Table ---')
print(header)
print('-' * len(header))
for scene, perf in zip(scenarios, perfs):
    scores = [1/p for p in perf]
    scores_str = [f"{s:.3f} MOps/s" for s in scores]
    print('|'.join(f' {x:<15} ' for x in [scene] + scores_str))
