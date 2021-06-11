"""
The machine-to-machine authentication utility class.

TODO: update this comment
"""

import requests

import time
from threading import RLock
from dataclasses import dataclass

@dataclass
class M2MAccessToken:
    access_token: str
    expires: float

class M2MClient:
    cache = {}
    lock = RLock()

    def __init__(self, auth0_domain, auth0_client_id, auth0_client_secret):
        self.request_url = f'https://{auth0_domain}/oauth/token'
        self.auth0_client_id = auth0_client_id
        self.auth0_client_secret = auth0_client_secret
    
    def __request_token(self):
        body = {
            "audience": "https://api.fractal.co",
            "grant_type": "client_credentials",
            "client_id": self.auth0_client_id,
            "client_secret": self.auth0_client_secret
        }
        res = requests.post(self.request_url, data = body)
        return res.json()
    
    def __regenerate(self):
        current_time = time.time()
        json = self.__request_token()
        return M2MAccessToken(json["access_token"], current_time + int(json["expires_in"]))
    
    def token(self):
        key = (self.auth0_client_id, self.auth0_client_secret)
        if key in self.cache and self.cache[key].expires > time.time():
            return self.cache[key]
        with self.lock:
            while key not in self.cache or self.cache[key].expires < time.time():
                self.cache[key] = self.__regenerate()
                return self.cache[key]
            return self.cache[key]
