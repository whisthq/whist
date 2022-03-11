#!/usr/bin/env python3

import pexpect
import json
import platform
import time

# Get tools to run operations on a dev instance via SSH
from dev_instance_tools import (
    reboot_instance,
    wait_until_cmd_done,
    attempt_ssh_connection,
    install_and_configure_aws,
    apply_dpkg_locking_fixup,
    clone_whist_repository_on_instance,
)


def run_host_setup_on_instance(
    pexpect_process, pexpect_prompt, ssh_cmd, timeout_value, logfile, running_in_ci
):
    """
    Run Whist's host setup on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        ssh_cmd (str): The shell command to use to establish a SSH connection to the remote machine. This is used if we need to reboot the machine.
        timeout_value (int): The amount of time to wait before timing out the attemps to gain a SSH connection to the remote machine.
        logfile (file object): The file (already opened) to use for logging the terminal output from the remote machine
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process to be used from now on to interact with the remote machine. This is equal to the first argument if a reboot of the remote machine was not needed.
    """

    print("Running the host setup on the instance ...")
    command = "cd ~/whist/host-setup && ./setup_host.sh --localdevelopment | tee ~/host_setup.log"
    pexpect_process.sendline(command)
    result = pexpect_process.expect([pexpect_prompt, "E: Could not get lock"])
    if platform.system() == "Darwin" or result == 1:
        pexpect_process.expect(pexpect_prompt)

    if result == 1:
        # If still getting lock issues, no alternative but to reboot
        print(
            "Running into severe locking issues (happens frequently), rebooting the instance and trying again!"
        )
        pexpect_process = reboot_instance(
            pexpect_process, ssh_cmd, timeout_value, logfile, pexpect_prompt, 5, running_in_ci
        )
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    print("Finished running the host setup script on the EC2 instance")
    return pexpect_process


def start_host_service_on_instance(pexpect_process):
    """
    Run Whist's host service on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
    """
    print("Starting the host service on the EC2 instance...")
    command = "sudo rm -rf /whist && cd ~/whist/backend/services && make run_host_service | tee ~/host_service.log"
    pexpect_process.sendline(command)
    pexpect_process.expect("Entering event loop...")
    print("Host service is ready!")


def build_server_on_instance(pexpect_process, pexpect_prompt, cmake_build_type, running_in_ci):
    """
    Build the Whist server (browsers/chrome mandelbox) on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        cmake_build_type (str): A string identifying whether to build the protocol in release, debug, metrics, or any other Cmake build mode that will be introduced later.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    """
    print(f"Building the server mandelbox in {cmake_build_type} mode ...")
    command = f"cd ~/whist/mandelboxes && ./build.sh browsers/chrome --{cmake_build_type} | tee ~/server_mandelbox_build.log"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    print("Finished building the browsers/chrome (server) mandelbox on the EC2 instance")


def run_server_on_instance(pexpect_process):
    """
    Run the Whist server (browsers/chrome mandelbox) on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host, that the Whist repository has already been cloned, and that the browsers/chrome mandelbox has already been built. Further, the host service must be already running on the remote machine.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine

    Return:
        server_docker_id (str): The Docker ID of the container running the Whist server (browsers/chrome mandelbox) on the remote machine
        json_data (dict): A dictionary containing the IP, AES KEY, and port mappings that are needed by the client to successfully connect to the Whist server.
    """
    command = "cd ~/whist/mandelboxes && ./run.sh browsers/chrome | tee ~/server_mandelbox_run.log"
    pexpect_process.sendline(command)
    # Need to wait for special mandelbox prompt ":/#". running_in_ci must always be set to True in this case.
    server_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", running_in_ci=True, return_output=True
    )
    server_docker_id = server_mandelbox_output[-2].replace(" ", "")
    print(f"Whist Server started on EC2 instance, on Docker container {server_docker_id}!")

    # Retrieve connection configs from server
    json_data = {}
    for line in server_mandelbox_output:
        if "linux/macos" in line:
            config_vals = line.strip().split()
            server_addr = config_vals[2]
            port_mappings = config_vals[3]
            # Remove leading '-p' chars
            port_mappings = port_mappings[2:].split(".")
            port_32262 = port_mappings[0].split(":")[1]
            port_32263 = port_mappings[1].split(":")[1]
            port_32273 = port_mappings[2].split(":")[1]
            aes_key = config_vals[5]
            json_data["dev_client_server_ip"] = server_addr
            json_data["dev_client_server_port_32262"] = port_32262
            json_data["dev_client_server_port_32263"] = port_32263
            json_data["dev_client_server_port_32273"] = port_32273
            json_data["dev_client_server_aes_key"] = aes_key
    return server_docker_id, json_data


