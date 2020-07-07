from tests import *


def create(vm_size, location, operating_system, admin_password, input_token):
    return requests.post(
        (SERVER_URL + "/azure_vm/create"),
        json={
            "vm_size": "Standard_NV6_Promo",
            "location": "eastus",
            "operating_system": "Windows",
            "resource_group": "FractalStaging",
        },
        headers={"Authorization": "Bearer " + input_token},
    )
