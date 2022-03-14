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

    The function assumes that the pexpect_process process has already successfully established a SSH
    connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to interact
                        with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is ready to
                        execute a command
        ssh_cmd: The shell command to use to establish a SSH connection to the remote machine.
                        This is used if we need to reboot the machine.
        timeout_value: The amount of time to wait before timing out the attemps to gain a SSH connection
                        to the remote machine.
        logfile: The file (already opened) to use for logging the terminal output from the remote machine
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        pexpect_process: The Pexpect process to be used from now on to interact with the remote machine.
                        This is equal to the first argument if a reboot of the remote machine was not needed.
    """
    print("Running the host setup on the instance ...")
    command = "cd ~/whist/host-setup && ./setup_host.sh --localdevelopment | tee ~/host_setup.log"
    pexpect_process.sendline(command)
    host_setup_output = wait_until_cmd_done(
        pexpect_process, pexpect_prompt, running_in_ci, return_output=True
    )

    error_msg = "E: Could not get lock"
    dpkg_lock_issue = any(error_msg in item for item in host_setup_output if isinstance(item, str))
    if dpkg_lock_issue == 1:
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


def start_host_service_on_instance(pexpect_process, pexpect_prompt):
    """
    Run Whist's host service on a remote machine accessible via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a SSH connection
    to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to interact with
                        the remote machine
    Returns:
        None
    """
    print("Starting the host service on the EC2 instance...")
    command = "sudo rm -rf /whist && cd ~/whist/backend/services && make run_host_service | tee ~/host_service.log"
    pexpect_process.sendline(command)

    desired_output = "Entering event loop..."

    result = pexpect_process.expect(
        [desired_output, pexpect_prompt, pexpect.exceptions.TIMEOUT, pexpect.EOF]
    )

    # If the desired output does not get printed, handle potential host service startup issues.
    if result != 0:
        print("Host service failed to start! Check the logs for troubleshooting!")
        sys.exit(-1)

    print("Host service is ready!")


def build_server_on_instance(pexpect_process, pexpect_prompt, cmake_build_type, running_in_ci):
    """
    Build the Whist server (browsers/chrome mandelbox) on a remote machine accessible via a SSH
    connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to
                        interact with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when
                        it is ready to execute a command
        cmake_build_type: A string identifying whether to build the protocol in release,
                        debug, metrics, or any other Cmake build mode that will be introduced later.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    print(f"Building the server mandelbox in {cmake_build_type} mode ...")
    command = f"cd ~/whist/mandelboxes && ./build.sh browsers/chrome --{cmake_build_type} | tee ~/server_mandelbox_build.log"
    pexpect_process.sendline(command)
    wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci)
    print("Finished building the browsers/chrome (server) mandelbox on the EC2 instance")