def build_client_on_instance(
    pexpect_process, pexpect_prompt, testing_time, cmake_build_type, running_in_ci
):
    """
    Build the Whist protocol client (development/client mandelbox) on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        testing_time(int): The amount of time to leave the connection open between the client and the server (when the client is started) before shutting it down
        cmake_build_type (str): A string identifying whether to build the protocol in release, debug, metrics, or any other Cmake build mode that will be introduced later.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    """
    # Edit the run-whist-client.sh to make the client quit after the experiment is over
    print(f"Setting the experiment duration to {testing_time}s...")
    command = f"sed -i 's/timeout 240s/timeout {testing_time}s/g' ~/whist/mandelboxes/development/client/run-whist-client.sh"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)

    print(f"Building the dev client mandelbox in {cmake_build_type} mode ...")
    command = f"cd ~/whist/mandelboxes && ./build.sh development/client --{cmake_build_type} | tee ~/client_mandelbox_build.log"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    print("Finished building the dev client mandelbox on the EC2 instance")


def setup_network_conditions_client(
    pexpect_process, pexpect_prompt, network_conditions, running_in_ci
):
    """
    Set up the network degradation conditions on the client instance. We apply the network settings on the client side to simulate the condition where the user is using Whist on a machine that is connected to the Internet through a unstable connection.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        network_conditions (str): The network conditions expressed as either 'normal' for no network degradation or max_bandwidth, delay, packet drop percentage, with the three values separated by commas and no space.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Return:
        None

    """

    if network_conditions == "normal":
        print("Setting up client to run on a instance with no degradation on network conditions")
    else:

        # Apply conditions below only for values that are actually set
        if len(network_conditions.split(",")) != 3:
            print(
                "Network conditions passed in incorrect format. Setting up client to run on a instance with no degradation on network conditions"
            )
            return

        max_bandwidth, net_delay, pkt_drop_pctg = network_conditions.split(",")
        if max_bandwidth == "none" and net_delay == "none" and pkt_drop_pctg == "none":
            print(
                "Setting up client to run on a instance with no degradation on network conditions"
            )
            return
        else:
            print(
                "Setting up client to run on a instance with the following networking conditions:"
            )
            if max_bandwidth != "none":
                print(f"\t* Max bandwidth: {max_bandwidth}")
            if net_delay != "none":
                print(f"\t* Delay: {net_delay}ms")
            if pkt_drop_pctg != "none":
                print(f"\t* Packet drop rate: {pkt_drop_pctg}")

        # Install ifconfig
        command = "sudo apt-get install -y net-tools"
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
        # Get network interface names (excluding loopback)
        command = "sudo ifconfig -a | sed 's/[ ].*//;/^\(lo:\|\)$/d'"
        pexpect_process.sendline(command)
        # Since we are grabbing the output, running_in_ci must always be set to True in this case.
        ifconfig_output = wait_until_cmd_done(
            pexpect_process, pexpect_prompt, running_in_ci=True, return_output=True
        )

        ifconfig_output = [
            x.replace(":", "")
            for x in ifconfig_output
            if "docker" not in x
            and "veth" not in x
            and "ifb" not in x
            and "sudo ifconfig -a" not in x
            and pexpect_prompt not in x
        ]

        commands = []

        # Set up infrastructure to apply degradations on incoming traffic (https://wiki.linuxfoundation.org/networking/netem#how_can_i_use_netem_on_incoming_traffic)
        commands.append("sudo modprobe ifb")
        commands.append("sudo ip link set dev ifb0 up")

        degradation_command = ""
        if net_delay != "none":
            degradation_command += f"delay {net_delay}ms "
        if pkt_drop_pctg != "none":
            degradation_command += f"loss {pkt_drop_pctg}% "
        if max_bandwidth != "none":
            degradation_command += f"rate {max_bandwidth}"

        for device in ifconfig_output:
            print(f"Applying network degradation to device {device}")
            # add devices to delay incoming packets
            commands.append(f"sudo tc qdisc add dev {device} ingress")
            commands.append(
                f"sudo tc filter add dev {device} parent ffff: protocol ip u32 match u32 0 0 flowid 1:1 action mirred egress redirect dev ifb0"
            )

            # Set outbound degradations
            command = f"sudo tc qdisc add dev {device} root netem "
            command += degradation_command
            commands.append(command)

        # Set inbound degradations
        command = "sudo tc qdisc add dev ifb0 root netem "
        command += degradation_command
        commands.append(command)

        # Execute all commands:
        for command in commands:
            pexpect_process.sendline(command)
            wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)


