from auth0 import scope_required
from payments import payment_required  # pylint: disable=unused-import

developer_required = scope_required("admin")
