from app import ma
from app.models.devops import *

class ReleaseGroupSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = ReleaseGroup
