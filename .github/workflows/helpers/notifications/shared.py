def apply_format(func, **kwargs):
    """Apply a function to notification keyword arguments

    A internal function used to apply formatting to the keyword arguments
    that configure notification functions.

    Args:
        func: a function to apply to keyword arguments
        kwargs: a dictionary of keyword arguments
    Returns:
        a dictionary of keyword arguments"""
    kwargs["title"] = func(kwargs["title"])
    return kwargs


def filter_identifier(identifier, comment):
    """Check if a comment body starts with a unique identifier

    Args:
        identifier: a string, representing a unique id for a comment
        comment: a PyGithub object with a 'body' attribute
    Returns:
        True or False
    """
    return fmt.startswith_id(identifier, comment.body)


# These small functions are partial applications of "apply_format"
# They "configure" it for use with each logging level
# Title formatting and default values can be changed here
format_debug = partial(apply_format, fmt.construction, title="DEBUG")
format_info = partial(apply_format, fmt.identity, title="INFO")
format_warning = partial(apply_format, fmt.exclamation, title="WARNING")
format_error = partial(apply_format, fmt.x, title="ERROR")
format_critical = partial(apply_format, fmt.rotating_light, title="CRITICAL")