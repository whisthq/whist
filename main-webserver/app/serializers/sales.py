from app import ma
from app.models import StripeProduct


class StripeProductSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = StripeProduct
