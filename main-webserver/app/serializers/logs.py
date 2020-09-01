from app import ma
from app.models.logs import *

class ProtocolLogSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = ProtocolLog


class MonitorLogSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = MonitorLog


class LoginHistorySchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = LoginHistory
