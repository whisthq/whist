from flask import Flask
import os
from .celery_utils import init_celery
from dotenv import *
from flask_sqlalchemy import SQLAlchemy

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]

def create_app(app_name=PKG_NAME, **kwargs):
    app = Flask(app_name)
    load_dotenv()
    app.config['SQLALCHEMY_DATABASE_URI'] = os.getenv('DATABASE_URL')
    app.config['SECRET_KEY'] = os.getenv('JWT_KEY')
    app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
    app.config['CELERY_BROKER_URL'] = os.getenv('REDIS_URL')
    app.config['CELERY_RESULT_BACKEND'] = os.getenv('REDIS_URL')
    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)
    from app.all import bp
    app.register_blueprint(bp)
    return app

def create_db(app):
    db = SQLAlchemy(app)
    class ManagedDisks(db.Model):
        __tablename__ = 'managed_disks'
        __table_args__ = {'extend_existing': True} 
        diskName = db.Column(db.String, primary_key = True, unique = True)
        userName = db.Column(db.String)
        vmName = db.Column(db.String)
        LUN = db.Column(db.Integer)
        diskType = db.Column(db.String) 

    class VMs(db.Model):
        __tablename__ = 'v_ms'
        __table_args__ = {'extend_existing': True} 
        vmName = db.Column(db.String, primary_key = True, unique = True)
        vmPassword = db.Column(db.String)
        vmUserName = db.Column(db.String)
        osDisk = db.Column(db.String)
        running = db.Column(db.Boolean)


    class LoginHistory(db.Model):
        __tablename__ = 'login_history'
        __table_args__ = {'extend_existing': True} 
        userName = db.Column(db.String, primary_key = True)
        startTime = db.Column(db.String)
        endTime =  db.Column(db.String)


    class VNets(db.Model):
        __tablename__ = 'v_nets'
        __table_args__ = {'extend_existing': True} 
        vnetName = db.Column(db.String, primary_key = True)
        subnetName = db.Column(db.String)
        ipConfigName = db.Column(db.String)
        nicName = db.Column(db.String)

    class Users(db.Model):
        __tablename__ = 'users'
        __table_args__ = {'extend_existing': True} 
        userName = db.Column(db.String, primary_key = True)
        currentVM = db.Column(db.String)

    db.create_all()
    return db