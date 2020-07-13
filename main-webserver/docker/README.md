# Docker with `main-webserver`

Docker is being leveraged to create a partial-stack (TBD on full) deployment of the `main-webserver` components, `web` and `celery`. To do so, it packages the application into an image with all necessary dependencies and then launches the application with the appropriate configurations, depending on if it's `web` or `celery` using `stem-cell.sh`.

Currently, the full environment is only partially replicated, so `retrieve_config.py` exists for collecting the appropriate environment variables needed to connect to the non-replicated portions of the environment (they are pulled from Heroku).

## Usage

### 1. Retrieve Environment Variables

Use `retrieve_config.py`. It provides a `-h` help menu for understanding parameters. On Mac/Linux, run

```sh
./retrieve_config.py [NAME OF BRANCH]
```

On Windows, run
```sh
py retrieve_config.py [NAME OF BRANCH]
```

To see the possible NAME_OF_BRANCH options, see Lines 34-38 of `retrieve_config.py`. This command will pull the config variables of NAME_OF_BRANCH and write them to `docker/.env`.

You can review `dev-base-config.json` to see which values will be overriden for local development. For example, the `REDIS_URL` will be changed to use the local Docker version.

This command requires the CLI tool `heroku` to be installed and logged in.

### 2. Spin Up Local Servers

Use `docker-compose` to run the stack locally. First, `cd` into the `docker/` folder. Then, run the `up` command. If you are on Windows, you should run this from a command prompt in Administrator mode.

```sh
docker-compose up --build
```

If you encounter a "daemon not running" error, this likely means that Docker is not actually running. To fix this, try restarting your computer and opening the Docker desktop app; if the app opens successfully, then the issue should go away.

Review `docker-compose.yml` to see which ports the various services are hosted on. For example, `"7810:6379"` means that the Redis service, running on port 6379 internally, will be available on `localhost:7810` from the host machine. Line 25 of `docker-compose.yml` will tell you where the web server itself is running.

If you make a change to the webserver, you'll need to restart docker by first killing the server (Ctrl-C) and re-running `docker-compose up --build`.

