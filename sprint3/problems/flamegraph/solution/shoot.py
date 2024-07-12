import argparse
import subprocess
import time
import random
import shlex
import os

RANDOM_LIMIT = 1000
SEED = 123456789
random.seed(SEED)

AMMUNITION = [
    'localhost:8080/api/v1/maps/map1',
    'localhost:8080/api/v1/maps'
]

SHOOT_COUNT = 100
COOLDOWN = 0.1
FLAMEGRAPH_REPO = 'https://github.com/brendangregg/FlameGraph.git'
FLAMEGRAPH_DIR = 'FlameGraph'


def start_server():
    parser = argparse.ArgumentParser()
    parser.add_argument('server', type=str)
    return parser.parse_args().server


def run(command, output=None):
    process = subprocess.Popen(shlex.split(command), stdout=output, stderr=subprocess.DEVNULL)
    return process


def stop(process, wait=False):
    if process.poll() is None and wait:
        process.wait()
    process.terminate()


def shoot(ammo):
    hit = run('curl ' + ammo, output=subprocess.DEVNULL)
    time.sleep(COOLDOWN)
    stop(hit, wait=True)


def make_shots():
    for _ in range(SHOOT_COUNT):
        ammo_number = random.randrange(RANDOM_LIMIT) % len(AMMUNITION)
        shoot(AMMUNITION[ammo_number])
    print('Shooting complete')


def clone_flamegraph_repo():
    if not os.path.exists(FLAMEGRAPH_DIR):
        subprocess.run(['git', 'clone', FLAMEGRAPH_REPO])
    else:
        print(f"{FLAMEGRAPH_DIR} already exists.")


def main():
    server_command = start_server()
    
    # Клонирование репозитория FlameGraph, если он еще не был клонирован
    clone_flamegraph_repo()
    
    # Запуск сервера
    server_process = run(server_command)
    
    # Запуск perf record для записи трассировки функций
    perf_command = "perf record -o perf.data -p {}".format(server_process.pid)
    perf_process = run(perf_command)
    
    # Выполнение обстрелов запросами
    make_shots()
    
    # Остановка perf record
    stop(perf_process, wait=True)
    
    # Остановка сервера
    stop(server_process)
    
    # Построение флеймграфа
    flamegraph_command = (
        "perf script -i perf.data | "
        "./FlameGraph/stackcollapse-perf.pl | "
        "./FlameGraph/flamegraph.pl > graph.svg"
    )
    subprocess.run(flamegraph_command, shell=True)
    
    print('Job done')


if __name__ == '__main__':
    main()