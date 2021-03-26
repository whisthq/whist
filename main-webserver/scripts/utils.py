import json
from urllib.parse import urljoin

import requests


def make_get_request(web_url: str, endpoint: str, admin_token: str = None):
    """
    Makes a GET request. Properly formats admin_token (if given).

    Args:
        web_url: URL of webserver instance to run operation on
        endpoint: endpoint at URL to hit
        admin_token: Optional token for admin access

    Returns:
        Response from requests library.
    """
    url = urljoin(web_url, endpoint)
    headers = None
    if admin_token is not None:
        headers = {
            "Authorization": f"Bearer {admin_token}",
        }
    return requests.get(url=url, headers=headers)


def make_post_request(
    web_url: str,
    endpoint: str,
    payload: dict,
    admin_token: str = None,
) -> requests.Response:
    """
    Makes a POST request. Properly formats admin_token (if given) and payload.

    Args:
        web_url: URL of webserver instance to run operation on
        endpoint: endpoint at URL to hit
        payload: data to POST to server. Can be None.
        admin_token: Optional token for admin access

    Returns:
        Response from requests library.
    """
    url = urljoin(web_url, endpoint)
    headers = None
    if admin_token is not None:
        headers = {
            "Authorization": f"Bearer {admin_token}",
        }
    payload_str = None
    if payload is not None:
        payload_str = json.dumps(payload)
    return requests.post(url=url, headers=headers, data=payload_str)
