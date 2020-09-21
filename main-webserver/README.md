# Fractal Main Webserver

[![Heroku CI Status](https://heroku-cibadge.herokuapp.com/last.svg)](https://dashboard.heroku.com/pipelines/22da0c0d-7555-4647-8765-031c14b8398f/tests)

This repository contains the code for our webserver, which is our REST API and provides backend support for our user interfaces, our internal tools, and our container/virtual machine management.

Our webservers and CD pipeline are hosted on Heroku. Our production database is attached as an Heroku Add-On PostgresSQL to the associated webserver in Heroku, `main-webserver`, and has automated backups in place daily at 2 AM PST. See [here](https://devcenter.heroku.com/articles/heroku-postgres-backups#creating-a-backup) for further information.

Our webserver logs are hosted on Datadog [here](https://app.datadoghq.com/logs?cols=core_host%2Ccore_service&from_ts=1593977274176&index=&live=true&messageDisplay=inline&stream_sort=desc&to_ts=1593978174176).

## Getting Started

### Local Setup

Docker is being leveraged to create a partial-stack deployment of the `main-webserver` components, `web` and `celery`. To do so, it packages the application into an image with all necessary dependencies and then launches the application with the appropriate configurations, depending on if it's `web` or `celery` using `stem-cell.sh`. 

Currently, the full environment is only partially replicated, so `retrieve_config.py` exists for collecting the appropriate environment variables needed to connect to the non-replicated portions of the environment. They are pulled from Heroku.

#### 1. Retrieve Environment Variables

First, ensure that the CLI tool `heroku`, and then type `heroku login` to log in (if you're not already logged in). Next, use `retrieve_config.py`. It provides a `-h` help menu for understanding parameters. Then, run:

```sh
# MacOS/Linux
python retrieve_config.py staging

# Windows
py retrieve_config.py staging
```

You can review `dev-base-config.json` to see which values will be overriden for local development. For example, the `REDIS_URL` will be changed to use the local Docker version.

#### 2. Spin Up Local Servers

Use `docker-compose` to run the stack locally. First, `cd` into the `docker/` folder. Then, run the `up` command. If you are on Windows, you should run this from a command prompt in Administrator mode.

```sh
docker-compose up --build
```

If you encounter a "daemon not running" error, this likely means that Docker is not actually running. To fix this, try restarting your computer or opening the Docker desktop app; if the app opens successfully, then the issue should go away.

Review `docker-compose.yml` to see which ports the various services are hosted on. For example, `"7810:6379"` means that the Redis service, running on port 6379 internally, will be available on `localhost:7810` from the host machine. Line 25 of `docker-compose.yml` will tell you where the webserver itself is running.

By default, hot-reloading of the Flask web server and Celery task queue is enabled. To disable, set `HOT_RELOAD=false` in your Docker `.env` file.

### Helper Software Setup

We recommend that you download several softwares to help you code and test:

#### Postman

We use Postman to send API requests to our server, to store our API endpoints, and to generate documentation. Our Postman team link is [here](https://fractalcomputers.postman.co/). If you are not part of the team, contact @mingy98. To better understand how Postman works, refer to our wiki [here](https://www.notion.so/fractalcomputers/Postman-API-Documentation-602cc6df23e04cd0a026340c406bd663).

#### TablePlus

We use TablePlus to visualize, search, and modify our SQL database. For instructions on how to set up TablePlus, refer to our wiki [here](https://www.notion.so/fractalcomputers/Using-TablePlus-to-Access-our-PostgresSQL-Database-d5badb38eb3841deb56a84698ccd20f5).

### Heroku Setup

For continuous integration and delivery, we leverage Heroku pipelines, which provides us with automated PR testing, isolation of environment variables, promotion/rollbacks, and auto-deploys from Github. Contributors should NOT push code to Heroku; only codeowners are expected to do this. Instead, contributors should PR their changes into the appropriate Github branch (most often `master`).

While our Heroku pipeline should not be modified without codeowner permission, it is helpful to understand how it works by consulting our wiki [here](https://www.notion.so/fractalcomputers/Heroku-CI-CD-Pipeline-Webservers-f8ef5b43edc84c969cf005fcac4641ba).

## Styling

To ensure that code formatting is standardized, and to minimize clutter in the commits, you should set up styling with [Python Black](https://github.com/psf/black) before making any PRs. We have [pre-commit hooks](https://pre-commit.com/) with Python Black support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Python Black.

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

## Coding Philosophy

Please read our in-depth guide on our coding philosophy for this repo [here](https://www.notion.so/fractalcomputers/Code-Philosophy-Webserver-backend-d036205444464f8b8a61dc36eeae7dbb).

## Testing

**Pytest**
NOTE: Currently in the process of being fixed. Skip for now.

We have pytest tests in the `/tests` folder. To run tests, just run `pytest -o log_cli=true -s` in terminal. To run tests in parallel, run `pytest -o log_cli=true -s -n <num>`, with `<num>` as the # of workers in parallel.

If tests are failing when you run the command above locally, make sure that the docker-compose stack is running. Start it if it's not. If the docker-compose stack has crashed, there's a good chance you haven't set the correct environment variables, especially if your terminal displays `AttributeError: 'NoneType' object has no attribute 'upper'`. Be sure that environment variables such as `DASHBOARD_USERNAME`, `DASHBOARD_PASSWORD`, and `USE_PRODUCTION_KEYS` are set to the correct values. The username and password should match the values stored in the relevant configuration database. You can define the environment variables in a `.env` file inside of the repository root, or the `docker` or `tests` subdirectories, or you can set them in your shell.

To get an idea of what environment variables you might be missing, try running `git grep 'os\.getenv'` in the repository root.

