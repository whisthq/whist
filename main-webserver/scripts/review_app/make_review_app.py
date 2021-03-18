import argparse


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Review app utility.")

    parser.add_argument("--setup-branch", type=int, default=100, help="Number of tasks to run.")
    parser.add_argument(
        "--frac_cpu", type=float, default=0.1, help="Fraction of time spent doing CPU work."
    )
    parser.add_argument(
        "--task_time_ms", type=int, default=100, help="How long a task should take, in ms."
    )
    parser.add_argument(
        "--poll_freq", type=float, default=0.5, help="How often to poll for results, in sec."
    )
