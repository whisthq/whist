import time

import pytest
import celery
from celery import shared_task, group

from app.helpers.utils.celery.celery_utils import find_group_task_with_state


@shared_task
def func_good(x):
    return x


@shared_task
def func_slow(x):
    time.sleep(2)
    return x


class CustomError(Exception):
    pass


@shared_task
def func_err(x):
    raise CustomError("This error is expected in testing")


@pytest.mark.usefixtures("celery_session_app")
@pytest.mark.usefixtures("celery_session_worker")
def test_find_group_task_with_state_timeout():
    ret1 = func_good.s(0)
    ret2 = func_slow.s(0)

    job = group([ret1, ret2])
    job_res = job.apply_async()

    with pytest.raises(celery.exceptions.TimeoutError):
        job_res.get(timeout=0.5)

    bad_idx = find_group_task_with_state(job_res, "PENDING")
    assert bad_idx == 1


@pytest.mark.usefixtures("celery_session_app")
@pytest.mark.usefixtures("celery_session_worker")
def test_find_group_task_with_state_err():
    ret1 = func_good.s(0)
    ret2 = func_err.s(0)

    job = group([ret1, ret2])
    job_res = job.apply_async()

    with pytest.raises(CustomError):
        job_res.get(timeout=0.5)

    bad_idx = find_group_task_with_state(job_res, "FAILURE")
    assert bad_idx == 1


@pytest.mark.usefixtures("celery_session_app")
@pytest.mark.usefixtures("celery_session_worker")
def test_find_group_task_with_state_noerr():
    ret1 = func_good.s(0)
    ret2 = func_good.s(0)

    job = group([ret1, ret2])
    job_res = job.apply_async()

    # should finish successfully
    job_res.get(timeout=0.5)

    with pytest.raises(Exception):
        # this should error out because no tasks failed
        find_group_task_with_state(job_res, "FAILURE")

    assert True
