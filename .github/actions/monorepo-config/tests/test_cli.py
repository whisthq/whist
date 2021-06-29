#!/usr/bin/env python
import pytest
import yaml
import os
import json
import click
import tests.mock_data as mock_data
import helpers.cli as cli
import helpers.validate as validate
from pathlib import Path
from click.testing import CliRunner
from contextlib import contextmanager
from helpers.utils import temporary_fs
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
cli_obj = cli.create_cli(parse)


def assert_cli_result(args, expected):
    result = runner.invoke(cli_obj, args)
    if result.exception:
        raise result.exception
    assert result.exit_code == 0
    assert json.loads(result.output) == expected
    return result


def test_cli_stdout(tmp_path):
    with temporary_fs(mock_data.config_simple_fs) as tempdir:
        config = str(tempdir.joinpath("config"))

        expect_1 = mock_data.config_simple_json_dev
        option_1 = ["--path", config, "--environment", "neil"]
        # result_1 = assert_cli_result(option_1, expect_1)

        # expect_2 = mock_data.config_simple_json_stg
        # option_2 = [config, "--profile", "stg"]
        # result_2 = assert_cli_result(option_2, expect_2)

        # expect_3 = mock_data.config_simple_json_prd
        # option_3 = [config, "--profile", "prd"]
        # result_3 = assert_cli_result(option_3, expect_3)

        # expect_4 = mock_data.config_simple_json_default
        # option_4 = [config]
        # result_4 = assert_cli_result([config], expect_4)


@pytest.mark.skip()
def test_parse_cli_profiles():
    parses_dict = "result dictionary should contain all keys and values"
    err_missing_value = "should raise an error on missing value"
    err_bad_flag_syntax = "should raise an error if values don't start with --"

    args = ["--arg1", "val1", "--arg2", "val2", "--arg3", "val3"]
    assert cli.parse_cli_profiles(None, None, args) == {
        "arg1": "val1",
        "arg2": "val2",
        "arg3": "val3",
    }, parses_dict

    with pytest.raises(click.BadParameter):
        args = ["--arg1", "val1", "--arg2"]
        assert cli.parse_cli_profiles(None, None, args), err_missing_value

    with pytest.raises(click.BadParameter):
        args = ["--arg1", "val1", "arg2", "val2"]
        assert cli.parse_cli_profiles(None, None, args), err_bad_flag_syntax
