from app.models.public import *
from app import db

class StripeProduct(db.Model):
    __tablename__ = 'stripe_products'
    __table_args__ = {'schema': 'sales', 'extend_existing': True}

    stripe_product_id = db.Column(db.String(250), primary_key=True)
    product_name = db.Column(db.String(250), nullable=False)

class MainNewsletter(db.Model):
    __tablename__ = 'main_newsletter'
    __table_args__ = {'schema': 'sales', 'extend_existing': True}

    user_id = db.Column(db.ForeignKey('users.user_id'), primary_key=True)
