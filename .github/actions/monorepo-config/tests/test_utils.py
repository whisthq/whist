#!/usr/bin/env python
import pytest
import itertools
import toolz
import helpers.utils as utils
from functools import partial
from collections.abc import MutableMapping
from pathlib import Path
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
    functions,
    recursive,
)

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


@composite
def nested_dict(draw):
    return draw(
        dictionaries(
            text(), dictionaries(text(), text(), max_size=10), max_size=10
        )
    )


def recursive_dicts():
    return recursive(
        dictionaries(text(), text()), lambda x: dictionaries(text(), x)
    )


def test_temporary_fs():
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


# @given(recursive_dicts())
# def test_walk_keys(dct):
#     has_keys = "all keys in outputs paths must exist in input dict"

#     for path in utils.walk_keys(dct):
#         subdct = dct
#         for key in path[:-1]:
#             assert key in subdct, has_keys
#             subdct = subdct[key]
#         assert not isinstance(path[-1], dict)


# @given(integers(max_value=10), text(max_size=100), text(max_size=5))
# @example(10, "longword", "...")
# def test_truncated(limit, string, trail):
#     has_trail = "result must include trail"
#     no_change = "result under limit must be returned unchanged"
#     be_string = "result must be a string"
#     limited = "result must have length greater than 0, less or equal to limit"
#     includes = "result must include input up to (limit - len(trail)) chars"

#     clamped = max(limit, 0)

#     assert isinstance(string, str), be_string

#     result = utils.truncated(limit, string, trail)
#     result_default_trail = utils.truncated(limit, string)

#     if len(string) > limit:
#         assert result_default_trail.endswith("..."[:clamped]), has_trail
#         assert result.endswith(trail[:clamped]), has_trail
#     else:
#         assert result == string, no_change

#     assert len(result) <= clamped, limited
#     assert result.startswith(string[: len(result) - clamped]), includes


# # result is dict with trunctated values, or same values
# @given(dictionaries(text(), text()))
# def test_truncated_children(dct):
#     truncates = "all values must be truncated up to limit"

#     result = utils.truncated_children(dct)
#     assert all(v == "..." for v in result.values()), truncates


# def test_nested_keys():
#     assert sorted(utils.nested_keys(example_dict3)) == list("abcdefghijxyz")


# def test_all_child_keys():
#     all_children = "fn must be called with all keys below top-level of dict"
#     no_top_level = "fn must not be called with top-level keys"
#     children = set("bcdefghijyz")
#     parents = set("ax")
#     assert utils.all_child_keys(
#         lambda x: x in children, example_dict3
#     ), all_children
#     assert utils.all_child_keys(
#         lambda x: x not in parents, example_dict3
#     ), no_top_level


# def test_all_items_in_set_partial():
#     superset = [1, 2, 3, 4, 5]
#     applied = utils.all_items_in_set_partial(superset)
#     assert applied([1, 2, 3])
#     assert not applied([4, 5, 6])


# def test_all_child_keys_in_set_partial():
#     all_child = "true when all child keys are in set"
#     no_child = "false when some child keys not in set"
#     no_parent = "parent keys should not be tested for inclusion"

#     children = set("bcdefghijyz")
#     parents = set("ax")
#     in_children = utils.all_child_keys_in_set_partial(children)
#     in_parents = utils.all_child_keys_in_set_partial(parents)
#     assert in_children(example_dict3), all_child
#     assert not in_children(example_dict1), no_child
#     assert not in_parents(example_dict3), no_parent


def test_find_duplicate():
    xs = [1, 2, 2, 3, 4, 5, 6, 6, 7, 8, 9, 10, 10, 11]
    dupes = utils.find_duplicate(xs)
    assert next(dupes) == 2
    assert next(dupes) == 6
    assert next(dupes) == 10
    for rest in dupes:
        assert False, f"generator should be empty, still has {rest}"


def test_find_missing():
    s = {"a", "b", "c", "d", "e", "f"}
    xs = ["a", "b", "c", "d", "g", "e", "f", "i"]
    missing = utils.find_missing(s, xs)
    assert next(missing) == "g"
    assert next(missing) == "i"
    for rest in missing:
        assert False, f"generator should be empty, still has {rest}"


def test_find_matching():
    s = {"a", "b", "c", "d", "e", "f"}
    xs = ["z", "y", "x", "c", "w", "f", "v", "u"]
    matching = utils.find_matching(s, xs)
    assert next(matching) == "c"
    assert next(matching) == "f"
    for rest in matching:
        assert False, f"generator should be empty, still has {rest}"


def test_find_duplicate_keys():
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
