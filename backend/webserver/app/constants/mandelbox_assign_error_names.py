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
    # The requested region(s) have not been enabled
    REGION_NOT_ENABLED = "REGION_NOT_ENABLED"
    # User is already conneceted to a mandelbox, possibly on another device
    USER_ALREADY_ACTIVE = "USER_ALREADY_ACTIVE"
    # We should not have reached this point, there is a logic error
    UNDEFINED = "UNDEFINED"

    @classmethod
    def contains(cls, value: str) -> bool:
        return value in [v.value for v in cls.__members__.values()]
