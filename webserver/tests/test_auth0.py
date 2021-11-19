"""Unit tests for Auth0 access control utilities."""
from typing import Any, Callable, Dict, Tuple

import pytest
from flask import Flask
from flask_jwt_extended import create_access_token, JWTManager

from app.utils.general.limiter import limiter
from app.utils.auth.auth0 import has_scope, scope_required, ScopeError
from tests.client import WhistAPITestClient


@pytest.fixture(name="app")
def fixture_app() -> Flask:
    app = Flask(__name__)
    app.test_client_class = WhistAPITestClient
    app.config["JWT_SECRET_KEY"] = "secret"

    JWTManager(app)
    limiter.init_app(app)

    return app


def test_has_scope(app: Flask) -> None:
    access_token = create_access_token(identity="alice", additional_claims={"scope": "admin"})

    with app.test_request_context("/", headers={"Authorization": f"Bearer {access_token}"}):
        assert has_scope("admin") is True


def test_has_no_scope_jwt_optional(app: Flask) -> None:
    with app.test_request_context("/"):
        assert has_scope("admin") is False


def test_has_no_scope(app: Flask) -> None:
    access_token = create_access_token(identity="eve")

    with app.test_request_context("/", headers={"Authorization": f"Bearer {access_token}"}):
        assert has_scope("admin") is False


@pytest.mark.parametrize(
    "login_kwargs, status_code",
    [[{"scope": "admin"}, 200], [{"scope": "hello admin world"}, 200], [{}, 401]],
)
def test_scope_required(
    app: Flask,
    client: WhistAPITestClient,
    login_kwargs: Dict[str, str],
    make_user: Callable[..., str],
    status_code: int,
) -> None:
    @app.errorhandler(ScopeError)
    def _handle_scope_error(_e: Any) -> Tuple[str, int]:
        return "Unauthorized", 401

    @app.route("/")
    @scope_required("admin")
    def _view() -> str:
        return "OK"

    user = make_user()

    client.login(user, admin=False, **login_kwargs)

    response = client.get("/")

    assert response.status_code == status_code
