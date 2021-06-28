# check client apps for this
PENDING = "PENDING"  # begun looking for a container to give a user
SPINNING_UP_NEW = "SPINNING_UP_NEW"  # making a new container
WAITING_FOR_CLIENT_APP = "WAITING_FOR_CLIENT_APP"  # found container, passed info to client app
READY = "READY"  # successfully spun up, ready to connect
FAILURE = "FAILURE"  # failed to spin up, cannot be connected to
CANCELLED = "CANCELLED"  # has been cancelled (treated as a FAILURE effectively)
