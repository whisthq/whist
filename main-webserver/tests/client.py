"""The custom Fractal test client."""

from flask.testing import FlaskClient
from werkzeug.test import EnvironBuilder

from .exceptions import AuthenticationError


def copy(builder):
    return EnvironBuilder.from_environ(builder.get_environ())


def omit_items(data, **omit):
    return {k: v for k, v in data.items() if not omit.get(f"omit_{k}", False)}


class BaseClient(FlaskClient):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

        self._session = EnvironBuilder()
        self._username = None

    @property
    def session(self):
        return self._session.get_environ()

    @property
    def username(self):
        return self._username

    def login(self, username, password):
        response = self.post("/account/login", json=dict(username=username, password=password),)
        token = response.json.pop("access_token", None)

        if not token:
            raise AuthenticationError(username, password)

        self._session.headers.set("Authorization", f"Bearer {token}")
        self._username = username


class FractalClient(BaseClient):
    def container_create(self, app=None, region="us-east-1", **kwargs):
        data = dict(app=app, username=self.username, region=region)
        body = omit_items(data, **kwargs)

        return self.post("/container/create", environ_base=self.session, json=body)

    def container_delete(self, container_id=None, **kwargs):
        data = dict(container_id=container_id, username=self.username)
        body = omit_items(data, **kwargs)

        return self.post("/container/delete", environ_base=self.session, json=body)

    def container_info(self, port=None, aes_key=None, **kwargs):
        data = dict(identifier=port, private_key=aes_key,)
        body = omit_items(data, **kwargs)

        return self.post("/container/protocol_info", json=body)

    def container_ping(self, available=None, port=None, aes_key=None, **kwargs):
        data = dict(
            available=available, identifier=port, private_key=aes_key, username=self.username
        )
        body = omit_items(data, **kwargs)

        return self.post("/container/ping", environ_base=self.session, json=body)

    def container_stun(self, container_id=None, stun=None, **kwargs):
        data = dict(container_id=container_id, stun=stun, username=self.username)
        body = omit_items(data, **kwargs)

        return self.post("/container/stun", environ_base=self.session, json=body)
