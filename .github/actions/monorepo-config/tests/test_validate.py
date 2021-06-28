#!/usr/bin/env python
import pytest
import helpers.validate as validate


def test_must():
    is_fn = "result must be function"
    # is_true = "result fn must return True if test_fn(value) is not falsey"
    is_false = "result fn must throw error if test_fn(value) is falsey"

    example = True

    result_true = validate.must(lambda x: x is True)
    result_false = validate.must(lambda x: x is False)
    assert callable(result_true), is_fn
    # assert result_true(example), is_true
    assert callable(result_false), is_fn
    # with pytest.raises(validate.ValidationError):
    #     assert result_false(example), is_false


def test_validate_safe():
    catches = "result must return list of any Assertion errors thrown"
    empties = "result should be an empty list if no errors thrown"
    must = validate.must

    result_true = validate.validate_safe(
        [1, 2, 3, 4, 5],
        must(lambda x: isinstance(x, list), "be a list"),
        must(lambda x: len(x) == 5, "length 5"),
        must(lambda x: sum(x) == 15, "sum to 15"),
    )

    result_false = validate.validate_safe(
        [1, 2, 3, 4, 5],
        must(lambda x: isinstance(x, list), "be a list"),
        must(lambda x: len(x) == 5, "length 5"),
        must(lambda x: sum(x) == 20, "sum to 20"),
    )

    assert len(list(result_true)) == 0, empties
    assert len(list(result_false)) == 1, catches


def test_validate():
    must = validate.must

    validate.validate(
        [1, 2, 3, 4, 5],
        must(lambda x: isinstance(x, list), "be a list"),
        must(lambda x: len(x) == 5, "length 5"),
        must(lambda x: sum(x) == 15, "sum to 15"),
    )

    with pytest.raises(AssertionError):
        validate.validate(
            [1, 2, 3, 4, 5],
            must(lambda x: isinstance(x, list), "be a list"),
            must(lambda x: len(x) == 5, "length 5"),
            must(lambda x: sum(x) == 20, "sum to 20"),
        )
