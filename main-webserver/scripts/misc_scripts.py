import argparse

from scripts.utils import make_post_request

# all currently supported scripts
SUPPORTED_SCRIPTS = ["login"]

# scripts that need admin token to run
SCRIPTS_NEEDING_ADMIN = []


def login(web_url: str, username: str, password: str):
    """
    Login and get the access tokens for a particular user.

    Args:
        web_url: URL to run script on
        username: user to login as
        password: password associated with user

    Returns:
        dict containing "access_token", "refresh_token"
    """
    endpoint = "/admin/login"
    payload = {
        "username": username,
        "password": password,
    }
    resp = make_post_request(web_url, endpoint, payload)
    assert resp.status_code == 200, f"got code: {resp.status_code}, content: {resp.content}"
    resp_json = resp.json()
    return resp_json


def handle_login(web_url: str) -> dict:
    """
    Handle login script. Steps:
    1. Parse `username` and `password` args.
    2. Call `login(...)`
    Errors out if anything goes wrong.

    Args:
        web_url: URL to run script on

    Returns:
        See `login(...)`
    """
    login_parser = argparse.ArgumentParser(description="Run a login parser.")
    login_parser.add_argument("--username", type=str, required=True, help="Username in payload.")
    login_parser.add_argument("--password", type=str, required=True, help="Password in payload.")
    args, _ = login_parser.parse_known_args()
    return login(web_url, args.username, args.password)


def run_script(web_url: str, script: str, admin_token: str = None):
    """
    Dispatch to a script handler based on `script`.

    Args:
        web_url: URL to run script on
        script: Script to run. Must be in SUPPORTED_SCRIPTS
        admin_token: Optionally provided admin token. Some need it (see SCRIPTS_NEEDING_ADMIN)
    """
    # dispatch based on method
    if script == "login":
        return handle_login(web_url)
    else:
        raise ValueError(f"Unknown script {script}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run a supported webserver script.")
    parser.add_argument("--web_url", type=str, required=True, help="URL of webserver to hit.")
    parser.add_argument(
        "--script",
        type=str,
        required=True,
        help=f"Script to invoke. Supported choices: {SUPPORTED_SCRIPTS}",
    )
    parser.add_argument(
        "--admin_token",
        type=str,
        default=None,
        required=False,
        help="Admin token for special permissions.",
    )
    args, _ = parser.parse_known_args()
    run_script(args.web_url, args.script)
