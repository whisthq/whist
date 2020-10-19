#Module Structure
At the module level, the code is separated between blueprints, models/ serializers, Celery tasks, and helpers. Code should only ever live in one of these, and each of these (besides models/serializers, which are only tested at the integration test level) should have their own unit tests with the other modules (as necessary) mocked out as well as integration tests.

Every module should be installable via pip install -e . from its base directory, to enable easy testing/mocking. This is trivial, but does require that any package/module have an init.py at its top level (and a setup.py for modules you'll be installing, atm primarily helpers).

##Blueprints
A blueprint is a collection of related API endpoints, and is the entry point for our API. Every API request that we receive is first processed by an endpoint, which resides in a blueprint.

#####Folder Structure

The blueprint folder is divided into sub-folders, which are high-level groupings of our endpoints. For instance, we have an auth sub-folder and an admin sub-folder. Each sub-folder is made up blueprint files, which are more granular groupings of endpoints. For instance, the auth sub-folder contains account_blueprint.py and google_auth_blueprint.py.

#####File Structure

Each blueprint is made up of routes, which is a collection of endpoints separated by HTTP request type. For most files, we use just POST and GET, so every blueprint has a big function for all POST requests and a big function for all GET requests.

Blueprints and Flask code should leave any computation besides directly pulling params out of requests and authorization to their defined celery tasks or blueprint helpers — celery tasks for long-lasting code that requires API requests, blueprint helpers for something that just is expected to take no more than 5 seconds (e.g. API calls or database updates).


##Celery Tasks
A celery task is one way to asynchronously execute some code — this is useful for e.g. code that pings an API or touches one of our databases so it doesn't block our server while it's running. By design, Heroku fails all API requests that take more than 30 seconds to execute, so celery tasks are necessary for long-running API calls. A high-level rule of thumb to follow is to outsource an endpoint to a celery task if the endpoint can be expected to take more than 5-10 seconds to run (most SQL operations take less than a second).

Celery tasks should never make their own API calls or direct SQL statements, instead wrapping those calls in one of our helpers. Helpers for APIs live in the helpers directory, and for SQL use fractalSQLCommit, fractalSQLUpdate, and fractalSQLDelete. Any exceptions raised by helpers should be directly reraised to aid debugging (that means that you should not wrap helpers in a try-catch block unless you anticipate some exception, and even then you should try to rewrite the code or the helper to detect that exception-causing condition without raising it, exceptions as control flow are an antipattern). For helpers that do not raise exceptions and instead have special error return values, check for those values and fail immediately after those helpers return a failing error code, again to aid debugging. All failures should both change the state of the celery worker and use fractalLog to log the error.

##Helpers
Our helpers are divided into 2 sections — blueprint-helpers, which directly perform blueprint tasks that don't belong in Celery (generally, querying our DB for information), and utils, which contain all our client wrappers and API callers. Nothing in utils should interact with fractal DBs unless explicitly necessary. In fact, none of your tests for those utils should in any way require fractal to work unless explicitly necessary — any code that interacts with fractal DBs should live in celery tasks or the blueprint helpers. This makes helpers tests significantly easier to mock, as the entire DB doesn't need to be mocked.

##Models/Serializers
We leverage flask-sqlalchemy as our object-relational mapping (ORM), which helps us to easily execute SQL commands without writing raw SQL and catch inconsistencies between the server's understanding of SQL structure and the actual SQL structure at compile-time rather than run-time.

Files in these directories are named after the DB schemata which they're representing in the ORM.

Models should exactly mimic the DB tables they're based on (down to column names, constraints, and foreign/primary keys), and one model should exist for every DB table. Serializers should use the pattern already shown in the serializers files and are a convenient tool to json-ify SQLAlchemy objects (e.g. rows).


# Most important code:

##Container creation flow:

This goes from app\blueprints\aws\aws_container_blueprint.py to app\celery\aws_ecs_creation.py through app\helpers\blueprint_helpers\aws\aws_container_post.py
and uses app\helpers\utils\aws\base_ecs_client.py to handle ECS client operations.

##Container status checking:
This goes from app\blueprints\aws\aws_container_blueprint.py to app\celery\aws_ecs_status.py through app\helpers\blueprint_helpers\aws\aws_container_post.py
.