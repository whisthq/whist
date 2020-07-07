from tests import *


def getStatus(id):
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()
