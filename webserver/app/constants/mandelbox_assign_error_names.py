from enum import Enum


class MandelboxAssignError(str, Enum):
    # These are all the possible reasons we would fail to find an instance for a user
    # and return a 503 error

    # Instance was found but the client app is out of date
    COMMIT_HASH_MISMATCH = "COMMIT_HASH_MISMATCH"
    # We successfully found an instance but by the time we tried to lock it, the instance
    # disappeared
    COULD_NOT_LOCK_INSTANCE = "COULD_NOT_LOCK_INSTANCE"
    # No instance was found e.g. a capacity issue
    NO_INSTANCE_AVAILABLE = "NO_INSTANCE_AVAILABLE"
    # We reached an unexpected condition
    UNDEFINED = "UNDEFINED"

    @classmethod
    def contains(cls, value):
        return value in [v.value for v in cls.__members__.values()] 
