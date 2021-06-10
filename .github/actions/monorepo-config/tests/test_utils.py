#!/usr/bin/env python
from helpers.utils import merge


def test_merge():
    t1 = {"a": 1}
    t2 = {"a": 2}
    t3 = {"b": 3}
    t4 = {"c": 4}

    result = merge(t1, t2, t3, t4)
    assert result == {"a": 2, "b": 3, "c": 4}, "incorrect merge"

    assert t1 == {"a": 1}, "mutated input"
    assert t2 == {"a": 2}, "mutated input"
    assert t3 == {"b": 3}, "mutated input"
    assert t4 == {"c": 4}, "mutated input"
