# This file is contains the main business logic for parsing configuration
# files, and merging them with external secrets. We import some helpers
# for utilities and validation, but the main logic all lives here.
import toolz
from pathlib import Path
from .utils import (
    child_keys,
    flatten_path_matching,
    load_profile_yaml,
    load_schema_yamls,
)
from .validate import (
    ValidationError,
    validate_paths,
    validate_inputs,
    validate_secrets,
)


def parse(dir_path, secrets=None, profiles=None):
    # Checking the structure of the config folder
    if error := validate_paths(dir_path):
        raise ValidationError(error)

    # Load dictionaries from yaml files
    profile_yaml = load_profile_yaml(Path(dir_path).joinpath("profile.yml"))
    schema_yamls = load_schema_yamls(Path(dir_path).joinpath("schema"))

    # Compare arguments to data in yaml files
    if error := validate_inputs(profiles, profile_yaml, schema_yamls):
        raise ValidationError(error)

    # Flatten the loaded dictionaries based on the profiles passed as arguments
    profile_set = profiles.values()
    schema = flatten_path_matching(profile_set, toolz.merge(*schema_yamls))

    # If no secrets to merge, return early
    if not secrets:
        return dict(sorted(schema))

    if error := validate_secrets(schema, secrets):
        raise ValidationError(error)

    # Merge secrets arguments into schema dictionary
    secrets = toolz.merge(*secrets)

    merged = toolz.merge(
        schema, {k: secrets[k] for k in schema if k in secrets}
    )

    return dict(sorted(merged))
