import sys
import os
import json
from pathlib import Path

sys.path.append(str(Path(__file__).parent.parent.joinpath("src")))
import main


def test_parse_inputs():
    secrets = {"a": 0, "b": 1, "c": 2}
    env = {
        "NON_INPUT_VALUE": "ignored",
        "INPUT_SECRETS": json.dumps(secrets),
        "INPUT_PARAM_1": "value1",
        "INPUT_PARAM_2": "value2",
    }

    assert main.parse_inputs(env) == {
        "secrets": secrets,
        "param_1": "value1",
        "param_2": "value2",
    }


def test_main(monkeypatch):
    def mock_source(**kwargs):
        return {"d": 3, "e": 4, "f": 5}

    secrets = {"a": 0, "b": 1, "c": 2}
    env = {
        "NON_INPUT_VALUE": "ignored",
        "INPUT_SECRETS": json.dumps(secrets),
        "INPUT_PARAM_1": "value1",
        "INPUT_PARAM_2": "value2",
    }
    monkeypatch.setattr(main, "SOURCES", [mock_source])
    monkeypatch.setattr(os, "environ", env)
    assert main.main() == {**secrets, **mock_source()}
