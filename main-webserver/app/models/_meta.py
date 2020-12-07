"""_meta.py

Instantiate the SQLAlchemy Flask extension.

It's necessary to instantiate SQLAlchemy here so that all of the models can
subclass db.Model without creating circular imports.
"""

from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy(engine_options={"pool_pre_ping": True})
