from tests import *


@pytest.fixture
def input_token():
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(
            username="fractal-admin@gmail.com", password="!!fractal-admin-password!!"
        ),
    )

    input_token = resp.json()["access_token"]

    return input_token


def pytest_collection_modifyitems(items, config):
    ci_node_total = int(os.getenv("CI_NODE_TOTAL", 1))
    ci_node_index = int(os.getenv("CI_NODE_INDEX", 0))

    print(enumerate(items))
    print(config)

    # items[:] = [
    #     item
    #     for index, item in enumerate(items)
    #     if index % ci_node_total == ci_node_index
    # ]
