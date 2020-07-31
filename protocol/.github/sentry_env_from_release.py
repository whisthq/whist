import sys


def get_env_from_release(release_str):
    if 'dev' in release_str:
        print("::set-output name=sentry_env::dev")
    elif 'staging' in release_str:
        print("::set-output name=sentry_env::staging")
    elif 'master' in release_str:
        print("::set-output name=sentry_env::production")
    else:
        print("::set-output name=sentry_env::dev")
    return


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Incorrect usgae. Usage: python sentry_env_from_release <release_name>")
        sys.exit(1)
    else:
        get_env_from_release(sys.argv[1])
