from app.imports import *
from app import db

class User(db.Model):
    __tablename__ = "users"

    user_id = db.Columm(Integer, primary_key=True)
    email = db.Columm(String(250), nullable=False, unique=True)
    name = db.Columm(String(250))
    password = db.Columm(String(250), nullable=False)
    release_stage = db.Columm(Integer, nullable=False)
    stripe_customer_id = db.Columm(String(250))
    created_timestamp = db.Columm(Integer)
    reason_for_signup = db.Columm(Text)
    referral_code = db.Columm(String(250))
    credits_outstanding = db.Columm(Integer, server_default=text("0"))
    using_google_login = db.Columm(Boolean, server_default=text("false"))
