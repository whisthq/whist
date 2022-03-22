# This file is contains the main business logic for parsing configuration
# files, and merging them with external secrets. We import some helpers
# for utilities and validation, but the main logic all lives here.
import toolz
from functools import partial
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
    validate_secrets_keys,
)


def parse(dir_path, secrets=None, profiles=None):
    secrets = {} if secrets is None else secrets
    profiles = {} if profiles is None else profiles
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
    profile_set = set(profiles.values())
    schema = {
        k: flatten_path_matching(profile_set, v)
        for k, v in toolz.merge(*schema_yamls).items()
    }

    # If no secrets to merge, return early
    if not secrets:
        return dict(sorted(schema.items()))

    if error := validate_secrets(secrets):
        raise ValidationError(error)

    key_errors = [e for e in validate_secrets_keys(schema, secrets)]
    if any(key_errors):
        error = {
            "message": key_errors[0].get("message"),
            "found": [err["found"] for err in key_errors],
        }
        raise ValidationError(error)

    # Merge secrets arguments into schema dictionary
    merged = toolz.merge(
        schema, {k: secrets[k] for k in schema if k in secrets}
    )

    return dict(sorted(merged.items()))