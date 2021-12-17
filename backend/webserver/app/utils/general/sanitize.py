import re


def sanitize_email(unsafe_email: str) -> str:
    """
    Helper function for sanitizing an email
    Args:
        unsafe_email (str): the unsanitized email
    Returns:
        str: the received email if it's valid
        an empty string if it's invalid
    """

    email_regex = r"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)"
    if re.fullmatch(email_regex, unsafe_email):
        return unsafe_email
    else:
        return ""
