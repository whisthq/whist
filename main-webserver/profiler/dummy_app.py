import time
import os


def run_profiler_and_loop_forever():
    os.system("python profiler.py")

    while True:
        time.sleep(1000)


if __name__ == "__main__":
    run_profiler_and_loop_forever()
