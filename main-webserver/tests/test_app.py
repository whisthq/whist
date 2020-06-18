import sys
import os
import tempfile
import pytest
import pytest_pgsql

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from app import create_app, init_app

@pytest.fixture
def app():
    app, jwtManager = create_app()
    app = init_app(app)
    return app

def register(client, username, password):
    return client.post(('/account/register'), json=dict(
        username=username,
        password=password
    ))

def fetchvms(client, username):
    return client.post(("/user/login"), json=dict(
        username=username
    ))

def test_fetchvms(client):
    email = 'isa.zheng@gmail.com'
    rv = fetchvms(client, email)
    assert rv.status_code == 200

    # json_data = rv.get_json()
    # print(json_data)
    # assert verify_token(email, json_data['access_token'])
