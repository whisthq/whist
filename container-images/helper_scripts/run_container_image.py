#!/usr/bin/env python3

# For usage, run this script with `--help`.

import argparse
from collections import namedtuple
import io
import os
import sys
import tarfile

import docker
import psutil
import requests


DESCRIPTION = """
This script runs a Fractal container by calling the `SpinUpMandelbox` endpoint
on the host service and emulating the client-app (but not the client protocol).
It is used by the scripts `run_local_container_image.sh` and
`run_remote_container_image.sh` and is usually not called directly.
"""


# Set current working directory to fractal/container-images
def reset_working_directory():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    os.chdir("..")


reset_working_directory()

# Parse arguments
parser = argparse.ArgumentParser(description=DESCRIPTION)
parser.add_argument(
    "image",
    help="Fractal container to run. Defaults to 'fractal/base:current-build'.",
    default="fractal/base:current-build",
)
parser.add_argument(
    "--host-address",
    help="IP address of the host service to send the request. Defaults to '127.0.0.1'.",
    default="127.0.0.1",
)
parser.add_argument(
    "--host-port",
    help="Port which the host service is listening to. Defaults to '4678'.",
    default="4678",
)
parser.add_argument(
    "--no-verify-tls",
    action="store-true",
    help=(
        "This flag skips TLS verification for requests to the host service. Not "
        "passing this flag means that this script will verify against the "
        "certificate '/fractalprivate/cert.pem'. By default, this flag is disabled."
    ),
)
parser.add_argument(
    "--update-protocol",
    type=bool,
    default=False,
    help=(
        "This flag causes the locally-built server protocol to be copied into "
        "the running container. Not passing in this flag causes the container to use "
        "the version of the server protocol already built into its image. By "
        "default, run_local_container_image.sh enables this flag, but "
        "run_remote_container_image.sh does not. This flag cannot be used in conjunction "
        "with '--host-address', as the container does not run locally in that case."
    ),
)
parser.add_argument(
    "--dpi",
    type=int,
    default=96,
    help="DPI to use for the X server inside the container. Defaults to 96.",
)
parser.add_argument(
    "--protocol-timeout",
    type=int,
    default=60,
    help=(
        "This arg sets the timeout for the protocol server inside the "
        "container, in seconds. Defaults to 60."
    ),
)
parser.add_argument(
    "--user-id",
    default="run_container_image_user",
    help="User ID to use for config retrieval. Defaults to 'run_container_image_user'.",
)
parser.add_argument(
    "--user-config-encryption-token",
    default="RaR9Olgvqj+/AtNUHAPXjRZ26FkrFIVd",
    help=(
        "Config encryption token for the user. This would normally be passed in "
        "by the client app, but by default we use a fake token instead."
    ),
)
args = parser.parse_args()


# Define some helper functions and variables
HOST_SERVICE_URL = f"https://{args.host_address}:{args.host_port}/"
HOST_SERVICE_AUTH_SECRET = "testwebserverauthsecretdev"
HOST_SERVICE_CERT_PATH = "/fractalprivate/cert.pem"
local_host_service = args.host_address == "127.0.0.1"
protocol_build_path = os.path.abspath("../protocol/build-docker/server/build64")
container_server_path = os.path.abspath("/usr/share/fractal/bin")
PortBindings = namedtuple(
    "PortBindings", ["host_port_32262tcp", "host_port_32263udp", "host_port_32273tcp"]
)


def ensure_root_privileges():
    if os.geteuid() != 0:
        sys.exit(
            (
                "You need to have root privileges to run this script.\nPlease try "
                "again, this time using 'sudo'. Exiting."
            )
        )


def get_public_ipv4_addr():
    if local_host_service:
        respobj = requests.get(url="https://ipinfo.io")
        response = respobj.json()
        respobj.raise_for_status()
        return response["ip"]
    else:
        return args.host_address


def ensure_host_service_is_running():
    """
    Check if the host service is running by checking if there is a process
    listening on TCP port 4678.
    """
    running = any(
        c.status == "LISTEN" and c.laddr.port == 4678 for c in psutil.net_connections(kind="tcp")
    )
    if not running:
        sys.exit(
            (
                "Cannot start container because the ecs-host-service is not "
                "listening on port 4678. Is it running successfully?"
            )
        )


