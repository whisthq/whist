# This file is contains the main business logic for parsing configuration
# files, and merging them with external secrets. We import some helpers
# for utilities and validation, but the main logic all lives here.
import yaml
from pathlib import Path
from .utils import merge
from .validate import (
    validate_schema_folder,
    validate_schema_data,
    validate_config_folder,
    validate_profiles_yaml,
    validate_profiles,
)


def resolve(profiles, schema, skip=True):
    """Given a list of profiles (nested keys of schema), and a nested
    schema (dictionary), recursively flatten the schema.

    Flattening is only performed on a child key if the key is present in the
    profiles list. If so, the value of the child key becomes its parent's
    value, replacing the child key and its siblings.

    If no child key in a nested dictionary is found in the profiles list, the
    nested dictionary remains unchanged.

    If two profiles are passed in that exist in the same map, then the
    right-most profile will be chosen. For example, with profiles=["mac, win"]
    and schema={"url": {"mac": "mac-url", "win" "win-url"}}, the result
    will be {"url: "win-url"}.

    If skip=True, then this layer of keys in the schema will not be flattened,
    and the function will recurse to the values of the keys. As the default
    is True, the top-level keys are not flattened by default.

    If the schema is not a dictionary, it is returned unchanged.

    Args:
        profiles: A list of keys (string) in a nested schema.
        schema:   A dictionary whose child dictionaries can be flattened.
    Returns:
        A dicitionary with flattened values in place of the "profiles" keys.
    """
    if not isinstance(schema, dict):
        return schema

    key_set = set(schema.keys())
    if not skip:
        for key in reversed(profiles):
            if key in key_set:
                return resolve(profiles, schema[key], skip=False)

    return {key: resolve(profiles, schema[key], skip=False) for key in key_set}


def yaml_load(path):
    with open(path) as f:
        return yaml.safe_load(f)


# We expect that developers will eventually make typing mistakes with the
# profiles in the configuration schema, so we pass in a valid_profiles
# list so that we can validate all the profile keys against a list of
# allowed profiles.
def schema_load(path, valid_profiles=()):
    data = yaml_load(path)
    validate_schema_data(data, valid_profiles)
    return data


def profile_load(path):
    profiles = yaml_load(path)
    validate_profiles_yaml(profiles)
    return profiles


# Schemas are parsed into a list of dictionaries, which are then merged.
# The merge happens from left to right, so that dictionaries later in the
# list will override keys that exist in dictionaries earlier in the list.
#
# The list of schema dictionaries is sorted by the path of the file from
# which it's loaded (alphabetically). We shouldn't have duplicate keys in
# the schema files, so this should never be an issue, but we are explicit
# about the behavior so that our output is consistent.


def parse(config_folder, secrets=(), profiles=()):
    schema_folder = Path(config_folder).joinpath("schema")
    profiles_yaml = Path(config_folder).joinpath("profiles.yml")

    validate_config_folder(config_folder)
    validate_schema_folder(schema_folder)

    valid_profiles = profile_load(profiles_yaml)

    # We don't want None values in the secrets map to override values in
    # the schema maps, so we'll make sure the secret maps do not contain None.
    merged_secrets = {k: v for k, v in merge(*secrets).items() if v is not None} if secrets else {}

    # We sort the paths because we're unsure about the order from iterdir().
    sorted_paths = sorted(Path(schema_folder).iterdir(), key=str)

    loaded_schemas = [schema_load(p, valid_profiles=valid_profiles) for p in sorted_paths]

    merged_schemas = merge(*loaded_schemas)

    # Merge all the schema maps, with the secrets map getting to override
    # existing schema keys if the key exists in the secrets map.
    merged_all = merge(
        merged_schemas,
        {k: merged_secrets[k] for k in merged_schemas if k in merged_secrets},
    )

    # Before we return, we sort the result alphabetically by key, as new
    # versions of Python now guarantee dictionary order. This is helpful to
    # keep the output consistent when logging and inspecting the data.
    return dict(sorted(resolve(profiles, merged_all).items()))
