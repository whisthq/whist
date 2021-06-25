#!/usr/bin/env python
import pytest
import yaml
import os
import tests.mock_data as mock_data
from pathlib import Path
from contextlib import contextmanager
from helpers.parse import resolve, parse
from helpers.utils import temporary_fs
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


def flatten_child(profiles, child):
    for profile in reversed(profiles):
        if profile in child:
            return child[profile]
    return child


def test_parse_profiles():
    with temporary_fs(mock_data.config_simple_fs) as tempdir:
        config = tempdir.joinpath("config")

        expect_1 = mock_data.config_simple_json_dev
        assert expect_1 == parse(config, profiles=["dev"])

        expect_2 = mock_data.config_simple_json_stg
        assert expect_2 == parse(config, profiles=["stg"])

        expect_3 = mock_data.config_simple_json_prd
        assert expect_3 == parse(config, profiles=["prd"])

        expect_4 = mock_data.config_simple_json_default
        assert expect_4 == parse(config)


def test_parse_secrets():
    with temporary_fs(mock_data.config_secrets_fs) as tempdir:
        config = tempdir.joinpath("config")
        secrets = [{"KEY": "secretKEY", "API": "secretAPI"}]
        expect_1 = mock_data.config_secrets_json
        assert expect_1 == parse(config, secrets=secrets)


def test_parse_secrets_profiles():
    with temporary_fs(mock_data.config_secrets_profiles_fs) as tempdir:
        config = tempdir.joinpath("config")
        secrets = [{"KEY": "secretKEY", "API": "secretAPI"}]

        expect_1 = mock_data.config_secrets_profiles_json_dev
        assert expect_1 == parse(config, profiles=["dev"], secrets=secrets)

        expect_1 = mock_data.config_secrets_profiles_json_stg
        assert expect_1 == parse(config, profiles=["stg"], secrets=secrets)

        expect_1 = mock_data.config_secrets_profiles_json_prd
        assert expect_1 == parse(config, profiles=["prd"], secrets=secrets)


@given(schemas(), non_dict())
def test_resolve(schema, random_value):
    profiles, _source = schema

    base1 = resolve(profiles, random_value)
    assert base1 == random_value, "a value that wasn't a dictionary was returned changed."


@given(schemas())
@example((mock_data.example_profiles, mock_data.example_nested_map))
def test_resolve_flattening(schema):
    profiles, source = schema
    result = resolve(profiles, source)
    for key, child in source.items():
        if isinstance(child, dict):
            if any(p in child for p in profiles):
                for profile in reversed(profiles):
                    if profile in child:
                        assert result[key] == child[profile], (
                            "resolved child value must match" + " value of highest index profile"
                        )

                        break
            else:
                assert (
                    result[key] == child
                ), "child value must be unchanged if no profile match is found"


@given(schemas())
@example((mock_data.example_profiles, mock_data.example_nested_map))
def test_resolve_same_keys(schema):
    profiles, source = schema
    result = resolve(profiles, source)
    assert result.keys() == source.keys(), "result must have same keys as source"
