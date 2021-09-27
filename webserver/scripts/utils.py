import json
from urllib.parse import urljoin

import requests


def make_get_request(web_url: str, endpoint: str, params: dict = None, bearer_token: str = None):
    """
    Makes an optionally authenticated GET request. Properly formats bearer_token (if given).

    Args:
        web_url: URL to hit
        endpoint: endpoint at URL to hit
        params: optional params to encode in GET request
        bearer_token: Optional bearer token for auth

    Returns:
        Response from requests library.
    """
    url = urljoin(web_url, endpoint)
    headers = None
    if bearer_token is not None:
        headers = {
            "Authorization": f"Bearer {bearer_token}",
        }
    return requests.get(url=url, headers=headers, params=params)


def make_post_request(
    web_url: str,
    endpoint: str,
    payload: dict,
    bearer_token: str = None,
) -> requests.Response:
    """
    Makes a POST request. Properly formats bearer_token (if given) and payload.

    Args:
        web_url: URL to hit
        endpoint: endpoint at URL to hit
        payload: data to POST to server. Can be None.
        bearer_token: Optional bearer token for auth

    Returns:
        Response from requests library.
    """
    url = urljoin(web_url, endpoint)
    headers = None
    if bearer_token is not None:
        headers = {
            "Authorization": f"Bearer {bearer_token}",
        }
    payload_str = None
    if payload is not None:
        payload_str = json.dumps(payload)
    return requests.post(url=url, headers=headers, data=payload_str)
