#!/usr/bin/env python
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


@composite
def schemas(draw, min_profiles=0):
    keys = text()
    values = one_of(integers(), text())
    profiles = draw(lists(text(), min_size=min_profiles, max_size=30))
    children = dictionaries(sampled_from(profiles) if profiles else just(()), values, max_size=50)
    return (
        profiles,
        draw(dictionaries(keys, one_of(values, children))),
    )


@composite
def non_dict(draw):
    lit = one_of(text(), integers())
    seq = lists(lit)
    return draw(one_of(lit, seq))
