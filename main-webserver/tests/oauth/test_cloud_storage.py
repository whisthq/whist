"""Tests for miscellaneous cloud storage-related code."""

import pytest
import requests

from app.celery.aws_ecs_creation import _mount_cloud_storage

from ..patches import function, Object


def test_dont_mount_cloud_storage(container, user):
    """Don't raise any errors if the Fractal user hasn't connected any external applications."""

    with container() as c:
        _mount_cloud_storage(user, c)


@pytest.mark.usefixtures("oauth_client_disabled")
def test_oauth_not_configured(container, make_credential, provider, user, monkeypatch):
    """Don't send cloud storage credentials if the Flask app isn't configured as an OAuth client.

    For the Flask app to be configured as an OAuth client for a particular provider, the OAuth
    client ID and client secret must be set in app.config. For example, to configure the Flask
    app as a Dropbox OAuth client, DROPBOX_APP_KEY and DROPBOX_APP_SECRET must be set.

    The reason why cloud storage OAuth tokens shouldn't be sent to the host service unless the
    Flask app is configured as an OAuth client is that rclone needs to know which client ID and
    client secret to use to refresh the access token if it expires during the streaming session.
    Without a client ID and client secret, it will not be able to refresh the access token.
    """

    # We'll know that the request containing the OAuth token was never sent to the host service
    # the call to _mount_cloud_storage doesn't raise an error.
    monkeypatch.setattr(requests, "post", function(raises=Exception))
    make_credential(user.user_id, provider)

    with container() as c:
        _mount_cloud_storage(user, c)


@pytest.mark.parametrize("ok", (True, False))
def test_handle_status(app, container, make_credential, provider, user, monkeypatch, ok):
    """Handle both successful and unsuccessful responses from the host service."""

    response = Object()

    monkeypatch.setattr(requests, "post", function(returns=response))
    monkeypatch.setattr(response, "ok", ok)
    monkeypatch.setattr(response, "text", "response text")
    make_credential(user.user_id, provider)

    with container() as c:
        _mount_cloud_storage(user, c)


def test_handle_exception(app, container, make_credential, provider, user, monkeypatch):
    """Handle failed connections to the host service."""

    monkeypatch.setattr(requests, "post", function(raises=requests.ConnectionError))
    make_credential(user.user_id, provider)

    with container() as c:
        _mount_cloud_storage(user, c)
