import argparse
import os
import subprocess

# move to directory where this file is written no matter where this is called from
CURRENT_DIR = os.path.join(os.getcwd(), os.path.dirname(__file__))

WEBSERVER_ROOT = os.path.join(CURRENT_DIR, "../..")


def setup_review_app(app_name: str):
    """
    Setup the review app. Specifically:
    1. Start the celery worker (disabled by default)
    2. Fetch the ephemeral db URL from the review app
    3. Setup the ephemeral db

    Args:
        app_name: given by Heroku when making a review app in the UI

    Returns:
        None, errors if anything goes wrong.
    """

    # capture stdout of this process so we get the eph db url
    print(f"Getting DATABASE_URL from review app {app_name}...")
    ret = subprocess.run(
        f"heroku config:get DATABASE_URL --app {app_name}",
        capture_output=True,
        shell=True,
    )
    assert ret.returncode == 0

    eph_db_url = ret.stdout.decode("utf-8").strip()

    # add DB_EXISTS=true and POSTGRES_URI=<eph_db_url> to env, then
    # run `ephemeral_db_setup/db_setup.sh`
    print(
        f"Setting up ephemeral db at {eph_db_url}.\n"
        "This writes your local schema (db_migration/schema.sql) and "
        "data (ephemeral_db_setup/db_data.sql) to the db.\n"
        "Please make sure your data is up-to-date. If not, delete "
        "ephemeral_db_setup/db_data.sql and rerun this script."
    )
    os.environ["DB_EXISTS"] = "true"
    os.environ["POSTGRES_URI"] = eph_db_url
    ret = subprocess.run(
        f"bash {os.path.join(WEBSERVER_ROOT, 'ephemeral_db_setup/db_setup.sh')}",
        start_new_session=True,
        shell=True,
    )
    assert ret.returncode == 0


if __name__ == "__main__":
    setup_review_app_parser = argparse.ArgumentParser(description="Setup the review app.")
    setup_review_app_parser.add_argument(
        "--app_name",
        type=str,
        required=True,
        help="Name of the Heroku Review App.",
    )

    args, _ = setup_review_app_parser.parse_known_args()
    setup_review_app(args.app_name)
