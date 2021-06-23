#!/usr/bin/env python
import pytest
from helpers.parse import resolve
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


example_profiles = ["dev", "prod"]

example_nested_map = {
    "API_KEY": {
        "dev": {"x": "t", "y": "u", "z": "v"},
        "prod": {"d": "q", "e": "r", "f": "s"},
    },
    "WEB_URL": {
        "dev": {"x": "t", "y": "u", "z": "v"},
        "prod": {"d": "q", "e": "r", "f": "s"},
    },
    "DEV_NUM": {"dev": "twenty", "prod": "thirty"},
}


@composite
def schemas(draw, min_profiles=0):
    keys = text()
    values = one_of(integers(), text())
    profiles = draw(lists(text(), min_size=min_profiles, max_size=30))
    children = dictionaries(
        sampled_from(profiles) if profiles else just(()), values, max_size=50
    )
    return (
        profiles,
        draw(dictionaries(keys, one_of(values, children))),
    )


@composite
def non_dict(draw):
    lit = one_of(text(), integers())
    seq = lists(lit)
    return draw(one_of(lit, seq))


def flatten_child(profiles, child):
    for profile in reversed(profiles):
        if profile in child:
            return child[profile]
    return child


@given(schemas(), non_dict())
def test_resolve_(schema, random_value):
    profiles, source = schema

    base1 = resolve(profiles, random_value)
    assert (
        base1 == random_value
    ), "a value that wasn't a dictionary was returned changed."


@given(schemas())
@example((example_profiles, example_nested_map))
def test_resolve_flattening(schema):
    profiles, source = schema
    result = resolve(profiles, source)
    for key, child in source.items():
        if isinstance(child, dict):
            if any(p in child for p in profiles):
                for profile in reversed(profiles):
                    if profile in child:
                        assert result[key] == child[profile], (
                            "resolved child value must match"
                            + " value of highest index profile"
                        )

                        break
            else:
                assert (
                    result[key] == child
                ), "child value must be unchanged if no profile match is found"


@given(schemas())
@example((example_profiles, example_nested_map))
def test_resolve_same_keys(schema):
    profiles, source = schema
    result = resolve(profiles, source)
    assert (
        result.keys() == source.keys()
    ), "result must have same keys as source"
