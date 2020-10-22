from app import ma
from app.models.hardware import *


class UserVMSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = UserVM
        include_fk = True


class OSDiskSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = OSDisk
        include_fk = True


class UserContainerSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = UserContainer
        include_fk = True


class ClusterInfoSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = ClusterInfo
        include_fk = True


class SecondaryDiskSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = SecondaryDisk


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
