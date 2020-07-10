from app import *

table_bp = Blueprint("table_bp", __name__)


@table_bp.route("/table", methods=["GET"])
@fractalPreProcess
@jwt_required
@adminRequired
def sql_table_get(**kwargs):
    table_name = request.args.get("table_name")

    output = fractalSQLSelect(table_name=table_name, params={})

    if output["success"] and output["rows"]:
        return jsonify({"output": output["rows"]}), SUCCESS
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
