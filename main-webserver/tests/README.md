# Testing

This document contains information about testing the Python web server code.

## Overview

Tests for the the Python web server are written using the `pytest` testing framework. This directory contains most of the tests, but some can also be found in `app/helpers/helpers-tests`. All future tests should be written in files in this directory&mdash;`tests`&mdash;and its subdirectories. Running `pytest` in this directory will not run the tests in `app/helpers/helpers-tests`. To run all of the tests at once, run `pytest` from the repository root.

## Writing tests

When writing tests, make sure to take advantage of test fixtures and mocking.

Test fixtures handle setup and teardown of test resources. Correctly designed test fixtures allow us to write tests that can run completely independently of one another and are entirely self-contained; tests that use test fixtures that set up and tear down all required test resources can be run in any order (even in parallel) and without much manual configuration of external resources like databases.

Mocking is another important strategy to employ when writing tests. It allows us to write unit tests that test the lines of code that set up calls to complex functions and handle return values from those functions without ever actually calling the functions.

### Test fixtures

Please read [this page](https://docs.pytest.org/en/stable/fixture.html) of the `pytest` documentation in its entirety. It explains test fixtures better than I could ever hope to.

### Mocking

Consider the following situation in which you might find yourself while writing tests. Suppose you want to test the following code, which calculates the sum of the number of COVID-19 cases reported in some number of countries:

`sum.py`

```python
"""Cacluate the total number of COVID-19 cases around the world."""

import requests


def sum_covid_cases(*countries):
    """Sum up the number of COVID-19 cases in the specified countries.

    Arguments:
        countries: The names of the countries whose COVID-19 cases should be
            summed up.

    Returns:
        An integer representing the total number of COVID-19 cases that have
        been identified in the specified countries.
    """

    total = 0

    for country in countries:
        response = requests.get("...", params={country=country})

        if response.ok:
           total += response.json["count"]
        else:
           raise Exception(f"Could not fetch data for country '{country}'.)

    return total
```

Suppose you want to test your code, but you have to pay for each API request you send. It would be a waste of money to send actual requests to the server from within your tests, but you need to know how your code handles certain responses. In this case, you may want "patch" the return value of `requests.get(...)` with some dummy value. Such a dummy value would have to be an object with an `ok` attribute and an `json` attribute.

Luckily, `pytest` comes with a built-in solution for patching certain parts of your code during testing. The [`monkeypatch`](https://docs.pytest.org/en/stable/reference.html#std-fixture-monkeypatch) test fixture allows you to use dummy data as a substitute for real data during testing. Unfortunately, I don't think the `pytest` documentation does a very good job of explaining just how powerful monkeypatching is.

In order to tap into the full potential of the `monkeypatch` fixture, it's important to have a couple of helper utilities handy.

`patches.py`

```python
"""Helpful patch utilities."""

def function(**kwargs):
    """Create a function that either raises or returns the specified value.

    Keyword arguments:
        raises: An exception to raise. This argument takes takes precedence
            over the return value argument.
        returns: A value to return.

    Returns: A function that either raises or returns the specified value.
    """

    def _function(*_args, **_kwargs):
        if kwargs.get("raises"):
            raise kwargs["raises"]

        return kwargs.get("returns")

    return _function


class Object():
    """A class used to create dummy objects.

    The __getattr__ method is called after normal attribute access results in
    an AttributeError. The custom implementation of __getattr__ here is
    intended to swallow those exceptions.
    """

    def __getattr__(self, name):
        """If the desired attribute cannot be found, don't do anything.

        The trivial implementation of this method allows us to do write the
        following:

            def test_patch(monkeypatch):
                patch = Object()
                monkeypatch.setattr(patch, "foo", "bar")

                assert patch.foo == "bar"

        Without the custom implementation, we would have to write:

            def test_patch(monkeypatch):
                patch = Object()
                monkeypatch.setattr(patch, "foo", "bar", raising=False)

                assert patch.foo == "bar"

        Arguments:
            name: The name of the attribute to look up.
        """
```

Equipped with these utilities, it is relatively straightforward to patch the return value of `requests.get(...)` in the code to be tested so the request is never sent. We can write the following two tests to test our `sum_covid_cases` function:

`test_sum.py`

```python
"""Tests for the sum_covid_cases function."""

import pytest
import requests

from patches import function, Object
from sum import sum_covid_cases


def test_sum(monkeypatch):
    """Calculate the correct total number of COVID-19 cases."""

    response = Object()

    monkeypatch.setattr(requests, "get", function(returns=response))
    monkeypatch.setattr(response, "json", {"count": 1000000})
    monkeypatch.setattr(response, "ok", True)

    assert sum_covid_cases("US") == 1000000


def test_exception(monkeypatch):
    """Raise an exception upon receipt of an error HTTP response."""

    response = Object()

    monkeypatch.setattr(requests, "get", function(returns=response)
    monkeypatch.setattr(response, "ok", False)

    with pytest.raises(Exception):
        sum_covid_cases("Narnia")
```

By following this pattern, it becomes much easier to simple unit tests, even when the code that is being tested is relatively complex.
