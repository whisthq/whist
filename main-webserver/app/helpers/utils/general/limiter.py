from flask_limiter import Limiter
from flask_limiter.util import get_remote_address

# initializes flask rate limiter
limiter = Limiter(key_func=get_remote_address)

LIMIT = "10 per minute"
