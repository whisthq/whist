from app import *
from app.celery.azure_resource_creation import *
from app.celery.azure_resource_deletion import *
from app.celery.azure_resource_modification import *

azure_disk_bp = Blueprint("azure_disk_bp", __name__)


@azure_disk_bp.route("/azure_disk/<action>", methods=["POST"])
@fractalPreProcess
def azure_disk_post(action, **kwargs):
    if action == "clone":
        # Clone a Fractal disk

        resource_group = os.getenv("VM_GROUP")
        if "resource_group" in kwargs["body"].keys():
            resource_group = kwargs["body"]["resource_group"]

        task = cloneDisk.apply_async(
            [
                kwargs["body"]["username"],
                kwargs["body"]["location"],
                kwargs["body"]["vm_size"],
                kwargs["body"]["operating_system"],
                kwargs["body"]["apps"],
                resource_group,
            ]
        )

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED

    elif action == "swap":
        # Swap a disk onto a specified VM

        disk_name, vm_name, resource_group = (
            kwargs["body"]["disk_name"],
            kwargs["body"]["vm_name"],
            kwargs["body"]["resource_group"],
        )

        task = swapSpecificDisk.apply_async([vm_name, disk_name, resource_group])

        if not task:
            return jsonify({"ID": None}), BAD_REQUEST

        return jsonify({"ID": task.id}), ACCEPTED
    
    elif action == "delete"
        # Delete a disk from Azure and database
        
        resource_group = None
        
        if "username" in kwargs["body"].keys():
            username = kwargs["body"]["username"]
            
            output = fractalSQLSelect(
                table_name="disks",
                params={
                    "username": username 
                }
            )
            
            if output["success"] and output["rows"]:
                task = None
                for disk in output["rows"]:
                    task = deleteDisk.apply_async([disk["disk_name"]])
                
                return jsonify({"ID": task.id}), ACCEPTED
            
        elif "disk_name" in kwargs["body"].keys():
            disk_name = kwargs["body"]["disk_name"]
            
            task = deleteDisk.apply_async([disk_name])
            return jsonify({"ID": task.id}), ACCEPTED
        
        return jsonify({"ID": None}), BAD_REQUEST
            
                    
                    
