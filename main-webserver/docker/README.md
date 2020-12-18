# Docker with `main-webserver`

This directory contains a Dockerfile used to containerize both the Flask server and the Celery task queue components of the web application stack. Furthermore, `docker-compose` (configured by `docker-compose.yml`) provides a way to deploy a partial web application stack, including the Flask web server, the Celery task queue, and the Redis worker output store, locally with a single command `docker-compose up --build)`.

## Deploying the partial stack locally

Before deploying the partial stack with `docker-compose`, it is necessary to save a `.env` file, which contains lines of the form `KEY=VALUE` specifiying environment variables that are used to configure the processes running inside of the containers. Specifically, the following five environment variables must be set (`REDIS_URL` is ignored):

- `CONFIG_DB_URL`
- `POSTGRES_DB`
- `POSTGRES_HOST`
- `POSTGRES_PASSWORD`
- `POSTGRES_USER`

See `main-webserver/README.md` for a description of each one.

### 1. Retrieve Environment Variables

In many circumstances, you can use the `retrieve_config.sh` script to quickly generate an initial `.env` file. Before running this script, you must have the [Heroku CLI](https://devcenter.heroku.com/articles/heroku-cli) installed and you must be logged into your Heroku account, which must have access to the tryfractal team on Heroku. If you do not meet one or more of these requirements, just ask someone to send you their `.env` file. Otherwise, run the following command:

    bash /path/to/retrieve_config.sh

Here is an example of what the contents of a `.env` file might look like:

```
CONFIG_DB_URL=postgresql://user@pass:host:port/database
POSTGRES_DB=dev
POSTGRES_PASSWORD=p@$$w0rd!
POSTGRES_USER=owen
HOT_RELOAD=
```

### 2. Spin Up Local Servers

Use `docker-compose` to run the stack locally. First, `cd` into the `docker/` folder. Then, run the `up` command. If you are on Windows, you should run this from a command prompt in Administrator mode.

```sh
docker-compose up --build
```

If you encounter a "daemon not running" error, this likely means that Docker is not actually running. To fix this, try restarting your computer and opening the Docker desktop app; if the app opens successfully, then the issue should go away.

Review `docker-compose.yml` to see which ports the various services are hosted on. For example, `"7810:6379"` means that the Redis service, running on port 6379 internally, will be available on `localhost:7810` from the host machine. Line 25 of `docker-compose.yml` will tell you where the web server itself is running.

If you make a change to the webserver, you'll need to restart docker by first killing the server (Ctrl-C) and re-running `docker-compose up --build` unless the environment variable `HOT_RELOAD` is set to a non-empty string in your `.env` file.
