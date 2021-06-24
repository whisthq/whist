from app import ma
from app.models import (
    InstanceInfo,
    RegionToAmi,
    SupportedAppImages,
)


class SupportedAppImagesSchema(ma.SQLAlchemyAutoSchema):  # type: ignore[name-defined]
    class Meta:
        model = SupportedAppImages


class RegionToAmiSchema(ma.SQLAlchemyAutoSchema):  # type: ignore[name-defined]
    class Meta:
        model = RegionToAmi


class InstanceInfoSchema(ma.SQLAlchemyAutoSchema):  # type: ignore[name-defined]
    class Meta:
        model = InstanceInfo
