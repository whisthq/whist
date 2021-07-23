# Module Structure

At the module level, the code is separated between blueprints, models, and helpers. Code should only ever live in one of these, and each of these (besides models which are only tested at the integration test level) should have their own unit tests with the other modules (as necessary) mocked out as well as integration tests.

## Blueprints

A blueprint is a collection of related API endpoints, and is the entry point for our API. Every API request that we receive is first processed by an endpoint, which resides in a blueprint.

##### Folder Structure

The blueprint folder is divided into sub-folders, which are high-level groupings of our endpoints. For instance, we have an auth sub-folder and an admin sub-folder. Each sub-folder is made up blueprint files, which are more granular groupings of endpoints. For instance, the auth sub-folder contains account_blueprint.py and token_blueprint.py.

##### File Structure

Each blueprint is made up of routes, which is a collection of endpoints separated by HTTP request type. For most files, we use just POST and GET, so every blueprint has a big function for all POST requests and a big function for all GET requests.

Blueprints and Flask code should leave any computation besides directly pulling params out of requests and authorization to their defined blueprint helpers — blueprint helpers for something that just is expected to take no more than 5 seconds (e.g. API calls or database updates).

Helpers for APIs live in the `helpers` directory, and for SQL use `fractalSQLCommit`, `fractalSQLUpdate`, and `fractalSQLDelete`. Any exceptions raised by helpers should be directly re-raised to aid debugging (that means that you should not wrap helpers in a try-catch block unless you anticipate some exception, and even then you should try to rewrite the code or the helper to detect that exception-causing condition without raising it, since using exceptions as control flow is an antipattern). For helpers that do not raise exceptions and instead have special error return values, check for those values and fail immediately after those helpers return a failing error code, again to aid debugging. All failures should both change the state of the celery worker and use fractalLog to log the error.

### Scaling (WIP):

Right now, Heroku handles scaling our web workers, but we need to scale our celery workers -- for that, we use Hirefire (hirefire.io) which is a plug-and-play solution that scales based on measured task queue length.

## Helpers

Our helpers are divided into 2 sections — `blueprint_helpers`, which directly perform blueprint tasks that don't belong in Celery (generally, querying our DB for information), and `utils`, which contain all our client wrappers and API callers. Nothing in `utils` should interact with fractal DBs unless explicitly necessary. In fact, none of your tests for those utilities should in any way require fractal to work unless explicitly necessary — any code that interacts with fractal DBs should live in the blueprint helpers. This makes helpers tests significantly easier to mock, as the entire DB doesn't need to be mocked.

For more details, see the [README for the `helpers` directory](helpers/README.md).

## Models

We leverage `flask-sqlalchemy` as our object-relational mapping (ORM), which helps us to easily execute SQL commands without writing raw SQL and catch inconsistencies between the server's understanding of SQL structure and the actual SQL structure at import-time rather than run-time. Specifically, `sqlalchemy` lets us represent database tables as Python objects, making programmatic table modifications more closely resemble the surrounding code.

Files in `app/models` are named after the DB schemata which they're representing in the ORM.

If you're adding a table to the DB, you should add a corresponding model class to the file for the schema that table is part of. Examples can be found in the files.

Models should exactly mimic the DB tables they're based on (down to column names, constraints, and foreign/primary keys), and one model should exist for every DB table.

# Most important code:

## Mandelbox creation flow:

TODO: Update this as per new flow.

~~This goes from `app\blueprints\aws\aws_mandelbox_blueprint.py` to `app\celery\aws_ecs_creation.py` through `app\helpers\blueprint_helpers\aws\aws_mandelbox_assign_post.py`
and uses `app\helpers\utils\aws\base_ecs_client.py` to handle ECS client operations.~~

## Mandelbox status checking:

~~This goes from `app\blueprints\aws\aws_mandelbox_blueprint.py` to `app\celery\aws_ecs_status.py` through `app\helpers\blueprint_helpers\aws\aws_mandelbox_assign_post.py`.~~
