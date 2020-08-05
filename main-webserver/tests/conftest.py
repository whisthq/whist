import os

from tests import *
from dotenv import *

load_dotenv(find_dotenv())

# parallelizes tests
def pytest_collection_modifyitems(config, items):
    ci_node_total = int(os.getenv("CI_NODE_TOTAL", 1))
    ci_node_index = int(os.getenv("CI_NODE_INDEX", 0))
    if ci_node_total == 2:
        fractalLog(
            function="conftest",
            label="conftest",
            logs="Parallelizing tests",
        )
        if ci_node_index == 0:
            items[:] = [
                item
                for index, item in enumerate(items)
                if "disk" in item.name
            ]
        else:
            items[:] = [
                item
                for index, item in enumerate(items)
                if "vm" in item.name
            ]

@pytest.fixture
def input_token():
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(
            username="fractal-admin@gmail.com", password="!!fractal-admin-password!!"
        ),
    )

    return resp.json()["access_token"]


@pytest.fixture
def admin_token():
    resp = requests.post(
        (SERVER_URL + "/admin/login"),
        json=dict(
            username=os.getenv("DASHBOARD_USERNAME"),
            password=os.getenv("DASHBOARD_PASSWORD"),
        ),
    )

    return resp.json()["access_token"]
