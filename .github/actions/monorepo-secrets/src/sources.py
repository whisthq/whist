import requests


def heroku(
    heroku_app_name=None,
    heroku_api_token=None,
    heroku_base_url="https://api.heroku.com",
    **kwargs,
):
    """Retrieves a map of configuration variables from Heroku app

    Given an app name, makes a call to the Heroku API to ask for all
    configuration variables stored under that app. This function relies on a
    valid API in the environement variable HEROKU_API_TOKEN.

    Args:
        secrets: A dictionary of secret keys and values. Must include keys:
                 - "HEROKU_APP_NAME"
                 - "HEROKU_API_TOKEN"

    Returns:
        A dictionary of the configuration variable names and values in Heroku.
    """
    if not heroku_app_name:
        raise Exception(f"Invalid argument: heroku_app_name={heroku_app_name}")
    if not heroku_api_token:
        raise Exception(f"Invalid argument: heroku_api_token={heroku_api_token}")

    url = f"{heroku_base_url}/apps/{heroku_app_name}/config-vars"

    resp = requests.get(
        url,
        headers={
            "Accept": "application/vnd.heroku+json; version=3",
            "Authorization": f"Bearer {heroku_api_token}",
        },
    )

    resp.raise_for_status()
    return resp.json()
