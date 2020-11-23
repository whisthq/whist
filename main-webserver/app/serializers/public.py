from app import ma
from app.models import User


class UserSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = User
