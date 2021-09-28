# Module Structure

At the module level, the code is separated between api, database, constants, helpers, and utils. Code should only ever live in one of these, and each of these should have their own unit tests with the other modules (as necessary) mocked out as well as integration tests.

## API (Blueprints)

A blueprint is a collection of related API endpoints, and is the entry point for our API. Every API request that we receive is first processed by an endpoint, which resides in a blueprint. Each API file is associated with a blueprint.

##### File Structure

Each file is associated with route(s) based on the resource (ie mandelbox or region) being requested. For most files, we use POST and/or GET requests.

API and Flask code should leave any computation besides directly pulling params out of requests and authorization to their defined  helpers — helpers for something that just is expected to take no more than 5 seconds (e.g. API calls or database updates).

Helpers for APIs live in the `helpers/` directory, and for SQL check out `database/models/` or `utils/db/`. Any exceptions raised by helpers should be directly re-raised to aid debugging (that means that you should not wrap helpers in a try-catch block unless you anticipate some exception, and even then you should try to rewrite the code or the helper to detect that exception-causing condition without raising it, since using exceptions as control flow is an antipattern). For helpers that do not raise exceptions and instead have special error return values, check for those values and fail immediately after those helpers return a failing error code, again to aid debugging.

### Scaling (WIP):

Right now, Heroku handles scaling our web workers, but we need to scale our celery workers -- for that, we use Hirefire (hirefire.io) which is a plug-and-play solution that scales based on measured task queue length.

## Helpers

Our helpers are divided into 2 sections — `aws/`, which directly perform tasks that relate to EC2 instances, and `ami/`, which specifically handles aws AMI.

For more details, see the [README for the `helpers` directory](helpers/README.md).

## Utils

Utils contain all our API callers, flask and client wrappers. Utils also contains code that is used to support functions in `helpers/`

For more details, see the [README for the `Utils` directory](utils/README.md).

## Models

We leverage `flask-sqlalchemy` as our object-relational mapping (ORM), which helps us to easily execute SQL commands without writing raw SQL and catch inconsistencies between the server's understanding of SQL structure and the actual SQL structure at import-time rather than run-time. Specifically, `sqlalchemy` lets us represent database tables as Python objects, making programmatic table modifications more closely resemble the surrounding code.

Files in `app/database/models/` are named after the DB schemata which they're representing in the ORM.

If you're adding a table to the DB, you should add a corresponding model class to the file for the schema that table is part of. Examples can be found in the files.

Models should exactly mimic the DB tables they're based on (down to column names, constraints, and foreign/primary keys), and one model should exist for every DB table.

We want to separate models based on schema. Feel free to create a new file if there is no file for a model in a specific schema.

# Most important code:

## Mandelbox creation flow:

~~This goes from `app\api\mandelbox.py` to `app\helper\aws\aws_instance_post.py` 
with `app\helpers\aws\aws_mandelbox_assign_post.py` and uses `app\utils\aws\base_ecs_client.py` to handle ECS client operations.~~

## Mandelbox status checking:

~~This goes to `app\api\mandelbox.py` with `app\helpers\aws\aws_mandelbox_assign_post.py`.~~
