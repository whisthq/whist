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
            verbose (str): Whether to print information about each SSH connection attempt

        Returns:
            On success:
                None
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
            None

        Returns:
            None
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

        Returns:
            A boolean indicating whether the expression was found.
        """
        return any(expression in item for item in self.pexpect_output if isinstance(item, str))

    def set_mandelbox_mode(self, value=True):
        self.mandelbox_mode = value

    def __remote_exec(self, command, description="command", max_retries=pexpect_max_retries):
        """
        Execute a single command on a remote instance via SSH, checking for pexpect/SSH errors such as timeouts,
        broken connections, or EOF and retrying if an error occurs. For instance, if you reboot a remote instance
        while __remote_exec is running a command on it, the function will reconnect and rerun.

        In addition to executing a command in a manner that is robust to SSH/pexpect issues, this function will
        detect whether we are sending commands to the proper shell (shell attached to the remote instance or
        shell attached to a mandelbox Docker container). For instance, when we call `run.sh browsers/chrome`, we
        expect to enter the mandelbox shell and see the corresponding prompt (`:/#`). Conversely, when we call exit
        we should revert to the instance shell and the corresponding prompt

        Finally, the function will save the cleaned command stdout text to the member variable `self.pexpect_output`

        Args:
            command (str): The command to execute on the remote instance
            description (str):  A human-readable description for the command being run. The description will be used to
                                print a easy-understandable error message if there is an error
            max_retries (int): The maximum number of times to retry running the command if an error is encountered.

        Returns:
            success (bool): Whether the execution was successful (no error were encountered in the last retry and the
                            correct shell was used)
        """
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
        """
        This function gets the exit code of the last command that was executed by this RemoteExecutor on the remote instance.
        In case there is an error in the process of getting the exit code itself, we assume the command succeeded and return 0,
        since it's tricky to retry getting the exit code.

        This function preserves the stdout output of the last command by saving the contents of the `self.pexpect_output` variable
        and restoring them before returning

        Args:
            None

        Returns:
            exit_code (int): The exit code of the last command that was executed on the remote instance.
        """
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
        This function is a wrapper around remote_exec, running a command remotely, checking if the command succeeded by looking at
        the command's exit code, handling any application-specific errors specified by the user, and applying relevant corrective
        actions if errors are encountered for which the user has specified a command that should be run to fix them.
        If a non-fatal error is encountered, we retry running the command, otherwise, we exit the whole E2E script.

        Args:
            command (str): The command to execute on the remote instance
            description (str):  A human-readable description for the command being run. The description is printed to stdout, unless
                                quiet=True, and is also used to print a easy-understandable error message if there is an error
            quiet (bool): Controls whether to print the command description to stdout
            max_retries (int):  The maximum number of times to retry running the command if an application-specific error (i.e. not a
                                SSH/pexpect/connection error) is encountered
            success_message (str):  An expression that, if found in the stdout from the command's remote execution, tells us that the
                                    execution was successful, so we can ignore any other error message / exit code that might indicate
                                    otherwise. If no such message can be identified for the command being executed, simply pass None to
                                    this parameter
            ignore_exit_codes (bool):   Whether to avoid getting the exit code after the command was run and factoring that into the
                                        determination of whether the execution was successful or not.
            errors_to_handle (list):    A list of string tuples of the form: (error_stdout_message, message_for_user, corrective_action)
                                        with command-specific errors to check for, and corresponding messages to print to stdout (if an
                                        error is found) to inform the user, as well as a (optional) corrective action to take before
                                        retrying to run the command if the error is fixable. If corrective_action is set to None for a
                                        given error, and the error occurs, we treat it as a fatal error.
        Returns:
            None
        """

        if description != "command" and not quiet:
            print(description)

        for i in range(max_retries):
            success = self.__remote_exec(command, description)
            if not success:
                break
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
        """
        This function shuts down the remote executor's SSH connection (if still open) to the remote instance,
        kills the pexpect process and closes the log file used to pipe all the commands' stdout.
        """

        self.pexpect_process.kill(0)
        self.logfile.close()
