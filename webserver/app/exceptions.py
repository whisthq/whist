"""A collection of exceptions that may be raised by the Flask application."""

_ARGUMENT_ERROR = "{}() takes exactly {} positional argument{} ({} given)"


class _FractalError(Exception):
    """The base class for custom exceptions.

    Subclass this class to define a custom exception. Subclasses must define params and message as
    class attributes. params should be a sequence of strings and message should be a format string
    such that

        class MyException(_FractalError):
            params = ("first", "second")
            message = "{first} is my first argument and {second} is my second"

    is equivalent to

        class MyException(Exception):
            def __init__(self, first, second):
                super().__init__(f"{first} is my first argument and {second} is my second")

    The exception _FractalError should not be handled directly.
    """

    def __init__(self, *args):
        argc_actual = len(args)
        argc_expected = len(self.params)

        # Make sure the correct number of arguments were supplied.
        if argc_actual != argc_expected:
            exc_name = type(self).__name__
            plural = "s" if argc_expected > 0 else ""

            raise TypeError(_ARGUMENT_ERROR.format(exc_name, argc_expected, plural, argc_actual))

        kwargs = dict(zip(self.params, args))

        super().__init__(self.message.format(**kwargs))

    @property
    def message(self):
        raise NotImplementedError

    @property
    def params(self):
        raise NotImplementedError


class SentryInitializationError(Exception):
    pass