def run_server_on_instance(pexpect_process):
    """
    Run the Whist server (browsers/chrome mandelbox) on a remote machine accessible via a SSH
    connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established
    a SSH connection to the host, that the Whist repository has already been cloned, and that
    the browsers/chrome mandelbox has already been built. Further, the host service must be
    already running on the remote machine.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to
                        interact with the remote machine

    Returns:
        server_docker_id: The Docker ID of the container running the Whist server
                        (browsers/chrome mandelbox) on the remote machine
        json_data: A dictionary containing the IP, AES KEY, and port mappings that are needed by
                        the client to successfully connect to the Whist server.
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
    Build the Whist protocol client (development/client mandelbox) on a remote machine accessible
    via a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host, and that the Whist repository has already been cloned.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to interact
                        with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is ready to
                        execute a command
        testing_time: The amount of time to leave the connection open between the client and the server
                        (when the client is started) before shutting it down
        cmake_build_type: A string identifying whether to build the protocol in release, debug, metrics,
                        or any other Cmake build mode that will be introduced later.
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        None
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
    Set up the network degradation conditions on the client instance. We apply the network settings
    on the client side to simulate the condition where the user is using Whist on a machine that
    is connected to the Internet through a unstable connection.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to
                        interact with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is
                        ready to execute a command
        network_conditions: The network conditions expressed as either 'normal' for no network
                        degradation or max_bandwidth, delay, packet drop percentage, with the
                        three values separated by commas and no space.
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
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

        ifconfig_output = wait_until_cmd_done(
            pexpect_process, pexpect_prompt, running_in_ci, return_output=True
        )

        blacklisted_expressions = [
            pexpect_prompt,
            "docker",
            "veth",
            "ifb",
            "ifconfig",
            "\\",
            "~",
            ";",
        ]
        network_devices = [
            x.replace(":", "")
            for x in ifconfig_output
            if not any(
                blacklisted_expression in x for blacklisted_expression in blacklisted_expressions
            )
        ]

        commands = []

        # Set up infrastructure to apply degradations on incoming traffic
        # (https://wiki.linuxfoundation.org/networking/netem#how_can_i_use_netem_on_incoming_traffic)
        commands.append("sudo modprobe ifb")
        commands.append("sudo ip link set dev ifb0 up")

        degradation_command = ""
        if net_delay != "none":
            degradation_command += f"delay {net_delay}ms "
        if pkt_drop_pctg != "none":
            degradation_command += f"loss {pkt_drop_pctg}% "
        if max_bandwidth != "none":
            degradation_command += f"rate {max_bandwidth}"

        for device in network_devices:
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
    Restore network conditions to factory ones. This function is idempotent and is safe to run even
    if no network degradation was initially applied.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used to
                        interact with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is
                        ready to execute a command
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    print("Restoring network conditions to normal on client!")
    # Get network interface names (excluding loopback)
    command = "sudo ifconfig -a | sed 's/[ ].*//;/^\(lo:\|\)$/d'"
    # Cannot use wait_until_cmd_done because we need to handle clase where ifconfig is not installed
    pexpect_process.sendline(command)

    ifconfig_output = wait_until_cmd_done(
        pexpect_process, pexpect_prompt, running_in_ci, return_output=True
    )

    # Since we use ifconfig to apply network degradations, if ifconfig is not installed, we know
    # that no network degradations have been applied to the machine.
    error_msg = "sudo: ifconfig: command not found"
    ifconfig_not_installed = any(
        error_msg in item for item in ifconfig_output if isinstance(item, str)
    )
    if ifconfig_not_installed:
        print(
            "ifconfig is not installed on the client instance, so we don't need to restore normal network conditions."
        )
        return

    # Get names of network devices
    blacklisted_expressions = [pexpect_prompt, "docker", "veth", "ifb", "ifconfig", "\\", "~", ";"]
    network_devices = [
        x.replace(":", "")
        for x in ifconfig_output
        if not any(
            blacklisted_expression in x for blacklisted_expression in blacklisted_expressions
        )
    ]

    commands = []

    for device in network_devices:
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
    Run the Whist dev client (development/client mandelbox) on a remote machine accessible via
    a SSH connection within a pexpect process.

    The function assumes that the pexpect_process process has already successfully established
    a SSH connection to the host, that the Whist repository has already been cloned, and that
    the browsers/chrome mandelbox has already been built. Further, the host service must be
    already running on the remote machine.

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used
                            to interact with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is
                            ready to execute a command
        simulate_scrolling: A boolean controlling whether the client should simulate scrolling
                            as part of the test.

    Returns:
        client_docker_id: The Docker ID of the container running the Whist dev client
                            (development/client mandelbox) on the remote machine
    """
    print("Running the dev client mandelbox, and connecting to the server!")
    command = f"cd ~/whist/mandelboxes && ./run.sh development/client --json-data='{json.dumps(json_data)}'"
    pexpect_process.sendline(command)

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
        wait_until_cmd_done(pexpect_process, ":/#", running_in_ci=True)

    return client_docker_id


