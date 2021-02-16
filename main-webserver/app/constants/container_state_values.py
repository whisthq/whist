# check client apps for this
PENDING = "PENDING"  # is spinning up
SPINNING_UP_NEW = "SPINNING_UP_NEW"  # making a new container
READY = "READY"  # successfully spun up, ready to connect
FAILURE = "FAILURE"  # failed to spin up, cannot be connected to
CANCELLED = "CANCELLED"  # has been cancelled (treated as a FAILURE effectivelly)
