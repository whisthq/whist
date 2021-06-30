# Fractal Main Webserver

[![codecov](https://codecov.io/gh/fractal/fractal/branch/dev/graph/badge.svg?token=QB0c3c2NBj)](https://codecov.io/gh/fractal/fractal)

This repository contains the code for our webserver, which is our REST API and provides backend support for our user interfaces, our internal tools, and our container/virtual machine management. This README contains general set-up information&mdash;for a high-level understanding of the code, read each of the READMEs in the the subdirectories here (`/app`, `/docker`, etc.)

Our webservers and CD pipeline are hosted on Heroku. Our production database is attached as an Heroku Add-On PostgresSQL to the associated webserver in Heroku, `main-webserver`, and has automated backups in place daily at 2 AM PST. See [here](https://devcenter.heroku.com/articles/heroku-postgres-backups#creating-a-backup) for further information.

#### Coding Philosophy

Before contributing to this project, please read our in-depth coding philosophy document [here](https://www.notion.so/tryfractal/Code-Philosophy-Webserver-backend-d036205444464f8b8a61dc36eeae7dbb).

## Development

### Getting Started

#### Local Setup

Right now, in order to be _sure_ that changes you make to the web stack won't break anything when deployed, we strongly advise locally building any changes you make -- particularly to the app startup section of the code (create_app, register_blueprints). Our CI covers the rest of the pipeline (so you can be relatively confident changes elsewhere won't set everything on fire) but even then the best way to test code is still with a local build. Instructions for that build can be found below.

The web application stack is comprised of two main components:

-   The web server itself, which is written in Python using the [Flask](https://flask.palletsprojects.com/en/1.1.x/) web framework.
-   A database. The database is a Postgres instance that is shared by multiple developers.

We use [`docker-compose`](https://docs.docker.com/compose/) to spin part of the web server stack up (the `docker-compose` stack does not include the Postgres database, which is shared between multiple developers and app deployments, as mentioned above) locally for development purposes. `docker-compose` builds Docker images for the Flask server. There is also a `pytest` test suite that developers may run locally. None of these commands are run directly, and are instead wrapped by bash scripts that do a bit of preparation (namely `docker/local_deploy.sh` and `tests/setup/setup_tests.sh`).

The following environment variables must also be set in `docker/.env` (neither the test suite nor the `docker-compose` stack will work without them).

-   `POSTGRES_DB` &ndash; The name of the Postgres database to which to connect.
-   `POSTGRES_HOST` &ndash; The hostname or IP address of the development Postgres instance.
-   `POSTGRES_PASSWORD` &ndash; The password used to authenticate with the local stack's PostgresQL instance.
-   `POSTGRES_USER` &ndash; The name of the user as whom to log into the development Postgres instance.

All of local deployment, local testing, and CI use ephemeral DBs that are mostly empty copies of the dev database. The copying script (see `db_setup/`) looks at the database specified by those environment variables.

If you'd like to retrieve more information from the dev db (to put in the ephemeral DBs) than the tables we currently have, open up `db_setup/fetch_db.sh` and find the _two_ lines with:

```shell
(pg_dump -h $POSTGRES_REMOTE_HOST -U $POSTGRES_REMOTE_USER -d $POSTGRES_REMOTE_DB --data-only --column-inserts ... ) > db_data.sql
```

For each table you'd like to fetch from dev, add `-t schema_name.table_name` to `...`. Remember to delete the existing `.sql` scripts and rerun any scripts afterwards.

**1. Set environment variables**

Luckily, there is an easy way to set all of the necessary environment variables using the script `docker/retrieve_config.sh`. To set all of the required environment variables, first make sure you have [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli) installed and configured. Then, run

```shell
bash docker/retrieve_config.sh
```

When the `docker/retrieve_config.sh` script terminates, it will print the name of the file containing the fetched environment variables that has been written to standard error.

**2. Set AWS credentials**

Whether you're running tests or the `docker-compose` stack locally, the web server needs to be able to access AWS APIs. You can set your AWS credentials using either the same files in the `~/.aws/` directory or environment variables that you can use to configure [`boto`](https://boto3.amazonaws.com/v1/documentation/api/latest/guide/credentials.html) and the [AWS CLI](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-configure.html). Environment variables set in `docker/.env` will be loaded by `pytest` during test run sessions and set in the execution environment within the containers in the docker-compose stack.

> I recommend downloading the AWS CLI, which you may find useful in the future, and running `aws configure`.
>
> -O

**3. Spin Up Local Servers**

Run the following to do a local deployment. If you are on Windows, you should run this from a command prompt in Administrator mode. This will create dummy SSL certificates and start the app at `run.py`.

```sh
bash docker/local_deploy.sh
```

If you encounter a "daemon not running" error, this likely means that Docker is not actually running. To fix this, try restarting your computer or opening the Docker desktop app; if the app opens successfully, then the issue should go away. You can optionally pass the argument `--use-dev-db`. Only do this if you absolutely need the dev db. Generally speaking, you should be able to recreate any resource on dev dbs in your ephemeral db. If you do this, please explain in `#webserver` why the ephemeral db did not meet your needs so we can improve it.

Review `docker/docker-compose.yml` to see which ports the various services are hosted on.For example, `"9999:5432"` means that the Postgres service, running on port 5432 internally, will be available on `localhost:9999` from the host machine. Line 25 of `docker-compose.yml` will tell you where the webserver itself is running. When you're done, end the containers with `bash docker/local_deploy.sh --down`.

### Flask CLI

Take advantage of the Flask CLI! Run `flask --help` for information about what commands are available. If, at any point, there is a script that you need to write that is useful for modifying the Flask application or any of its resources (e.g. databases), you are encourage to write it as a Flask CLI command. Start reading about the Flask CLI documentation [here](https://flask.palletsprojects.com/en/1.1.x/cli/?highlight=cli#custom-commands).

### Helper Software Setup

We recommend that you download several softwares to help you code and test:

**Postman**

We use Postman to send API requests to our server, to store our API endpoints, and to generate documentation. Our Postman team link is [here](https://tryfractal.postman.co/). If you are not part of the team, contact @mingy98. To better understand how Postman works, refer to our wiki [here](https://www.notion.so/tryfractal/Postman-API-Documentation-602cc6df23e04cd0a026340c406bd663).

Please be very careful while using Postman. It's all we've got, but it's buggy and its contents are not effectively version controlled, so be **100% sure** _before_ you save anything or overwrite any "initial values" for environment variables. When in doubt, reach out to someone more experienced!

Here are the steps to being able to use Postman to call webserver endpoints with the Auth0 integration (If you want to see screenshots and get a little more detail, check out [this write-up by @owenniles](https://www.notion.so/tryfractal/Authorizing-Postman-b56195ac62ff4570a6b15d5c60fa3d5b):

1. Add your desired username and password for Auth0 as Postman variables in the `dev` environment. The variables should already exist, but without "initial values". **Don't put your username/pass as "initial values"**, but rather put them as the "current value". Putting them as the "initial values" will expose them to everyone!
2. Use the "Register New User" endpoint in Postman to register yourself an Auth0 account **using your @fractal.co e-mail address**. You shouldn't need to add or change anything, just click the "Send" button. Make sure you verify your e-mail.
3. Ask someone with Auth0 dashboard access (currently @owenniles and @MYKatz) to sufficiently elevate your permissions to the level you need. TODO: Make all **verified** `@fractal.co` e-mail addresses automatically have developer-level privileges.
4. Use the "Authorize Postman" endpoint in Postman to get an access token. You shouldn't need to add or change anything, just click the "Send" button.
5. Copy the resulting `access_token` value into the "existing value" of the `access_token` environment variable in the dev environment. As before, don't update the "initial value".
6. Call whatever other endpoints you need in Postman! You should be all set.

**TablePlus**

We use TablePlus to visualize, search, and modify our SQL database. For instructions on how to set up TablePlus, refer to our wiki [here](https://www.notion.so/tryfractal/Using-TablePlus-to-Access-our-PostgresSQL-Database-d5badb38eb3841deb56a84698ccd20f5).

### Heroku Setup

For continuous integration and delivery, we leverage Heroku pipelines, which provides us with automated PR testing, isolation of environment variables, promotion/rollbacks, and auto-deploys from Github. Contributors should NOT push code to Heroku; only codeowners are expected to do this. Instead, contributors should PR their changes into the appropriate Github branch (most often `prod`).

While our Heroku pipeline should not be modified without codeowner permission, it is helpful to understand how it works by consulting our wiki [here](https://www.notion.so/tryfractal/Heroku-CI-CD-Pipeline-Webservers-f8ef5b43edc84c969cf005fcac4641ba).

### GraphQL

We leverage Hasura GraphQL (hosted on Heroku) to enable real-time database access and serverless database retrieval. For pure SQL requests, we encourage using GraphQL instead of writing your own server endpoint to minimize the amount of code we write and because GraphQL has a lot of really nice built-in features.

GraphQL is already set up, but here's a [setup doc](https://hasura.io/docs/1.0/graphql/core/deployment/deployment-guides/heroku.html) for reference. Hasura GraphQL also provides a console for easy interfacing with the database. The prod database console is [here](prod-database.fractal.co) using access key stored as a config variable in the Heroku `fractal-graphql` application.

If you want to test with local GraphQL endpoints, running `bash hasura_run.sh` in the `docker` subdirectory will create a local instance of Hasura at `localhost:8080`. It currently uses the dev database with admin secret `secret` and auth hook `http://host.docker.internal:7730/hasura/auth` (assuming that you're running a local instance of the webserver through docker), but change as needed to get the ports you want.

## Testing

### Setting Up

You need the CLI utilities `pg_dump` and `psql`. On Mac:

```
brew install postgresql
```

First, we need to setup a local Postgres instance. Navigate to `tests/setup` and run `bash setup_tests.sh`. This only has to be run once for as long as you are testing. This script will use `docker-compose` to set up a local db that looks like a fresh version of the remote dev db. It'll be mostly empty except for a few tables. You can use TablePlus to connect to it locally at `localhost:9999`. You can find the username and database (pwd optional) in `docker/.env`. When you are done testing, end the containers with `docker-compose down`. Note: the `setup_tests.sh` script saves SQL scripts to `main-webserver/db-setup`. Delete these once in a while to get an updated pull of the database.

### Testing

Now, navigate to `tests` and run `bash run_tests.sh`. This loads the environment variables in `docker/.env` and uses `pytest` to run the tests. If something goes wrong during testing and you kill it early, clean up clusters using the AWS console. Note that since the db is local and ephemeral, any db changes can be safely done.

To generate test coverage statistics, set `COV=1` when running the `run_tests.sh` script. For example:

    /path/to/fractal/main-webserver/tests $ COV=1 bash run_tests.sh

### Manual Testing

Sometimes, it is helpful to manual test changes that don't neatly fit into the current unit testing / integration testing framework. For example, let's say you want to make sure you properly wrote a database filtering command using the SQLAlchemy ORM. This section briefly describes different kinds of manual testing and how to do them.

_Manual Testing - Database_

To test database specific things, you can make a file `db_manual_test.py` in main-webserver root with the following contents:

```python
# export the URL of the database as DATABASE_URL
# if it's a local ephemeral database (launched with docker/local_deploy.sh):
# export DATABASE_URL=postgres://<USER>@localhost:9999/<DB>
# You can find USER and DB in docker/.env after retrieving the config
from app.factory import create_app
from app.models import MandelboxInfo
# TODO: is this even correct?

app = create_app()

with app.app_context():
    base_mandelbox = (
        MandelBox.query.filter(
            MandelBox.location == "us-east-1",
            MandelBox.user_id is not None,
        )
        .with_for_update()
        .limit(1)
        .first()
    )

    print(base_mandelbox)
```

If you need to populate some fake data into the ephemeral db, use TablePlus or PgAdmin. This kind of manual testing provides a neat way to quickly test and debug the database logic in isolation.

_Manual Testing - Deployments_

TODO: explain review apps here, link notion.

## How To Contribute

Before making a pull request, ensure that the following steps are taken:

1. Make sure that your code follows the guidelines outlined in our [Python Webserver Coding Philosophy](https://www.notion.so/tryfractal/Code-Philosophy-Webserver-backend-d036205444464f8b8a61dc36eeae7dbb).

2. Lint your code by running `black .` and `pylint app` from the `main-webserver` directory. If this does not pass, your code will fail Github CI. NOTE: Depending on where the directory containing your virtual environment is located, `black .` may attempt to lint the source code for all of the packages specified in your requirements files. In this case, use the --exclude flag.

3. Run all test files by running `bash run_tests.sh` in the `main-webserver/tests` directory. NOTE: If you have written new functions, make sure it has a corresponding test, or code reviewers will request changes on your PR.

4. Rebase against `dev` by pulling `dev` and running `git rebase dev`.

Finally, you can open a PR to `dev`.

## Styling

The code in this directory is formatted with [`black`](https://black.readthedocs.io/en/stable/) and linted with [`pylint`](https://www.pylint.org/). [`mypy`](http://mypy-lang.org/) is used to perform static type checking based on type hints. You can run each program in your terminal emulator from the `main-webserver` directory using the following commands:

```bash
# Format code with black
$ black .

# Lint with pylint
$ pylint --rcfile=../pylintrc app

# Perform static type checking with mypy
$ mypy
```

We have [pre-commit hooks](https://pre-commit.com/) with Python Black support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Python Black. We may also add Pylint/flake8 in future, which enables import error checking. If you prefer, you can also install `black` directly within your IDE by via the following instructions:

### [VSCode](https://medium.com/@marcobelo/setting-up-python-black-on-visual-studio-code-5318eba4cd00)

1. Install it on your virtual env or in your local Python with the command: `pip install black`

2. Now install the Python extension for VS-Code by opening your VSCode, typing “Ctrl + P”, and pasting the line below:

```
ext install ms-python.python
```

3. Go to the settings in your VSCode by typing “Ctrl + ,” or clicking at the gear on the bottom left and selecting “Settings [Ctrl+,]” option.

4. Type “format on save” at the search bar on top of the Settings tab and check the box.

5. Search for “Python formatting provider” and select “Black”.

6. Now open/create a Python file, write some code and save it to see the magic happen!

# Real-Time Management

TODO: improve this section with explanations on how to debug using Logz.io and run admin level things with Postman/Heroku.

# FAQ

### What is the general structure of our codebase and the purpose of specific files?

A tree structure is provided below:

Note that all conftest files contain test fixtures for their respective directories, and all other files in the directory not shown are empty/autogenerated.

```
.
├── Procfile --> the file that tells Heroku what workers we need
(web)
├── analytics --> a directory for webserver-related load testing/analytics
│   └── profiler
│       ├── profiler.py --> load testing code
│       └── tasks.py --> mock tasks for load testing
├── app
│   ├── __init__.py --> initialization code/global preprocessors for the app
│   ├── blueprints -->  our API endpoints are described here
│   │   ├── aws
│   │   │   └── aws_mandelbox_blueprint.py --> endpoints we use to create, delete, and manipulate AWS resources
│   │   ├── host_service
│   │   │   └── host_service_blueprint.py --> endpoints that handle host service handshakes
│   │   ├── mail
│   │   │   ├── mail_blueprint.py --> endpoints for generating and sending emails
│   │   │   └── newsletter_blueprint.py --> endpoints for sending out emails to a mailing list
│   ├── config.py --> General app config and setup utils
│   ├── constants --> Constants used throughout our program
│   │   ├── http_codes.py -->  HTTP response codes we return in our app
│   │   └── time.py --> useful constants about time (like 60 seconds per minute)
│   ├── exceptions.py --> exceptions we use throughout the app
│   ├── factory.py -->  general application setup scripts
│   ├── helpers --> helper utils we use throughout the codebase
│   │   ├── blueprint_helpers --> any complex synchronous computation that's part of our endpoints
│   │   │   ├── mail
│   │   │   │   └── mail_post.py --> helpers that generate emails to send to users
│   │   ├── helpers_tests
│   │   │   ├── aws_tests
│   │   │   │   └── test_ecs_client.py --> basic tests of ECSClient functionality
│   │   │   │   └── test_ec2_client.py --> basic tests of EC2Client functionality
│   │   └── utils
│   │       ├── cloud_interface -- > our cloud-general library/interface
│   │       │   ├── base_cloud_interface.py --> our abstract interface for cloud provisioning clients
│   │       ├── aws -- > utility scripts for interfacing with AWS
│   │       │   ├── autoscaling.py --> scripts for manipulating ASGs and their associated clusters
│   │       │   ├── aws_general.py --> a few general utilities for AWS
│   │       │   ├── aws_resource_integrity.py --> scripts that ensure certain AWS resources exist
│   │       │   ├── aws_resource_locks.py --> scripts to ensure atomicity on AWS resource use
│   │       │   ├── base_ec2_client.py -->  Utility libraries for monitoring and orchestrating EC2 instances.
│   │       │   ├── ecs_deletion.py --> code used for cluster deletion
│   │       │   └── utils.py --> general utility scripts for API reqs -- mostly retry code
│   │       ├── general
│   │       │   ├── auth.py  --> decorators for authenticating users/devs
│   │       │   ├── crypto.py --> code for password encryption/hashing
│   │       │   ├── limiter.py --> our rate limiter config
│   │       │   ├── logs.py --> our webserver logging config
│   │       │   ├── sql_commands.py --> helpers for SQL commit, update, and deletion
│   │       │   └── tokens.py --> utils for generating JWTs
│   │       └── mail
│   │           └── mail_client.py --> helpers for mail generation
│   ├── models --> Python classes, on which arbitrary methods may be defined, corresponding to our DB tables
│   │   ├── _meta.py -->  the scripts initializing SQLAlchemy
│   │   ├── hardware.py -->  tables in our hardware schema
│   │   ├── logs.py -->  tables in our logs schema
│   │   └── sales.py -->  tables in our sales schema
│   └── serializers --> object-to-json conversion for our ORM.  Structure mirrors models
│       ├── hardware.py
│       ├── logs.py
│       └── sales.py
├── app.json -->  structure of our app/heroku config
├── db_migration --> code that governs DB migration
│   ├── config.py  --> config values for the migration
│   ├── errors.py --> exception handling for the migration
│   ├── migration_containers.py --> scripts handling docker for migrations
│   ├── postgres.py --> scripts initializing postgres for migrations
│   ├── schema.sql --> the current schema of our db
│   ├── schema_diff.py --> a diff between the current schema on branch and the one we're merging into
│   ├── schema_dump.py --> a schema autodumper script
│   └── utilities.py --> assorted utils for the above
├── db_setup --> scripts to make an ephemeral DB
│   ├── db_setup.sh --> script that preps ephemeral DB
│   ├── fetch_db.sh --> script that fetches the current dev DB
│   └── modify_ci_db_schema.py --> scripts that eliminate extraneous info (db users) from the schema
├── docker --> scripts for running a local config of the webserver
│   ├── Dockerfile --> docker container for local webserver
│   ├── docker-compose.yml --> container instructions for local webserver
│   ├── hasura_run.sh --> script to enable hasura console on local server
│   ├── local_deploy.sh --> script to run to turn on local server
│   ├── pgparse.py --> util for pulling down config info for local server
│   └── retrieve_config.sh --> script to pull down config info for local server
├── entry.py --> Heroku app entrypoint, starts up web workers
├── pyproject.toml --> Black config
├── pytest.ini --> pytest config
├── requirements-test.txt --> test package requirements
├── requirements.txt --> packages you need to run the webserver
├── runtime.txt --> desired python version
├── run-web.sh --> script to spawn the web process.
└── tests --> tests for our assorted endpoints.  All files without docs
    should be assumed to unit test the endpoints/bps they name.
    ├── admin
    │   └── test_logs.py
    ├── aws
    │   ├── config.py
    │   ├── test_assign.py
    │   ├── test_instance_scaling.py
    │   ├── test_instance_selection.py
    ├── constants --> useful constants for testing
    │   └── settings.py --> pytest settings
    ├── helpers
    │   └── general
    │       └── progress.py
    ├── misc
    │   └── test_rate_limiter.py
    ├── patches.py --> useful monkeypatches for all our tests
    ├── run_tests.sh --> shell script that runs our tests with all desired setup/settings/teardown
    ├── setup
    │   ├── docker-compose.yml --> stands up all docker containers you need for local testing
    │   └── setup_tests.sh --> script that prepares local db/services for testing
    └── test_misc.py --> tests some of our utility scripts
```
