"""Helpful patch utilities."""


def function(**kwargs):
    """Create a function that either raises or returns the specified value.

    Args:
        raises: Optional. An exception to raise. This argument takes takes precedence over the
            return value argument.
        returns: Optional. A value to return.

    Returns:
        A function that either raises or returns the specified value.
    """

    def _function(*_args, **_kwargs):
        if kwargs.get("raises"):
            raise kwargs["raises"]

        return kwargs.get("returns")

    return _function


class Object:
    """A class used to create dummy objects.

    The __getattr__ method is called after normal attribute access results in an AttributeError.
    The custom implementation of __getattr__ here is intended to swallow those exceptions.
    """

    def __getattr__(self, name):
        """If the desired attribute cannot be found, don't do anything.

        The trivial implementation of this method allows us to do write the following:

            def test_patch(monkeypatch):
                patch = Object()
                monkeypatch.setattr(patch, "foo", "bar")

                assert patch.foo == "bar"

        Without the custom implementation, we would have to write:

            def test_patch(monkeypatch):
                patch = Object()
                monkeypatch.setattr(patch, "foo", "bar", raising=False)

                assert patch.foo == "bar"

        Args:
            name: The name of the attribute to look up.

        Returns:
            None
        """
