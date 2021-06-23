#!/usr/bin/env python
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
    return draw(
        dictionaries(
            text(), dictionaries(text(), text(), max_size=10), max_size=10
        )
    )


@given(lists(nested_dict(), max_size=8))
@example([example_dict1, example_dict2])
def test_merge(dicts):
    cache = {}
    for d in dicts:
        for k, v in d.items():
            cache[k] = v

    assert cache == utils.merge(*dicts)


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


# def test_all_chars():
#     is_true = "result must be true if fn(char) True for all chars"
#     is_false = "result must be false if fn(char) is False for any char"

#     def all_t(c):
#         return c == "t"

#     assert utils.all_chars(all_t, "ttttttttttttt"), is_true
#     assert not utils.all_chars(all_t, "ttttxxxtt"), is_false


# def test_all_string():
#     is_true = "result must be true if all items are strings"
#     is_false = "result must be false if any item is not a string"

#     assert utils.all_string(["a", "b", "c", "d"]), is_true
#     assert not utils.all_string(["a", "b", 5, 6]), is_false


# def test_all_keys():
#     is_true = "result must be true if fn(key) is true for all keys in dict"
#     is_false = "result must be false if fn(key) is not true for a key in dict"

#     def all_abc(k):
#         return k in {"a", "b", "c"}

#     assert utils.all_keys(all_abc, {"a": 1, "b": 2, "c": 3}), is_true
#     assert not utils.all_keys(all_abc, {"a": 1, "e": 2, "f": 3}), is_false


# def test_any_keys():
#     is_true = "result must be true if fn(key) is true for any key in dict"
#     is_false = "result must be false if fn(key) is true for no key in dict"

#     def all_abc(k):
#         return k in {"a", "b", "c"}

#     assert utils.any_keys(all_abc, {"a": 1, "e": 2, "f": 3}), is_true
#     assert not utils.any_keys(all_abc, {"d": 1, "e": 2, "f": 3}), is_false


def test_nested_keys():
    assert sorted(utils.nested_keys(example_dict3)) == list("abcdefghijxyz")


def test_all_child_keys():
    keys = set("abcdefghijxyz")
    assert utils.all_child_keys(lambda x: x in keys, example_dict3)


def test_all_items_in_set_partial():
    superset = [1, 2, 3, 4, 5]
    applied = utils.all_items_in_set_partial(superset)
    assert applied([1, 2, 3])
    assert not applied([4, 5, 6])


def test_all_child_keys_in_set_partial():
    superset = list("abcdefghijxyz")
    applied = utils.all_child_keys_in_set_partial(superset)
    assert applied(example_dict3)
    assert not applied(example_dict2)
