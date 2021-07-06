#!/usr/bin/env python
import pytest
import itertools
import toolz
import helpers.utils as utils
from functools import partial
from pathlib import Path

example_dict1 = {
    "dev": {"x": "t", "y": "u", "z": "v"},
    "prod": {"d": "q", "e": "r", "f": "s"},
}

example_dict2 = {
    "dev": {"d": "q", "e": "r", "f": "s"},
    "prod": {"d": "q", "e": "r", "f": "s"},
}


example_dict3 = {
    "a": {
        "b": {"e": {"i": 3}, "f": {"j": 4}},
        "c": {"g": 1},
        "d": {"h": 2},
    },
    "x": {"y": 5, "z": 6},
}


def test_temporary_fs():
    """Test that the temporary file system creator creates the expected
    files/folders in the temporary locations, and deletes everything upon
    exiting."""
    fs_exists = "path should exist in contextmanager"
    fs_delete = "path should not exist outside of contextmanager"
    fs_data = "file at path should contain input data"

    paths = []
    with utils.temporary_fs(example_dict3) as tempdir:
        for *file_path, file_name, data in utils.walk_keys(example_dict3):
            path = Path(tempdir).joinpath(*file_path, file_name)
            assert path.exists()

            file_mode = "rb"
            if not isinstance(data, bytes):
                file_mode = "r"

            with open(path, file_mode) as f:
                if file_mode == "rb":
                    assert data == f.read(), fs_exists
                else:
                    assert str(data) == f.read(), fs_data

            paths.append(path)
    for pth in paths:
        assert not pth.exists(), fs_delete


def test_find_duplicate():
    """Test that duplicate elements in a list are yielded."""
    xs = [1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10, 10, 11]
    dupes = utils.find_duplicate(xs)
    assert next(dupes) == 2
    assert next(dupes) == 6
    assert next(dupes) == 10
    for rest in dupes:
        assert False, f"generator should be empty, still has {rest}"


def test_find_missing():
    """Test that missing elements in a list are yielded."""
    s = {"a", "b", "c", "d", "e", "f"}
    xs = ["a", "b", "c", "d", "g", "e", "f", "i"]
    missing = utils.find_missing(s, xs)
    assert next(missing) == "g"
    assert next(missing) == "i"
    for rest in missing:
        assert False, f"generator should be empty, still has {rest}"


def test_find_matching():
    """Test that elements in a list matching a set are yielded."""
    s = {"a", "b", "c", "d", "e", "f"}
    xs = ["z", "y", "x", "c", "w", "f", "v", "u"]
    matching = utils.find_matching(s, xs)
    assert next(matching) == "c"
    assert next(matching) == "f"
    for rest in matching:
        assert False, f"generator should be empty, still has {rest}"


def test_find_duplicate_keys():
    """Test that keys in a dict matching another dict are yielded."""
    a = {"a": 0, "b": 1, "c": 2, "d": 3, "e": 4}
    b = {"d": 5, "e": 6, "f": 7, "g": 8, "h": 9}
    c = {"h": 5, "i": 6, "j": 7, "k": 8, "l": 9}
    duplicate = utils.find_duplicate_keys(a, b, c)
    assert next(duplicate) == "d"
    assert next(duplicate) == "e"
    assert next(duplicate) == "h"
    for rest in duplicate:
        assert False, f"generator should be empty, still has {rest}"


def test_find_matching_keys():
    """Test that keys in a dict matching another dict are yielded."""
    a = {"a": 0, "b": 1, "c": 2, "d": 3, "e": 4}
    b = {"d": 5, "e": 6, "f": 7, "g": 8, "h": 9}
    c = {"h": 5, "i": 6, "j": 7, "k": 8, "l": 9}
    xs = {"a", "c", "j", "k"}
    matching = utils.find_matching_keys(xs, a, b, c)
    assert next(matching) == "a"
    assert next(matching) == "c"
    assert next(matching) == "j"
    assert next(matching) == "k"
    for rest in matching:
        assert False, f"generator should be empty, still has {rest}"


def test_flatten_path_matching():
    """Test that a dict is flattened so that nested dicts with keys matching
    a set are replaced with their corresponding value."""
    key_set = {"a", "b", "c"}
    d = {
        "x": {"a": {"b": 10}},
        "y": {"z": {"w": 15}},
        "z": {"c": {"a": 20}},
    }

    result = {k: utils.flatten_path_matching(key_set, v) for k, v in d.items()}

    assert result["x"] == 10, "keys in set should be flattened"
    assert result["y"] == d["y"], "keys not in set should not be flattened"
    assert result["z"] == 20, "keys in set should be flattened"

    e = toolz.merge(
        {"API": None, "KEY": None},
        {
            "URL": {
                "dev": "http://url-dev.com",
                "prd": "http://url-prd.com",
                "stg": "http://url-stg.com",
            }
        },
    )

    expected = {"API": None, "KEY": None, "URL": "http://url-dev.com"}

    result = {k: utils.flatten_path_matching({"dev"}, v) for k, v in e.items()}
    assert result == expected
