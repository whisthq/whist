# Blueprints

The Python files under this directory contain definitions of Flask [Blueprints][0], which are modular groups of (related) routes. All Blueprints are registered to our Flask application in [`factory.py`][1]. When a Blueprint is registered to a Flask application instance, all of the routes registered to the Blueprint are in turn registered to the application.

[0]: https://flask.palletsprojects.com/en/2.0.x/blueprints/
[1]: webserver/app/factory.py
