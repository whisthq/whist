from app import ma
from app.models.public import *


class UserSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = User
