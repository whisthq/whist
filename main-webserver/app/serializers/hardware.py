from app import ma
from app.models import (
    ClusterInfo,
    InstanceInfo,
    RegionToAmi,
    SupportedAppImages,
    UserContainer,
    UserContainerState,
)


class UserContainerSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = UserContainer
        include_fk = True


class ClusterInfoSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = ClusterInfo
        include_fk = True


class SupportedAppImagesSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = SupportedAppImages


class RegionToAmiSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = RegionToAmi


class InstanceInfoSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = InstanceInfo


class UserContainerStateSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = UserContainerState
        include_fk = True
