import docker
import postgres
import utilities
from functools import wraps, partial
from contextlib import contextmanager
from errors import catch_timeout_error
import config


client = docker.from_env()


def with_docker(func):
    """A decorator to pass on a default Docker client to functions

    Passes a default Docker client as the first argument to decorated
    functions.
    """

    @wraps(func)
    def with_docker_wrapper(*args, **kwargs):
        return func(client, *args, **kwargs)

    return with_docker_wrapper


@with_docker
def all_containers(client):
    """A utility function to list all running containers

    It accepts a client argument to establish the server connection.
    """
    return client.containers.list()


def remove_container(container):
    "A utility function to stop and remove a container"
    container.stop()
    container.remove()


def remove_all_containers():
    """A utility function to stop and remove all running Docker containers"""
    for c in all_containers():
        remove_container(c)
    return all_containers()


@with_docker
def is_ready(client, container_id):
    return client.containers.get(container_id).status == "running"


@catch_timeout_error
@with_docker
def run_postgres_container(client, **kwargs):
    """Starts a docker PostgresSQL container and waits for it to be ready

    Using the configuring passed in through the client paramater,
    connects to the Docker server and runs a PostgreSQL image. It waits
    until both the Docker container and PostgreSQL database are ready for
    connections, subject to a timeout.

    The "port" keyword argument is used to determine the host port that
    will be used to communicate with the container. It will be mapped to a
    container port that is pulled from configuration.

    All other keyword arguments are passed on to the PostgreSQL implementation
    to establish a connection.
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

    utilities.ensure_ready(
        partial(is_ready, container.id), partial(postgres.is_ready, **kwargs)
    )

    return container
