#!/usr/bin/env python3
import tests.mock_data as mock_data
import helpers.validate as valid
import helpers.utils as utils


def test_validate_profiles():
    """Test the validation functions that verify the structure of a
    'profile map', which is a dictionary mapping of 'profiles'
    e.g.(macos, win32, linux) to config values e.g (mac.dmg, win.exe)"""
    profile_map = {"deploy": ["b", "c"], "os": ["e", "f"]}

    not_string = {"deploy": [1, 2], "os": "e"}

    assert valid.validate_profiles(
        profile_map, not_string
    ), "should fail if profile argument has non-string value"

    not_in_map_keys = {"deploy": ["b", "c"], "badkey": ["e", "f"]}
    assert valid.validate_profiles(
        profile_map, not_in_map_keys
    ), "should fail if profile argument has key missing from profile.yml"

    not_in_map_vals = {"deploy": "b", "os": "z"}
    assert valid.validate_profiles(
        profile_map, not_in_map_vals
    ), "should fail if profile argument has value missing from profile.yml"

    good_profiles = {"deploy": "b", "os": "f"}

    assert not valid.validate_profiles(
        profile_map, good_profiles
    ), "should pass on well-formed profile argument"


def test_validate_secrets():
    """Test the validation functions that verify the structure of a
    'secrets map', which is a dictionary mapping of 'secrets'
    e.g.(HEROKU_API_KEY) to config values"""
    schema = {"key1": None, "key2": None, "key3": "value3"}
    not_dict = [{"key1": "value1", "key2": "value2"}]
    assert valid.validate_secrets(
        not_dict
    ), "should fail if secrets argument is not a dictionary"

    missing = {"key1": "value1", "key4": "value4"}
    assert valid.validate_secrets_keys(
        schema, missing
    ), "should fail if secrets argument contains key not in config schema"

    secrets = {"key1": "value1", "key2": "value2"}
    assert not valid.validate_secrets(
        secrets
    ), "should pass with well-formed secrets argument"


def test_validate_profile_yaml():
    """Test the validation functions that verify the structure of a
    'profile.yml', which is a the yaml file in the config folder that
    list valid profiles for config values."""
    is_list = [{"deploy": ["b", "c"], "os": ["e", "f"]}]

    assert valid.validate_profile_yaml(
        is_list
    ), "should fail if not a dictionary"

    not_str_list = {"deploy": ["b", [1, 2, 3]], "os": ["e", "f"]}

    assert valid.validate_profile_yaml(
        not_str_list
    ), "should fail if a child key is not a list of strings"

    not_str_value = {"deploy": ["b", 1], "os": ["e", "f"]}

    assert valid.validate_profile_yaml(
        not_str_value
    ), "should fail if a child key is not a list of strings"

    duplicate_child_value = {"deploy": ["b", "c"], "os": ["b", "f"]}

    assert valid.validate_profile_yaml(
        duplicate_child_value
    ), "should fail if any leaf values are duplicated"


def test_validate_schema_yamls():
    """Test the validation functions that verify the structure of a
    '.yml' files in the config/schema folder, which is where config values
    are stored."""
    profile_map = {"deploy": ["b", "c"], "os": ["e", "f"]}
    not_dict = [
        [{"a": {"b": {"c": 1}}, "d": {"e": 2}}],
        {"a": {"b": {"b": 1}}, "x": {"e": 2}},
    ]
    assert valid.validate_schema_yamls(
        profile_map, not_dict
    ), "should fail if a schema is not a dictionary"

    duplicate_parent_keys = [
        {"a": {"b": {"c": 1}}, "d": {"e": 2}},
        {"a": {"b": {"b": 1}}, "x": {"e": 2}},
    ]
    name_collision_parent = [
        {"deploy": {"b": {"c": 1}}, "d": {"e": 2}},
        {"a": {"b": {"b": 1}}, "x": {"e": 2}},
    ]

    name_collision_child = [
        {"f": {"b": {"c": 1}}, "d": {"e": 2}},
        {"a": {"b": {"b": 1}}, "x": {"e": 2}},
    ]

    assert valid.validate_schema_yamls(
        profile_map, duplicate_parent_keys
    ), "should fail if a duplicate top-level key is found"

    assert valid.validate_schema_yamls(
        profile_map, name_collision_parent
    ), "should fail if a top-level key collides with profile key"

    assert valid.validate_schema_yamls(
        profile_map, name_collision_child
    ), "should fail if a top-level key collides with profile key"

    duplicate_desc_keys = [{"a": {"b": {"b": 1}}, "d": {"e": 2}}]

    assert valid.validate_schema_yamls(
        profile_map, duplicate_desc_keys
    ), "should fail with a duplicate descendant key"

    missing_key_sets = [
        {
            "a": {"b": {"c": 1}},
            "d": {"f": 2},
            "m": {"x": 0, "c": 1},
        }
    ]

    assert valid.validate_schema_yamls(
        profile_map, missing_key_sets
    ), "should fail if any child key set doesn't match any profile map values"

    good_schemas = [
        {
            "a": {"b": 3, "c": {"e": 4, "f": 5}},
            "d": {"e": {"b": 6, "c": 7}, "f": 7},
        }
    ]

    assert not valid.validate_schema_yamls(profile_map, good_schemas)
