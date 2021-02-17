# Fratal Database Migration Workflow

This subfolder contains all the Python logic to automatically perform database migrations.

## High-Level Overview

The Fractal migration strategy revolves around treating our database schema as code. Like anything else in the codebase, this involves checking a schema "source of truth" into version control. This takes the form of a file in the root of `fractal/fractal` called `db.sql`.

`db.sql` represents the shape of the database schema for the branch that it resides in. So the `db.sql` in the `dev` branch represents the database at `fractal-dev-server`, and the `db.sql` in the `staging` branch represents the database at `fractal-staging-server`.

This means that standard `git` discipline applies to how we manage our database schema. Making a change to the database schema involves these steps:

1. Create a branch off `origin/dev` to work on your feature.
2. Run and test your branch with a local copy of the `dev` database. Change the schema of your local database until fits your needs.
3. Run `pg_dump` on your local database, using the output to overwrite`db.sql` in your branch. This new version of `db.sql` is the "source of truth" for your branch.
4. Commit `db.sql` and open a PR. A GitHub Action (this one) will "diff" your `db.sql` with `origin/dev/db.sql`, placing the "diff" in the PR conversation for convenience. This "diff" represents the changes that are necessary to migrate the `dev` database to conform to your schema changes.
5. The "diff" is reviewed and discussed by the team. If the schema changes are approved, your PR is accepted and merged.
6. Upon merging your PR to `origin/dev`, GitHub Actions automatically executes the "diff" SQL commands on the "live" `dev` database, e.g. `fractal-dev-server`. The database migration is now complete!

As with all good `git` practice, this means that we only ever make changes through the `git` flow. Sure, you _could_ update the database though the UI, or manually through SQL commands. You also _could_ push your code right to `origin/master`, bypassing the review process. Even though that would save you some steps, we don't do it because we know it breaks the integrity of our codebase.

I, for one, welcome our new `db.sql` overlord.

## Low-Level Overview

A standard way to represent database schemas is through a `.sql` file containing SQL statements describing the schema. Think of this `.sql` file as "the SQL statements required to rebuild the schema from scratch". A "schema dump", then, analyzes a running database (let's call it **DB_A**) and creates a `.sql` file with lots of SQL statements. If you were to run the statements in this `.sql` file against an empty database (**DB_B**), it would result in **DB_B** having an identical schema to **DB_A**.

The difficult part of this is that the "schema dump" is not necessarily deterministic. The schema dump is not "dumping" the same SQL commands that you've executed over the life of the database, or even "dumping" SQL commands in the same order. It's simply looking at the current schema of the running database, and "dumping" some SQL commands that would rebuild it. Even if the schemas of **DB_A** and **DB_B** differ by only a single SQL command, the schema dumps of each of them could look completely different.

This means that you can't rely on a regular `git diff` on the `.sql` schema dumps of **DB_A** and **DB_B**. To calculate an accurate "diff" between two database schemas, those schemas need to be loaded into two running databases. This all happens automatically within a GitHub Actions during `fractal` push events.

When the GitHub Actions migration workflow is triggered by a pull request, a container is spun up from a PostgreSQL image. The steps taken in the container are roughly as follows:

1. Dump the schema from the live database, determined by the `HEROKU_APP_NAME` environment variable.
2. Load the live database ("current") schema (e.g. `fractal-dev-server`) into a fresh PostgreSQL database (**DB_A**).
3. Read the schema from `db.sql` in the ("merging") PR branch. Load the `db.sql` schema into a fresh PostgreSQL database (**DB_B**).
4. Run `migra` diff tool on **DB_A** ("current") and **DB_B** ("merging"), produce a **DIFF** `.sql` file. **DIFF** contains the SQL commands necessary to perform the migration.
5. Run the SQL commands in **DIFF** on **DB_A**. If **DB_A** matches **DB_B**, the **DIFF** SQL commands are valid.
6. If the `PERFORM_DATABASE_MIGRATION` environment variable is `True` (e.g. during merge with `origin/dev`), then run the SQL commands in **DIFF** on the live database (e.g. `fractal-dev-server`).
7. Print output to GitHub pull request conversation.

This workflow applies to every branch, including "promoting" `dev` to `staging` etc. When you're developing a feature, the **DB_B** schema is `db.sql` in your feature branch, and the **DB_A** schema is the schema of `fractal-dev-server`. When you're promoting `dev` to `staging`, the **DB_B** schema is the `db.sql` in the `dev` branch, and the **DB_A** schema is the schema of `fractal-staging-server`.

Actual execution of the SQL commands to perform a database migration is always handled by GitHub Actions, with the commands available inside of pull requests for our team to review.
