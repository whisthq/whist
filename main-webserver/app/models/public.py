from app.imports import *
from app import db

class User(db.Model):
    __tablename__ = "users"


    user_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    name = db.Column(db.String(250))
    token = db.Column(db.String(250))
    password = db.Column(db.String(250), nullable=False)
    release_stage = db.Column(db.Integer, nullable=False)
    stripe_customer_id = db.Column(db.String(250))
    created_timestamp = db.Column(db.Integer)
    reason_for_signup = db.Column(Text)
    referral_code = db.Column(db.String(250))
    credits_outstanding = db.Column(db.Integer, server_default=text("0"))
    using_google_login = db.Column(db.Boolean, server_default=text("false"))
