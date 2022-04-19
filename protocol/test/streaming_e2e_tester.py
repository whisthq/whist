#!/usr/bin/env python3

import os, sys, json, time, argparse, multiprocessing
import boto3

from helpers.aws.boto3_tools import (
    create_or_start_aws_instance,
    get_instance_ip,
)

from helpers.common.git_tools import (
    get_whist_branch_name,
    get_whist_github_sha,
)

from helpers.common.ssh_tools import (
    attempt_ssh_connection,
)

from helpers.common.timestamps_and_exit_tools import (
    TimeStamps,
    exit_with_error,
)

from helpers.setup.instance_setup_tools import (
    start_host_service,
)

from helpers.setup.network_tools import (
    setup_artificial_network_conditions,
)

from helpers.setup.teardown_tools import complete_experiment_and_save_results

from helpers.whist_client_tools import (
    client_setup_process,
    run_client_on_instance,
)

from helpers.whist_server_tools import (
    server_setup_process,
    run_server_on_instance,
)
from protocol.test.helpers.common.timestamps_and_exit_tools import printyellow


# Add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

DESCRIPTION = """
This script will spin one or two g4dn.xlarge EC2 instances (depending on the parameters you pass in), \
start two Docker containers (one for the Whist client, one for the Whist server), and run a protocol \
streaming performance test between the two of them. In case one EC2 instance is used, the two Docker \
containers are started on the same instance.
"""

parser = argparse.ArgumentParser(description=DESCRIPTION)

parser.add_argument(
    "--ssh-key-name",
    help="The name of the AWS RSA SSH key you wish to use to access the E2 instance(s). If you are running the \
    script locally, the key name is likely of the form <yourname-key>. Make sure to pass the key-name for the \
    region of interest. Required.",
    required=True,
)
parser.add_argument(
    "--ssh-key-path",
    help="The path to the .pem certificate on this machine to use in connection to the RSA SSH key passed to \
    the --ssh-key-name arg. Required.",
    required=True,
)
parser.add_argument(
    "--github-token",
    help="The GitHub Personal Access Token with permission to fetch the whisthq/whist repository. Required.",
    required=True,
)

parser.add_argument(
    "--region-name",
    help="The AWS region to use for testing. If you are looking to re-use an instance for the client and/or server, \
    the instance(s) must live on the region passed to this parameter.",
    type=str,
    choices=[
        "us-east-1",
        "us-east-2",
        "us-west-1",
        "us-west-2",
        "af-south-1",
        "ap-east-1",
        "ap-south-1",
        "ap-northeast-3",
        "ap-northeast-2",
        "ap-southeast-1",
        "ap-southeast-2",
        "ap-southeast-3",
        "ap-northeast-1",
        "ca-central-1",
        "eu-central-1",
        "eu-west-1",
        "eu-west-2",
        "eu-south-1",
        "eu-west-3",
        "eu-north-1",
        "sa-east-1",
    ],
    default="us-east-1",
)

parser.add_argument(
    "--use-existing-server-instance",
    help="The instance ID of an existing instance to use for the Whist server during the E2E test. You can only \
    pass a value to this parameter if you passed `true` to --use-two-instances. Otherwise, the server will be \
    installed and run on the same instance as the client. The instance will be stopped upon completion. \
    If left empty (and --use-two-instances=true), a clean new instance will be generated instead, and it will \
    be terminated (instead of being stopped) upon completion of the test.",
    type=str,
    default="",
)

parser.add_argument(
    "--use-existing-client-instance",
    help="The instance ID of an existing instance to use for the Whist dev client during the E2E test. If the flag \
    --use-two-instances=false is used (or if the flag --use-two-instances is not used), the Whist server will also \
    run on this instance. The instance will be stopped upon completion. If left empty, a clean new instance will \
    be generated instead, and it will be terminated (instead of being stopped) upon completion of the test.",
    type=str,
    default="",
)

parser.add_argument(
    "--use-two-instances",
    help="Whether to run the client on a separate AWS instance, instead of the same as the server.",
    type=str,
    choices=["false", "true"],
    default="false",
)