def restore_network_conditions_client(pexpect_process, pexpect_prompt, running_in_ci):
    """
    Restore network conditions to factory ones. This function is idempotent and is safe to run even if no network degradation was initially applied.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is ready to execute a command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Return:
        None

    """
    print("Restoring network conditions to normal on client!")
    # Get network interface names (excluding loopback)
    command = "sudo ifconfig -a | sed 's/[ ].*//;/^\(lo:\|\)$/d'"
    # Cannot use wait_until_cmd_done because we need to handle clase where ifconfig is not installed
    pexpect_process.sendline(command)

    # Since we are grabbing the output, running_in_ci must always be set to True in this case.
    ifconfig_output = wait_until_cmd_done(
        pexpect_process, pexpect_prompt, running_in_ci=True, return_output=True
    )
    # Since we use ifconfig to apply network degradations, if ifconfig is not installed, we know that no network degradations have been applied to the machine.
    for line in ifconfig_output:
        if "sudo: ifconfig: command not found" in line:
            print(
                "ifconfig is not installed on the client instance, so we don't need to restore normal network conditions."
            )
            return

    # If ifconfig is installed, restore default network conditions (the code below is idempotent, so we don't need to check whether network degradations exist)
    ifconfig_output = [
        x.replace("\r", "").replace(":", "")
        for x in ifconfig_output
        if "docker" not in x
        and "veth" not in x
        and "ifb" not in x
        and "sudo ifconfig -a" not in x
        and pexpect_prompt not in x
    ]

    commands = []

    for device in ifconfig_output:
        print(f"Restoring normal network conditions on device {device}")
        # Inbound degradations
        commands.append(f"sudo tc qdisc del dev {device} handle ffff: ingress")
        # Outbound degradations
        commands.append(f"sudo tc qdisc del dev {device} root netem")

    commands.append("sudo modprobe -r ifb")

    # Execute all commands:
    for command in commands:
        pexpect_process.sendline(command)
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)


def run_client_on_instance(pexpect_process, json_data, simulate_scrolling):
    """
    Run the Whist dev client (development/client mandelbox) on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection to the host, that the Whist repository has already been cloned, and that the browsers/chrome mandelbox has already been built. Further, the host service must be already running on the remote machine.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to interact with the remote machine
        json_data (dict): A dictionary containing the IP, AES KEY, and port mappings that are needed by the client to successfully connect to the Whist server.

    Return:
        client_docker_id (str): The Docker ID of the container running the Whist dev client (development/client mandelbox) on the remote machine

    """
    print("Running the dev client mandelbox, and connecting to the server!")
    command = f"cd ~/whist/mandelboxes && ./run.sh development/client --json-data='{json.dumps(json_data)}'"
    pexpect_process.sendline(command)
    pexpect_process.expect(":/#")

    # Need to wait for special mandelbox prompt ":/#". running_in_ci must always be set to True in this case.
    client_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", running_in_ci=True, return_output=True
    )
    client_docker_id = client_mandelbox_output[-2].replace(" ", "")
    print(f"Whist dev client started on EC2 instance, on Docker container {client_docker_id}!")

    if simulate_scrolling:
        # Sleep for sometime so that the webpage can load.
        time.sleep(5)
        print("Simulating the mouse scroll events in the client")
        command = "python3 /usr/share/whist/mouse_events.py"
        pexpect_process.sendline(command)
        pexpect_process.expect(":/#")

    return client_docker_id


def prune_containers_if_needed(pexpect_process, pexpect_prompt, running_in_ci):
    # Check if we are running out of space
    pexpect_process.sendline("df -h | grep --color=never /dev/root")
    # Since we are grabbing the output, running_in_ci must always be set to True in this case.
    space_used_output = wait_until_cmd_done(
        pexpect_process, pexpect_prompt, running_in_ci=True, return_output=True
    )

    for line in reversed(space_used_output):
        if "/dev/root" in line:
            space_used_output = line.split()
            break
    space_used_pctg = int(space_used_output[-2][:-1])

    if not running_in_ci:
        pexpect_process.expect(pexpect_prompt)

    # Clean up space on the instance by pruning all Docker containers if the disk is 75% (or more) full
    if space_used_pctg >= 75:
        print(f"Disk is {space_used_pctg}% full, pruning the docker containers...")
        pexpect_process.sendline("docker system prune -af")
        wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    else:
        print(f"Disk is {space_used_pctg}% full, no need to prune containers.")


