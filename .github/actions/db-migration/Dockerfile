# FROM postgres:13.3
FROM python:3.9.5

# Install lsb-release right away, as we need it to create a repository
# configuration so we can install postgres.
# Note that lsb-release (with a -) is the package to install, while
# lsb_release (with a _) is the command.
RUN apt-get update
RUN apt-get install -y lsb-release

# We will only have permissions enabled (in pg_hba.conf) for the user named
# "postgres", which is created automatically upon postgres installation.
# By setting PGUSER, our commands don't need to pass '-U postgres' every time.
ENV PGUSER="postgres"

# Configure the postgresql repository.
RUN echo \
    "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" \
    > /etc/apt/sources.list.d/pgdg.list
RUN wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc \
    | apt-key add -

# It's important to match the version of postgres locally to the version
# of the database server. As of writing, the server is on postgres 12.
# The version here cannot be lower than 12, but 13 has been tested and it
# works.
RUN apt-get update
RUN apt-get install -y postgresql-13 postgresql-client-13

# We copy a local, modified version of pg_hba.conf (host-based authentication).
# The only change from the postgres default is the first line, where the method
# has been changed to "trust". Without this, we get "peer authentication failed"
# when we try and pass a user to a postgres command.
COPY pg_hba.conf /etc/postgresql/13/main/pg_hba.conf

# Copy this entire folder over to /root
COPY . /root

# Install Python dependencies.
RUN pip install -r /root/requirements.txt

# Our entrypoint will be a series of commands that will start the database
# server, create databases for the diff process, and then run our python
# program. Because multiple steps are required, we use an external script.
# RUN chmod +x /root/entrypoint.sh

# The entrypoint script will append any additional arguments to its command.
# Because we're using the "exec" form of ENTRYPOINT, we can pass additional
# arguments through our CLI call to "docker run". All additional arguments
# will be forwarded into the entrypoint script.
#
# The arguments are expected to describe process that will run in the same
# shell as the database server.
#
# The "exec" form of ENTRYPOINT accepts a CMD list as "default" arguments.
# If no additional "docker run" arguments are passed, then ENTRYPOINT will
# use the CMD list below. By default, we run the migration process.
ENTRYPOINT ["python", "/root/entrypoint.py"]
CMD ["python", "/root/main.py"]
