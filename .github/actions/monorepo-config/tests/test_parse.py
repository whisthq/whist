#!/usr/bin/env python
import pytest
import yaml
import os
import tests.mock_data as mock_data
import helpers.parse as parse
from pathlib import Path
from contextlib import contextmanager
from helpers.utils import temporary_fs
from helpers.validate import ValidationError


def flatten_child(profiles, child):
    for profile in reversed(profiles):
        if profile in child:
            return child[profile]
    return child


@pytest.mark.skip()
def test_parse_profiles():
    with temporary_fs(mock_data.config_simple_fs) as tempdir:
        config = tempdir.joinpath("config")

        expect_1 = mock_data.config_simple_json_dev
        assert expect_1 == parse.parse(config, profiles=["dev"])

        expect_2 = mock_data.config_simple_json_stg
        assert expect_2 == parse.parse(config, profiles=["stg"])

        expect_3 = mock_data.config_simple_json_prd
        assert expect_3 == parse.parse(config, profiles=["prd"])

        expect_4 = mock_data.config_simple_json_default
        assert expect_4 == parse.parse(config)


@pytest.mark.skip()
def test_parse_secrets():
    with temporary_fs(mock_data.config_secrets_fs) as tempdir:
        config = tempdir.joinpath("config")
        secrets = [{"KEY": "secretKEY", "API": "secretAPI"}]
        expect_1 = mock_data.config_secrets_json
        assert expect_1 == parse.parse(config, secrets=secrets)


@pytest.mark.skip()
def test_parse_secrets_profiles():
    with temporary_fs(mock_data.config_secrets_profiles_fs) as tempdir:
        config = tempdir.joinpath("config")
        secrets = [{"KEY": "secretKEY", "API": "secretAPI"}]

        expect_1 = mock_data.config_secrets_profiles_json_dev
        assert expect_1 == parse.parse(
            config, profiles=["dev"], secrets=secrets
        )

        expect_1 = mock_data.config_secrets_profiles_json_stg
        assert expect_1 == parse.parse(
            config, profiles=["stg"], secrets=secrets
        )

        expect_1 = mock_data.config_secrets_profiles_json_prd
        assert expect_1 == parse.parse(
            config, profiles=["prd"], secrets=secrets
        )


a = {
    "dev": {"macos": "dev-mac", "win32": "dev-win", "linux": "dev-lnx"},
    "stg": {"macos": "stg-mac", "win32": "stg-win", "linux": "stg-lnx"},
    "prd": {"macos": "prd-mac", "win32": "prd-win", "linux": "prd-lnx"},
}
