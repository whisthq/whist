# check client apps for this
PENDING = "PENDING"  # is spinning up
READY = "READY"  # successfully spun up, ready to connect
FAILURE = "FAILURE"  # failed to spin up, cannot be connected to
CANCELLED = "CANCELLED"  # has been cancelled (treated as a FAILURE effectivelly)
