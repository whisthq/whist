import pytest
import tests.mock_data as mock_data
import helpers.parse as parse
from pathlib import Path
from helpers.utils import temporary_fs
from helpers.validate import ValidationError


def test_parse_secrets_profiles():
    """Test that the parse functions properly validates input data and
    outputs the expected configuration JSON."""
    mock = mock_data.config_secrets_profiles_fs
    with temporary_fs(mock) as tempdir:
        config = tempdir.joinpath("config")
        secrets = {"KEY": "secretKEY", "API": "secretAPI"}
        profiles = {"deploy": "dev"}

        result = parse.parse(config, secrets=secrets, profiles=profiles)
        assert result == {
            **mock_data.config_secrets_profiles_json_dev,
            **secrets,
        }, "invalid output data structure"

        with pytest.raises(ValidationError):
            # should raise error if secrets key arg not in schema
            parse.parse(
                config, secrets={**secrets, "missing": 0}, profiles=profiles
            )

        with pytest.raises(ValidationError):
            # should raise error if profile arg not in profiles.yml
            parse.parse(
                config, secrets=secrets, profiles={**profiles, "missing": "x"}
            )

    mock = mock_data.config_simple_fs
    with temporary_fs(mock) as tempdir:
        config = tempdir.joinpath("config")

        result = parse.parse(config, profiles=profiles)
        assert (
            result == mock_data.config_simple_json_dev
        ), "invalid output data structure"
