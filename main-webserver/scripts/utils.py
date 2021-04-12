import json
from urllib.parse import urljoin

import requests


def make_get_request(web_url: str, endpoint: str, params: dict = None, admin_token: str = None):
    """
    Makes a GET request. Properly formats admin_token (if given).

    Args:
        web_url: URL of webserver instance to run operation on
        endpoint: endpoint at URL to hit
        params: optional params to encode in GET request
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
    return requests.get(url=url, headers=headers, params=params)


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


def make_put_request(web_url: str, endpoint: str, payload: dict, **kwargs) -> requests.Response:
    """
    Makes a PUT request. Properly formats payload.

    Args:
        web_url: URL of webserver instance to run operation on
        endpoint: endpoint at URL to hit
        payload: data to POST to server. Can be None.

    Returns:
        Response from requests library.
    """
    url = urljoin(web_url, endpoint)
    print(url)
    return requests.put(url, json=payload, **kwargs)
