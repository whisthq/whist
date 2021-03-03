from app import ma
from app.models import LoginHistory


# similarly another foreign key so we need to be explicit
class LoginHistorySchema(ma.SQLAlchemySchema):
    class Meta:
        model = LoginHistory

    user_id = ma.auto_field()  # important!
    action = ma.auto_field()
    timestamp = ma.auto_field()
