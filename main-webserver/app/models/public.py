from app.imports import *
from app import db

class User(db.Model):
    __tablename__ = "users"

    user_id = Column(Integer, primary_key=True)
    email = Column(String(250), nullable=False, unique=True)
    name = Column(String(250))
    password = Column(String(250), nullable=False)
    release_stage = Column(Integer, nullable=False)
    stripe_customer_id = Column(String(250))
    created_timestamp = Column(Integer)
    reason_for_signup = Column(Text)
    referral_code = Column(String(250))
    credits_outstanding = Column(Integer, server_default=text("0"))
    using_google_login = Column(Boolean, server_default=text("false"))
