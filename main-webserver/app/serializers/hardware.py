from app import ma
from app.models import (
    InstanceInfo,
    RegionToAmi,
    SupportedAppImages,
)


class SupportedAppImagesSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = SupportedAppImages


class RegionToAmiSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = RegionToAmi


class InstanceInfoSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = InstanceInfo