def kill_container(cont):
    cont.kill()
    cont.remove()


def copy_locally_built_protocol(cont):
    # The docker python API is stupid, so we first need to tar the protocol directory
    fileobj = io.BytesIO()
    with tarfile.open(fileobj=fileobj, mode="w") as tar:
        os.chdir(protocol_build_path)
        tar.add(".")
        reset_working_directory()

    cont.put_archive(container_server_path, fileobj.getvalue())


def send_spin_up_mandelbox_request():
    """
    Sends the host service a SpinUpMandelbox request and returns a Container
    object corresponding to that container, along with its identifying host
    port (i.e. host port corresponding to tcp/32262 in the container),
    aeskey, and fractalID.
    """
    print("Sending SpinUpMandelbox request to host service!")
    url = HOST_SERVICE_URL + "spin_up_mandelbox"
    payload = {
        "auth_secret": HOST_SERVICE_AUTH_SECRET,
        "app_image": args.image,
        "dpi": args.dpi,
        "user_id": args.user_id,
        "config_encryption_token": args.user_config_encryption_token,
    }
    tls_verification = False if args.no_verify_tls else HOST_SERVICE_CERT_PATH
    respobj = requests.put(url=url, json=payload, verify=tls_verification)
    response = respobj.json()
    print(f"Response from host service: {response}")
    respobj.raise_for_status()

    host_port_32262tcp = response["result"]["port_32262"]
    host_port_32263udp = response["result"]["port_32263"]
    host_port_32273tcp = response["result"]["port_32273"]
    key = response["result"]["aes_key"]
    resulting_fractal_id = response["result"]["fractal_id"]

    return (
        PortBindings(host_port_32262tcp, host_port_32263udp, host_port_32273tcp),
        key,
        resulting_fractal_id,
    )


def write_protocol_timeout(fractalid):
    """
    Takes in a fractalID, and writes the protocol timeout to the corresponding container.
    """
    with open(f"/fractal/{fractalid}/containerResourceMappings/timeout", "w") as timeout_file:
        timeout_file.write(f"{args.protocol_timeout}")


if __name__ == "__main__":
    # pylint: disable=line-too-long
    if local_host_service:
        # This is running locally on the same machine as the host service, so
        # we will need root privileges and can easily check whether the host
        # service is up.
        ensure_root_privileges()
        ensure_host_service_is_running()

    host_ports, aeskey, fractal_id = send_spin_up_mandelbox_request()

    if local_host_service:
        # This is running locally on the same machine as the host service, so
        # we can safely use the Docker client and copy files to the mandelbox.
        docker_client = docker.from_env()

        # Find the Container object corresponding to the container that was just created
        matching_containers = docker_client.containers.list(
            filters={
                "publish": f"{host_ports.host_port_32262tcp}/tcp",
            }
        )

        assert len(matching_containers) == 1
        container = matching_containers[0]

        if args.update_protocol:
            copy_locally_built_protocol(container)
        try:
            write_protocol_timeout(fractal_id)
        except Exception as err:
            kill_container(container)
            raise err

    print(
        f"""Successfully started container with identifying hostPort {host_ports.host_port_32262tcp}.
To connect to this container using the client protocol, run one of the following commands, depending on your platform:

    windows:        .\\build\\fclient.bat {get_public_ipv4_addr()} -p32262:{host_ports.host_port_32262tcp}.32263:{host_ports.host_port_32263udp}.32273:{host_ports.host_port_32273tcp} -k {aeskey}
    linux/macos:    ./fclient {get_public_ipv4_addr()} -p32262:{host_ports.host_port_32262tcp}.32263:{host_ports.host_port_32263udp}.32273:{host_ports.host_port_32273tcp} -k {aeskey}
"""
    )

    if local_host_service:
        print(
            f"""Name of resulting container:
{container.name}
Docker ID of resulting container:"""
        )
        # Note that ./run_container_image.sh relies on the dockerID being printed
        # alone on the last line of the output of this script!!
        print(f"{container.id}")
