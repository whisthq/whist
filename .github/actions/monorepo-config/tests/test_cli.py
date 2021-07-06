#!/usr/bin/env python
import pytest
import yaml
import os
import json
import click
import tests.mock_data as mock_data
import helpers.cli
import helpers.validate as validate
import helpers.parse as parse
from pathlib import Path
from click.testing import CliRunner
from contextlib import contextmanager
from helpers.utils import temporary_fs

runner = CliRunner()
cli_obj = helpers.cli.create_cli(parse.parse)


def cli_result(cli, args):
    result = runner.invoke(cli, args)
    if result.exception:
        raise result.exception
    assert result.exit_code == 0
    return result.output


# It's difficult to debug a failing test with the click CLI test runner.
# It's easier for us to test here that the function run by the CLI
# (helpers.parse.parse) receives the correct arguments. We'll be testing
# the helpers.parse.parse function separately, so we can be happy with
# the test result if parse receives its expected arguments.
def test_parse_receives_args(mocker):
    mock = mocker.Mock(spec=parse.parse, return_value={"mock": {}})

    cli = helpers.cli.create_cli(mock)
    with temporary_fs(mock_data.config_simple_fs) as tempdir:
        config = str(tempdir.joinpath("config"))

        cli_result(cli, ["--path", config])
        mock.assert_called_once_with(config, profiles=None, secrets=None)
        mock.reset_mock()

        cli_result(cli, ["--path", config, "--env", "dev"])
        mock.assert_called_once_with(
            config, profiles={"env": "dev"}, secrets=None
        )
        mock.reset_mock()

        cli_result(cli, ["--path", config, "--env", "dev"])
        mock.assert_called_once_with(
            config, profiles={"env": "dev"}, secrets=None
        )
        mock.reset_mock()

        cli_result(cli, ["--path", config, "--env", "dev", "--os", "macos"])
        mock.assert_called_once_with(
            config, profiles={"env": "dev", "os": "macos"}, secrets=None
        )
        mock.reset_mock()

        cli_result(
            cli,
            [
                "--path",
                config,
                "--out",
                str(tempdir.joinpath("outfile")),
                "--secrets",
                json.dumps({"KEY": "secret001", "API": "secret002"}),
                "--secrets",
                json.dumps({"WEB": "secret003", "COM": "secret004"}),
                "--env",
                "dev",
                "--os",
                "macos",
            ],
        )
        mock.assert_called_once_with(
            config,
            profiles={"env": "dev", "os": "macos"},
            secrets={
                "KEY": "secret001",
                "API": "secret002",
                "WEB": "secret003",
                "COM": "secret004",
            },
        )
        mock.reset_mock()


# We perform a
def test_cli_out():
    cli = helpers.cli.create_cli(parse.parse)
    with temporary_fs(mock_data.config_secrets_profiles_fs) as tempdir:
        config = str(tempdir.joinpath("config"))
        outfile = tempdir.joinpath("outfile")
        expected = {
            "URL": "http://url-dev.com",
            "KEY": "secretKEY",
            "API": "secretAPI",
        }
        assert expected == json.loads(
            cli_result(
                cli,
                [
                    "--path",
                    config,
                    "--out",
                    outfile,
                    "--env",
                    "dev",
                    "--secrets",
                    json.dumps({"KEY": "secretKEY", "API": "secretAPI"}),
                ],
            )
        )

        with open(outfile) as f:
            assert expected == json.load(f)