def prune_containers_if_needed(pexpect_process, pexpect_prompt, running_in_ci):
    """
    Check whether the remote instance is running out of space (more specifically,
    check if >= 75 % of disk space is used). If the disk is getting full, free some space
    by pruning old Docker containers by running the command `docker system prune -af`

    Args:
        pexpect_process: The Pexpect process created with pexpect.spawn(...) and to be used
                            to interact with the remote machine
        pexpect_prompt: The bash prompt printed by the shell on the remote machine when it is
                            ready to execute a command
        running_in_ci: A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    # Check if we are running out of space
    pexpect_process.sendline("df -h | grep --color=never /dev/root")
    space_used_output = wait_until_cmd_done(
        pexpect_process, pexpect_prompt, running_in_ci, return_output=True
    )
    for line in reversed(space_used_output):
        if "/dev/root" in line:
            space_used_output = line.split()
            break
    space_used_pctg = int(space_used_output[-2][:-1])

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
    - installing the AWS CLI and setting up the AWS credentials
    - pruning the docker containers if we are running out of space on disk
    - cloning the current branch of the Whist repository if needed
    - launching the Whist host setup if needed
    - building the server mandelbox

    In case of error, this function makes the process running it exit with exitcode -1.

    Args:
        args_dict: A dictionary containing the configs needed to access the remote
                    machine and get a Whist server ready for execution

    Returns:
        None
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
        server_cmd, aws_timeout_seconds, server_log, pexpect_prompt_server, 5, running_in_ci
    )

    print("Configuring AWS credentials on server instance...")
    result = install_and_configure_aws(
        hs_process,
        pexpect_prompt_server,
        aws_timeout_seconds,
        running_in_ci,
        aws_credentials_filepath,
    )

    if not result:
        sys.exit(-1)

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
        - restoring the default network conditions, if some artificial network
          degradation was previously applied on the instance
        - installing the AWS CLI and setting up the AWS credentials
        - pruning the docker containers if we are running out of space on disk
        - cloning the current branch of the Whist repository if needed
        - launching the Whist host setup if needed
    - building the client mandelbox

    In case of error, this function makes the process running it exit with exitcode -1.

    Args:
        args_dict: A dictionary containing the configs needed to access the remote machine
                and get a Whist dev client ready for execution

    Returns:
        None
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
            client_cmd, aws_timeout_seconds, client_log, pexpect_prompt_client, 5, running_in_ci
        )

        # Restore network conditions in case a previous run failed / was canceled before restoring the normal conditions.
        restore_network_conditions_client(hs_process, pexpect_prompt_client, running_in_ci)

        print("Configuring AWS credentials on client instance...")
        result = install_and_configure_aws(
            hs_process,
            pexpect_prompt_client,
            aws_timeout_seconds,
            running_in_ci,
            aws_credentials_filepath,
        )
        if not result:
            sys.exit(-1)

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
        client_cmd, aws_timeout_seconds, client_log, pexpect_prompt_client, 5, running_in_ci
    )
    build_client_on_instance(
        client_pexpect_process, pexpect_prompt_client, testing_time, cmake_build_type, running_in_ci
    )
    client_pexpect_process.kill(0)

    client_log.close()


def shutdown_and_wait_server_exit(pexpect_process, exit_confirm_exp):
    """
    Initiate shutdown and wait for server exit to see if the server hangs or exits gracefully

    Args:
        pexpect_process: Server pexpect process - MUST BE AFTER DOCKER COMMAND WAS RUN - otherwise
                        behavior is undefined
        exit_confirm_exp: Target expression to expect on a graceful server exit

    Returns:
        server_has_exited: A boolean set to True if server has exited gracefully, false otherwise
    """
    # Shut down Chrome
    pexpect_process.sendline("pkill chrome")
    # We set running_in_ci=True because the Docker bash does not print in color
    # (check wait_until_cmd_done docstring for more details about handling color bash stdout)
    wait_until_cmd_done(pexpect_process, ":/#", running_in_ci=True)

    # Give WhistServer 20s to shutdown properly
    pexpect_process.sendline("sleep 20")
    wait_until_cmd_done(pexpect_process, ":/#", running_in_ci=True)

    # Check the log to see if WhistServer shut down gracefully or if there was a server hang
    pexpect_process.sendline("tail /var/log/whist/protocol-out.log")
    server_mandelbox_output = wait_until_cmd_done(
        pexpect_process, ":/#", running_in_ci=True, return_output=True
    )
    server_has_exited = any(
        exit_confirm_exp in item for item in server_mandelbox_output if isinstance(item, str)
    )

    # Kill tail process
    pexpect_process.sendcontrol("c")

    return server_has_exited
