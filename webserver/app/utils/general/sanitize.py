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
    email_regex = r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b"
    if re.fullmatch(email_regex, unsafe_email):
        return unsafe_email
    else:
        return ""
