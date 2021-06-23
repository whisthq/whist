#!/usr/bin/env python
import helpers.utils as utils
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
)

example_dict1 = {
    "dev": {"x": "t", "y": "u", "z": "v"},
    "prod": {"d": "q", "e": "r", "f": "s"},
}

example_dict2 = {
    "dev": {"d": "q", "e": "r", "f": "s"},
    "prod": {"d": "q", "e": "r", "f": "s"},
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
@given(dictionaries(text(), any()))
def test_truncated_children(dct):
    truncates = "all values must be truncated up to limit"

    result = utils.truncated_children()

    # if not isinstance(d, MutableMapping):
    #     return d
    # return {k: "{" + f"{truncated(0, v)}" + "}" for k, v in d.items()}


# def test_to_namedtuple(class_name, **fields):
#     return collections.namedtuple(class_name, fields)(*fields.values())


# def test_children(path):
#     return Path(path).iterdir()


# def test_has_suffix(suffix, path):
#     return Path(path).suffix == suffix


# def test_is_yml(path):
#     return has_suffix(".yml", path)


# def test_is_file(path):
#     return Path(path).is_file()


# def test_is_dir(path):
#     return Path(path).is_dir()


# def test_file_exists(path):
#     return Path(path).exists()


# def test_dir_exists(path):
#     return Path(path).is_dir()


# def test_all_chars(fn, string):
#     return all(fn(i) for i in string)


# def test_all_string(lst):
#     return all(isinstance(i, str) for i in lst)


# def test_any_keys(fn, dct):
#     return any(fn(i) for i in dct.keys())


# def test_all_keys(fn, dct):
#     return all(fn(i) for i in dct.keys())


# def test_all_children(fn, path):
#     return all(fn(i) for i in children(path))


# def test_all_yml_children(path):
#     return all_children(is_yml, path)


# def test_is_dict(data):
#     return isinstance(data, dict)


# def test_is_list(data):
#     return isinstance(data, list)


# def test_not_empty_dict(data):
#     return len(data.keys()) > 0


# def test_is_string(data):
#     return isinstance(data, str)


# def test_start_with_number(data):
#     return str(data[0]) in set("1234567890")


# def test_start_with_underscore(data):
#     return str(data[0]) == "_"


# def test_no_keys_start_with_number(dct):
#     return not any_keys(start_with_number, dct)


# def test_no_keys_start_with_underscore(dct):
#     return not any_keys(start_with_underscore, dct)


# def test_is_uppercase(string):
#     lowercase = set("abcdefghijklmnopqrstuvwxyz")
#     return all(c not in lowercase for c in string)


# def test_all_keys_uppercase(dct):
#     return all_keys(is_uppercase, dct)


# def test_valid_IEEE_variable(string):
#     allowed = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890_")
#     return all(c in allowed for c in string)


# def test_all_keys_valid_IEEE_variable(dct):
#     return all_keys(valid_IEEE_variable, dct)


# def test_has_child_path_partial(child_path):
#     def has_child_path(parent_path):
#         return Path(parent_path).joinpath(child_path).exists()

#     return has_child_path


# def test_child_keys(test_fn, dct):
#     def inner_recursion(data, child=False):
#         if not isinstance(data, dict):
#             return True
#         for key, value in data.items():
#             if child:
#                 if not test_fn(key):
#                     return False
#             if isinstance(value, dict):
#                 return inner_recursion(data[key], child=True)
#         return True

#     return inner_recursion(dct)


# def test_all_items_in_set_partial(lst):
#     allowed = set(lst)

#     def test_all_items_in_set(items):
#         return set(items).issubset(allowed)

#     return all_items_in_set


# def test_all_child_keys_in_set_partial(profiles):
#     profiles_set = set(profiles)

#     def all_child_keys_in_set(data):
#         return test_child_keys(lambda x: x in profiles_set, data)

#     return all_child_keys_in_set
