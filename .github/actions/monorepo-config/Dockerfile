FROM python:3.9.5

# Note the build-time commands below are run relative to this file. The
# working directory will change to the monorepo root when ENTRYPOINT is
# executed (this change happens only in GitHub Actions).
#
# To avoid confusion, it's best to always build and run this image from the
# monorepo root so the working directories are the same.

COPY requirements.txt /root/monorepo-config/requirements.txt

RUN pip install -r /root/monorepo-config/requirements.txt

COPY entrypoint.sh /root/monorepo-config/entrypoint.sh

RUN chmod +x /root/monorepo-config/entrypoint.sh

# If you want to pass positional arguments to your ENTRYPOINT, you need to
# use the "exec" form that you see below, which takes the form of a JSON array.
# This is so that the container runs your process directly, as opposed to using
# a shell. If you pass arguments using the "shell" from (no brackets or quotes),
# then the positional arguments passed to "docker run ..." will be passed to the
# shell, not your entrypoint process.
#
# Note that when this image is run in GitHub Actions, the working directory
# for ENTRYPOINT will be the monorepo root, so commands inside entrypoint.sh
# should be relative to the monorepo root.
ENTRYPOINT ["/root/monorepo-config/entrypoint.sh"]
