from app.models.public import *

class UserVM(Base):
    __tablename__ = 'user_vms'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}

    vm_id = Column(String(250), primary_key=True, unique=True)
    ip = Column(String(250), nullable=False)
    location = Column(String(250), nullable=False)
    os = Column(String(250), nullable=False)
    state = Column(String(250), nullable=False)
    lock = Column(Boolean, nullable=False)
    user_id = Column(ForeignKey('users.user_id'))
    temporary_lock = Column(Integer)

    user = relationship('User')

class OSDisk(Base):
    __tablename__ = 'os_disks'
    __table_args__ = {'schema': 'hardware', 'extend_existing': True}
    
    disk_id = Column(String(250), nullable=False, unique=True, primary_key=True)
    location =  Column(String(250), nullable=False)
    os = Column(String(250), nullable=False)
    disk_size = Column(Integer, nullable=False)
    allow_autoupdate = Column(Boolean, nullable=False)
    has_dedicated_vm = Column(Boolean, nullable=False)
    version = Column(String(250))
    rsa_private_key = Column(String(250))
    using_stun = Column(Boolean, nullable=False)
    ssh_password = Column(String(250))
    user_id = Column(ForeignKey('users.user_id'))
    last_pinged = Column(Integer)
    branch = Column(String(250), nullable=False, server_default=text("'master'::character varying"))

class SecondaryDisk(Base):
    __tablename__ = 'secondary_disks'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}
    
    disk_id = Column(String(250), nullable=False, primary_key=True)
    os = Column(String(250), nullable=False)
    disk_size = Column(Integer, nullable=False)
    location = Column(String(250), nullable=False)
    user_id = Column(Integer)

class InstallCommand(Base):
    __tablename__ = 'install_commands'
    __table_args__ = {'extend_existing': True, 'schema': 'hardware'}
    
    install_command_id = Column(Integer, nullable=False, primary_key=True)
    windows_install_command = Column(Text)
    linux_install_command = Column(Text)
    app_name = Column(Text)

class AppsToInstall(Base):
    __tablename__ = 'apps_to_install'
    __table_args__ = (
        Index('fkIdx_115', 'disk_id', 'user_id'),
        {'schema': 'hardware', 'extend_existing': True}
    )

    disk_id = Column(String(250), primary_key=True, nullable=False)
    user_id = Column(String(250), primary_key=True, nullable=False)
    app_id = Column(String(250), nullable=False, index=True)

