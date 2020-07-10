# Docker with `main-webserver`

Docker is being leveraged to create a partial-stack (TBD on full) deployment of the `main-webserver` components, `web` and `celery`. To do so, it packages the application into an image with all necessary dependencies and then launches the application with the appropriate configurations, depending on if it's `web` or `celery` using `stem-cell.sh`.

Currently, the full environment is only partially replicated, so `retrieve_config.py` exists for collecting the appropriate environment variables needed to connect to the non-replicated portions of the environment (they are pulled from Heroku).

## Usage

### 1. Retrieve Environment Variables

Use `retrieve_config.py`. It provides a `-h` help menu for understanding parameters. Most likely you will want:

```sh
./retrieve_config.py staging
```

Which will pull the env vars for _staging_ and write them to `docker/.env`.

You can review `dev-base-config.json` to see which values will be overriden for local development. For example, the `REDIS_URL` will be changed to use the local Docker version.

This command requires the CLI tool `heroku` to be installed and logged in.

### 2. Spin Up Local Servers

Use `docker-compose` to run the stack locally. This can be done via the `up` command.

```sh
docker-compose up --build
```

If this command is run from outside the `docker/` directory then use something like `docker-compose -f docker/docker-compose.yml up` instead.

This should show build information in the console (subsequent builds will be close to instantaneous because they will be cached) and then start showing stdout/err logs from the various services.

Review `docker-compose.yml` to see which ports the various services are hosted on. For example, `"7810:6379"` means that the Redis service, running on port 6379 internally, will be available on `localhost:7810` from the host machine.

