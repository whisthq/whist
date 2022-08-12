#!/usr/bin/env python3

import os, sys, re, time
import pexpect

from e2e_helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
)
from e2e_helpers.common.constants import (
    username,
    running_in_ci,
    pexpect_max_retries,
    ssh_connection_retries,
    aws_timeout_seconds,
)

from e2e_helpers.common.timestamps_and_exit_tools import (
    printformat,
    exit_with_error,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


class RemoteExecutor:
    def __init__(self, public_ip, private_ip, ssh_key_path, log_filename):
        self.public_ip = public_ip
        self.ssh_command = f"ssh {username}@{public_ip} -i {ssh_key_path} -o TCPKeepAlive=yes -o ServerAliveInterval=15"
        self.pexpect_prompt = f"{username}@ip-{private_ip}"
        self.mandelbox_prompt = ":/#"
        self.mandelbox_mode = False
        self.logfile = open(log_filename, "a")
        self.prompt_printed_twice = not running_in_ci
        self.__connect_to_instance()

    def __connect_to_instance(self, verbose=False):
        """
        Attempt to establish a SSH connection to a remote machine. It is normal for the function to
        need several attempts before successfully opening a SSH connection to the remote machine.

        Args:
            ssh_command (str): The shell command to use to establish a SSH connection to the remote machine.
            log_file_handle (file): The file (already opened) to use for logging the terminal output from the
                            remote machine
            pexpect_prompt (file): The bash prompt printed by the shell on the remote machine when it is ready
                            to execute a command

        Returns:
            On success:
                pexpect_process (pexpect.pty_spawn.spawn): The Pexpect process to be used from now on to interact
                            with the remote machine.
            On failure:
                None and exit with exitcode -1
        """
        for retries in range(ssh_connection_retries):
            self.pexpect_process = pexpect.spawn(
                self.ssh_command, timeout=aws_timeout_seconds, logfile=self.logfile.buffer
            )
            result_index = self.pexpect_process.expect(
                [
                    "Connection refused",
                    "Are you sure you want to continue connecting (yes/no/[fingerprint])?",
                    self.pexpect_prompt,
                    pexpect.EOF,
                    pexpect.TIMEOUT,
                ]
            )
            if result_index == 0:
                # If the connection was refused, sleep for 30s and then retry
                # (unless we exceeded the max number of retries)
                if verbose:
                    print(
                        f"\tSSH connection refused by host (retry {retries + 1}/{ssh_connection_retries})"
                    )
                    self.pexpect_process.kill(0)
                time.sleep(30)
            elif result_index == 1 or result_index == 2:
                if verbose:
                    print(f"SSH connection established with EC2 instance!")
                # Confirm that we want to connect, if asked
                if result_index == 1:
                    self.pexpect_process.sendline("yes")
                    self.pexpect_process.expect(self.pexpect_prompt)
                # Wait for one more print of the prompt if required (ssh shell prompts are printed twice to stdout outside of CI)
                if self.prompt_printed_twice:
                    self.pexpect_process.expect(self.pexpect_prompt)
                # Success!
                return
            elif result_index >= 3:
                # If the connection timed out, sleep for 30s and then retry
                # (unless we exceeded the max number of retries)
                if verbose:
                    print(
                        f"\tSSH connection timed out (retry {retries + 1}/{ssh_connection_retries})"
                    )
                self.pexpect_process.kill(0)
                time.sleep(30)
        # Give up if the SSH connection was refused too many times.
        exit_with_error(
            f"SSH connection refused by host {ssh_connection_retries} times. Giving up now.",
            timestamps=None,
        )

    def reboot_instance(self):
        """
        Reboot a remote machine and establish a new SSH connection after the machine comes back up.

        Args:
            pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process created with pexpect.spawn(...) and to
                                                        be used to interact with the remote machine
            ssh_cmd (str):  The shell command to use to establish a new SSH connection to the remote machine after
                            the current connection is broken by the reboot.
            log_file_handle (file): The file (already opened) to use for logging the terminal output from the remote
                                    machine
            pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is ready to
                                    execute a command

        Returns:
            pexpect_process (pexpect.pty_spawn.spawn):  The new Pexpect process created with pexpect.spawn(...) and to
                                                        be used to interact with the remote machine after the reboot
        """
        # Trigger the reboot
        self.pexpect_process.sendline("sudo reboot")
        self.pexpect_process.kill(0)
        # Connect again
        self.__connect_to_instance()
        print("Reboot complete")

    def expression_in_pexpect_output(self, expression):
        """
        This function is used to search whether the output of a pexpect command contains an expression of interest

        Args:
            expression (string): This is the expression we are searching for
            pexpect_output (list):  the stdout output of the pexpect command, with one entry for each line of the
                                    original output.

        Returns:
            A boolean indicating whether the expression was found.
        """
        return any(expression in item for item in self.pexpect_output if isinstance(item, str))

    def set_mandelbox_mode(self, value=True):
        self.mandelbox_mode = value

    def __remote_exec(self, command, description="command", max_retries=pexpect_max_retries):
        result = 0
        for i in range(max_retries):
            ssh_connection_broken_msg1 = "client_loop: send disconnect: Broken pipe"
            ssh_connection_broken_msg2 = f"Connection to {self.public_ip} closed by remote host."
            ssh_connection_broken_msg3 = f"Connection to {self.public_ip} closed."

            self.pexpect_process.sendline(command)
            result = self.pexpect_process.expect(
                [
                    self.pexpect_prompt,
                    self.mandelbox_prompt,
                    ssh_connection_broken_msg1,
                    ssh_connection_broken_msg2,
                    ssh_connection_broken_msg3,
                    pexpect.exceptions.TIMEOUT,
                    pexpect.EOF,
                ]
            )
            if result == 0 or result == 1:
                ansi_escape = re.compile(r"(\x9B|\x1B\[)[0-?]*[ -/]*[@-~]")
                # Clean the stdout output and save it in a list, one line per element. We need to do this before calling
                # expect again (if we are in a situation that requires it), otherwise the output will be overwritten.
                self.pexpect_output = [
                    ansi_escape.sub("", line.replace("\r", "").replace("\n", ""))
                    for line in self.pexpect_process.before.decode("utf-8").strip().split("\n")
                ]
                if result == 0:
                    if self.prompt_printed_twice:
                        # Colored/formatted stdout (such as the bash prompt) is sometimes duplicated when piped to a pexpect process.
                        # This is likely a platform-dependent behavior and does not occur when running this script in CI, or when running
                        # a command on nested bash shells on remote instances (e.g. the bash script used to run commands on a docker
                        # container). If we are in a situation where the prompt is printed twice, we need to wait for the duplicated
                        # prompt to be printed before moving forward, otherwise the duplicated prompt will interfere with the next
                        # command that we send.
                        self.pexpect_process.expect(self.pexpect_prompt)
                    if self.mandelbox_mode:
                        printformat(
                            f"Warning: `{description}` did not start mandelbox properly or the shell attached to it crashed",
                            "yellow",
                        )
                    else:
                        break
                else:
                    if not self.mandelbox_mode:
                        printformat(
                            f"Warning: `{description}` did not exit the shell attached to a mandelbox",
                            "yellow",
                        )
                    else:
                        break
            elif result == 2 or result == 3 or result == 4:
                if not self.mandelbox_mode:
                    printformat("Warning: Remote instance dropped the SSH connection", "yellow")
                    self.__connect_to_instance()
                else:
                    exit_with_error(
                        f"Error: Remote instance dropped the SSH connection while shell was attached to a mandelbox."
                    )
            elif result == 5:
                printformat(f"Warning: {description} timed out!", "yellow")
            elif result == 6:
                exit_with_error(f"Warning: {description} got an EOF before completing.")
        return result == self.mandelbox_mode

    def get_exit_code(self):
        previous_cmd_output = self.pexpect_output
        exit_code = 0
        success = self.__remote_exec("echo $?", description="getting exit code", max_retries=1)
        if success:
            filtered_output = [
                x
                for x in self.pexpect_output
                if "~" not in x
                and "\\" not in x
                and "?" not in x
                and ";" not in x
                and self.pexpect_prompt not in x
            ]
            if len(filtered_output) == 0 or not filtered_output[-1].isnumeric():
                exit_code = -1
            else:
                exit_code = int(filtered_output[-1])
        else:
            # If we get a pexpect error, there is no way to retrieve the exit code of the previous command, so give it the benefit of the
            # doubt and assume it succeeded.
            pass
        self.pexpect_output = previous_cmd_output
        return exit_code

    def run_command(
        self,
        command,
        description="command",
        quiet=False,
        max_retries=1,
        success_message=None,
        ignore_exit_codes=False,
        errors_to_handle=[],
    ):
        """
        This function is a wrapper around remote_exec, running commands remotely, handling application-specific errors and
        returning the output and exit codes if needed. The original command, together with any retries or relevant corrective
        actions are executed with remote_exec.
        """

        if description != "command" and not quiet:
            print(description)

        for i in range(max_retries):
            success = self.__remote_exec(command, description)
            if not (self.mandelbox_mode or ignore_exit_codes):
                exit_code = self.get_exit_code()
                if exit_code:
                    printformat(f"Warning: {description} got exit code: {exit_code}", "yellow")
                success *= exit_code == 0

            # Check against additional errors (if specified), and apply relevant corrective actions
            corrective_actions = []
            for error in errors_to_handle:
                error_stdout_message, message_for_user, corrective_action = error
                if self.expression_in_pexpect_output(error_stdout_message):
                    success = False
                    if corrective_action:
                        corrective_actions.append(corrective_action)
                        printformat(f"Warning: {message_for_user}", "yellow")
                    else:
                        exit_with_error(f"Error: {message_for_user}")
            for cmd in set(corrective_actions):
                self.__remote_exec(cmd)

            # If the command succeded, return without raising errors
            if success_message is not None:
                # Finding the success message (if specified) in the output means the command succeeded
                if self.expression_in_pexpect_output(success_message):
                    return
            if success:
                return

        exit_with_error(f"Error: {description} failed ({max_retries} time(s))!")

    def destroy(self):
        self.pexpect_process.kill(0)
        self.logfile.close()
