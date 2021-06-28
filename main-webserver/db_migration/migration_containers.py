import docker
import postgres
import utilities
from functools import wraps, partial
from contextlib import contextmanager
from errors import catch_timeout_error
import config

# Initialize the connection to the Docker server
client_connection = docker.from_env()


def with_docker(func):
    """A decorator to pass on default Docker client to functions.

    Passes a default Docker client as the first argument to decorated
    functions.

    Args:
        func: A function that accepts a Docker client as its first argument.
    Returns:
        The input function, partially applied with the Docker client object.
    """

    @wraps(func)
    def with_docker_wrapper(*args, **kwargs):
        return func(client_connection, *args, **kwargs)

    return with_docker_wrapper


@with_docker
def all_containers(client):
    """A utility function to list all running Docker containers.

    Lists all Docker clients with status "running" in the current environment.
    Accepts a "client" object used to establish the server connection.

    Args:
        client: A Docker client connection object.
    Returns:
        A list of Docker container objects.
    """
    return client.containers.list()


def remove_container(container):
    """A utility function to stop, and then remove a Docker container.

    Args:
        container: A Docker container object.
    Returns:
        None
    """
    container.stop()
    container.remove()


def remove_all_containers():
    """A utility function to stop and remove all running Docker containers.

    Returns:
        A list of currently running Docker container objects, which should be
        empty if the function is successful.
    """
    for c in all_containers():
        remove_container(c)
    return all_containers()


@with_docker
def is_ready(client, container_id):
    """Tests if a Docker container has finished starting up.

    We use the container_id to query the list of running containers.
    It's important to do this instead of just checking the status on the
    container object, as the Docker Python API does not always correctly
    update the status attribute on "live" container objects.

    This function gets a fresh container object from the Docker server,
    and checks the status on it.

    Args:
        client: A Docker connection object.
        container_id: The string id of the container to ping.
    Returns:
        True if the status of the container is "running", False otherwise.
    """
    return client.containers.get(container_id).status == "running"


@catch_timeout_error
@with_docker
def run_postgres_container(client, **kwargs):
    """Starts a docker PostgresSQL container and waits for it to be ready

    Using the configuration passed in through the client paramater,
    connects to the Docker server and runs a PostgreSQL image. It waits
    until both the Docker container and PostgreSQL database are ready for
    connections, subject to a timeout.

    The "port" keyword argument is used to determine the host port that
    will be used to communicate with the container. It will be mapped to a
    container port that is pulled from configuration.

    All other keyword arguments are passed on to the PostgreSQL implementation
    to establish a connection.

    Args:
        client: A Docker connection object.
        kwargs: keyword arguments that will be passed on to the PostgreSQL API.
    Returns:
        A Docker container object.

    """
    # Note that ports are backwards from the Docker CLI.
    # Docker CLI takes: --ports host:container
    # docker-py takes: ports={container: host}
    ports = {}
    ports[config.POSTGRES_DEFAULT_PORT] = kwargs["port"]
    container = client.containers.run(
        config.POSTGRES_DEFAULT_IMAGE,
        environment={"POSTGRES_PASSWORD": config.POSTGRES_DEFAULT_PASSWORD},
        ports=ports,
        detach=True,
    )

    utilities.ensure_ready(partial(is_ready, container.id), partial(postgres.is_ready, **kwargs))

    return container
