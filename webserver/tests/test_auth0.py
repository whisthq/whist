"""Unit tests for Auth0 access control utilities."""

from flask_jwt_extended import create_access_token

from auth0 import has_scope


def test_has_scope(app):
    access_token = create_access_token(identity="alice", additional_claims={"scope": "admin"})

    with app.test_request_context("/", headers={"Authorization": f"Bearer {access_token}"}):
        assert has_scope("admin") == True


def test_has_no_scope_jwt_optional(app):
    with app.test_request_context("/"):
        assert has_scope("admin") == False


def test_has_no_scope(app):
    access_token = create_access_token(identity="eve")

    with app.test_request_context("/", headers={"Authorization": f"Bearer {access_token}"}):
        assert has_scope("admin") == False
