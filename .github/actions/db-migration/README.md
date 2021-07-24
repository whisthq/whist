# Fractal Database Migration Workflow

This subfolder contains all the Python logic to automatically perform database migrations.

## Running the workflow locally

To keep consistency with the GitHub Actions environment, you should run all commands below with a working directory of the monorepo root (`fractal`). You must also set your `HEROKU_API_TOKEN` environment variable through the `--env` flag. You can access your API token from the Heroku CLI with `heroku auth:token`.

Note that to run locally, you _must_ mount the monorepo to the container as a Docker `--volume`, and your container working directory must be in the same location. It doesn't matter which location you choose, as long as you also set the working directory. In CI, the working directory will be forced to `/github/workspace`, so we use that in the example below.

```sh
# Build the docker image
docker build --tag fractal/db-migration .github/actions/db-migration

# Run database migration, output schema diff

docker run \
    --rm \
    --env "HEROKU_API_TOKEN=*********" \
    --volume $(pwd):/github/workspace \
    --workdir /github/workspace \
    fractal/db-migration
```

You can also add a `--entrypoint` and `-it` flag to start a shell inside the container.

```sh
docker run \
    -it \
    --rm \
    --env "HEROKU_API_TOKEN=*********" \
    --volume $(pwd):/github/workspace \
    --entrypoint /bin/bash \
    fractal/db-migration
```

You can have multiple `--volume` flags, and it can be helpful to add `.github/actions/db-migration` as a `--volume` if you're working on the `db-migration` code. This saves you from having to rebuild the container on each code change.

## Command to dump the database schema

With the `fractal` repo as your working directory and the variable `DB_URL` set to the URL of the running database, run this command to dump the database schema to correct file. Make sure to use a single `>` to overwrite the file.

```sh
pg_dump --no-owner --no-privileges --schema-only $DB_URL > webserver/db_migration/schema.sql

```

## High-Level Overview

The Fractal migration strategy revolves around treating our database schema as code. Like anything else in the codebase, this involves checking a schema "source of truth" into version control. This takes the form of a file in `fractal/webserver/db-migration` called `schema.sql`.

`schema.sql` represents the shape of the database schema for the branch that it resides in. So the `schema.sql` in the `dev` branch represents the database at `fractal-dev-server`, and the `schema.sql` in the `staging` branch represents the database at `fractal-staging-server`.

This means that standard `git` discipline applies to how we manage our database schema. Making a change to the database schema involves these steps:

1. Create a branch off `origin/dev` to work on your feature.
2. Run and test your branch with a local copy of the `dev` database. Change the schema of your local database until fits your needs.
3. Run `pg_dump` on your local database, using the output to overwrite `schema.sql` in your branch. This new version of `schema.sql` is the "source of truth" for your branch.
4. Commit `schema.sql` and open a PR. A GitHub Action (`database-migration.yml`) will "diff" your `schema.sql` with `origin/dev/webserver/db-migration/schema.sql`, placing the "diff" in the PR conversation for convenience. This "diff" represents the changes that are necessary to migrate the `dev` database to conform to your branch's schema changes.
5. The "diff" is reviewed and discussed by the team. If the schema changes are approved, your PR is accepted and merged.
6. Upon merging your PR to `origin/dev`, GitHub Actions automatically executes the "diff" SQL commands on the "live" `dev` database, e.g. `fractal-dev-server`. The database migration is now complete!

Any change to `fractal/webserver` will trigger the migration workflow.

As with all good `git` practice, this means that we only ever make changes through the `git` flow. Sure, you _could_ update the database though the UI, or manually through SQL commands. You also _could_ push your code right to `origin/prod`, bypassing the review process. Even though that would save you some steps, we don't do it because we know it breaks the integrity of our codebase.

I, for one, welcome our new `schema.sql` overlord.

## Low-Level Overview

A standard way to represent database schemas is through a `.sql` file containing SQL statements describing the schema. Think of this `.sql` file as "the SQL statements required to rebuild the schema from scratch". A "schema dump", then, analyzes a running database (let's call it **DB_A**) and creates a `.sql` file with lots of SQL statements. If you were to run the statements in this `.sql` file against an empty database (**DB_B**), it would result in **DB_B** having an identical schema to **DB_A**.

The difficult part of this is that the "schema dump" is not necessarily deterministic. The schema dump is not "dumping" the same SQL commands that you've executed over the life of the database, or even "dumping" SQL commands in the same order. It's simply looking at the current schema of the running database, and "dumping" some SQL commands that would rebuild it. Even if the schemas of **DB_A** and **DB_B** differ by only a single SQL command, the schema dumps of each of them could look completely different.

This means that you can't rely on a regular `git diff` on the `.sql` schema dumps of **DB_A** and **DB_B**. To calculate an accurate "diff" between two database schemas, those schemas need to be loaded into two running databases. This all happens automatically within a GitHub Actions during `fractal` push events.

When the GitHub Actions migration workflow is triggered by a pull request, a container is spun up from a PostgreSQL image. The steps taken in the container are roughly as follows:

1. Dump the schema from the live database, determined by the `HEROKU_APP_NAME` environment variable.
2. Load the live database ("current") schema (on dev branch this would be `fractal-dev-server`) into a fresh PostgreSQL database (**DB_A**).
3. Read the schema from `schema.sql` in the ("merging") PR branch. Load the `schema.sql` schema into a fresh PostgreSQL database (**DB_B**).
4. Run `migra` diff tool on **DB_A** ("current") and **DB_B** ("merging"), produce a **DIFF** `.sql` file. **DIFF** contains the SQL commands necessary to perform the migration.
5. Run the SQL commands in **DIFF** on **DB_A**. If **DB_A** matches **DB_B**, the **DIFF** SQL commands are valid.
6. If the `PERFORM_DATABASE_MIGRATION` environment variable is `True` (e.g. during merge with `origin/dev`), then run the SQL commands in **DIFF** on the live database (e.g. `fractal-dev-server`).
7. Print output to GitHub pull request conversation.

This workflow applies to every branch, including "promoting" `dev` to `staging` etc. When you're developing a feature, the **DB_B** schema is `schema.sql` in your feature branch, and the **DB_A** schema is the schema of `fractal-dev-server`. When you're promoting `dev` to `staging`, the **DB_B** schema is the `schema.sql` in the `dev` branch, and the **DB_A** schema is the schema of `fractal-staging-server`.

Actual execution of the SQL commands to perform a database migration is always handled by GitHub Actions, with the commands available inside of pull requests for our team to review.
