# Fractal Main Webserver

[![Heroku CI Status](https://heroku-cibadge.herokuapp.com/last.svg)](https://dashboard.heroku.com/pipelines/22da0c0d-7555-4647-8765-031c14b8398f/tests) ![Python Black Linting](https://github.com/fractal/main-webserver/workflows/Python%20Black%20Linting/badge.svg) ![Sentry Release](https://github.com/fractal/main-webserver/workflows/Sentry%20Release/badge.svg)

This repository contains the code for our webserver, which is our REST API and provides backend support for our user interfaces, our internal tools, and our container/virtual machine management.

Our webservers and CD pipeline are hosted on Heroku. Our production database is attached as an Heroku Add-On PostgresSQL to the associated webserver in Heroku, `main-webserver`, and has automated backups in place daily at 2 AM PST. See [here](https://devcenter.heroku.com/articles/heroku-postgres-backups#creating-a-backup) for further information.

Our webserver logs are hosted on Datadog [here](https://app.datadoghq.com/logs?cols=core_host%2Ccore_service&from_ts=1593977274176&index=&live=true&messageDisplay=inline&stream_sort=desc&to_ts=1593978174176).

#### Coding Philosophy

Before contributing to this project, please read our in-depth coding philosophy document [here](https://www.notion.so/tryfractal/Code-Philosophy-Webserver-backend-d036205444464f8b8a61dc36eeae7dbb).

## Development

### Getting Started

#### Local Setup

The web application stack is comprised of three main components: the web server itself, an asynchronous task queue, and a database. The web server is written in Python using the [Flask](https://flask.palletsprojects.com/en/1.1.x/) web framework. The task queue is a Redis-backed [Celery](https://docs.celeryproject.org/en/stable/index.html) task queue. As such, the task queue can be broken down into two more granular sub-components: a pool of worker processes and a Redis store. The database is a Postgres instance that is shaared by multiple developers. In summary, there are a total of _four_ components that make up the web application stack: a Flask server, a Celery worker pool, a Redis store, and a PostgreSQL database.

We use [`docker-compose`](https://docs.docker.com/compose/) to spin part of the web server stack up (the `docker-compose` stack does not include the Postgres database, which is shared between multiple developers and app deployments, as mentioned above) locally for development purposes. `docker-compose` builds Docker images for the Flask server and the Celery worker pool and deploys them alongside containerized Redis. There is also a `pytest` test suite that developers may run locally. Note that it is necessary to spin up each of the components (excluding the Postgres database) manually if you would like to run the test suite.

We use environment variables to configure our local development environments. Environment variables should be set by adding lines of the form `KEY=VALUE` to the file `docker/.env`. **The main environment variable that _must_ be set in order to do any kind of local development, whether with the `docker-compose` stack or the `pytest` test suite, is the `CONFIG_DB_URL` environment variable.** `CONFIG_DB_URL` specifies the PostgreSQL connection URI of the Fractal configuration database. This database contains default values for many of the variables that are used to configure the various parts of the web application stack. These default values may be overridden locally by setting alternatives in the same `docker/.env` file.

The following environment variables must also be set in `docker/.env` (neither the test suite nor the `docker-compose` stack will work without them):
* `POSTGRES_DB` &ndash; The name of the Postgres database to which to connect.
* `POSTGRES_HOST` &ndash; The hostname or IP address of the development Postgres instance.
* `POSTGRES_PASSWORD` &ndash; The password used to authenticate with the local stack's PostgresQL instance.
* `POSTGRES_USER` &ndash; The name of the user as whom to log into the development Postgres instance.

Finally, if the Redis instance used for testing is running anywhere other than `redis://localhost:6379/0`, `REDIS_URL` should be set to indicate the correct connection URI. Take a moment to understand that setting `REDIS_URL` has no effect on the Flask instance running in the `docker-compose`; it only affects Flask applications launched manually (e.g. Flask applications created by `pytest` for testing purposes).

**1. Set environment variables**

Luckily, there is an easy way to set all of the necessary environment variables using the script `docker/retrieve_config.sh`. To set all of the required environment variables, first make sure you have [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli) installed and configured. Then, run

    bash /path/to/docker/retrieve_config.sh

When the `docker/retrieve_config.sh` script terminates, it will print the name of the file containing the fetched environment variables that has been written to standard error.

**2. Spin Up Local Servers**

Use `docker-compose` to run the stack locally. First, `cd` into the `docker/` folder. Then, run the `up` command. If you are on Windows, you should run this from a command prompt in Administrator mode.

```sh
docker-compose up --build
```

If you encounter a "daemon not running" error, this likely means that Docker is not actually running. To fix this, try restarting your computer or opening the Docker desktop app; if the app opens successfully, then the issue should go away.

Review `docker-compose.yml` to see which ports the various services are hosted on. For example, `"7810:6379"` means that the Redis service, running on port 6379 internally, will be available on `localhost:7810` from the host machine. Line 25 of `docker-compose.yml` will tell you where the webserver itself is running.

By default, hot-reloading of the Flask web server and Celery task queue is disabled (`HOT_RELOAD=`). To enable it, set `HOT_RELOAD` to a non-empty string in your `docker/.env` file.

### Helper Software Setup

We recommend that you download several softwares to help you code and test:

**Postman**

We use Postman to send API requests to our server, to store our API endpoints, and to generate documentation. Our Postman team link is [here](https://tryfractal.postman.co/). If you are not part of the team, contact @mingy98. To better understand how Postman works, refer to our wiki [here](https://www.notion.so/tryfractal/Postman-API-Documentation-602cc6df23e04cd0a026340c406bd663).

**TablePlus**

We use TablePlus to visualize, search, and modify our SQL database. For instructions on how to set up TablePlus, refer to our wiki [here](https://www.notion.so/tryfractal/Using-TablePlus-to-Access-our-PostgresSQL-Database-d5badb38eb3841deb56a84698ccd20f5).

### Heroku Setup

For continuous integration and delivery, we leverage Heroku pipelines, which provides us with automated PR testing, isolation of environment variables, promotion/rollbacks, and auto-deploys from Github. Contributors should NOT push code to Heroku; only codeowners are expected to do this. Instead, contributors should PR their changes into the appropriate Github branch (most often `master`).

While our Heroku pipeline should not be modified without codeowner permission, it is helpful to understand how it works by consulting our wiki [here](https://www.notion.so/tryfractal/Heroku-CI-CD-Pipeline-Webservers-f8ef5b43edc84c969cf005fcac4641ba).

### GraphQL

We leverage Hasura GraphQL (hosted on Heroku) to enable real-time database access and serverless database retrieval. For pure SQL requests, we encourage using GraphQL instead of writing your own server endpoint to minimize the amount of code we write and because GraphQL has a lot of really nice built-in features.

GraphQL is already set up, but here's a [setup doc](https://hasura.io/docs/1.0/graphql/core/deployment/deployment-guides/heroku.html) for reference. Hasura GraphQL also provides a console for easy interfacing with the database. The prod database console is [here](prod-database.tryfractal.com) using access key stored as a config variable in the Heroku `fractal-graphql` application.

## Testing

**Pytest**

We have pytest tests in the `tests` subdirectory. To run tests, just run `pytest` in a terminal. Refer to the [pytest documentation](https://docs.pytest.org/en/stable/contents.html) to learn how to use pytest.

The docker-compose stack does **not** need to be running in order to run tests. However, the tests do need access to Redis. By default, the tests attempt to connect to `redis://localhost:6379/0`. If your Redis service is running elsewhere, expose the correct connection URI to the tests via the environment variable `REDIS_URL`.

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Python Black](https://github.com/psf/black) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Python Black support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Python Black. We may also add Pylint/flake8 in future, which enables import error checking.

You can always run Python Black directly from a terminal by first installing it via `pip install black` (requires Python 3.6+) and running `black .` to format the whole project. You can see and modify the project's Python Black settings in `pyproject.toml` and list options by running `black --help`. If you prefer, you can also install it directly within your IDE by via the following instructions:

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
