# This file contains some general purpose utilities to be imported by the
# rest of the configuration program. Most of them are defined for one-time
# use by validation functions.
import os
import yaml
import toolz
import itertools
import tempfile
from contextlib import contextmanager
from collections.abc import MutableMapping
from pathlib import Path


def child_keys(dct):
    for v in dct.values():
        for k in v.keys():
            yield k


def yaml_load(path):
    with open(path) as f:
        return yaml.safe_load(f)


def load_profile_yaml(path):
    return yaml_load(path)


def load_schema_yamls(path):
    return [yaml_load(p) for p in Path(path).iterdir()]


def walk_keys(d, parents=()):
    for k, v in d.items():
        if isinstance(v, dict):
            for path in walk_keys(v, parents=(*parents, k)):
                yield path
        else:
            yield (*parents, k, v)


def find_duplicate(xs):
    found = set()
    for x in xs:
        if x in found:
            yield x
        found.add(x)


def find_missing(s, xs):
    for x in xs:
        if x not in s:
            yield x


def find_matching(s, xs):
    for x in xs:
        if x in s:
            yield x


def find_duplicate_keys(*dcts):
    all_keys = toolz.concat((d.keys() for d in dcts))
    for duplicate in find_duplicate(all_keys):
        yield duplicate


def find_matching_keys(xs, *dcts):
    all_keys = toolz.concat((d.keys() for d in dcts))
    for match in find_matching(xs, all_keys):
        yield match


def find_missing_keys(xs, *dcts):
    all_keys = toolz.concat((d.keys() for d in dcts))
    for missing in find_missing(xs, all_keys):
        yield missing


def flatten_path_matching(key_set, dct):
    if not isinstance(dct, dict):
        return dct
    result = {k: flatten_path_matching(key_set, v) for k, v in dct.items()}
    for key, value in sorted(result.items()):
        if key in key_set:
            return value
    return result


@contextmanager
def temporary_fs(tree):
    """Given a dictionary representing a file system directory structure,
    return a context manager that creates the corresponding folders and files
    in Python's temporary file location. All created folders and files will
    be deleted when the context manager exits.

    The dictionary should take the shape of a tree of nested dictionaries. The
    "leaves" of the tree (non-dictionary values) will be written to files with
    their key as the file name, otherwise a folder will be created with the
    key as the folder name.

    If a "leaf" if of type 'bytes', it will be written to file as bytes.
    Otherwise, it will be coerced to a string with str() and written to a
    file as a UTF-8 string.
    """
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