parser.add_argument(
    "--leave-instances-on",
    help="This option allows you to override the default behavior and leave the instances running upon completion \
    of the test, instead of stopping (if reusing existing ones) or terminating (if creating new ones) them.",
    type=str,
    choices=["false", "true"],
    default="false",
)

parser.add_argument(
    "--skip-host-setup",
    help="This option allows you to skip the host-setup on the instances to be used for the test. \
    This will save you a good amount of time.",
    type=str,
    choices=["false", "true"],
    default="false",
)

parser.add_argument(
    "--skip-git-clone",
    help="This option allows you to skip cloning the Whist repository on the instance(s) to be used for the test. \
    The test will also not checkout the current branch or pull from Github, using the code from the /whist folder \
    on the existing instance(s) as is. This option is useful if you need to run several tests in succession \
    using code from the same commit.",
    type=str,
    choices=["false", "true"],
    default="false",
)


parser.add_argument(
    "--simulate-scrolling",
    help="Number of rounds of scrolling that should be simulated on the client side. One round of scrolling = \
    Slow scroll down + Slow scroll up + Fast scroll down + Fast scroll up",
    type=int,
    default=0,
)

parser.add_argument(
    "--testing-url",
    help="The URL to visit during the test. The default is a 4K video stored on our AWS S3.",
    type=str,
    default="https://whist-test-assets.s3.amazonaws.com/SpaceX+Launches+4K+Demo.mp4",
)

parser.add_argument(
    "--testing-time",
    help="The time (in seconds) to wait at the URL from the `--testing-url` flag before shutting down the \
    client/server and grabbing the logs and metrics. The default value is the duration of the default 4K video \
    from AWS S3.",
    type=int,
    default=126,  # Length of the video in link above is 2mins, 6seconds
)

parser.add_argument(
    "--cmake-build-type",
    help="The Cmake build type to use when building the protocol.",
    type=str,
    choices=["dev", "prod", "metrics"],
    default="metrics",
)

parser.add_argument(
    "--aws-credentials-filepath",
    help="The path (on the machine running this script) to the file containing the AWS credentials to use \
    to access the Whist AWS console. The file should contain the access key ID and the secret access key. \
    It is created/updated when running `aws configure`",
    type=str,
    default=os.path.join(os.path.expanduser("~"), ".aws", "credentials"),
)

parser.add_argument(
    "--network-conditions",
    help="The network condition for the experiment. The input is in the form of three comma-separated floats \
    indicating the max bandwidth, delay (in ms), and percentage of packet drops (in the range [0.0,1.0]). \
    'normal' will allow the network to run with no degradation. For example, pass --network-conditions \
    1Mbit,100,0.1 to simulate a bandwidth of 1Mbit/s, 100ms delay and 10 percent probability of packet drop",
    type=str,
    default="normal",
)

parser.add_argument(
    "--aws-timeout-seconds",
    help="The timeout after which we give up on commands that have not finished on a remote AWS EC2 instance. \
    This value should not be set to less than 20mins (1200s)",
    type=int,
    default=1200,
)

parser.add_argument(
    "--username",
    help="The username to use to access the AWS EC2 instance(s)",
    type=str,
    default="ubuntu",
)

parser.add_argument(
    "--ssh-connection-retries",
    help="The number of times to retry if a SSH connection is refused or if the connection attempt times out",
    type=int,
    default=5,
)

args = parser.parse_args()


