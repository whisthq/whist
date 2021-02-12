import docker
import postgres
import utilities
from functools import wraps, partial
from contextlib import contextmanager
from errors import catch_timeout_error
import config


client = docker.from_env()


def with_docker(func):
    @wraps(func)
    def with_docker_wrapper(*args, **kwargs):
        return func(client, *args, **kwargs)
    return with_docker_wrapper


# def with_ports_map(func):
#     @wraps(func)
#     def with_ports_map_wrapper(config_map):
#         container_port = config_map["container"]
#         localhost_port = config_map["localhost"]
#         ports = {}
#         ports[container_port] = localhost_port
#         return func(ports)
#     return with_ports_map_wrapper


@with_docker
def all_containers(client):
    return client.containers.list()


def remove_container(container):
    container.stop()
    container.remove()


def remove_all_containers():
    for c in all_containers():
        remove_container(c)
    return all_containers()

@with_docker
def is_ready(client, container_id):
    return client.containers.get(container_id).status == "running"


@catch_timeout_error
@with_docker
def run_postgres_container(client, **kwargs):
    # Note that ports are backwards from the Docker CLI.
    # Docker CLI takes: --ports host:container
    # docker-py takes: ports={container: host}
    ports = {}
    ports[config.POSTGRES_DEFAULT_PORT] = kwargs["port"]
    container = client.containers.run(
        config.POSTGRES_DEFAULT_IMAGE,
        environment={"POSTGRES_PASSWORD": config.POSTGRES_DEFAULT_PASSWORD},
        ports=ports,
        detach=True)

    utilities.ensure_ready(partial(is_ready, container.id),
                           partial(postgres.is_ready, **kwargs))

    return container
