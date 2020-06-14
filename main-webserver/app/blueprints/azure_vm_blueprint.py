from app import *
from app.helpers.blueprint_helpers.azure_vm_get import *
from app.helpers.blueprint_helpers.azure_vm_post import *
from app.celery.azure_vm_celery_get import *
from app.celery.azure_vm_celery_post import *

azure_vm_bp = Blueprint("azure_vm_bp", __name__)


@azure_vm_bp.route("/azure_vm/<action>", methods=["POST"])
@fractalPreProcess
def azure_vm_post(action, **kwargs):
    if action == "create":
        vm_size = kwargs["body"]["vm_size"]
        location = kwargs["body"]["location"]
        operating_system = (
            "Windows"
            if kwargs["body"]["operating_system"].upper() == "Windows"
            else "Linux"
        )

        admin_password = None
        if "admin_password" in kwargs["body"].keys():
            admin_password = kwargs["body"]["admin_password"]

        task = createVM.apply_async(
            [vm_size, location, operating_system, admin_password]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
