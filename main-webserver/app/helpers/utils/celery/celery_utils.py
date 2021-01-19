import celery


def find_group_task_with_state(group_res, state):
    """
    Finds the failed task in a celery.result.GroupResult object.

    Args:
        group_res (celery.result.GroupResult): a GroupResult object with at least one failed task
        state (str): one of PENDING, STARTED, SUCCESS, FAILURE, RETRY, REVOKED

    Returns:
        (int) the index of the task in the original job that experienced an error. If there are multiple,
            this finds the first (lowest index) task that failed.
    """
    if not isinstance(group_res, celery.result.GroupResult):
        raise ValueError(f"Expected a celery.result.GroupResult, got {type(group_res)}")

    results = group_res.results
    for i, async_res in enumerate(results):
        if async_res.state == state:
            return i

    raise ValueError("Expected a failed task, found none")
