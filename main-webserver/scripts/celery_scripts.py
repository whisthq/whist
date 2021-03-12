import time
import argparse
import sys
import os

# this adds the webserver repo root to the python path no matter where
# this file is called from. We can now import from `scripts`.
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), ".."))

from scripts.utils import make_get_request

# all currently supported scripts
SUPPORTED_SCRIPTS = ["poll_celery_task"]

# scripts that need admin token to run
SCRIPTS_NEEDING_ADMIN = []


def poll_celery_task(web_url: str, task_id: str, admin_token: str = None) -> str:
    """
    Poll celery task `task_id` for up to 10 minutes.
    Args:
        web_url: URL to run script on
        task_id: Task to poll
        admin_token: Optional; can provide traceback info.

    Returns:
        The task output on success, otherwise errors out.
    """
    # 300 * 2 sec = 600 sec = 10 min
    num_tries = 300
    sleep_time = 2
    endpoint = f"/status/{task_id}"
    resp_json = None
    for _ in range(num_tries):
        resp = make_get_request(web_url, endpoint, admin_token=admin_token)
        assert resp.status_code == 200, f"got code: {resp.status_code}, content: {resp.content}"
        resp_json = resp.json()
        if resp_json["state"] in ("PENDING", "STARTED"):
            # sleep and try again
            time.sleep(sleep_time)
        else:
            # non-pending/started states should exit
            break
    assert resp_json is not None
    assert resp_json["state"] == "SUCCESS", f"got response: {resp_json}"
    return resp_json["output"]


def handle_poll_celery_task(web_url: str, admin_token: str):
    """
    Handle poll_celery script. Steps:
    1. Parse `task_id` arg.
    2. Poll for success of `task_id` (see `poll_celery_task_id`)

    Args:
        web_url: URL to run script on
        admin_token: Needed to authorize use of the update_region endpoint.
    """
    poll_celery_parser = argparse.ArgumentParser(description="Run a login parser.")
    poll_celery_parser.add_argument(
        "--task_id", type=str, required=True, help="Celery task id to poll."
    )
    args, _ = poll_celery_parser.parse_known_args()

    poll_celery_task(web_url, admin_token, args.task_id)


def run_script(web_url: str, script: str, admin_token: str = None):
    """
    Dispatch to a script handler based on `script`.

    Args:
        web_url: URL to run script on
        script: Script to run. Must be in SUPPORTED_SCRIPTS
        admin_token: Optionally provided admin token. Some need it (see SCRIPTS_NEEDING_ADMIN)
    """
    assert script in SUPPORTED_SCRIPTS
    if script in SCRIPTS_NEEDING_ADMIN:
        assert admin_token is not None

    # dispatch based on method
    if script == "poll_celery_task":
        return handle_poll_celery_task(web_url, admin_token)
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
    run_script(args.web_url, args.script, args.admin_token)