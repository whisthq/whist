from app import ma
from app.models import (
    InstanceInfo,
    RegionToAmi,
    SupportedAppImages,
    UserContainerState,
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


class UserContainerStateSchema(ma.SQLAlchemyAutoSchema):  # type: ignore[name-defined]
    class Meta:
        model = UserContainerState
        include_fk = True
