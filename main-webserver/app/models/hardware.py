from app.models.public import *
from app import db

class UserVM(db.Model):
    __tablename__ = 'user_vms'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}

    vm_id = db.Column(db.String(250), primary_key=True, unique=True)
    ip = db.Column(db.String(250), nullable=False)
    location = db.Column(db.String(250), nullable=False)
    os = db.Column(db.String(250), nullable=False)
    state = db.Column(db.String(250), nullable=False)
    lock = db.Column(db.Boolean, nullable=False)
    user_id = db.Column(db.ForeignKey('users.user_id'))
    temporary_lock = db.Column(db.Integer)

    user = db.relationship('User')

class OSDisk(db.Model):
    __tablename__ = 'os_disks'
    __table_args__ = {'schema': 'hardware', 'extend_existing': True}

    disk_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    location =  db.Column(db.String(250), nullable=False)
    os = db.Column(db.String(250), nullable=False)
    disk_size = db.Column(db.Integer, nullable=False)
    allow_autoupdate = db.Column(db.Boolean, nullable=False)
    has_dedicated_vm = db.Column(db.Boolean, nullable=False)
    version = db.Column(db.String(250))
    rsa_private_key = db.Column(db.String(250))
    using_stun = db.Column(db.Boolean, nullable=False)
    ssh_password = db.Column(db.String(250))
    user_id = db.Column(db.ForeignKey('users.user_id'))
    last_pinged = db.Column(db.Integer)
    branch = db.Column(db.String(250), nullable=False, server_default=text("'master'::character varying"))

class SecondaryDisk(db.Model):
    __tablename__ = 'secondary_disks'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}

    disk_id = db.Column(db.String(250), nullable=False, primary_key=True)
    os = db.Column(db.String(250), nullable=False)
    disk_size = db.Column(db.Integer, nullable=False)
    location = db.Column(db.String(250), nullable=False)
    user_id = db.Column(db.Integer)

class InstallCommand(db.Model):
    __tablename__ = 'install_commands'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}

    install_command_id = db.Column(db.Integer, nullable=False, primary_key=True)
    windows_install_command = db.Column(Text)
    linux_install_command = db.Column(Text)
    app_name = db.Column(Text)

class AppsToInstall(db.Model):
    __tablename__ = 'apps_to_install'
    __table_args__ = (
        Index('fkIdx_115', 'disk_id', 'user_id'),
        {'schema': 'hardware', 'extend_existing': True}
    )

    disk_id = db.Column(db.String(250), primary_key=True, nullable=False)
    user_id = db.Column(db.String(250), primary_key=True, nullable=False)
    app_id = db.Column(db.String(250), nullable=False, index=True)
