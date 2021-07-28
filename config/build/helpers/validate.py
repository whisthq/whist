# This file contains validation functions to help provide useful errors to
# the user in case improper data is passed. This is necessary because our
# configuration has some rather specific constraints.
#
# There are two types of functions in this file. The early functions define
# a general-purpose validation framework. They represent a simple way to
# describe a validation scheme, and have no knowledge of our actual domain
# constraints. The later functions employ this validation framework, performing
# validations based on the actual needs of our configuration program. Multiple
# validation errors are grouped into a single error on the console.
#
# The import list for this file is a little long. This is because we define
# many small (often one-line) functions elsewhere, which saves us from using
# a bunch of lambda functions here. This makes the file a little more readable,
# but it also ensures that these predicate functions have a name. The name is
# important, because our general-purpose validation framework will try and
# helpfully print the name of the failing function along with the error.
import typing
import inspect
import toolz
from pprint import pformat
from pathlib import Path
from .utils import (
    walk_keys,
    child_keys,
    find_matching,
    find_missing,
    find_duplicate,
    find_missing_keys,
    find_matching_keys,
    find_duplicate_keys,
)

from dataclasses import dataclass


class ValidationError(Exception):
    """A custom error class that is initialized with a validation failure
    dictionary. Validation failure dictionaries are returned by a validation
    function when a input does not pass validation. If an input passes,
    then validation functions return None.

    The only required key in the validation failure dictionary is 'message',
    which will be come the error message for the exception raised. Other keys
    will be printed as a dictionary as a part of the error message."""

    def __init__(self, data):
        chart = ""
        message = data.get("message", "Validation error")
        if data:
            for key, value in toolz.dissoc(data, "message").items():
                chart = f"{chart}\n    {key} = {pformat(value)}"
        super().__init__(f"{message}{chart}")


def validate_profile_yaml(profile_map):
    if not isinstance(profile_map, dict):
        return {
            "message": "profile map must be a dictionary",
            "found": profile_map,
        }

    groups = profile_map.values()

    for invalid in (g for g in groups if not isinstance(g, (list, tuple))):
        return {
            "message": "profile groups should be lists of strings",
            "found": invalid,
        }

    all_profiles = list(toolz.concat(groups))

    for invalid in (g for g in all_profiles if not isinstance(g, str)):
        return {
            "message": "all profiles in groups must be strings",
            "found": invalid,
        }

    for duplicate in find_duplicate(all_profiles):
        return {"message": "no duplicate profile keys", "found": duplicate}


def validate_schema_yamls(profile_map, schemas):
    for invalid in (s for s in schemas if not (isinstance(s, dict))):
        return {
            "message": "all schemas must be dictionaries",
            "found": invalid,
        }

    merged = toolz.merge(*schemas)

    profile_groups = profile_map.keys()
    profile_sets = [set(v) for v in profile_map.values()]
    profile_keys = set(item for sublist in profile_sets for item in sublist)
    reserved = set(toolz.concat([profile_keys, profile_groups]))

    # No duplicate top-level keys
    for duplicate in find_duplicate_keys(*schemas):
        return {"message": "no duplicate top-level keys", "found": duplicate}

    # No top-level keys matching profile names or group_names
    for match in find_matching_keys(reserved, merged):
        return {"message": "schema/profile name collision", "found": match}

    verified_child = (
        validate_child(profile_sets, v, path=[k]) for k, v in merged.items()
    )

    return validate_root(merged) or next(
        (i for i in verified_child if i is not None), None
    )


def validate_root(dct):
    leaf_paths = list(walk_keys(dct))

    for path in leaf_paths:
        for duplicate in find_duplicate(path[:-1]):  # drop leaf
            index = path.index(duplicate)
            return {
                "message": "no duplicate parent and child keys",
                "path": path[:index],
                "found": duplicate,
            }


def validate_child(key_sets, dct, path=()):
    if not isinstance(dct, dict):
        return

    keys = list(dct.keys())

    if not any(find_matching([set(keys)], [set(s) for s in key_sets])):
        return {
            "message": "no matching profile set,"
            + " should match a valid group in profile.yml",
            "path": path,
            "found": keys,
            "valid": key_sets,
        }

    for key in keys:
        if message := validate_child(key_sets, dct[key], [*path, key]):
            return message


def validate_profiles(profile_map, profiles):
    for group, value in profiles.items():
        if not isinstance(value, str):
            return {
                "message": "profile argument must be string",
                "found": value,
                "valid": profile_map,
            }
        if group not in profile_map:
            return {
                "message": "invalid profile argument, not a profile.yml group",
                "found": group,
                "valid": profile_map,
            }
        if value not in set(profile_map[group]):
            return {
                "message": "invalid profile argument, not a profile.yml value",
                "found": value,
                "valid": profile_map,
            }


def validate_secrets(secrets):
    if not isinstance(secrets, dict):
        return {
            "message": "wrong type for secrets map, expected dict",
            "found": secrets,
        }


def validate_secrets_keys(schema, secrets):
    for missing in find_missing_keys(set(schema.keys()), secrets):
        yield {
            "message": "key in secrets map not present in config schema",
            "found": missing,
        }


def validate_inputs(profile_args, profile_yaml, schema_yamls):
    errors = [
        validate_profile_yaml(profile_yaml),
        validate_schema_yamls(profile_yaml, schema_yamls),
        validate_profiles(profile_yaml, profile_args),
    ]
    for error in (e for e in errors if e):
        return error


def validate_schema_path(path):
    expected = f"{Path(path).name}/schema"
    for p in Path(path).joinpath("schema").iterdir():
        if p.suffix != ".yml":
            return {
                "message": f"file in {expected} without .yml extension",
                "path": str(path),
                "found": p.name,
            }


def validate_profile_path(path):
    child_names = set(p.name for p in Path(path).iterdir())
    if "profile.yml" not in child_names:
        return {"message": "no profile.yml found in config folder"}


def validate_paths(dir_path):
    errors = [
        validate_profile_path(dir_path),
        validate_schema_path(dir_path),
    ]
    for error in (e for e in errors if e):
        return error
