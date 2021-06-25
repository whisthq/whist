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
from pathlib import Path
from .utils import (
    truncated,
    truncated_children,
    is_yml,
    children,
    file_exists,
    dir_exists,
    is_dir,
    is_list,
    is_dict,
    all_string,
    all_yml_children,
    not_empty_dict,
    has_child_path_partial,
    no_keys_start_with_number,
    no_keys_start_with_underscore,
    all_keys_uppercase,
    all_keys_valid_IEEE_variable,
    all_items_in_set_partial,
    all_child_keys_in_set_partial,
)

# Some common errors that are likely to result from validation mistakes.
# The errors below will be caught and reported as a group. Other errors will
# be raised normally.
PRINT_LIMIT = 100  # truncate message after this many characters.
GROUPABLE_ERRORS = (
    AttributeError,
    KeyError,
    IndexError,
    TypeError,
    ValueError,
    ZeroDivisionError,
)


def must(test_fn, msg=None):
    def from_value(value):
        error = False
        try:
            result = test_fn(value)
        except GROUPABLE_ERRORS as e:
            result = e
            error = e

        tstr = error if error else f"<{type(value).__name__}>"

        val_string = "got: " + truncated(
            PRINT_LIMIT, f"{truncated_children(value)}: {tstr}"
        )

        msg_present = f"{msg}... {val_string}."
        msg_default = f"{test_fn.__name__} returns {result}... {val_string}"

        message = msg_present if msg else msg_default
        assert result and not error, message
        return True

    return from_value


def validate_safe(i, *validators):
    errors = []
    for valid_fn in validators:
        try:
            valid_fn(i)
        except AssertionError as err:
            errors.append(err)
    return errors


def validate(i, *validators):
    errors = validate_safe(i, *validators)
    if errors:
        value = truncated(PRINT_LIMIT, truncated_children(i))
        message = f"'{value}' failed validation.\n    Errors:"
        for index, error in enumerate(errors):
            message += f"\n    {index + 1}. "
            message += error.args[0]
        raise AssertionError(message)
    return True


def validate_schema_folder(schema_folder_path):
    validate(
        schema_folder_path,
        must(dir_exists, "path must exist"),
        must(is_dir, "path must be folder"),
        must(all_yml_children, "folder must contain only '.yml'"),
    )


def validate_config_folder(config_folder_path):
    validate(
        config_folder_path,
        must(dir_exists, "path must exist"),
        must(is_dir, "path must be folder"),
        must(
            has_child_path_partial("schema"),
            "schema must be a subfolder of working directory",
        ),
        must(
            has_child_path_partial("profiles.yml"),
            "profiles.yml must be a file in working directory",
        ),
    )


def validate_profiles(profiles, valid_profiles):
    validate(
        profiles,
        must(
            all_items_in_set_partial(valid_profiles),
            f"all profiles must be in {valid_profiles}",
        ),
    )


def validate_profiles_yaml(profiles):
    validate(
        profiles,
        must(is_list, "profiles.yml must contain only a list"),
        must(all_string, "profiles.yml must contain only strings"),
    )


# In case schema keys are used as environment variables, we'll only allow
# uppercase letters, numbers, and underscore. No starting with a number.
def validate_schema_data(data, valid_profiles):
    validate(
        data,
        must(is_dict, "schema data must be a dict"),
        must(not_empty_dict, "schema data must not be empty"),
        must(
            no_keys_start_with_number,
            "schema keys cannot start with a number ",
        ),
        must(
            no_keys_start_with_underscore,
            "schema keys cannot start with an underscore ",
        ),
        must(all_keys_uppercase, "schema keys must be uppercase"),
        must(
            all_keys_valid_IEEE_variable,
            "schema key characters can only be A-Z, 0-9, and _",
        ),
        must(
            all_child_keys_in_set_partial(valid_profiles),
            f"nested schema keys must be one of {valid_profiles}",
        ),
    )
