from app import ma
from app.models import (
    InstanceInfo,
    RegionToAmi,
)


class RegionToAmiSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = RegionToAmi


class InstanceInfoSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = InstanceInfo
