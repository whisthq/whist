# This file contains some general purpose utilities to be imported by the
# rest of the configuration program. Most of them are defined for one-time
# use by validation functions.
import os
import itertools
import tempfile
from contextlib import contextmanager
from collections.abc import MutableMapping
from pathlib import Path


def merge(*dicts):
    """Combine dictionaries into a single dictionary. This is a "shallow"
    merge function, so only the top-level keys will be merged.

    Conflicting keys will be overriden by the "right-most" dictionary."""
    result = {}
    for d in dicts:
        result.update(d)
    return result


def truncated(limit, string, trail="..."):
    clamped = max(limit, 0)
    if len(str(string)) <= clamped:
        return string
    else:
        if clamped < len(trail):
            return trail[:clamped]
        end = clamped - len(trail)
        return str(string)[:end] + trail


def truncated_children(d):
    if not isinstance(d, MutableMapping):
        return d
    return {k: "..." for k, v in d.items()}


def children(path):
    return Path(path).iterdir()


def has_suffix(suffix, path):
    return Path(path).suffix == suffix


def is_yml(path):
    return has_suffix(".yml", path)


def is_file(path):
    return Path(path).is_file()


def is_dir(path):
    return Path(path).is_dir()


def file_exists(path):
    return Path(path).exists()


def dir_exists(path):
    return Path(path).is_dir()


def all_chars(fn, string):
    return all(fn(i) for i in string)


def all_string(lst):
    return all(isinstance(i, str) for i in lst)


def any_keys(fn, dct):
    return any(fn(i) for i in dct.keys())


def all_keys(fn, dct):
    return all(fn(i) for i in dct.keys())


def all_children(fn, path):
    return all(fn(i) for i in children(path))


def all_yml_children(path):
    return all_children(is_yml, path)


def is_dict(data):
    return isinstance(data, dict)


def is_list(data):
    return isinstance(data, list)


def not_empty_dict(data):
    return len(data.keys()) > 0


def is_string(data):
    return isinstance(data, str)


def start_with_number(data):
    return str(data[0]) in set("1234567890")


def start_with_underscore(data):
    return str(data[0]) == "_"


def no_keys_start_with_number(dct):
    return not any_keys(start_with_number, dct)


def no_keys_start_with_underscore(dct):
    return not any_keys(start_with_underscore, dct)


def is_uppercase(string):
    lowercase = set("abcdefghijklmnopqrstuvwxyz")
    return all(c not in lowercase for c in string)


def all_keys_uppercase(dct):
    return all_keys(is_uppercase, dct)


def valid_IEEE_variable(string):
    allowed = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_")
    return all(c in allowed for c in string)


def all_keys_valid_IEEE_variable(dct):
    return all_keys(valid_IEEE_variable, dct)


def has_child_path_partial(child_path):
    def has_child_path(parent_path):
        return Path(parent_path).joinpath(child_path).exists()

    return has_child_path


def walk_keys(d, parents=()):
    for k, v in d.items():
        if isinstance(v, dict):
            for path in walk_keys(v, parents=(*parents, k)):
                yield path
        else:
            yield (*parents, k, v)


def nested_keys(dct):
    if not isinstance(dct, dict):
        return []
    child_keys = (nested_keys(v) for v in dct.values() if isinstance(v, dict))
    return [*dct.keys(), *itertools.chain(*child_keys)]


def child_nested_keys(dct):
    for v in dct.values():
        if isinstance(v, dict):
            for k in v.keys():
                yield k


def all_child_keys(fn, dct):
    for path in walk_keys(dct):
        child_keys = path[1:-1]
        if not all(fn(k) for k in child_keys):
            return False
    return True


def all_items_in_set_partial(lst):
    allowed = set(lst)

    def all_items_in_set(items):
        return set(items).issubset(allowed)

    return all_items_in_set


def all_child_keys_in_set_partial(profiles):
    profiles_set = set(profiles)

    def all_child_keys_in_set(data):
        return all_child_keys(lambda x: x in profiles_set, data)

    return all_child_keys_in_set


@contextmanager
def temporary_fs(tree):
    with tempfile.TemporaryDirectory() as tempdir:

        for path in walk_keys(tree):
            file_data = path[-1]
            file_name = path[-2]
            file_path = path[:-2]
            file_mode = "wb"

            parent = Path(tempdir).joinpath(*file_path)
            os.makedirs(parent, exist_ok=True)

            if not isinstance(file_data, bytes):
                file_mode = "w"
                file_data = str(file_data)

            with open(parent.joinpath(file_name), file_mode) as f:
                f.write(file_data)
        yield Path(tempdir)
