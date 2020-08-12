from app.imports import *

import hashlib, binascii


def hash_value(value):
    dk = hashlib.pbkdf2_hmac(
        "sha256", value.encode("utf-8"), os.getenv("SECRET_KEY").encode("utf-8"), 100000
    )
    return binascii.hexlify(dk).decode("utf-8")


def check_value(hashed_value, raw_value):
    dk = hashlib.pbkdf2_hmac(
        "sha256",
        raw_value.encode("utf-8"),
        os.getenv("SECRET_KEY").encode("utf-8"),
        100000,
    )
    hr = binascii.hexlify(dk).decode("utf-8")
    return hashed_value == hr
