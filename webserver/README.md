# Fractal Webserver

[![codecov](https://codecov.io/gh/fractal/fractal/branch/dev/graph/badge.svg?token=QB0c3c2NBj)](https://codecov.io/gh/fractal/fractal)

This directory contains the source code for Fractal's main web server. The Webserver is the central component of Fractal's backend. Its primary responsibility is load-balancing requests across the other backend services.

![Fractal Backend](https://user-images.githubusercontent.com/31637652/127786757-50ec9cde-fa93-4558-a7aa-432a21a2ae21.png)

The diagram above is a simplified representation of Fractal's backend. Directional arrows between services represent requests. Arrows A and B represent requests that the backend receives from clients.

We can use this diagram to better understand the Webserver's load balancing responsibilities. When a user would like to start streaming an application, their client sends a request of type A to the backend followed immediately by a request of type B. The recipient of the first request is the Webserver. Included in the Webserver's response is the IP address of a compute instance running the Fractal host service. Upon receipt of the Webserver's response, the client sends a request of type B to the host service running at the address specified in the response. The Webserver examines compute instance resource utilization metrics to determine which instance's IP address to send back to the client. In summary, the Webserver responds to requests of type A such that all compute instances running the Fractal host service share the responsibility of handling requests of type B.

Several other minor responsibilities of the Fractal Webserver are documented in the [Webserver documentation](https://docs.fractal.co/webserver/responsibilities.html).

## Contributing

All Webserver patches must adhere to our [code standards](https://www.notion.so/tryfractal/Documentation-Code-Standards-54f2d68a37824742b8feb6303359a597#a119aceede764be08b8990c0605e8d39) and pass continuous integration checks. When submitting a patch, you should be able to answer "yes" to each of the following questions:

- [ ] Is my code free of errors that would be uncovered by static analysis tools (Black, mypy, Pylint)?
- [ ] Do all existing test cases pass?
- [ ] Do my changes non-negatively impact test coverage?
- [ ] Are my changes documented well enough that a new intern could understand them?

We don't like merge commits at Fractal. Be sure to rebase your feature branches off of the latest version of the dev branch to eliminate any conflicts that might prevent them from being automatically merged into dev when they're ready.

Module documentation for every module in the `webserver` directory is available at https://docs.fractal.co/webserver/modules.

## Development environment setup

1. **Install development tools**

   The following software is required to run tests on your code locally:

   - AWS CLI (see the AWS documentation for instructions on how to [install](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-install.html) and [configure](https://docs.aws.amazon.com/cli/latest/userguide/cli-chap-configure.html) the AWS CLI)
   - [Docker](https://docs.docker.com/get-docker/)
   - [docker-compose](https://docs.docker.com/compose/install/)
   - [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli)
   - [PostgreSQL](https://www.postgresql.org/download/)

     Specifically, the `pg_dump` and `psql` binaries must be accessible from your `PATH`.

2. **Set up Python 3.9.5**

   All Webserver development should take place in a Python 3.9.5 environment. We recommend using a tool such as [virtualenv](https://virtualenv.pypa.io/en/latest/) to set up a virtual environment. Setting up a Python virtual environment can feel dauting and confusing, but virtual environments provide for a vastly superior development experience. Don't be afraid to ask for help if you're having trouble setting one up or don't know where to start.

3. **Install Python dependencies**

   Once your Python environment is ready, you'll need to run `pip install -r requirements.txt -r requirements-test.txt` or an equivalent command to install all of the dependencies listed in `requirements.txt` and `requirements-test.txt`.

4. **Download environment variables**

   ```bash
   webserver/ $ bash docker/retrieve_config.sh
   ```

   The `retrieve_config.sh` script downloads environment variables that must be set in your development environment for your code to behave correctly when it is deployed locally. It writes them to `docker/.env`, which is automatically loaded every time your code runs.

## Testing

Deploying your code locally is not currently supported. However, it is possible to test your code programmatically.

1. First, you'll need to spin up a local PostgreSQL database.

   ```bash
   webserver/ $ bash tests/setup/setup_tests.sh
   ```

   The `setup_tests.sh` script launches the database and populates it with some necessary seed data. When you're done, you can tear your database down with `bash tests/setup/setup_tests.sh --down`.

2. Invoke `pytest` via a wrapper script:

   ```bash
   webserver/ $ bash tests/run_tests.sh
   ```

   The `run_tests.sh` script accepts the same arguments as `pytest`. Try `bash tests/run_tests.sh --help` for more information. Set `COV=1` to generate a test coverage report. You can view the report with `coverage report` (see `coverage --help` for more options).

   Note that, since `run_tests.sh` calls `pytest` under the hood, pytest must be accessible from your `PATH`. This may mean that your Python virtual environment needs to be active when you run the script.

## Static analysis

In CI, we use Black, mypy, and Pylint to format and catch errors in our code without actually having to run it. You can use the following commands to run each of these tools locally:

```bash
# Format code with Black
webserver/ $ black --config ../pyproject.toml .

# Perform static type checking with mypy
webserver/ $ mypy

# Lint with Pylint
webserver/ $ pylint --rcfile=../pylintrc app auth0 payments
```
