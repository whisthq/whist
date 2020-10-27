from app import ma
from app.models import (
    AppsToInstall,
    Banners,
    ClusterInfo,
    InstallCommand,
    SupportedAppImages,
    UserContainer,
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


class AppsToInstallSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = AppsToInstall


class SupportedAppImagesSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = SupportedAppImages


class BannersSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = Banners
