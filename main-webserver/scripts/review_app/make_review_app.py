import argparse
import shutil

WEBSERVER_ROOT = 


def setup_branch():
    shutil.copytree("bin", "")


def setup_db():
    pass


def clean_branch():
    pass


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Review app utility.")

    parser.add_argument(
        "--setup_branch",
        action="store_true",  # True iff --setup_branch passed
        help="Add files to this branch so that it can run a review app and commit them.",
    )
    parser.add_argument(
        "--setup_db",
        action="store_true",  # True iff --setup_db passed
        help="Setup the review app db.",
    )
    parser.add_argument(
        "--clean_branch",
        action="store_true",  # True iff --clean_branch passed
        help="Undoes setup_branch and commits.",
    )

    args, _ = parser.parse_known_args()

    # only one can be true
    assert (
        args.setup_branch + args.setup_db + args.clean_branch == 1
    ), "Only one of setup_branch, setup_db, clean_branch can be passed."

    if args.setup_branch:
        setup_branch()
    elif args.setup_db():
        setup_db()
    elif args.clean_branch():
        clean_branch()
    else:
        raise ValueError("Which arg was set? Not handled by utility.")
