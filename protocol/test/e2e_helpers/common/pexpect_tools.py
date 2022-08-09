#!/usr/bin/env python3

import os, sys, re, time
import pexpect

from e2e_helpers.common.timestamps_and_exit_tools import (
    exit_with_error,
)
from e2e_helpers.common.constants import (
    running_in_ci,
    pexpect_max_retries,
    username,
)
from e2e_helpers.common.ssh_tools import (
    attempt_ssh_connection,
)
from e2e_helpers.common.timestamps_and_exit_tools import (
    printformat,
    exit_with_error,
)

# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))


class RemoteExecutor:
    def __init__(self, hostname, ssh_key_path, private_ip, log_filename):
        self.ssh_command = f"ssh {username}@{hostname} -i {ssh_key_path} -o TCPKeepAlive=yes -o ServerAliveInterval=15"
        self.pexpect_prompt = f"{username}@ip-{private_ip}"
        self.logfile = open(log_filename, "a")
        self.prompt_printed_twice = not running_in_ci

    def connect_to_instance(self):
        self.pexpect_process = attempt_ssh_connection(
            self.ssh_command, self.logfile, self.pexpect_prompt
        )

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

    def __remote_exec(self, command):
        def handle_pexpect_errors(result):
            if result == 1:
                printformat("Warning, remote instance dropped the SSH connection", "yellow")
                self.connect_to_instance()
            elif result == 2:
                printformat("Warning, remote operation timed out!", "yellow")
            elif result == 3:
                exit_with_error("Warning, remote operation got an EOF before completing")

        ssh_connection_broken_msg = "client_loop: send disconnect: Broken pipe"

        self.pexpect_process.sendline(command)
        result = self.pexpect_process.expect(
            [
                self.pexpect_prompt,
                ssh_connection_broken_msg,
                pexpect.exceptions.TIMEOUT,
                pexpect.EOF,
            ]
        )
        if result > 0:
            handle_pexpect_errors(result)
        return result

    def __handle_pexpect_output(self, save_output):
        ansi_escape = re.compile(r"(\x9B|\x1B\[)[0-?]*[ -/]*[@-~]")
        # If save_output is set to True, clean the stdout output and save it in a list, one line per element. We need
        # to do this before calling expect again (if we are in a situation that requires it), otherwise the output
        # will be overwritten.
        self.pexpect_output = (
            [
                ansi_escape.sub("", line.replace("\r", "").replace("\n", ""))
                for line in self.pexpect_process.before.decode("utf-8").strip().split("\n")
            ]
            if save_output
            else None
        )
        if self.prompt_printed_twice:
            # Colored/formatted stdout (such as the bash prompt) is sometimes duplicated when piped to a pexpect process.
            # This is likely a platform-dependent behavior and does not occur when running this script in CI, or when running
            # a command on nested bash shells on remote instances (e.g. the bash script used to run commands on a docker
            # container). If we are in a situation where the prompt is printed twice, we need to wait for the duplicated
            # prompt to be printed before moving forward, otherwise the duplicated prompt will interfere with the next
            # command that we send.
            self.pexpect_process.expect(self.pexpect_prompt)

    def remote_exec_safe(self, command, save_output=False):
        """
        This function executes a shell command on a remote instance using pexpect, and handles common issues
        such as the SSH connection being dropped, timeouts, and EOF being returned before the end of the
        execution. The function does not handle command-specific errors, such as mandelbox build failures, etc.

        This function also has the option to save the shell stdout in a list of strings format.

        Args:
            command (str): The shell command to run on the remote instance
            prompt_printed_twice (bool):    A boolean indicating whether the bash prompt will be printed twice every time
                                            a command finishes its execution. When running a command in a shell instance
                                            connected to a Docker container, we need to pass False. Otherwise, by default,
                                            we set the parameter to True if running outside of CI, False otherwise.
        Returns:
            On Success:
                None
            On Failure:
                None and exit with exitcode -1
        """
        for i in range(pexpect_max_retries):
            result = self.__remote_exec(command)
            if result == 0:
                self.__handle_pexpect_output(save_output)
                break

    def get_exit_code(self):
        """
        This function is used to get the exit code of the last command that was run on a shell

        Args:
            pexpect_process (pexpect.pty_spawn.spawn):  The Pexpect process monitoring the execution of the process
                                                        on the remote machine
            pexpect_prompt (str):   The bash prompt printed by the shell on the remote machine when it is ready to
                                    execute a new command
            prompt_printed_twice (bool):    A boolean indicating whether the bash prompt will be printed twice every time
                                            a command finishes its execution. When running a command in a shell instance
                                            connected to a Docker container, we need to pass False. Otherwise, by default,
                                            we set the parameter to True if running outside of CI, False otherwise.

        Returns:
            exit_code (int):    The exit code of the last command that ran in the shell
        """

        result = self.__remote_exec("echo $?")
        if result > 0:
            # If we get a pexpect error, there is no way to retrieve the exit code of the previous command, so give it the benefit of the
            # doubt and assume it succeeded.
            return 0

        self.__handle_pexpect_output(save_output=True)
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
            return -1
        return int(filtered_output[-1])

    def run_command(self, command, errors_to_handle=[]):
        """
        This function is a wrapper around remote_exec, running commands remotely, handling application-specific errors and
        returning the output and exit codes if needed. The original command, together with any retries or relevant corrective
        actions are executed with remote_exec.
        """

        def handle_command_errors():
            for error in errors_to_handle:
                error_stdout_message, message_for_user, corrective_action = error
                if self.expression_in_pexpect_output(error_stdout_message, self.pexpect_output):
                    printformat(f"Warning, {message_for_user}", "yellow")
                    if corrective_action:
                        self.remote_exec_safe(self, corrective_action)

        for i in range(pexpect_max_retries):
            self.remote_exec_safe(self, command, save_output=len(errors_to_handle) > 0)
            saved_output = self.pexpect_output
            exit_code = self.get_exit_code()
            self.pexpect_output = saved_output
            errors_found = handle_command_errors()
            if not errors_found and exit_code == 0:
                break

        exit_with_error(
            f"Error: command execution failed {pexpect_max_retries}! Giving up now and exiting the script"
        )
