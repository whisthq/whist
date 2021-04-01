"""
This file defines the SQLAlchemy Model for the LoadTesting table.
It is used to save load test results in a Pythonic fashion via an ORM,
like the rest of webserver code.
"""

from flask import Flask
from flask_sqlalchemy import SQLAlchemy

dummy_flask_app = Flask(__name__)
db = SQLAlchemy(engine_options={"pool_pre_ping": True})


def initialize_db(db_uri: str):
    """
    Initialize the db at `db_uri` for reading/writing via SQLAlchemy ORM.

    Args:
        db_uri: URI of the db to reference
    """
    dummy_flask_app.config["SQLALCHEMY_DATABASE_URI"] = db_uri
    dummy_flask_app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False
    db.init_app(dummy_flask_app)


class LoadTesting(db.Model):
    __tablename__ = "load_testing"
    __table_args__ = {"schema": "analytics"}

    run_id = db.Column(db.Text, primary_key=True)
    num_users = db.Column(db.Integer, nullable=False)
    avg_container_time = db.Column(db.Numeric, nullable=False)
    max_container_time = db.Column(db.Numeric, nullable=False)
    std_container_time = db.Column(db.Numeric, nullable=False)
    avg_web_time = db.Column(db.Numeric, nullable=False)
    max_web_time = db.Column(db.Numeric, nullable=False)
    user_errors = db.Column(db.Integer, nullable=False)

    def __str__(self):
        data = {}
        for key in self.__mapper__.c.keys():
            data[key] = getattr(self, key)
        return str(data)
