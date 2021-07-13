from app import ma
from app.models import (
    ClusterInfo,
    InstanceInfo,
    RegionToAmi,
    SupportedAppImages,
    UserContainer,
    UserContainerState,
)


class UserContainerSchema(ma.SQLAlchemyAutoSchema):  # type: ignore[name-defined]
    class Meta:
        model = UserContainer
        include_fk = True


class ClusterInfoSchema(ma.SQLAlchemyAutoSchema):  # type: ignore[name-defined]
    class Meta:
        model = ClusterInfo
        include_fk = True


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
