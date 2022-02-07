# Whist Webserver

This directory contains the source code for Whist's main web server. Its primary responsibility is assigning instances to users, as well as handling payments. Eventually, the webserver will be fully replaced by the scaling service.

![Whist Backend drawio](https://user-images.githubusercontent.com/19579265/152873311-0c7095ce-368d-44a9-a256-00ede333af96.svg)

The diagram above gives a general overview of Whist's backend. First, the user starts the client app and gets signed in. Once the user is authenticated, the client app sends a request to the webserver looking for a free instance it can connect to. The webserver queries the database for a free instance and registers the mandelbox, which in turn triggers a database event on the host service. The host service reacts to this event by starting the process of spinning up and preparing the mandelbox container.

Meanwhile, the client sends an http request to the JSON transport endpoint on the host service. Finally, the host service receives and validates the JSON transport request and marks the mandelbox as ready, returning the assigned ports to the client.

This is a general description of the process which allows us to serve Whist to our users.

Several other minor responsibilities of the Whist Webserver are documented in the [Webserver documentation](https://docs.whist.com/webserver/responsibilities.html).

## Contributing

All Webserver patches must adhere to our [code standards](https://www.notion.so/whisthq/Documentation-Code-Standards-54f2d68a37824742b8feb6303359a597#a119aceede764be08b8990c0605e8d39) and pass continuous integration checks. When submitting a patch, you should be able to answer "yes" to each of the following questions:

- [ ] Is my code free of errors that would be uncovered by static analysis tools (Black, mypy, Pylint)?
- [ ] Do all existing test cases pass?
- [ ] Do my changes non-negatively impact test coverage?
- [ ] Are my changes documented well enough that a new intern could understand them?

We don't like merge commits at Whist. Be sure to rebase your feature branches off of the latest version of the dev branch to eliminate any conflicts that might prevent them from being automatically merged into dev when they're ready.

Module documentation for every module in the `webserver` directory is available at https://docs.whist.com/webserver/modules.

## Development environment setup

1. **Install development tools**

   The following software is required to run tests on your code locally:

   - AWS CLI (see the AWS documentation for instructions on how to [install](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-install.html) and [configure](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-configure.html) the AWS CLI)
   - [Docker](https://docs.docker.com/get-docker/)
   - [docker-compose](https://docs.docker.com/compose/install/)
   - [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli)
   - [PostgreSQL](https://www.postgresql.org/download/)

     Specifically, the `pg_dump` and `psql` binaries must be accessible from your `PATH`.

2. **Set up Python 3.10.0**

   All Webserver development should take place in a Python 3.10.0 environment. We recommend using a tool such as [virtualenv](https://virtualenv.pypa.io/en/latest/) to set up a virtual environment. Setting up a Python virtual environment can feel dauting and confusing, but virtual environments provide for a vastly superior development experience. Don't be afraid to ask for help if you're having trouble setting one up or don't know where to start.

3. **Install Python dependencies**

   Once your Python environment is ready, you'll need to run `pip install -r requirements.txt -r requirements-test.txt` or an equivalent command to install all of the dependencies listed in `requirements.txt` and `requirements-test.txt`.

4. **Download environment variables**

   ```bash
   backend/webserver/ $ bash docker/retrieve_config.sh
   ```

   The `retrieve_config.sh` script downloads environment variables that must be set in your development environment for your code to behave correctly when it is deployed locally. It writes them to `docker/.env`, which is automatically loaded every time your code runs.

## Testing

Deploying your code locally is not currently supported. However, it is possible to test your code locally.

1. First, you'll need to spin up a local PostgreSQL database.

   ```bash
   backend/webserver/ $ bash tests/setup/setup_tests.sh
   ```

   The `setup_tests.sh` script launches the database and populates it with some necessary seed data. When you're done, you can tear your database down with `bash tests/setup/setup_tests.sh --down`.

2. Invoke `pytest` via a wrapper script:

   ```bash
   backend/webserver/ $ bash tests/run_tests.sh
   ```

   The `run_tests.sh` script accepts the same arguments as `pytest`. Try `bash tests/run_tests.sh --help` for more information, or go to the [pytest usage docs](https://docs.pytest.org/en/latest/how-to/usage.html) to view the arguments that can be passed in. As an example, to run the single test `test_terminate_single_ec2_fails` in `test_instance_scaling.py`, you run the following command:

   ```bash
   ./tests/run_tests.sh tests/aws/test_instance_scaling.py::test_terminate_single_ec2_fails
   ```

   You can also set the environment variable `COV=1` before executing `run_tests.sh` to generate a test coverage report. You can view the report with `coverage report` (see `coverage --help` for more options).

   Note that, since `run_tests.sh` calls `pytest` under the hood, pytest must be accessible from your `PATH`. This may mean that your Python virtual environment needs to be active when you run the script.

## Static analysis

In CI, we use Black, mypy, and Pylint to format and catch errors in our code without actually having to run it. You can use the following commands to run each of these tools locally:

```bash
# Format code with Black
backend/webserver/ $ black --config ../../pyproject.toml .

# Perform static type checking with mypy
# Note: We need to re-enable this for scripts and tests!
backend/webserver/ $ mypy --strict app scripts tests

# Lint with Pylint
backend/webserver/ $ pylint --rcfile=../../pylintrc app auth0 payments
```
