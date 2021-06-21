"""
The Auth0 utility class.

This class provides utility methods for Auth0.
Currently, it offers a method to generate machine-to-machine access tokens, which may
be used to authenticate the webserver to the host service or Auth0's internal management API.

In the future, this class will contain additional methods for various Auth0 management tasks,
such as triggering a 2FA enrollment.
"""

import time
from functools import lru_cache
from dataclasses import dataclass

import requests


@dataclass
class M2MAccessToken:
    access_token: str
    expires: float


class Auth0Client:
    @classmethod
    @lru_cache(maxsize=None)
    def __generate_token(cls, auth0_client_id, auth0_client_secret, request_url):
        current_time = time.time()
        body = {
            "audience": "https://api.fractal.co",
            "grant_type": "client_credentials",
            "client_id": auth0_client_id,
            "client_secret": auth0_client_secret,
        }
        res = requests.post(request_url, data=body)
        json = res.json()
        return M2MAccessToken(json["access_token"], current_time + int(json["expires_in"]))

    def __init__(self, auth0_domain, auth0_client_id, auth0_client_secret):
        """
        Initializes a machine-to-machine client.

        Args:
            auth0_domain: Auth0 tenant root domain (eg. "fractal-dev.us.auth0.com")
            auth0_client_id: Auth0 machine-to-machine client ID
            auth0_client_secret: Auth0 machine-to-machine client secret
        """

        self.request_url = f"https://{auth0_domain}/oauth/token"
        self.auth0_client_id = auth0_client_id
        self.auth0_client_secret = auth0_client_secret

    def token(self):
        """
        Returns a machine-to-machine access token
        """

        access_token = self.__generate_token(
            self.auth0_client_id, self.auth0_client_secret, self.request_url
        )
        if access_token.expires < time.time():
            # The token expired, clear the cache
            self.__generate_token.cache_clear()
            return self.token()
        return access_token
