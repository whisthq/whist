import docker
import postgres
import utilities
from functools import wraps, partial
from contextlib import contextmanager
from errors import catch_timeout_error


client = docker.from_env()


def with_docker(func):
    @wraps(func)
    def with_docker_wrapper(*args):
        return func(client, *args)
    return with_docker_wrapper


def with_port_map(func):
    @wraps(func)
    def with_port_map_wrapper(config_map):
        # if not kwargs.get("ports"):
        #     return func(port_map=None)
        container_port = config_map["ports"]["container"]
        localhost_port = config_map["ports"]["localhost"]
        ports = {}
        ports[container_port] = localhost_port
        return func(ports)
    return with_port_map_wrapper


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
@with_port_map
def run_postgres_container(port_map):
    # Note that ports are backwards from the Docker CLI.
    # Docker CLI takes: --ports host:container
    # docker-py takes: ports={container: host}
    print(port_map)
    host_port = next(iter(port_map.values()))
    container = client.containers.run(
        "postgres",
        environment={
            "POSTGRES_HOST_AUTH_METHOD": "trust"},
        ports=port_map,
        detach=True)

    # utilities.ensure_ready(partial(is_ready, container.id),
    #                        partial(postgres.is_ready, host_port))

    return container
