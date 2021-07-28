import json
import click
import platform
import tests.mock_data as mock_data
import helpers.cli
import helpers.parse as parse
from click.testing import CliRunner
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
    """Test that the parse function receives the expected arguments from
    the CLI wrapper."""
    mock = mocker.Mock(spec=parse.parse, return_value={"mock": {}})

    cli = helpers.cli.create_cli(mock)

    os_name = {"Darwin": "macos", "Windows": "win32", "Linux": "linux"}.get(
        platform.system()
    )

    with temporary_fs(mock_data.config_simple_fs) as tempdir:
        config = str(tempdir.joinpath("config"))

        cli_result(cli, ["--path", config])
        mock.assert_called_once_with(config, profiles={}, secrets={})
        mock.reset_mock()

        cli_result(cli, ["--path", config, "--deploy", "dev"])
        mock.assert_called_once_with(
            config, profiles={"deploy": "dev"}, secrets={}
        )
        mock.reset_mock()

        cli_result(cli, ["--path", config, "--deploy", "dev"])
        mock.assert_called_once_with(
            config, profiles={"deploy": "dev"}, secrets={}
        )
        mock.reset_mock()

        cli_result(cli, ["--path", config, "--deploy", "dev", "--os", "macos"])
        mock.assert_called_once_with(
            config, profiles={"deploy": "dev", "os": "macos"}, secrets={}
        )
        mock.reset_mock()

        cli_result(cli, ["--path", config, "--deploy", "dev", "--os", "auto"])
        mock.assert_called_once_with(
            config, profiles={"deploy": "dev", "os": "linux"}, secrets={}
        )
        mock.reset_mock()

        cli_result(
            cli,
            [
                "--path",
                config,
                "--secrets",
                json.dumps({"KEY": "secret001", "API": "secret002"}),
                "--secrets",
                json.dumps({"WEB": "secret003", "COM": "secret004"}),
                "--deploy",
                "dev",
                "--os",
                "macos",
            ],
        )
        mock.assert_called_once_with(
            config,
            profiles={"deploy": "dev", "os": "macos"},
            secrets={
                "KEY": "secret001",
                "API": "secret002",
                "WEB": "secret003",
                "COM": "secret004",
            },
        )
        mock.reset_mock()
