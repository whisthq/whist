#!/usr/bin/env python3

import os, sys

from e2e_helpers.common.ssh_tools import (
    wait_until_cmd_done,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def setup_artificial_network_conditions(
    pexpect_process, pexpect_prompt, network_conditions, running_in_ci
):
    """
    Set up the network degradation conditions on the client instance. We apply the network settings
    on the client side to simulate the condition where the user is using Whist on a machine that
    is connected to the Internet through an unstable connection.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to
                        be used to interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is
                        ready to execute a command
        network_conditions (str): The network conditions expressed as either 'normal' for no network
                        degradation or max_bandwidth, delay, packet drop percentage, with the
                        three values separated by commas and no space.
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        None
    """
    if network_conditions == "normal":
        print("Setting up client to run on a instance with no degradation on network conditions")
    else:
        # Apply conditions below only for values that are actually set
        if len(network_conditions.split(",")) != 3:
            print(
                "Network conditions passed in incorrect format. Setting up client to run on a instance with no degradation \
                on network conditions"
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
            # Add devices to delay incoming packets
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


def restore_network_conditions(pexpect_process, pexpect_prompt, running_in_ci):
    """
    Restore network conditions to factory ones. This function is idempotent and is safe to run even
    if no network degradation was initially applied.

    The function assumes that the pexpect_process process has already successfully established a
    SSH connection to the host

    Args:
        pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process created with pexpect.spawn(...) and to be used to
                         interact with the remote machine
        pexpect_prompt (str): The bash prompt printed by the shell on the remote machine when it is
                        ready to execute a command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

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
