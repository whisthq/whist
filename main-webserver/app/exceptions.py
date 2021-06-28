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


class ClusterNotIdle(_FractalError):
    """Raised when a cluster may not be modified because it is busy.

    A cluster may not be deleted while there are tasks still running on it.

    Args:
        cluster: The name of the cluster that is not idle as a string.
        region: The name of the region in which the cluster is located as a string.
    """

    params = ("cluster", "region")
    message = "Cluster {cluster} in {region} is not idle"


class ClusterNotFoundException(_FractalError):
    """
    Raised by any method that fails to find a given cluster in the db
    Args:
        cluster_id (str): the ARN of the cluster
    """

    params = ("cluster_id",)
    message = "cluster_id: {cluster_id}"


class TemplateNotFound(_FractalError):
    """This exception is raised when an email template ID is requested that does not exist
    Args:
        email_template_id (str): id of the selected email template
    """

    params = ("email_template_id",)
    message = "Email_template_id: {email_template_id}"


class SendGridException(_FractalError):
    """This exception is raised when SendGrid email sending API throws an exception
    Args: None
    """

    params = ()
    message = "Sendgrid sending API threw an exception"


class SentryInitializationError(Exception):
    pass
