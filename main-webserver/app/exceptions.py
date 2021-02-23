class ContainerNotFoundException(Exception):
    """
    Raised by any method that fails to find a given container in the db
    Args:
        container_id (str): the ARN of the container
    """

    def __init__(self, container_id):
        super().__init__(f"container_id: {container_id}")


class ClusterNotFoundException(Exception):
    """
    Raised by any method that fails to find a given cluster in the db
    Args:
        cluster_id (str): the ARN of the cluster
    """

    def __init__(self, cluster_id):
        super().__init__(f"cluster_id: {cluster_id}")
