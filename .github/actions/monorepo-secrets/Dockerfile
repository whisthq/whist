FROM python:3.9.5

# Note the build-time commands below are run relative to this file. The
# working directory will change to the monorepo root when ENTRYPOINT is
# executed (this change happens only in GitHub Actions).
#
# To avoid confusion, it's best to always build and run this image from the
# monorepo root so the working directories are the same.

COPY requirements.txt /root/requirements.txt

RUN pip install --no-cache-dir -r /root/requirements.txt

COPY . /root

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
ENTRYPOINT ["/bin/bash"]
