import pytest
from flask import g
from app.helpers.utils.general import limiter


# @pytest.fixture
def test_rate_limiter(client):
    limiter.enabled = True
    for i in range(10):
        resp = client.post("/newsletter/post")
        assert resp.status_code == 200
        g._rate_limiting_complete = False

    resp = client.post("/newsletter/post")
    assert resp.status_code == 429
