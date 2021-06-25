#!/usr/bin/env python
import pytest
import yaml
import os
import json
import tests.mock_data as mock_data
from pathlib import Path
from click.testing import CliRunner
from contextlib import contextmanager
from helpers.utils import temporary_fs
from helpers.cli import create_cli
from helpers.parse import parse
from tests.generators import schemas, non_dict
from hypothesis import given, example
from hypothesis.strategies import (
    text,
    just,
    lists,
    one_of,
    dictionaries,
    composite,
    integers,
    sampled_from,
)


runner = CliRunner()
cli = create_cli(parse)


def assert_cli_result(args, expected):
    result = runner.invoke(cli, args)
    if result.exception:
        raise result.exception
    assert result.exit_code == 0
    assert json.loads(result.output) == expected
    return result


def test_cli_stdout(tmp_path):
    with temporary_fs(mock_data.config_simple_fs) as tempdir:
        config = str(tempdir.joinpath("config"))

        expect_1 = mock_data.config_simple_json_dev
        option_1 = [config, "--profile", "dev"]
        result_1 = assert_cli_result(option_1, expect_1)

        expect_2 = mock_data.config_simple_json_stg
        option_2 = [config, "--profile", "stg"]
        result_2 = assert_cli_result(option_2, expect_2)

        expect_3 = mock_data.config_simple_json_prd
        option_3 = [config, "--profile", "prd"]
        result_3 = assert_cli_result(option_3, expect_3)

        expect_4 = mock_data.config_simple_json_default
        option_4 = [config]
        result_4 = assert_cli_result([config], expect_4)
