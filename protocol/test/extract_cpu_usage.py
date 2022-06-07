#!/usr/bin/env python3

import os

if __name__ == "__main__":

    # Check that server/server.log and client/client.log exist
    # Extract all lines containing SERVER_CPU_USAGE or CLIENT_CPU_USAGE

    # Access last folder in perf_logs
    logs_root_dir = os.path.join(".", "perf_logs")
    if not os.path.isdir(logs_root_dir):
        print(f"Error: {logs_root_dir} does not exit")
        sys.exit(-1)

    folder_names = sorted(os.listdir(logs_root_dir))
    if len(folder_names) < 1:
        print(f"Error: {logs_root_dir} is empty")
        sys.exit(-1)

    last_folder = folder_names[-1]
    last_folder = os.path.join(logs_root_dir, last_folder)
    print(f"Extracting CPU usage from logs in folder {last_folder}!")

    client_log_filepath = os.path.join(last_folder, "client", "client.log")
    server_log_filepath = os.path.join(last_folder, "server", "server.log")
    if not os.path.isfile(client_log_filepath):
        print(f"Error: Client log file {client_log_filepath} does not exist!")
        sys.exit(-1)
    if not os.path.isfile(server_log_filepath):
        print(f"Error: Server log file {server_log_filepath} does not exist!")
        sys.exit(-1)

    client_cpu_usage = 0.0
    client_cpu_count = 0

    with open(client_log_filepath, "r") as client_file:
        for line in client_file.readlines():
            if "CLIENT_CPU_USAGE" in line:
                cleaned_line = line.strip().split()
                cpu_usage = float(cleaned_line[3].replace(",", ""))
                count = int(cleaned_line[5])
                # print(cpu_usage, count)
                client_cpu_usage += cpu_usage * count
                client_cpu_count += count
    print(f"Average Client CPU usage: {client_cpu_usage/client_cpu_count:.3}%")

    server_cpu_usage = 0.0
    server_cpu_count = 0

    with open(server_log_filepath, "r") as server_file:
        for line in server_file.readlines():
            if "SERVER_CPU_USAGE" in line:
                cleaned_line = line.strip().split()
                cpu_usage = float(cleaned_line[3].replace(",", ""))
                count = int(cleaned_line[5])
                # print(cpu_usage, count)
                server_cpu_usage += cpu_usage * count
                server_cpu_count += count
    print(f"Average Server CPU usage: {server_cpu_usage/server_cpu_count:.3}%")
