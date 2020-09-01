from app import ma
from app.models.sales import *

class StripeProductSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = StripeProduct


class MainNewsletterSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = MainNewsletter
        include_fk = True
