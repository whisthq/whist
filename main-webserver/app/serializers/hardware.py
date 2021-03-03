from app import ma
from app.models import (
    Banners,
    ClusterInfo,
    InstallCommand,
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


class InstallCommandSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = InstallCommand


class SupportedAppImagesSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = SupportedAppImages


class RegionToAmiSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = RegionToAmi


class BannersSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = Banners


class UserContainerStateSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = UserContainerState
        include_fk = True
