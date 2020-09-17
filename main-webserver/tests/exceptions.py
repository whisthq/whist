"""Custom exceptions that may be raised while testing ECS endpoints."""


class AuthenticationError(Exception):
    """Raised when authentication fails."""

    def __init__(self, username, password):
        self._username = username
        self._password = password


class UnauthenticatedError(Exception):
    """Raised when a test performs an action before authenticating."""

    def __init__(self, action):
        self._action = action
