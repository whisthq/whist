from app import *
from app.helpers.utils.general.auth import *
from app.helpers.blueprint_helpers.admin.report_get import *

table_bp = Blueprint("table_bp", __name__)


@table_bp.route("/table", methods=["GET"])
@fractalPreProcess
@jwt_required
@adminRequired
def sql_table_get(**kwargs):
    table_name = request.args.get("table_name")

    if table_name == "user_vms":
        output = fetchVMsHelper()
    elif table_name == "users":
        output = fetchUsersHelper()
    elif table_name == "os_disks":
        output = fetchDisksHelper()

    if output:
        return jsonify({"output": output}), SUCCESS
    else:
        fractalLog(
            function="sql_table_get",
            label=table_name,
            logs=str(output["error"]),
            level=logging.ERROR,
        )
        if not output["rows"]:
            return jsonify({"output": None}), NOT_FOUND
        else:
            return jsonify({"output": None}), BAD_REQUEST
