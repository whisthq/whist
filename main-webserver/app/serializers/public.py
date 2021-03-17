from app import ma
from app.models import User


class UserSchema(ma.SQLAlchemySchema):
    class Meta:
        model = User

    user_id = ma.auto_field()
    name = ma.auto_field()
    token = ma.auto_field()
    password = ma.auto_field()
    stripe_customer_id = ma.auto_field()
    reason_for_signup = ma.auto_field()
    verified = ma.auto_field()
    created_timestamp = ma.Function(lambda obj: int(obj.created_at.timestamp()))
