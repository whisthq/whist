from app import ma
from app.models import ReleaseGroup


class ReleaseGroupSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = ReleaseGroup
