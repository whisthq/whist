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

# class SecondaryDisk(Base):
#     __table__ = Table(
#         'secondary_disks', metadata,
#         Column('disk_id', String(250), nullable=False),
#         Column('os', String(250), nullable=False),
#         Column('disk_size', Integer, nullable=False),
#         Column('location', String(250), nullable=False),
#         Column('user_id', Integer),
#         schema='hardware'
#     )

# class InstallCommand(Base):
#     __table__ = Table(
#         'install_commands', metadata,
#         Column('install_command_id', Integer, nullable=False),
#         Column('windows_install_command', Text),
#         Column('linux_install_command', Text),
#         Column('app_name', Text),
#         schema='hardware'
#     )

# class AppsToInstall(Base):
#     __tablename__ = 'apps_to_install'
#     __table_args__ = (
#         Index('fkIdx_115', 'disk_id', 'user_id'),
#         {'schema': 'hardware'}
#     )

#     disk_id = Column(String(250), primary_key=True, nullable=False)
#     user_id = Column(String(250), primary_key=True, nullable=False)
#     app_id = Column(String(250), nullable=False, index=True)

