from typing import Any, Callable, Dict, Generator, List

import pytest

import app.helpers.aws.aws_instance_post as aws_funcs
from app.utils.aws.base_ec2_client import EC2Client
from app.models import db


@pytest.fixture
def hijack_ec2_calls(
    monkeypatch: pytest.MonkeyPatch,
) -> Generator[List[Dict[str, Any]], None, None]:
    """
    This fixture mocks any calls to EC2client side-effecting endpoints
    and just puts arguments to those calls into a list.
    Use it to confirm that things interfacing with the EC2 client are
    doing so correctly, _without_ risk of side effects.
    """
    call_list = []

    def _set_state_helper(*args: Any, **kwargs: Any) -> List[str]:
        call_list.append({"args": args, "kwargs": kwargs})
        return ["test_id"]

    def _set_state_helper_stop_instances(*args: Any, **kwargs: Any) -> Dict[Any, Any]:
        call_list.append({"args": args, "kwargs": kwargs})
        return {}

    def _trivial_true(*args: Any, **kwargs: Any) -> bool:  # pylint: disable=unused-argument
        return True

    def _get_state_helper(
        *args: Any, **kwargs: Any  # pylint: disable=unused-argument
    ) -> List[str]:
        # Pretend the instance is running when we call get_instance_states!
        return ["running"]

    monkeypatch.setattr(EC2Client, "start_instances", _set_state_helper)
    monkeypatch.setattr(EC2Client, "stop_instances", _set_state_helper_stop_instances)
    monkeypatch.setattr(EC2Client, "all_running", _trivial_true)
    monkeypatch.setattr(EC2Client, "get_instance_states", _get_state_helper)
    yield call_list


@pytest.fixture
def mock_get_num_new_instances(
    monkeypatch: pytest.MonkeyPatch,
) -> Generator[Callable[[Any], None], None, None]:
    """
    This fixture mocks the _get_num_new_instances function
    from aws_instance_post, making it easy to set what that function should return.
    """

    def _patcher(return_value: Any) -> None:
        monkeypatch.setattr(
            aws_funcs, "_get_num_new_instances", (lambda *args, **kwargs: return_value)
        )

    yield _patcher


@pytest.fixture
def hijack_db(monkeypatch: pytest.MonkeyPatch) -> Generator[List[Dict[str, Any]], None, None]:
    """
    This fixture mocks any calls to DB side effecting endpoints,
    putting added objects into a list.
    Use it to confirm that things interfacing with the DB in generic ways
    do so correctly, without risk of side effects.
    """
    call_list = []

    def _helper(*args: Any, **kwargs: Any) -> None:
        call_list.append({"args": args, "kwargs": kwargs})

    def _add_all_helper(*args: Any) -> None:
        for obj in args[0]:
            call_list.append({"args": obj})

    def _empty(*args: Any) -> None:  # pylint: disable=unused-argument
        return

    monkeypatch.setattr(db.session, "add", _helper)
    monkeypatch.setattr(db.session, "add_all", _add_all_helper)
    monkeypatch.setattr(db.session, "commit", _empty)
    monkeypatch.setattr(db.session, "delete", _empty)
    yield call_list