def server_setup_process(args_dict):
    """
    Prepare a remote Ubuntu machine to host the Whist server. This entails:
    - opening a SSH connection to the machine
    - setting up the AWS credentials
    - cloning the current branch of the Whist repository
    - launching the Whist host setup (after having applied a small dpkg-related fixup)
    - building the server mandelbox

    Args:
        args_dict (dict): A dictionary containing the configs needed to access the remote machine and get a Whist server ready for execution

    """
    username = args_dict["username"]
    server_hostname = args_dict["server_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    aws_timeout_seconds = args_dict["aws_timeout_seconds"]
    server_log_filepath = args_dict["server_log_filepath"]
    pexpect_prompt_server = args_dict["pexpect_prompt_server"]
    github_token = args_dict["github_token"]
    aws_credentials_filepath = args_dict["aws_credentials_filepath"]
    cmake_build_type = args_dict["cmake_build_type"]
    running_in_ci = args_dict["running_in_ci"]
    skip_git_clone = args_dict["skip_git_clone"]
    skip_host_setup = args_dict["skip_host_setup"]

    server_log = open(server_log_filepath, "w")

    # Initiate the SSH connections with the instance
    print("Initiating the SETUP ssh connection with the server AWS instance...")
    server_cmd = f"ssh {username}@{server_hostname} -i {ssh_key_path}"
    hs_process = attempt_ssh_connection(
        server_cmd, aws_timeout_seconds, server_log, pexpect_prompt_server, 5
    )
    if not running_in_ci:
        hs_process.expect(pexpect_prompt_server)

    print("Configuring AWS credentials on server instance...")
    install_and_configure_aws(
        hs_process,
        pexpect_prompt_server,
        aws_timeout_seconds,
        running_in_ci,
        aws_credentials_filepath,
    )

    prune_containers_if_needed(hs_process, pexpect_prompt_server, running_in_ci)

    if skip_git_clone == "false":
        clone_whist_repository_on_instance(
            github_token, hs_process, pexpect_prompt_server, running_in_ci
        )
    else:
        print("Skipping git clone whisthq/whist repository on server instance.")

    if skip_host_setup == "false":
        # 1- Reboot instance for extra robustness
        hs_process = reboot_instance(
            hs_process,
            server_cmd,
            aws_timeout_seconds,
            server_log,
            pexpect_prompt_server,
            5,
            running_in_ci,
        )

        # 2 - Fix DPKG issue in case it comes up
        apply_dpkg_locking_fixup(hs_process, pexpect_prompt_server, running_in_ci)

        # 3- run host-setup
        hs_process = run_host_setup_on_instance(
            hs_process,
            pexpect_prompt_server,
            server_cmd,
            aws_timeout_seconds,
            server_log,
            running_in_ci,
        )
    else:
        print("Skipping host setup on server instance.")

    # 2- reboot and wait for it to come back up
    print("Rebooting the server EC2 instance (required after running the host setup)...")
    hs_process = reboot_instance(
        hs_process,
        server_cmd,
        aws_timeout_seconds,
        server_log,
        pexpect_prompt_server,
        5,
        running_in_ci,
    )

    # 3- Build the protocol server
    build_server_on_instance(hs_process, pexpect_prompt_server, cmake_build_type, running_in_ci)

    hs_process.kill(0)

    server_log.close()


def client_setup_process(args_dict):
    """
    Prepare a remote Ubuntu machine to host the Whist dev client. This entails:
    - opening a SSH connection to the machine
    - if the client is not running on the same machine as the server:
        - setting up the AWS credentials
        - cloning the current branch of the Whist repository
        - launching the Whist host setup (after having applied a small dpkg-related fixup)
    - building the client mandelbox

    Args:
        args_dict (dict): A dictionary containing the configs needed to access the remote machine and get a Whist dev client ready for execution

    """
    username = args_dict["username"]
    client_hostname = args_dict["client_hostname"]
    ssh_key_path = args_dict["ssh_key_path"]
    aws_timeout_seconds = args_dict["aws_timeout_seconds"]
    client_log_filepath = args_dict["client_log_filepath"]
    pexpect_prompt_client = args_dict["pexpect_prompt_client"]
    github_token = args_dict["github_token"]
    use_two_instances = args_dict["use_two_instances"]
    testing_time = args_dict["testing_time"]
    aws_credentials_filepath = args_dict["aws_credentials_filepath"]
    cmake_build_type = args_dict["cmake_build_type"]
    running_in_ci = args_dict["running_in_ci"]
    skip_git_clone = args_dict["skip_git_clone"]
    skip_host_setup = args_dict["skip_host_setup"]

    client_log = open(client_log_filepath, "w")

    client_cmd = f"ssh {username}@{client_hostname} -i {ssh_key_path}"

    # If we are using the same instance for client and server, all the operations in this if-statement have already been done by server_setup_process
    if use_two_instances:
        # Initiate the SSH connections with the client instance
        print("Initiating the SETUP ssh connection with the client AWS instance...")
        hs_process = attempt_ssh_connection(
            client_cmd, aws_timeout_seconds, client_log, pexpect_prompt_client, 5
        )
        if not running_in_ci:
            hs_process.expect(pexpect_prompt_client)

        # Restore network conditions in case a previous run failed / was canceled before restoring the normal conditions.
        restore_network_conditions_client(hs_process, pexpect_prompt_client, running_in_ci)

        print("Configuring AWS credentials on client instance...")
        install_and_configure_aws(
            hs_process,
            pexpect_prompt_client,
            aws_timeout_seconds,
            running_in_ci,
            aws_credentials_filepath,
        )

        prune_containers_if_needed(hs_process, pexpect_prompt_client, running_in_ci)

        if skip_git_clone == "false":
            clone_whist_repository_on_instance(
                github_token, hs_process, pexpect_prompt_client, running_in_ci
            )
        else:
            print("Skipping git clone whisthq/whist repository on client instance.")

        if skip_host_setup == "false":
            # 1- Reboot instance for extra robustness
            hs_process = reboot_instance(
                hs_process,
                client_cmd,
                aws_timeout_seconds,
                client_log,
                pexpect_prompt_client,
                5,
                running_in_ci,
            )

            # 2 - Fix DPKG issue in case it comes up
            apply_dpkg_locking_fixup(hs_process, pexpect_prompt_client, running_in_ci)

            # 3- run host-setup
            hs_process = run_host_setup_on_instance(
                hs_process,
                pexpect_prompt_client,
                client_cmd,
                aws_timeout_seconds,
                client_log,
                running_in_ci,
            )
        else:
            print("Skipping host setup on server instance.")

        # 2- reboot and wait for it to come back up
        print("Rebooting the client EC2 instance (required after running the host setup)...")
        hs_process = reboot_instance(
            hs_process,
            client_cmd,
            aws_timeout_seconds,
            client_log,
            pexpect_prompt_client,
            5,
            running_in_ci,
        )

        hs_process.kill(0)

    # 6- Build the dev client
    print("Initiating the BUILD ssh connection with the client AWS instance...")
    client_pexpect_process = attempt_ssh_connection(
        client_cmd, aws_timeout_seconds, client_log, pexpect_prompt_client, 5
    )
    build_client_on_instance(
        client_pexpect_process, pexpect_prompt_client, testing_time, cmake_build_type, running_in_ci
    )
    client_pexpect_process.kill(0)

    client_log.close()


def shutdown_and_wait_server_exit(pexpect_process, exit_confirm_exp, timeout_value=20):
    """
    Initiate shutdown and wait for server exit by
    - killing chrome application
    - tailing out log output until one of the two happens
        - exit line is detected
        - timeout occurs

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): Server pexpect process - MUST BE AFTER DOCKER COMMAND WAS RUN - otherwise behavior is undefined
        exit_confirm_exp (str): Target expression to expect on a graceful server exit

    Return:
        server_has_exited (bool): True if server has exited gracefully, false otherwise

    """

    pexpect_process.sendline("sleep 1")
    pexpect_process.expect(":/#")
    pexpect_process.sendline("pkill chrome")
    pexpect_process.expect(":/#")
    pexpect_process.sendline("tail -f /var/log/whist/protocol-out.log")

    try:
        pexpect_process.expect(exit_confirm_exp, timeout=timeout_value)
        server_has_exited = True
    except pexpect.exceptions.TIMEOUT:
        server_has_exited = False

    # Kill tail process
    pexpect_process.sendcontrol("c")

    return server_has_exited
