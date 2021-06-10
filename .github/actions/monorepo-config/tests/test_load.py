#!/usr/bin/env python
import pytest
from helpers.load import resolve


@pytest.fixture
def nested_map():
    return {
        "a": {
            "h": {"x": "t", "y": "u", "z": "v"},
            "i": {"d": "q", "e": "r", "f": "s"},
        },
        "b": {
            "h": {"x": "t", "y": "u", "z": "v"},
            "i": {"d": "q", "e": "r", "f": "s"},
        },
        "c": {"f": "twenty"},
    }


def test_resolve(nested_map):
    original_map = {**nested_map}

    base1 = resolve(["a", "b"], "hello")
    assert base1 == "hello", "non-dict schema was changed"

    base2 = resolve(["a", "b"], ["hello"])
    assert base2 == ["hello"], "non-dict schema was changed"

    ctwenty = resolve(["f"], nested_map)
    assert ctwenty["c"] == "twenty", "incorrect flattening"

    fullflat = resolve(["h", "x"], nested_map)
    assert fullflat["a"] == "t", "incorrect flattening"

    nochange = resolve(["m ", "j"], nested_map)
    assert nochange["a"] == original_map["a"], "changed without profile match"
    assert nochange["a"]["h"] == original_map["a"]["h"], "changed without profile match"

    assert nochange["a"]["h"]["y"] == original_map["a"]["h"]["y"], "changed without profile match"

    assert original_map == nested_map, "map was mutated"