if __name__ == "__main__":

    timestamps = TimeStamps()

    # 1 - Parse args from the command line
    ssh_key_name = args.ssh_key_name  # In CI, this is "protocol_performance_testing_sshkey"
    ssh_key_path = args.ssh_key_path
    github_token = args.github_token  # The PAT allowing us to fetch code from GitHub
    testing_url = args.testing_url
    region_name = args.region_name
    use_existing_client_instance = args.use_existing_client_instance
    use_existing_server_instance = args.use_existing_server_instance
    skip_git_clone = args.skip_git_clone
    skip_host_setup = args.skip_host_setup
    network_conditions = args.network_conditions
    aws_credentials_filepath = args.aws_credentials_filepath
    leave_instances_on = args.leave_instances_on
    aws_timeout_seconds = args.aws_timeout_seconds
    username = args.username
    ssh_connection_retries = args.ssh_connection_retries
    # Convert boolean 'true'/'false' strings to Python booleans
    use_two_instances = args.use_two_instances == "true"
    simulate_scrolling = args.simulate_scrolling
    running_in_ci = os.getenv("CI") == "true"
    # Each call to the mouse scrolling simulator script takes a total of 25s to complete, including 5s in-between runs
    testing_time = max(args.testing_time, simulate_scrolling * 25)

    # 2 - Perform a sanity check on the arguments and load the SSH key from file
    if use_existing_client_instance != "" and not use_two_instances:
        exit_with_error(
            "Error: the `use-two-instances` flag is set to `false` but a non-empty instance ID was passed with the `use-existing-client-instance` flag.",
            timestamps=timestamps,
        )
    if not os.path.isfile(ssh_key_path):
        print(f"SSH key file {ssh_key_path} does not exist")
        exit()

    # 3 - Create a local folder to store the experiment metadata and all the logs
    # (monitoring logs and metrics logs)
    experiment_start_time = time.strftime("%Y_%m_%d@%H-%M-%S")
    perf_logs_folder_name = os.path.join("perf_logs", experiment_start_time)
    os.makedirs(os.path.join(perf_logs_folder_name, "server"))
    os.makedirs(os.path.join(perf_logs_folder_name, "client"))

    metadata_filename = os.path.join(perf_logs_folder_name, "experiment_metadata.json")
    server_log_filepath = os.path.join(perf_logs_folder_name, "server_monitoring.log")
    client_log_filepath = os.path.join(perf_logs_folder_name, "client_monitoring.log")

    client_metrics_file = os.path.join(perf_logs_folder_name, "client", "client.log")
    server_metrics_file = os.path.join(perf_logs_folder_name, "server", "server.log")

    experiment_metadata = {
        "start_time": experiment_start_time + " local time"
        if not running_in_ci
        else experiment_start_time + " UTC",
        "testing_url": testing_url,
        "testing_time": testing_time,
        "simulate_scrolling": simulate_scrolling,
        "network_conditions": network_conditions,
        "using_two_instances": use_two_instances,
        "branch_name": get_whist_branch_name(running_in_ci),
        "github_sha": get_whist_github_sha(running_in_ci),
        "server_hang_detected": False,
    }

    with open(metadata_filename, "w") as metadata_file:
        json.dump(experiment_metadata, metadata_file)

    timestamps.add_event("Initialization")

    # 4 - Create a boto3 client, connect to the EC2 console, and create or start the instance(s).
    boto3client = boto3.client("ec2", region_name=region_name)
    server_instance_id = create_or_start_aws_instance(
        boto3client, region_name, use_existing_server_instance, ssh_key_name, running_in_ci
    )
    if server_instance_id == "":
        exit_with_error("Creating new instance for the server failed!", timestamps=timestamps)
    client_instance_id = (
        create_or_start_aws_instance(
            boto3client, region_name, use_existing_client_instance, ssh_key_name, running_in_ci
        )
        if use_two_instances
        else server_instance_id
    )
    if client_instance_id == "":
        exit_with_error(
            "Creating/starting new instance for the client failed!", timestamps=timestamps
        )

    # 5 - Create a todo-list of EC2 cleanup steps we need to do at the end of the test.
    # Save the todo-list to a file named `instances_to_remove.txt` so that we can retrieve
    # it in case  this script crashes.
    instances_to_be_terminated = []
    instances_to_be_stopped = []

    if server_instance_id != use_existing_server_instance:
        instances_to_be_terminated.append(server_instance_id)
        # Turning off skipping git clone and host setup if we created a new instance
        skip_git_clone = "false"
        skip_host_setup = "false"
    else:
        instances_to_be_stopped.append(server_instance_id)

    if client_instance_id != server_instance_id:
        if client_instance_id != use_existing_client_instance:
            instances_to_be_terminated.append(client_instance_id)
            # Turning off skipping git clone and host setup if we created a new instance
            skip_git_clone = "false"
            skip_host_setup = "false"
        else:
            instances_to_be_stopped.append(client_instance_id)

    instances_file = open("instances_to_remove.txt", "a+")
    for i in instances_to_be_terminated:
        instances_file.write(f"terminate {region_name} {i}\n")
    for i in instances_to_be_stopped:
        instances_file.write(f"stop {region_name} {i}\n")
    instances_file.close()

    timestamps.add_event("Creating/starting instance(s)")

    # 6 - Get the IP address of the instance(s) that are now running
    server_instance_ip = get_instance_ip(boto3client, server_instance_id)
    server_hostname = server_instance_ip[0]["public"]
    server_private_ip = server_instance_ip[0]["private"].replace(".", "-")

    client_instance_ip = (
        get_instance_ip(boto3client, client_instance_id)
        if use_two_instances
        else server_instance_ip
    )
    client_hostname = client_instance_ip[0]["public"]
    client_private_ip = client_instance_ip[0]["private"].replace(".", "-")

    # Notify the user that we are connecting to the EC2 instance(s)
    if not use_two_instances:
        print(f"Connecting to server/client AWS instance with hostname: {server_hostname}...")
    else:
        print(
            f"Connecting to server AWS instance with hostname: {server_hostname} and client AWS instance with hostname: {client_hostname}..."
        )

    # Create variables containing the commands to launch SSH connections to the client/server instance(s) and
    # generate strings containing the shell prompt(s) that we expect on the EC2 instance(s) when running commands.
    server_cmd = f"ssh {username}@{server_hostname} -i {ssh_key_path} -o TCPKeepAlive=yes -o ServerAliveCountMax=20 -o ServerAliveInterval=15"
    client_cmd = f"ssh {username}@{client_hostname} -i {ssh_key_path} -o TCPKeepAlive=yes -o ServerAliveCountMax=20 -o ServerAliveInterval=15"
    pexpect_prompt_server = f"{username}@ip-{server_private_ip}"
    pexpect_prompt_client = (
        f"{username}@ip-{client_private_ip}" if use_two_instances else pexpect_prompt_server
    )

    timestamps.add_event("Connecting to the instance(s)")

    # 7 - Setup the client and the server. Use multiprocesssing to parallelize the work in case
    # we are using two instances. If we are using one instance, the setup will happen sequentially.
    manager = multiprocessing.Manager()

    # We pass all parameters and other data to the setup processes via a dictionary
    args_dict = manager.dict(
        {
            "username": username,
            "server_hostname": server_hostname,
            "client_hostname": client_hostname,
            "ssh_key_path": ssh_key_path,
            "aws_timeout_seconds": aws_timeout_seconds,
            "server_log_filepath": server_log_filepath,
            "client_log_filepath": client_log_filepath,
            "pexpect_prompt_server": pexpect_prompt_server,
            "pexpect_prompt_client": pexpect_prompt_client,
            "github_token": github_token,
            "use_two_instances": use_two_instances,
            "testing_time": testing_time,
            "aws_credentials_filepath": aws_credentials_filepath,
            "cmake_build_type": args.cmake_build_type,
            "running_in_ci": running_in_ci,
            "skip_git_clone": skip_git_clone,
            "skip_host_setup": skip_host_setup,
            "ssh_connection_retries": ssh_connection_retries,
        }
    )

    # If using two instances, parallelize the host-setup and building of the Docker containers to save time
    p1 = multiprocessing.Process(target=server_setup_process, args=[args_dict])
    p2 = multiprocessing.Process(target=client_setup_process, args=[args_dict])
    if use_two_instances:
        p1.start()
        p2.start()
        # Monitor the processes and immediately exit if one of them errors
        p_done = []
        while len(p_done) < 2:
            for p in [p1, p2]:
                if p is not None:
                    if p.exitcode == 0:
                        p_done.append(p)
                    else:
                        if p == p1:
                            printyellow(
                                "Server setup process failed. Terminating the client setup process and exiting."
                            )
                            p2.terminate()
                        else:
                            printyellow(
                                "Client setup process failed. Terminating the server setup process and exiting."
                            )
                            p1.terminate()
                        exit_with_error(None, timestamps=timestamps)
            time.sleep(1)
        p1.join()
        p2.join()
    else:
        p1.start()
        p1.join()
        # Exit if the server setup process has failed
        if p1.exitcode == -1:
            exit_with_error(None, timestamps=timestamps)
        p2.start()
        p2.join()
        # Exit if the client setup process has failed
        if p2.exitcode == -1:
            exit_with_error(None, timestamps=timestamps)

    timestamps.add_event("Setting up the instance(s) and building the mandelboxes")

    # 8 - Open the server/client monitoring logs
    server_log = open(os.path.join(perf_logs_folder_name, "server_monitoring.log"), "a")
    client_log = open(os.path.join(perf_logs_folder_name, "client_monitoring.log"), "a")

    # 9 - Run the host-service on the client and the server. We don't parallelize here for simplicity, given
    # that all operations below do not take too much time.

    # Start SSH connection(s) to the EC2 instance(s) to run the host-service commands
    server_hs_process = attempt_ssh_connection(
        server_cmd,
        aws_timeout_seconds,
        server_log,
        pexpect_prompt_server,
        ssh_connection_retries,
        running_in_ci,
    )
    client_hs_process = (
        attempt_ssh_connection(
            client_cmd,
            aws_timeout_seconds,
            client_log,
            pexpect_prompt_client,
            ssh_connection_retries,
            running_in_ci,
        )
        if use_two_instances
        else server_hs_process
    )

    # Launch the host-service on the client and server instance(s)
    start_host_service(server_hs_process, pexpect_prompt_server)
    if use_two_instances:
        start_host_service(client_hs_process, pexpect_prompt_client)

    timestamps.add_event("Starting the host service on the mandelboxes ")

    # 10 - Run the browser/chrome server mandelbox on the server instance
    # Start SSH connection(s) to the EC2 instance(s) to run the browser/chrome server mandelbox
    server_pexpect_process = attempt_ssh_connection(
        server_cmd,
        aws_timeout_seconds,
        server_log,
        pexpect_prompt_server,
        ssh_connection_retries,
        running_in_ci,
    )
    # Launch the browser/chrome server mandelbox, and retrieve the connection configs that
    # we need to pass the client for it to connect
    server_docker_id, server_configs_data = run_server_on_instance(server_pexpect_process)

    # Augment the configs with the initial URL we want to visit
    server_configs_data["initial_url"] = testing_url

    # 11 - Run the development/client client mandelbox on the client instance
    # Start SSH connection(s) to the EC2 instance(s) to run the development/client
    # client mandelbox on the client instance
    client_pexpect_process = attempt_ssh_connection(
        client_cmd,
        aws_timeout_seconds,
        client_log,
        pexpect_prompt_client,
        ssh_connection_retries,
        running_in_ci,
    )

    # Set up the artifical network degradation conditions on the client, if needed
    setup_artificial_network_conditions(
        client_pexpect_process,
        pexpect_prompt_client,
        network_conditions,
        testing_time,
        running_in_ci,
    )

    # Run the dev client on the client instance, using the server configs obtained above
    client_docker_id = run_client_on_instance(
        client_pexpect_process, server_configs_data, simulate_scrolling
    )

    timestamps.add_event("Starting the mandelboxes and setting up the network conditions")

    # 12 - Sit down and wait Wait <testing_time> seconds to let the test run to completion
    time.sleep(testing_time)

    timestamps.add_event("Waiting for the test to finish")

    # 13 - Grab the logs, check for errors, restore default network conditions, cleanup, shut down the instances, and save the results
    complete_experiment_and_save_results(
        server_hostname,
        server_instance_id,
        server_docker_id,
        server_cmd,
        server_log,
        server_metrics_file,
        use_existing_server_instance,
        server_pexpect_process,
        server_hs_process,
        pexpect_prompt_server,
        client_hostname,
        client_instance_id,
        client_docker_id,
        client_cmd,
        client_log,
        client_metrics_file,
        use_existing_client_instance,
        client_pexpect_process,
        client_hs_process,
        pexpect_prompt_client,
        aws_timeout_seconds,
        ssh_connection_retries,
        username,
        ssh_key_path,
        boto3client,
        running_in_ci,
        use_two_instances,
        leave_instances_on,
        network_conditions,
        perf_logs_folder_name,
        experiment_metadata,
        metadata_filename,
        timestamps,
    )

    # 23 - Success!
    print("Done")
