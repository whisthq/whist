# Docker with `webserver`

This directory contains a Dockerfile used to containerize the Flask server component of the web application stack. Furthermore, `docker-compose` (configured by `docker-compose.yml`) provides a way to deploy the web stack, including the Flask web server, and the PostgreSQL database, locally with a single command: `bash docker/local_deploy.sh`.

## Deploying the stack locally

Before deploying `bash docker/local_deploy.sh`, it is necessary to save a `.env` file, which contains lines of the form `KEY=VALUE` specifiying environment variables that are used to configure the processes running inside of the containers. Specifically, the following five environment variables must be set:

- `CONFIG_DB_URL`
- `POSTGRES_DB`
- `POSTGRES_HOST`
- `POSTGRES_PASSWORD`
- `POSTGRES_USER`

See `webserver/README.md` for a description of each one.

### 1. Retrieve Environment Variables

In many circumstances, you can use the `retrieve_config.sh` script to quickly generate an initial `.env` file. Before running this script, you must have the [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli) installed and you must be logged into your Heroku account, which must have access to the `tryfractal` team on Heroku. If you do not meet one or more of these requirements, just ask someone to send you their `.env` file. Otherwise, run the following command:

    bash /path/to/retrieve_config.sh

Here is an example of what the contents of a `.env` file might look like:

```bash
CONFIG_DB_URL=postgresql://user@pass:host:port/database
POSTGRES_DB=dev
POSTGRES_PASSWORD=p@$$w0rd!
POSTGRES_USER=owen
```

### 2. Spin Up Local Servers

Use the following command to deploy the stack locally:

    bash docker/local_deploy.sh

If you encounter a "daemon not running" error, this likely means that Docker is not actually running. To fix this, try restarting your computer and opening the Docker desktop app; if the app opens successfully, then the issue should go away.

Review `docker-compose.yml` to see which ports the various services are hosted on. For example, `"9999:5432"` means that the Postgres service, running on port 5432 internally, will be available on `localhost:9999` from the host machine. Line 25 of `docker-compose.yml` will tell you where the web server itself is running.

If you make a change to the webserver, you'll need to restart the docker stack with `bash docker/local_deploy.sh --down` and `bash docker/local_deploy.sh`.
