#!/usr/bin/env python
import pytest
import itertools
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
    return draw(dictionaries(text(), dictionaries(text(), text(), max_size=10), max_size=10))


def recursive_dicts():
    return recursive(dictionaries(text(), text()), lambda x: dictionaries(text(), x))


@given(lists(nested_dict(), max_size=8))
@example([example_dict1, example_dict2])
def test_merge(dicts):
    cache = {}
    for d in dicts:
        for k, v in d.items():
            cache[k] = v

    assert cache == utils.merge(*dicts)


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


@given(recursive_dicts())
def test_walk_keys(dct):
    has_keys = "all keys in outputs paths must exist in input dict"

    for path in utils.walk_keys(dct):
        subdct = dct
        for key in path[:-1]:
            assert key in subdct, has_keys
            subdct = subdct[key]
        assert not isinstance(path[-1], dict)


@given(integers(max_value=10), text(max_size=100), text(max_size=5))
@example(10, "longword", "...")
def test_truncated(limit, string, trail):
    has_trail = "result must include trail"
    no_change = "result under limit must be returned unchanged"
    be_string = "result must be a string"
    limited = "result must have length greater than 0, less or equal to limit"
    includes = "result must include input up to (limit - len(trail)) chars"

    clamped = max(limit, 0)

    assert isinstance(string, str), be_string

    result = utils.truncated(limit, string, trail)
    result_default_trail = utils.truncated(limit, string)

    if len(string) > limit:
        assert result_default_trail.endswith("..."[:clamped]), has_trail
        assert result.endswith(trail[:clamped]), has_trail
    else:
        assert result == string, no_change

    assert len(result) <= clamped, limited
    assert result.startswith(string[: len(result) - clamped]), includes


# result is dict with trunctated values, or same values
@given(dictionaries(text(), text()))
def test_truncated_children(dct):
    truncates = "all values must be truncated up to limit"

    result = utils.truncated_children(dct)
    assert all(v == "..." for v in result.values()), truncates


def test_nested_keys():
    assert sorted(utils.nested_keys(example_dict3)) == list("abcdefghijxyz")


def test_all_child_keys():
    all_children = "fn must be called with all keys below top-level of dict"
    no_top_level = "fn must not be called with top-level keys"
    children = set("bcdefghijyz")
    parents = set("ax")
    assert utils.all_child_keys(lambda x: x in children, example_dict3), all_children
    assert utils.all_child_keys(lambda x: x not in parents, example_dict3), no_top_level


def test_all_items_in_set_partial():
    superset = [1, 2, 3, 4, 5]
    applied = utils.all_items_in_set_partial(superset)
    assert applied([1, 2, 3])
    assert not applied([4, 5, 6])


# @pytest.mark.skip()
def test_all_child_keys_in_set_partial():
    all_child = "true when all child keys are in set"
    no_child = "false when some child keys not in set"
    no_parent = "parent keys should not be tested for inclusion"

    children = set("bcdefghijyz")
    parents = set("ax")
    in_children = utils.all_child_keys_in_set_partial(children)
    in_parents = utils.all_child_keys_in_set_partial(parents)
    assert in_children(example_dict3), all_child
    assert not in_children(example_dict1), no_child
    assert not in_parents(example_dict3), no_parent
