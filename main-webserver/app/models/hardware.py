from app.models.public import *
from app import db

class UserVM(db.Model):
    __tablename__ = 'user_vms'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}

    vm_id = db.Columm(String(250), primary_key=True, unique=True)
    ip = db.Columm(String(250), nullable=False)
    location = db.Columm(String(250), nullable=False)
    os = db.Columm(String(250), nullable=False)
    state = db.Columm(String(250), nullable=False)
    lock = db.Columm(Boolean, nullable=False)
    user_id = db.Columm(ForeignKey('users.user_id'))
    temporary_lock = db.Columm(Integer)

    user = db.relationship('User')

class OSDisk(db.Model):
    __tablename__ = 'os_disks'
    __table_args__ = {'schema': 'hardware', 'extend_existing': True}

    disk_id = db.Columm(String(250), nullable=False, unique=True, primary_key=True)
    location =  db.Columm(String(250), nullable=False)
    os = db.Columm(String(250), nullable=False)
    disk_size = db.Columm(Integer, nullable=False)
    allow_autoupdate = db.Columm(Boolean, nullable=False)
    has_dedicated_vm = db.Columm(Boolean, nullable=False)
    version = db.Columm(String(250))
    rsa_private_key = db.Columm(String(250))
    using_stun = db.Columm(Boolean, nullable=False)
    ssh_password = db.Columm(String(250))
    user_id = db.Columm(ForeignKey('users.user_id'))
    last_pinged = db.Columm(Integer)
    branch = db.Columm(String(250), nullable=False, server_default=text("'master'::character varying"))

class SecondaryDisk(db.Model):
    __tablename__ = 'secondary_disks'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}

    disk_id = db.Columm(String(250), nullable=False, primary_key=True)
    os = db.Columm(String(250), nullable=False)
    disk_size = db.Columm(Integer, nullable=False)
    location = db.Columm(String(250), nullable=False)
    user_id = db.Columm(Integer)

class InstallCommand(db.Model):
    __tablename__ = 'install_commands'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}

    install_command_id = db.Columm(Integer, nullable=False, primary_key=True)
    windows_install_command = db.Columm(Text)
    linux_install_command = db.Columm(Text)
    app_name = db.Columm(Text)

class AppsToInstall(db.Model):
    __tablename__ = 'apps_to_install'
    __table_args__ = (
        Index('fkIdx_115', 'disk_id', 'user_id'),
        {'schema': 'hardware', 'extend_existing': True}
    )

    disk_id = db.Columm(String(250), primary_key=True, nullable=False)
    user_id = db.Columm(String(250), primary_key=True, nullable=False)
    app_id = db.Columm(String(250), nullable=False, index=True)
