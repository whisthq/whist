from flask import Blueprint
import os
from .tasks import *

bp = Blueprint("all", __name__)

@bp.route("/")
def index():
    return "Hello!"

@bp.route("/test")
def makefile():
    make_file.delay()
    return "test"
