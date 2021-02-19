from flask_limiter import Limiter
from flask_limiter.util import get_remote_address

# initializes flask rate limiter
limiter = Limiter(key_func=get_remote_address)

RATE_LIMIT_PER_MINUTE = "10 per minute"
