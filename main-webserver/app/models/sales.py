from app.models.public import *

class StripeProduct(Base):
    __tablename__ = 'stripe_products'
    __table_args__ = {'schema': 'sales', 'extend_existing': True}

    stripe_product_id = Column(String(250), primary_key=True)
    product_name = Column(String(250), nullable=False)

class MainNewsletter(Base):
    __tablename__ = 'main_newsletter'
    __table_args__ = {'schema': 'sales', 'extend_existing': True}

    user_id = Column(ForeignKey('users.user_id'), primary_key=True)