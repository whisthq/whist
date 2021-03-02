import pytest
from flask import g


# @pytest.fixture
def test_rate_limiter(client):
    for i in range(20):
        resp = client.post("/newsletter/post")
        assert resp.status_code == 200
        g._rate_limiting_complete = False

    resp = client.post("/newsletter/post")
    assert resp.status_code == 429
