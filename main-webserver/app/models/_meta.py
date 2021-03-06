"""_meta.py

Instantiate the SQLAlchemy Flask extension.

It's necessary to instantiate SQLAlchemy here so that all of the models can
subclass db.Model without creating circular imports.
"""

from flask import current_app
from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy(engine_options={"pool_pre_ping": True})


def secret_key():
    """Retrieve the secret key from the Flask application's configuration.

    Returns:
        The value of the SECRET_KEY configuration variable, a string.
    """

    return current_app.secret_key
