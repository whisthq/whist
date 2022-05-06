#!/usr/bin/env python3

import os, sys, time
import pexpect

from helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


def wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci, return_output=False):
    """
    Wait until the currently-running command on a remote machine finishes its execution on the shell
    monitored to by a pexpect process.

    If the machine running this script is not a CI runner, handling color/formatted stdout on the remote
    machine will require special care. In particular, the shell prompt will be printed twice, and we have
    to wait for that to happen before sending another command.

    This function also has the option to return the shell stdout in a list of strings format.

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process monitoring the execution of the process
                                                    on the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is ready to
                                execute a new command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI
        return_output (bool): A boolean controlling whether to return the stdout output in a list of strings format

    Returns:
        On Success:
            pexpect_output (list):  the stdout output of the command, with one entry for each line of the original
                                    output. If return_output=False, pexpect_output is set to None
        On Failure:
            None and exit with exitcode -1
    """
    result = pexpect_process.expect([pexpect_prompt, pexpect.exceptions.TIMEOUT, pexpect.EOF])

    # Handle timeout and error cases
    if result == 1:
        exit_with_error("Error: pexpect process timed out! Check the logs for troubleshooting.")
    elif result == 2:
        exit_with_error(
            "Error: pexpect process encountered an unexpected exception! Check the logs for troubleshooting."
        )

    # Clean stdout output and save it in a list, one line per element. We need to do this before calling expect
    # again (if we are not in CI), otherwise the output will be overwritten.
    pexpect_output = None
    if return_output:
        pexpect_output = [
            line.replace("\r", "").replace("\n", "")
            for line in pexpect_process.before.decode("utf-8").strip().split("\n")
        ]

    # Colored/formatted stdout (such as the bash prompt) is duplicated when piped to a pexpect process.
    # This is likely a platform-dependent behavior and does not occur when running this script in CI. If
    # we are not in CI, we need to wait for the duplicated prompt to be printed before moving forward, otherwise
    # the duplicated prompt will interfere with the next command that we send.
    if not running_in_ci:
        pexpect_process.expect(pexpect_prompt)

    return pexpect_output


def expression_in_pexpect_output(expression, pexpect_output):
    """
    This function is used to search whether the output of a pexpect command contains an expression of interest

    Args:
        expression (string): This is the expression we are searching for
        pexpect_output (list):  the stdout output of the pexpect command, with one entry for each line of the
                                original output.

    Returns:
        A boolean indicating whether the expression was found.
    """
    return any(expression in item for item in pexpect_output if isinstance(item, str))


def get_command_exit_code(pexpect_process, pexpect_prompt, running_in_ci):
    """
    This function is used to get the exit code of the last command that was run on a shell

    Args:
        pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process monitoring the execution of the process
                                                    on the remote machine
        pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is ready to
                                execute a new command
        running_in_ci (bool): A boolean indicating whether this script is currently running in CI

    Returns:
        exit_code (int):    The exit code of the last command that ran in the shell
    """
    pexpect_process.sendline("echo $?")
    output = wait_until_cmd_done(pexpect_process, pexpect_prompt, running_in_ci, return_output=True)
    return int(output[1])
