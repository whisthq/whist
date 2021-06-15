from app import ma
from app.models import (
    InstanceInfo,
    RegionToAmi,
    SupportedAppImages,
    UserContainerState,
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


class UserContainerStateSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = UserContainerState
        include_fk = True
