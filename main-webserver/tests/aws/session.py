"""A custom Session object for testing ECS container management endpoints."""

import requests

from tests.constants.resources import SERVER_URL

from .exceptions import AuthenticationError, UnauthenticatedError


# TODO: Move this to a separate file that also contains an abstract parent
# class for all custom session classes.
def authentication_required(func):
    def _authentication_required(session, *args, **kwargs):
        if not session.authenticated:
            raise UnauthenticatedError(func)

        return func(session, *args, **kwargs)

    return _authentication_required


def omit_items(data, **omit):
    return {k: v for k, v in data.items() if not omit.get(f"omit_{k}", False)}


class Session(requests.Session):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._username = None

    # TODO: Make this an abstract method on a superclass.
    @property
    def authenticated(self):
        return self._username is not None

    def login(self, username, password):
        response = self.post(
            f"{SERVER_URL}/account/login",
            json=dict(username=username, password=password),
        )
        token = response.json().pop("access_token", None)

        if not token:
            raise AuthenticationError(username, password)

        self.headers.update({"Authorization": f"Bearer {token}"})
        self._username = username

    @authentication_required
    def container_create(self, app=None, **kwargs):
        data = dict(app=app, username=self._username)
        body = omit_items(data, **kwargs)

        return self.post(f"{SERVER_URL}/container/create", json=body)

    @authentication_required
    def container_delete(self, container_id=None, **kwargs):
        data = dict(container_id=container_id, username=self._username)
        body = omit_items(data, **kwargs)

        return self.post(f"{SERVER_URL}/container/delete", json=body)

    def container_info(self):
        return self.get(f"{SERVER_URL}/container/protocol_info")

    def container_ping(self, available=None, **kwargs):
        data = dict(available=available, username=self._username)
        body = omit_items(data, **kwargs)

        return self.post(f"{SERVER_URL}/container/ping", json=body)

    @authentication_required
    def container_stun(self, container_id=None, stun=None, **kwargs):
        data = dict(container_id=container_id, stun=stun, username=self._username)
        body = omit_items(data, **kwargs)

        return self.post(f"{SERVER_URL}/container/stun", json=body)
