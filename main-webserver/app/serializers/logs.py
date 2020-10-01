from app import ma
from app.models.logs import *


# this one must be different because we have a foreign key
# SQLAlchemyAutoSchema is not smart enough to recognize that
# foreign keys here should also be read in so it doesn't find
# the user leaving the dashboard in complete (for example)
class ProtocolLogSchema(ma.SQLAlchemySchema):
    class Meta:
        model = ProtocolLog
    
    user_id = ma.auto_field() # important!
    server_logs = ma.auto_field()
    connection_id = ma.auto_field()
    bookmarked = ma.auto_field()
    timestamp = ma.auto_field()
    version = ma.auto_field()
    client_logs = ma.auto_field()


class MonitorLogSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = MonitorLog


# similarly another foreign key so we need to be explicit
class LoginHistorySchema(ma.SQLAlchemySchema):
    class Meta:
        model = LoginHistory

    user_id = ma.auto_field() # important!
    action = ma.auto_field()
    timestamp = ma.auto_field()
