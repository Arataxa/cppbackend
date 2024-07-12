import argparse
import subprocess
import time
import random
import shlex

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1

PERF_DATA = "perf.data"
RESULT_SVG = "graph.svg"

def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str, help='Command to start the server')
    return parser.parse_args().server

def run(command, output=None, shell=False):
    process = subprocess.Popen(shlex.split(command) if not shell else command, stdout=output, stderr=subprocess.DEVNULL, shell=shell)
    return process

def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()

def record_perf(pid):
    perf_command = f'sudo perf record -o ./{PERF_DATA} -p {pid} -g'
    perf_process = run(perf_command)
    return perf_process

def convert_to_flamegraph():
    collapse = f'./FlameGraph/stackcollapse-perf.pl'
    flamegraph = f'./FlameGraph/flamegraph.pl'
 
    convert_command = f'sudo perf script -i ./{PERF_DATA} | {collapse} | {flamegraph} > ./{RESULT_SVG}'
    convert_process = run(convert_command, shell=True)
    return convert_process

def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)

def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')

server = run(start_server())  
perf_process = record_perf(server.pid)

make_shots()

stop(server)
stop(perf_process, wait=True)

convert_process = convert_to_flamegraph()
stop(convert_process, wait=True)

time.sleep(1)
print('Job done')
