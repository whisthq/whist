#!/usr/bin/env python
import pytest
from helpers.validate import validate_schema_data


def test_validate_schema_data():
    with pytest.raises(AssertionError):
        validate_schema_data(25, ())

    with pytest.raises(AssertionError):
        validate_schema_data({}, ())

    with pytest.raises(AssertionError):
        validate_schema_data({"a": 1, "b": 1, "3": "c"}, ())

    with pytest.raises(AssertionError):
        validate_schema_data({"a": 1, "b": 1, "r": "c"}, ())

    with pytest.raises(AssertionError):
        validate_schema_data({"_JDKF": "b"}, ())

    with pytest.raises(AssertionError):
        validate_schema_data({"123DKF": "b"}, ())

    with pytest.raises(AssertionError):
        validate_schema_data({"HDF*SSF": "b"}, ())

    with pytest.raises(AssertionError):
        validate_schema_data({}, ())

    validate_schema_data({"A13_3": "good data", "LJDF993_": "good data"}, ())

    # validate_schema_data(
    #     {
    #         "a": {
    #             "h": {},
    #             "i": {},
    #         },
    #         "b": {
    #             "h": {},
    #             "i": {},
    #         },
    #         "c": {},
    #     },
    #     ("h", "i"),
    # )
    with pytest.raises(AssertionError):
        validate_schema_data(
            {
                "a": {
                    "g": {},
                    "r": {},
                },
                "b": {
                    "k": {},
                    "j": {},
                },
                "c": {},
            },
            ("h", "i"),
        )
